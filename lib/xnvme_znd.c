// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>
#include <xnvme_spec.h>

/**
 * TODO: there should be a bunch of boundary checks here, e.g. slba + limit >
 * nzones * zcap... round down slba to zslba... all that jazz...
 *
 * Allocate struct 'znd_report' with memory allowing it to contain all
 * zone-entries, despite the intended 'limit'
 */
static inline struct xnvme_znd_report *
znd_report_init(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended)
{
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	const size_t zdext_nbytes = zns->lbafe[nvm->flbas.format].zdes * 64;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	const size_t zd_nbytes = sizeof(struct xnvme_spec_znd_descr);
	size_t nentries;
	size_t zrent_nbytes;
	size_t entries_nbytes;
	size_t report_nbytes;
	struct xnvme_znd_report *report;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		errno = EINVAL;
		return NULL;
	}

	if (extended && (!zdext_nbytes)) {
		XNVME_DEBUG("FAILED: device does not support extended report");
		errno = ENOSYS;
		return NULL;
	}

	// Determine number of entries
	nentries = limit ? limit : geo->nzone;
	if (nentries > geo->nzone) {
		XNVME_DEBUG("FAILED: nentries: %" PRIu64 " > geo->nzone: %u", nentries,
			    geo->nzone);
		errno = EINVAL;
		return NULL;
	}

	// Determine size of an entry
	zrent_nbytes = extended ? zd_nbytes + zdext_nbytes : zd_nbytes;

	entries_nbytes = nentries * zrent_nbytes;
	report_nbytes = sizeof(*report) + entries_nbytes;

	// Initialize report structure
	report = xnvme_buf_virt_alloc(0x1000, report_nbytes);
	if (!report) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}
	memset(report, 0, report_nbytes);

	report->nzones = geo->nzone;
	report->extended = extended;
	report->nentries = nentries;

	report->zd_nbytes = zd_nbytes;
	report->zdext_nbytes = zdext_nbytes;
	report->zrent_nbytes = zrent_nbytes;

	report->entries_nbytes = entries_nbytes;
	report->report_nbytes = report_nbytes;

	// TODO: round to zslba, that is, do not assume that slba is a zslba
	report->zslba = slba;

	// TODO: check boundary
	report->zelba = slba + (report->nentries - 1) * geo->nsect;

	return report;
}

/**
 * At this point then dev->geo has been filled and we can rely on the derived
 * value for dev->geo.nzone instead of probing for it here.
 *
 * As such, we can choose to skip the header for the log, or defensively fetch
 * and verify that is matches dev->geo.nzone.
 *
 * zd_nbytes: Zone Descriptor Size in BYTES
 * zdext_nbytes: Zone Descriptor Extension Size in BYTES
 * zrent_nbytes: Size of an entry in the report, that is, descr + ext
 */
struct xnvme_znd_report *
xnvme_znd_report_from_dev(struct xnvme_dev *dev, uint64_t slba, size_t limit, uint8_t extended)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	size_t dbuf_nentries_max;
	size_t dbuf_nbytes;
	void *dbuf;

	struct xnvme_znd_report *report = NULL;
	enum xnvme_spec_znd_cmd_mgmt_recv_action action;

	int err;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		errno = EINVAL;
		return NULL;
	}

	report = znd_report_init(dev, slba, limit, extended);
	if (!report) {
		XNVME_DEBUG("FAILED: znd_report_init()");
		return NULL;
	}

	dbuf_nentries_max = (geo->mdts_nbytes / report->zrent_nbytes) + 2;
	do {
		--dbuf_nentries_max;

		dbuf_nbytes = report->zrent_nbytes * dbuf_nentries_max;
		dbuf_nbytes += sizeof(*report);
	} while ((dbuf_nbytes > geo->mdts_nbytes) && dbuf_nentries_max);

	XNVME_DEBUG("INFO: dbuf_nentries_max: %" PRIu64, dbuf_nentries_max);
	XNVME_DEBUG("INFO: dbuf_nbytes: %" PRIu64, dbuf_nbytes);

	if (!dbuf_nentries_max) {
		xnvme_buf_virt_free(report);
		return NULL;
	}

	// Allocate device buffer for mgmt-recv commands
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		xnvme_buf_virt_free(report);
		return NULL;
	}

	// Determine Management Receive Action
	action = report->extended ? XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED
				  : XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT;

	for (uint64_t zslba = report->zslba; zslba <= report->zelba;) {
		struct xnvme_spec_znd_report_hdr *hdr = (void *)dbuf;
		size_t nentries =
			XNVME_MIN(dbuf_nentries_max, ((report->zelba - zslba) / geo->nsect) + 1);

		XNVME_DEBUG("INFO: nentries: %" PRIu64, nentries);
		if ((!nentries) || (nentries > dbuf_nentries_max)) {
			XNVME_DEBUG("ERR: invalid nentries");
			break;
		}

		err = xnvme_znd_mgmt_recv(&ctx, nsid, zslba, action,
					  XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL, 0x0, dbuf,
					  dbuf_nbytes);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
			xnvme_buf_virt_free(report);
			xnvme_buf_free(dev, dbuf);
			errno = err ? -err : EIO;
			return NULL;
		}
		XNVME_DEBUG("INFO: hdr->nzones: %" PRIu64, hdr->nzones);

		if (nentries > hdr->nzones) {
			XNVME_DEBUG("ERR: invalid nentries");
			break;
		}

		// Skip the header and copy the remainder
		memcpy(report->storage + ((zslba - slba) / geo->nsect) * report->zrent_nbytes,
		       ((uint8_t *)dbuf) + sizeof(struct xnvme_spec_znd_report_hdr),
		       report->zrent_nbytes * nentries);

		zslba += nentries * geo->nsect;
	}

	xnvme_buf_free(dev, dbuf);

	return report;
}

int
xnvme_znd_report_get_descr(const struct xnvme_znd_report *report, uint32_t index,
			   struct xnvme_spec_znd_descr **descr, void **ext)
{
	if (index >= report->nentries) {
		XNVME_DEBUG("FAILED: index(%" PRu32 ") >= report->nentries(%zu)", index,
			    report->nentries);
		return -EINVAL;
	}
	if (ext && (!report->extended)) {
		XNVME_DEBUG("FAILED: gave 'ext' but report does not contain extensions.");
		return -EINVAL;
	}

	if (descr) {
		*descr = (struct xnvme_spec_znd_descr *)&(
			report->storage[index * report->zrent_nbytes]);
	}
	if (ext) {
		*ext = (void *)&report->storage[index * report->zrent_nbytes + report->zd_nbytes];
	}

	return 0;
}

int
xnvme_znd_report_find_arbitrary(const struct xnvme_znd_report *report,
				enum xnvme_spec_znd_state state, uint64_t *zlba, int opts)
{
	uint64_t arb;

	srand(opts ? opts : time(NULL));
	arb = rand() % UINT32_MAX;

	for (uint32_t ci = 0; ci < report->nentries; ++ci) {
		const uint64_t idx = (arb++) % ((uint64_t)report->nentries);
		struct xnvme_spec_znd_descr *zdescr;
		int err;

		err = xnvme_znd_report_get_descr(report, idx, &zdescr, NULL);
		if (err) {
			XNVME_DEBUG("xnvme_znd_report_get_descr(), err: %d", err);
			return err;
		}

		if ((zdescr->zs == state) && (zdescr->zt == XNVME_SPEC_ZND_TYPE_SEQWR) &&
		    (zdescr->zcap)) {
			*zlba = zdescr->zslba;
			return 0;
		}
	}

	return -ENXIO;
}

int
xnvme_znd_descr_from_dev(struct xnvme_dev *dev, uint64_t slba, struct xnvme_spec_znd_descr *zdescr)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_znd_report_hdr *hdr;
	size_t dbuf_nbytes;
	void *dbuf;
	int err;

	dbuf_nbytes = sizeof(*hdr) + sizeof(*zdescr);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		return -errno;
	}
	memset(dbuf, 0, dbuf_nbytes);

	err = xnvme_znd_mgmt_recv(&ctx, nsid, slba, XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT,
				  XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL, 0x0, dbuf, dbuf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	hdr = (void *)dbuf;
	if (!hdr->nzones) {
		XNVME_DEBUG("FAILED: hdr->nzones: %" PRIu64, hdr->nzones);
		err = -EIO;
		goto exit;
	}
	memcpy(zdescr, dbuf + sizeof(*hdr), sizeof(*zdescr));

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

int
xnvme_znd_descr_from_dev_in_state(struct xnvme_dev *dev, enum xnvme_spec_znd_state state,
				  struct xnvme_spec_znd_descr *zdescr)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_znd_report_hdr *hdr;
	size_t dbuf_nbytes;
	void *dbuf;
	uint8_t sfield;
	int err;

	switch (state) {
	case XNVME_SPEC_ZND_STATE_EMPTY:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY;
		break;
	case XNVME_SPEC_ZND_STATE_IOPEN:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN;
		break;
	case XNVME_SPEC_ZND_STATE_EOPEN:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN;
		break;
	case XNVME_SPEC_ZND_STATE_CLOSED:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED;
		break;
	case XNVME_SPEC_ZND_STATE_FULL:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL;
		break;
	case XNVME_SPEC_ZND_STATE_RONLY:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY;
		break;
	case XNVME_SPEC_ZND_STATE_OFFLINE:
		sfield = XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported state: %d", state);
		return -EINVAL;
	}

	dbuf_nbytes = sizeof(*hdr) + sizeof(*zdescr);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(%" PRIu64 ")", dbuf_nbytes);
		return -errno;
	}
	memset(dbuf, 0, dbuf_nbytes);

	err = xnvme_znd_mgmt_recv(&ctx, nsid, 0x0, XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT,
				  sfield, 0x1, dbuf, dbuf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	hdr = (void *)dbuf;
	if (!hdr->nzones) {
		XNVME_DEBUG("FAILED: hdr->nzones: %" PRIu64, hdr->nzones);
		err = -EIO;
		goto exit;
	}
	memcpy(zdescr, dbuf + sizeof(*hdr), sizeof(*zdescr));

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

int
xnvme_znd_stat(struct xnvme_dev *dev, enum xnvme_spec_znd_cmd_mgmt_recv_action_sf sfield,
	       uint64_t *nzones)
{
	const uint32_t nsid = xnvme_dev_get_nsid(dev);
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_znd_report_hdr *hdr;
	const size_t hdr_nbytes = sizeof(*hdr);
	int err;

	hdr = xnvme_buf_alloc(dev, hdr_nbytes);
	if (!hdr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		return -errno;
	}
	memset(hdr, 0, hdr_nbytes);

	err = xnvme_znd_mgmt_recv(&ctx, nsid, 0x0, XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT,
				  sfield, 0x0, hdr, hdr_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
		err = err ? err : -EIO;
		goto exit;
	}

	*nzones = hdr->nzones;

exit:
	xnvme_buf_free(dev, hdr);

	return err;
}

struct xnvme_spec_znd_log_changes *
xnvme_znd_log_changes_from_dev(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_znd_log_changes *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	int err;

	// Allocate buffer for the maximum possible amount of log entries
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc");
		return NULL;
	}

	// Retrieve Changed Zone Information List for namespace with nsid=1
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_ZND_CHANGES, 0, 0, xnvme_dev_get_nsid(dev), 0,
			    log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: xnvme_adm_log(XNVME_SPEC_LOG_TBD_ZCHG)");
		xnvme_buf_free(dev, log);
		return NULL;
	}

	return log;
}

int
xnvme_znd_mgmt_send(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t zslba, bool select_all,
		    enum xnvme_spec_znd_cmd_mgmt_send_action action,
		    enum xnvme_spec_znd_mgmt_send_action_so action_so, void *dbuf)
{
	uint32_t dbuf_nbytes = 0;

	ctx->cmd.common.opcode = XNVME_SPEC_ZND_OPC_MGMT_SEND;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.znd.mgmt_send.slba = zslba;
	ctx->cmd.znd.mgmt_send.select_all = select_all;
	ctx->cmd.znd.mgmt_send.zsa = action;
	ctx->cmd.znd.mgmt_send.zsaso = action_so;

	if (dbuf) {
		struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(ctx->dev);
		struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(ctx->dev);

		dbuf_nbytes = zns->lbafe[nvm->flbas.format].zdes * 64;
	}

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_znd_mgmt_recv(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
		    enum xnvme_spec_znd_cmd_mgmt_recv_action action,
		    enum xnvme_spec_znd_cmd_mgmt_recv_action_sf sf, uint8_t partial, void *dbuf,
		    uint32_t dbuf_nbytes)
{
	ctx->cmd.common.opcode = XNVME_SPEC_ZND_OPC_MGMT_RECV;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.znd.mgmt_recv.slba = slba;
	ctx->cmd.znd.mgmt_recv.ndwords = (dbuf_nbytes >> 2) - 1;
	ctx->cmd.znd.mgmt_recv.zra = action;
	ctx->cmd.znd.mgmt_recv.zrasf = sf;
	ctx->cmd.znd.mgmt_recv.partial = partial;

	return xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0);
}

int
xnvme_znd_append(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t zslba, uint16_t nlb,
		 const void *dbuf, const void *mbuf)
{
	void *cdbuf = (void *)dbuf;
	void *cmbuf = (void *)mbuf;

	size_t dbuf_nbytes = cdbuf ? ctx->dev->geo.lba_nbytes * (nlb + 1) : 0;
	size_t mbuf_nbytes = cmbuf ? ctx->dev->geo.nbytes_oob * (nlb + 1) : 0;

	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_ZND_OPC_APPEND;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.znd.append.zslba = zslba;
	ctx->cmd.znd.append.nlb = nlb;

	return xnvme_cmd_pass(ctx, cdbuf, dbuf_nbytes, cmbuf, mbuf_nbytes);
}

int
xnvme_znd_zrwa_flush(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t lba)
{
	ctx->cmd.common.opcode = XNVME_SPEC_ZND_OPC_MGMT_SEND;
	ctx->cmd.common.nsid = nsid;
	ctx->cmd.znd.mgmt_send.slba = lba;
	ctx->cmd.znd.mgmt_send.zsa = XNVME_SPEC_ZND_CMD_MGMT_SEND_FLUSH;

	return xnvme_cmd_pass(ctx, NULL, 0x0, NULL, 0x0);
}

int
xnvme_znd_report_fpr(FILE *stream, const struct xnvme_znd_report *report, int flags)
{
	int wrtn = 0;

	switch (flags) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(0x%x)", flags);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_znd_report:\n");
	if (!report) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "  report_nbytes: %" PRIu64 "\n", report->report_nbytes);
	wrtn += fprintf(stream, "  entries_nbytes: %" PRIu64 "\n", report->entries_nbytes);

	wrtn += fprintf(stream, "  zd_nbytes: %" PRIu32 "\n", report->zd_nbytes);
	wrtn += fprintf(stream, "  zdext_nbytes: %" PRIu32 "\n", report->zdext_nbytes);
	wrtn += fprintf(stream, "  zrent_nbytes: %" PRIu64 "\n", report->zrent_nbytes);

	wrtn += fprintf(stream, "  zslba: 0x%016" PRIx64 "\n", report->zslba);
	wrtn += fprintf(stream, "  zelba: 0x%016" PRIx64 "\n", report->zelba);

	wrtn += fprintf(stream, "  nzones: %" PRIu64 "\n", report->nzones);
	wrtn += fprintf(stream, "  nentries: %" PRIu32 "\n", report->nentries);
	wrtn += fprintf(stream, "  extended: %" PRIu8 "\n", report->extended);

	wrtn += fprintf(stream, "  entries:");

	if (!report->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (uint32_t idx = 0; idx < report->nentries; ++idx) {
		struct xnvme_spec_znd_descr *descr;
		int err;

		err = xnvme_znd_report_get_descr(report, idx, &descr, NULL);
		if (err) {
			XNVME_DEBUG("xnvme_znd_report_get_descr(), err: %d", err);
			return err;
		}

		wrtn += fprintf(stream, "    - {");
		wrtn += xnvme_spec_znd_descr_fpr_yaml(stream, descr, 0, ", ");
		wrtn += fprintf(stream, "}\n");
	}

	return wrtn;
}

int
xnvme_znd_report_pr(const struct xnvme_znd_report *report, int flags)
{
	return xnvme_znd_report_fpr(stdout, report, flags);
}

const struct xnvme_spec_znd_idfy_ctrlr *
xnvme_znd_dev_get_ctrlr(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_spec_znd_idfy_ctrlr *zctrlr;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		errno = EINVAL;
		return NULL;
	}

	zctrlr = (void *)xnvme_dev_get_ctrlr_css(dev);
	if (!zctrlr) {
		XNVME_DEBUG("FAILED: !xnvme_dev_get_ns()");
		return NULL;
	}

	return zctrlr;
}

const struct xnvme_spec_znd_idfy_ns *
xnvme_znd_dev_get_ns(struct xnvme_dev *dev)
{
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	struct xnvme_spec_znd_idfy_ns *zns;

	if (geo->type != XNVME_GEO_ZONED) {
		XNVME_DEBUG("FAILED: device is not zoned, got; %d", geo->type);
		errno = EINVAL;
		return NULL;
	}

	zns = (void *)xnvme_dev_get_ns_css(dev);
	if (!zns) {
		XNVME_DEBUG("FAILED: !xnvme_dev_get_ns_css()");
		return NULL;
	}

	return zns;
}

const struct xnvme_spec_znd_idfy_lbafe *
xnvme_znd_dev_get_lbafe(struct xnvme_dev *dev)
{
	const struct xnvme_spec_idfy_ns *ns;
	const struct xnvme_spec_znd_idfy_ns *zns;
	const struct xnvme_spec_znd_idfy_lbafe *lbafe;

	ns = xnvme_dev_get_ns(dev);
	if (!ns) {
		XNVME_DEBUG("FAILED: !xnvme_de_get_ns()");
		return NULL;
	}

	zns = xnvme_znd_dev_get_ns(dev);
	if (!zns) {
		XNVME_DEBUG("FAILED: !zns_get_ns()");
		return NULL;
	}

	lbafe = &zns->lbafe[ns->flbas.format];

	return lbafe;
}

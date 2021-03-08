#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_adm.h>
#include <libxnvme_nvm.h>
#include <libxnvme_znd.h>
#include <libxnvmec.h>

// TODO: Have this enumeration only show zoned namespaces
static int
cmd_enumerate(struct xnvmec *cli)
{
	struct xnvme_enumeration *listing = NULL;
	int err;

	xnvmec_pinf("xnvme_enumerate()");

	err = xnvme_enumerate(&listing, cli->args.sys_uri, cli->args.flags);
	if (err) {
		xnvmec_perr("xnvme_enumerate()", err);
		goto exit;
	}

	xnvme_enumeration_pr(listing, XNVME_PR_DEF);

exit:
	free(listing);

	return 0;
}

static int
cmd_info(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
cmd_idfy_ctrlr(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_ZONED;
	struct xnvme_spec_znd_idfy *idfy = NULL;
	int err;

	xnvmec_pinf("xnvme_adm_idfy_ctrlr: {nsid: 0x%x, csi: %s}", nsid,
		    xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, &idfy->base);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_idfy_ctrlr_csi()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_znd_idfy_ctrlr_pr(&idfy->zctrlr, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file((char *) idfy, sizeof(*idfy),
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}

static inline int
cmd_idfy_ns(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.dev);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_ZONED;
	struct xnvme_spec_znd_idfy *idfy = NULL;
	int err;

	xnvmec_pinf("xnvme_adm_idfy_ns: {nsid: 0x%x, csi: %s}", nsid,
		    xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ns_csi(&ctx, nsid, XNVME_SPEC_CSI_ZONED, &idfy->base);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_idfy_ns_csi()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_znd_idfy_ns_pr(&idfy->zns, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file((char *) idfy, sizeof(*idfy),
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}


// TODO: add support for dumping extension as well
static int
cmd_report(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	uint64_t zslba = cli->args.slba;
	uint64_t limit = cli->args.limit;

	struct xnvme_znd_report *report = NULL;
	int err;

	xnvmec_pinf("Zone Information Report for lba: 0x%016lx, limit: %zu",
		    zslba, limit ? limit : cli->args.geo->nzone);

	report = xnvme_znd_report_from_dev(dev, zslba, limit, 0);
	if (!report) {
		err = -errno;
		xnvmec_perr("xnvme_znd_report_from_dev()", err);
		goto exit;
	}

	xnvme_znd_report_pr(report, XNVME_PR_DEF);
	err = 0;

	if (cli->args.data_output) {
		xnvmec_pinf("dumping to: '%s'", cli->args.data_output);
		xnvmec_pinf("NOTE: log-header is omitted, only entries");
		err = xnvmec_buf_to_file((char *)report->storage,
					 report->report_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_virt_free(report);

	return err;
}

static int
cmd_changes(struct xnvmec *cli)
{
	struct xnvme_spec_znd_log_changes *changes = NULL;
	int err;

	xnvmec_pinf("Retrieving the Changed Zone List");

	changes = xnvme_znd_log_changes_from_dev(cli->args.dev);
	if (!changes) {
		err = -errno;
		xnvmec_perr("xnvme_znd_log_changes_from_dev()", err);
		goto exit;
	}

	xnvme_spec_znd_log_changes_pr(changes, XNVME_PR_DEF);
	err = 0;

exit:
	xnvme_buf_free(cli->args.dev, changes);

	return err;
}

static int
cmd_errors(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t nsid = cli->args.nsid;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;

	size_t nvalid = 0;
	int err;

	log_nentries = (uint32_t) xnvme_dev_get_ctrlr(cli->args.dev)->elpe + 1;
	log_nbytes = log_nentries * sizeof(*log);

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	log = xnvme_buf_alloc(cli->args.dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_ERRI, nsid, 0, 0, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_log(XNVME_SPEC_LOG_ERRI)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	for (size_t i = 0; i < log_nentries; ++i) {
		if (!log[i].ecnt) {
			break;
		}
		nvalid++;
	}

	xnvmec_pinf("Error-Information-Log has %zu valid entries", nvalid);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

exit:
	xnvme_buf_free(cli->args.dev, log);

	return err;
}

static int
cmd_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint8_t nsid = cli->args.nsid;

	void *dbuf = NULL, *mbuf = NULL;
	size_t dbuf_nbytes, mbuf_nbytes;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	dbuf_nbytes = (nlb + 1) * geo->lba_nbytes;
	mbuf_nbytes = geo->lba_extended ? 0 : (nlb + 1) * geo->nbytes_oob;

	xnvmec_pinf("Reading nsid: 0x%x, slba: 0x%016lx, nlb: %zu",
		    nsid, slba, nlb);

	xnvmec_pinf("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(dbuf, 0, dbuf_nbytes);

	if (mbuf_nbytes) {
		xnvmec_pinf("Alloc/clear mbuf, mbuf_nbytes: %zu", mbuf_nbytes);
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes);
		if (!mbuf) {
			err = -errno;
			xnvmec_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		memset(mbuf, 0, mbuf_nbytes);
	}

	xnvmec_pinf("Sending the command...");
	err = xnvme_nvm_read(&ctx, nsid, slba, nlb, dbuf, mbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_nvm_read()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	if (cli->args.data_output) {
		xnvmec_pinf("dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file(dbuf, dbuf_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

static int
cmd_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = cli->args.nsid;

	void *dbuf = NULL, *mbuf = NULL;
	size_t dbuf_nbytes, mbuf_nbytes;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	dbuf_nbytes = (nlb + 1) * geo->lba_nbytes;
	mbuf_nbytes = geo->lba_extended ? 0 : (nlb + 1) * geo->nbytes_oob;

	xnvmec_pinf("Writing nsid: 0x%x, slba: 0x%016lx, nlb: %zu",
		    nsid, slba, nlb);

	xnvmec_pinf("Alloc/fill dbuf, dbuf_nbytes: %zu", dbuf_nbytes);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(dbuf, dbuf_nbytes, cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	if (mbuf_nbytes) {
		xnvmec_pinf("Alloc/fill mbuf, mbuf_nbytes: %zu", mbuf_nbytes);
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes);
		if (!mbuf) {
			err = -errno;
			xnvmec_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		err = xnvmec_buf_fill(mbuf, mbuf_nbytes, "anum");
		if (err) {
			xnvmec_perr("xnvmec_buf_fill()", err);
			goto exit;
		}
	}

	xnvmec_pinf("Sending the command...");
	err = xnvme_nvm_write(&ctx, nsid, slba, nlb, dbuf, mbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_nvm_write()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

static int
cmd_append(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint64_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	uint16_t nlb = cli->args.nlb;

	size_t dbuf_nbytes = (size_t)cli->args.geo->nbytes * (size_t)(nlb + 1);
	char *dbuf = NULL;

	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Zone Append nlb: %d to zslba: 0x%016lx",
		    nlb, zslba);

	xnvmec_pinf("Allocating dbuf");
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	err = xnvme_znd_append(&ctx, nsid, zslba, nlb, dbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_znd_append()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("Appended to slba: 0x%016x", ctx.cpl.cdw0);

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
_cmd_mgmt(struct xnvmec *cli, uint8_t action)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	const struct xnvme_spec_znd_idfy_lbafe *lbafe = xnvme_znd_dev_get_lbafe(dev);
	int asf = 0;

	size_t dbuf_nbytes = lbafe ? lbafe->zdes : 0;
	char *dbuf = NULL;

	int err;

	if (cli->given[XNVMEC_OPT_ALL]) {
		asf = XNVME_SPEC_ZND_MGMT_SEND_ASF_SALL;
	}
	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("MGMT: zslba: 0x%016lx, action: 0x%x, str: %s", zslba,
		    action, xnvme_spec_znd_cmd_mgmt_send_action_str(action));

	if ((action == XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR) && cli->args.data_input) {
		xnvmec_pinf("Allocating dbuf");
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
		if (!dbuf) {
			err = -errno;
			xnvmec_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		err = xnvmec_buf_fill(dbuf, dbuf_nbytes, cli->args.data_input ? cli->args.data_input : "anum");
		if (err) {
			xnvmec_perr("xnvmec_buf_fill()", err);
			goto exit;
		}
	}

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, action, asf, dbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_znd_mgmt_send()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_mgmt(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, cli->args.action);
}

static int
cmd_mgmt_open(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN);
}

static int
cmd_mgmt_finish(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH);
}

static int
cmd_mgmt_close(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE);
}

static int
cmd_mgmt_reset(struct xnvmec *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET);
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
	{
		"enum", "Enumerate Zoned Namespaces on the system",
		"Enumerate Zoned Namespaces on the system", cmd_enumerate, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
			{XNVMEC_OPT_FLAGS, XNVMEC_LOPT},
		}
	},
	{
		"info", "Retrieve device info",
		"Retrieve device info", cmd_info, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy-ctrlr", "Zoned Command Set specific identify-controller",
		"Zoned Command Set specific identify-controller", cmd_idfy_ctrlr, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy-ns", "Zoned Command Set specific identify-controller",
		"Zoned Command Set specific identify-controller", cmd_idfy_ns, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"report", "Retrieve Zone Information",
		"Retrieve Zone Information", cmd_report, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_LIMIT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"changes", "Retrieve the Changed Zone list",
		"Retrieve the Changed Zone list", cmd_changes, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"errors", "Retrieve the Error-Log",
		"Retrieve the Error-Log", cmd_errors, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"read", "Execute a Read Command",
		"Execute a Read Command", cmd_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"write", "Execute a Write Command",
		"Execute a Write Command", cmd_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"append", "Execute an Append Command",
		"Execute an Append Command", cmd_append, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NLB, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"mgmt-open", "Open a Zone",
		"Open a Zone", cmd_mgmt_open, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-close", "Close a Zone",
		"Close a Zone", cmd_mgmt_close, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-finish", "Finish a Zone",
		"Finish a Zone", cmd_mgmt_finish, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt-reset", "Reset a Zone",
		"Reset a Zone", cmd_mgmt_reset, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
	{
		"mgmt", "Zone Management Send Command with custom action",
		"Zone Management Send Command with custom action",
		cmd_mgmt, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LREQ},
			{XNVMEC_OPT_ACTION, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_ALL, XNVMEC_LFLG},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Zoned Namespace Utility",
	.descr_short = "Enumerate Zoned Namespaces, manage, inspect properties, state, and send IO to them",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

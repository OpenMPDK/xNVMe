// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

int
enumerate_cb(struct xnvme_dev *dev, void *cb_args)
{
	uint32_t *ns_count_ref = cb_args;
	const struct xnvme_ident *ident;

	ident = xnvme_dev_get_ident(dev);
	if (ident->csi != XNVME_SPEC_CSI_ZONED) {
		return XNVME_ENUMERATE_DEV_CLOSE;
	}

	if (*ns_count_ref == 0) {
		fprintf(stdout, "\n");
	}

	fprintf(stdout, "  - {");
	xnvme_ident_yaml(stdout, ident, 0, ", ", 0);
	fprintf(stdout, "}\n");

	*ns_count_ref = *ns_count_ref + 1;

	return XNVME_ENUMERATE_DEV_CLOSE;
}

static int
cmd_enumerate(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err;

	err = xnvme_cli_to_opts(cli, &opts);
	if (err) {
		xnvme_cli_perr("xnvme_cli_to_opts()", err);
		return err;
	}

	fprintf(stdout, "xnvme_cli_enumeration:");

	err = xnvme_enumerate(cli->args.sys_uri, &opts, *enumerate_cb, &ns_count);
	if (err) {
		xnvme_cli_perr("xnvme_enumerate()", err);
		return err;
	}

	if (ns_count == 0) {
		fprintf(stdout, "~\n");
	}

	return 0;
}

static int
cmd_info(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
cmd_idfy_ctrlr(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_ZONED;
	struct xnvme_spec_znd_idfy *idfy = NULL;
	int err;

	xnvme_cli_pinf("xnvme_adm_idfy_ctrlr: {nsid: 0x%x, csi: %s}", nsid,
		       xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, &idfy->base);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy_ctrlr_csi()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_znd_idfy_ctrlr_pr(&idfy->zctrlr, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file((char *)idfy, sizeof(*idfy), cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}

static inline int
cmd_idfy_ns(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);
	enum xnvme_spec_csi csi = XNVME_SPEC_CSI_ZONED;
	struct xnvme_spec_znd_idfy *idfy = NULL;
	int err;

	xnvme_cli_pinf("xnvme_adm_idfy_ns: {nsid: 0x%x, csi: %s}", nsid, xnvme_spec_csi_str(csi));

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ns_csi(&ctx, nsid, XNVME_SPEC_CSI_ZONED, &idfy->base);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy_ns_csi()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_znd_idfy_ns_pr(&idfy->zns, XNVME_PR_DEF);

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file((char *)idfy, sizeof(*idfy), cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, idfy);

	return err;
}

// TODO: add support for dumping extension as well
static int
cmd_report(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	uint64_t zslba = cli->args.slba;
	uint64_t limit = cli->args.limit;

	struct xnvme_znd_report *report = NULL;
	int err;

	xnvme_cli_pinf("Zone Information Report for lba: 0x%016lx, limit: %zu", zslba,
		       limit ? limit : cli->args.geo->nzone);

	report = xnvme_znd_report_from_dev(dev, zslba, limit, 0);
	if (!report) {
		err = -errno;
		xnvme_cli_perr("xnvme_znd_report_from_dev()", err);
		goto exit;
	}

	xnvme_znd_report_pr(report, XNVME_PR_DEF);
	err = 0;

	if (cli->args.data_output) {
		xnvme_cli_pinf("dumping to: '%s'", cli->args.data_output);
		xnvme_cli_pinf("NOTE: log-header is omitted, only entries");
		err = xnvme_buf_to_file((char *)report->storage, report->report_nbytes,
					cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_virt_free(report);

	return err;
}

static int
cmd_changes(struct xnvme_cli *cli)
{
	const struct xnvme_spec_idfy_ctrlr *idfy_ctrlr = xnvme_dev_get_ctrlr_css(cli->args.device);
	struct xnvme_spec_znd_log_changes *changes = NULL;
	int err;

	xnvme_cli_pinf("Retrieving the Changed Zone List");

	if (!idfy_ctrlr->oaes.zone_changes) {
		xnvme_cli_pinf("Zoned Changes log page is an optional feature");
		xnvme_cli_pinf("This devices does not support it; expect failure");
	}

	changes = xnvme_znd_log_changes_from_dev(cli->args.device);
	if (!changes) {
		err = -errno;
		xnvme_cli_perr("xnvme_znd_log_changes_from_dev()", err);
		goto exit;
	}

	xnvme_spec_znd_log_changes_pr(changes, XNVME_PR_DEF);
	err = 0;

exit:
	xnvme_buf_free(cli->args.device, changes);

	return err;
}

static int
cmd_errors(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t nsid = cli->args.nsid;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;

	size_t nvalid = 0;
	int err;

	log_nentries = (uint32_t)xnvme_dev_get_ctrlr(cli->args.device)->elpe + 1;
	log_nbytes = log_nentries * sizeof(*log);

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	log = xnvme_buf_alloc(cli->args.device, log_nbytes);
	if (!log) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_ERRI, nsid, 0, 0, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_log(XNVME_SPEC_LOG_ERRI)", err);
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

	xnvme_cli_pinf("Error-Information-Log has %zu valid entries", nvalid);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

exit:
	xnvme_buf_free(cli->args.device, log);

	return err;
}

static int
cmd_read(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint8_t nsid = cli->args.nsid;

	void *dbuf = NULL, *mbuf = NULL;
	size_t dbuf_nbytes, mbuf_nbytes;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	dbuf_nbytes = (nlb + 1) * geo->lba_nbytes;
	mbuf_nbytes = geo->lba_extended ? 0 : (nlb + 1) * geo->nbytes_oob;

	xnvme_cli_pinf("Reading nsid: 0x%x, slba: 0x%016lx, nlb: %zu", nsid, slba, nlb);

	xnvme_cli_pinf("Alloc/clear dbuf, dbuf_nbytes: %zu", dbuf_nbytes);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(dbuf, 0, dbuf_nbytes);

	if (mbuf_nbytes) {
		xnvme_cli_pinf("Alloc/clear mbuf, mbuf_nbytes: %zu", mbuf_nbytes);
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes);
		if (!mbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		memset(mbuf, 0, mbuf_nbytes);
	}

	xnvme_cli_pinf("Sending the command...");
	err = xnvme_nvm_read(&ctx, nsid, slba, nlb, dbuf, mbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_read()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	if (cli->args.data_output) {
		xnvme_cli_pinf("dumping to: '%s'", cli->args.data_output);
		err = xnvme_buf_to_file(dbuf, dbuf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

static int
cmd_write(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = cli->args.nsid;

	void *dbuf = NULL, *mbuf = NULL;
	size_t dbuf_nbytes, mbuf_nbytes;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	dbuf_nbytes = (nlb + 1) * geo->lba_nbytes;
	mbuf_nbytes = geo->lba_extended ? 0 : (nlb + 1) * geo->nbytes_oob;

	xnvme_cli_pinf("Writing nsid: 0x%x, slba: 0x%016lx, nlb: %zu", nsid, slba, nlb);

	xnvme_cli_pinf("Alloc/fill dbuf, dbuf_nbytes: %zu", dbuf_nbytes);
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvme_buf_fill(dbuf, dbuf_nbytes,
			     cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	if (mbuf_nbytes) {
		xnvme_cli_pinf("Alloc/fill mbuf, mbuf_nbytes: %zu", mbuf_nbytes);
		mbuf = xnvme_buf_alloc(dev, mbuf_nbytes);
		if (!mbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		err = xnvme_buf_fill(mbuf, mbuf_nbytes, "anum");
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			goto exit;
		}
	}

	xnvme_cli_pinf("Sending the command...");
	err = xnvme_nvm_write(&ctx, nsid, slba, nlb, dbuf, mbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_nvm_write()", err);
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
cmd_append(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint64_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	uint16_t nlb = cli->args.nlb;

	size_t dbuf_nbytes = (size_t)cli->args.geo->nbytes * (size_t)(nlb + 1);
	char *dbuf = NULL;

	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	xnvme_cli_pinf("Zone Append nlb: %d to zslba: 0x%016lx", nlb, zslba);

	xnvme_cli_pinf("Allocating dbuf");
	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvme_buf_fill(dbuf, dbuf_nbytes, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	err = xnvme_znd_append(&ctx, nsid, zslba, nlb, dbuf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_znd_append()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("Appended to slba: 0x%016x", ctx.cpl.cdw0);

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
_cmd_mgmt(struct xnvme_cli *cli, uint8_t action)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	const uint64_t zslba = cli->args.slba;
	const struct xnvme_spec_znd_idfy_lbafe *lbafe = xnvme_znd_dev_get_lbafe(dev);
	bool select_all = false;

	size_t dbuf_nbytes = lbafe ? lbafe->zdes : 0;
	char *dbuf = NULL;

	int err;

	select_all = cli->given[XNVME_CLI_OPT_ALL] ? true : false;
	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	xnvme_cli_pinf("MGMT: zslba: 0x%016lx, action: 0x%x, str: %s", zslba, action,
		       xnvme_spec_znd_cmd_mgmt_send_action_str(action));

	if ((action == XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR) && cli->args.data_input) {
		xnvme_cli_pinf("Allocating dbuf");
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
		if (!dbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
		err = xnvme_buf_fill(dbuf, dbuf_nbytes,
				     cli->args.data_input ? cli->args.data_input : "anum");
		if (err) {
			xnvme_cli_perr("xnvme_buf_fill()", err);
			goto exit;
		}
	}

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, select_all, action, 0x0, dbuf);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_znd_mgmt_send()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
cmd_mgmt(struct xnvme_cli *cli)
{
	return _cmd_mgmt(cli, cli->args.action);
}

static int
cmd_mgmt_open(struct xnvme_cli *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN);
}

static int
cmd_mgmt_finish(struct xnvme_cli *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH);
}

static int
cmd_mgmt_close(struct xnvme_cli *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE);
}

static int
cmd_mgmt_reset(struct xnvme_cli *cli)
{
	return _cmd_mgmt(cli, XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET);
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {
	{
		"enum",
		"Enumerate Zoned Namespaces on the system",
		"Enumerate Zoned Namespaces on the system",
		cmd_enumerate,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"info",
		"Retrieve device info",
		"Retrieve device info",
		cmd_info,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy-ctrlr",
		"Zoned Command Set specific identify-controller",
		"Zoned Command Set specific identify-controller",
		cmd_idfy_ctrlr,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy-ns",
		"Zoned Command Set specific identify-namespace",
		"Zoned Command Set specific identify-namespace",
		cmd_idfy_ns,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"changes",
		"Retrieve the Changed Zone list",
		"Retrieve the Changed Zone list",
		cmd_changes,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"errors",
		"Retrieve the Error-Log",
		"Retrieve the Error-Log",
		cmd_errors,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"read",
		"Execute a Read Command",
		"Execute a Read Command",
		cmd_read,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"write",
		"Execute a Write Command",
		"Execute a Write Command",
		cmd_write,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"append",
		"Execute an Append Command",
		"Execute an Append Command",
		cmd_append,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mgmt-open",
		"Open a Zone",
		"Open a Zone",
		cmd_mgmt_open,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ALL, XNVME_CLI_LFLG},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mgmt-close",
		"Close a Zone",
		"Close a Zone",
		cmd_mgmt_close,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ALL, XNVME_CLI_LFLG},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mgmt-finish",
		"Finish a Zone",
		"Finish a Zone",
		cmd_mgmt_finish,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ALL, XNVME_CLI_LFLG},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mgmt-reset",
		"Reset a Zone",
		"Reset a Zone",
		cmd_mgmt_reset,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ALL, XNVME_CLI_LFLG},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"mgmt",
		"Zone Management Send Command with custom action",
		"Zone Management Send Command with custom action",
		cmd_mgmt,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_ACTION, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ALL, XNVME_CLI_LFLG},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"report",
		"Retrieve Zone Information",
		"Retrieve Zone Information",
		cmd_report,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_LIMIT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Zoned Namespace Utility",
	.descr_short = "Enumerate Zoned Namespaces, manage, inspect properties, state, and send "
		       "IO to them",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

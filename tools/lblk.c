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
	if (ident->csi != XNVME_SPEC_CSI_NVM) {
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
sub_enumerate(struct xnvme_cli *cli)
{
	struct xnvme_opts opts = {0};
	uint32_t ns_count = 0;
	int err = 0;

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
sub_info(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
sub_idfy(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_idfy *idfy = NULL;
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);
	int err;

	xnvme_cli_pinf("xnvme_adm_idfy: {nsid: 0x%x}", nsid);

	idfy = xnvme_buf_alloc(dev, sizeof(*idfy));
	if (!idfy) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_idfy_ns(&ctx, nsid, idfy);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_idfy()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_idfy_ns_pr(&idfy->ns, XNVME_PR_DEF);

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

static int
sub_read(struct xnvme_cli *cli)
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
		nsid = xnvme_dev_get_nsid(dev);
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
sub_write(struct xnvme_cli *cli)
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
sub_write_zeroes(struct xnvme_cli *cli)
{
	int err = 0;
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);

	err = xnvme_nvm_write_zeroes(&ctx, nsid, slba, nlb);
	if (err) {
		xnvme_cli_perr("xnvme_nvm_write_zeroes()", err);
		goto exit;
	}

exit:
	return err;
}

static int
sub_write_uncor(struct xnvme_cli *cli)
{
	int err = 0;
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);

	err = xnvme_nvm_write_uncorrectable(&ctx, nsid, slba, nlb);
	if (err) {
		xnvme_cli_perr("xnvme_nvm_write_uncorrectable()", err);
		goto exit;
	}

exit:
	return err;
}

static int
sub_dir_send(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);
	uint32_t doper = cli->args.doper;
	uint32_t dtype = cli->args.dtype;
	uint32_t dspec = cli->args.dspec;
	uint32_t endir = cli->args.endir;
	uint32_t tgtdir = cli->args.tgtdir;
	uint32_t cdw12 = 0;
	int err = -EINVAL;

	xnvme_cli_pinf(
		"cmd_dsend: {nsid: 0x%x, doper:0x%x, dtype: 0x%x, dspec: 0x%x, endir: 0x%x, "
		"tgtdir: 0x%x}",
		nsid, doper, dtype, dspec, endir, tgtdir);

	switch (dtype) {
	case XNVME_SPEC_DIR_IDENTIFY:
		if (doper == XNVME_SPEC_DSEND_IDFY_ENDIR) {
			cdw12 = (tgtdir << 8) | endir;
			break;
		} else {
			xnvme_cli_perr("invalid directive operation for identify directives", err);
			return err;
		}
	case XNVME_SPEC_DIR_STREAMS:
		if ((doper == XNVME_SPEC_DSEND_STREAMS_RELID) ||
		    (doper == XNVME_SPEC_DSEND_STREAMS_RELRS)) {
			break;
		} else {
			xnvme_cli_perr("invalid directive operation for stream directives", err);
			return err;
		}
	default:
		xnvme_cli_perr("invalid directive type", err);
		return err;
	}

	err = xnvme_adm_dir_send(&ctx, nsid, doper, dtype, dspec, cdw12);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_dsend()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_dir_receive(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = xnvme_dev_get_nsid(cli->args.device);
	uint32_t doper = cli->args.doper;
	uint32_t dtype = cli->args.dtype;
	uint32_t nsr = cli->args.nsr;
	uint32_t cdw12 = 0;
	int err = -EINVAL;
	size_t dbuf_nbytes = 0;
	void *dbuf = NULL;

	xnvme_cli_pinf("cmd_drecv: {nsid: 0x%x, doper:0x%x, dtype: 0x%x, nsr: 0x%x}", nsid, doper,
		       dtype, nsr);

	switch (dtype) {
	case XNVME_SPEC_DIR_IDENTIFY:
		if (doper == XNVME_SPEC_DRECV_IDFY_RETPR) {
			dbuf_nbytes = 4096;
			break;
		} else {
			xnvme_cli_perr("invalid directive operation for identify directive", err);
			return err;
		}
	case XNVME_SPEC_DIR_STREAMS:
		switch (doper) {
		case XNVME_SPEC_DRECV_STREAMS_RETPR:
			dbuf_nbytes = 32;
			break;
		case XNVME_SPEC_DRECV_STREAMS_GETST:
			dbuf_nbytes = 128 * 1024;
			break;
		case XNVME_SPEC_DRECV_STREAMS_ALLRS:
			cdw12 = nsr & 0xFFFF;
			break;
		default:
			xnvme_cli_perr("invalid directive operation for streams directive", err);
			return err;
		}
		break;
	default:
		xnvme_cli_perr("invalid directive type", err);
		return err;
	}

	if (dbuf_nbytes) {
		dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
		if (!dbuf) {
			err = -errno;
			xnvme_cli_perr("xnvme_buf_alloc()", err);
			goto exit;
		}
	}
	err = xnvme_adm_dir_recv(&ctx, nsid, doper, dtype, cdw12, dbuf, dbuf_nbytes);

	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_adm_drecv()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	switch (dtype) {
	case XNVME_SPEC_DIR_IDENTIFY:
		if (doper == XNVME_SPEC_DRECV_IDFY_RETPR) {
			xnvme_spec_drecv_idfy_pr((struct xnvme_spec_idfy_dir_rp *)dbuf,
						 XNVME_PR_DEF);
			break;
		} else {
			xnvme_cli_perr("invalid directive operation for identify directive", err);
			goto exit;
		}
	case XNVME_SPEC_DIR_STREAMS:
		switch (doper) {
		case XNVME_SPEC_DRECV_STREAMS_RETPR:
			xnvme_spec_drecv_srp_pr((struct xnvme_spec_streams_dir_rp *)dbuf,
						XNVME_PR_DEF);
			break;
		case XNVME_SPEC_DRECV_STREAMS_GETST:
			xnvme_spec_drecv_sgs_pr((struct xnvme_spec_streams_dir_gs *)dbuf,
						XNVME_PR_DEF);
			break;
		case XNVME_SPEC_DRECV_STREAMS_ALLRS: {
			struct xnvme_spec_alloc_resource ar = {.val = ctx.cpl.cdw0};
			xnvme_spec_drecv_sar_pr(ar, XNVME_PR_DEF);
		} break;
		default:
			xnvme_cli_perr("invalid directive operation for streams directive", err);
			goto exit;
		}
		break;
	default:
		xnvme_cli_perr("invalid directive type", err);
		goto exit;
	}

exit:
	xnvme_buf_free(dev, dbuf);
	return err;
}

static int
sub_write_directive(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	const struct xnvme_geo *geo = cli->args.geo;
	const uint64_t slba = cli->args.slba;
	const size_t nlb = cli->args.nlb;
	uint32_t nsid = cli->args.nsid;
	uint32_t dtype = cli->args.dtype;
	uint32_t dspec = cli->args.dspec;

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

	xnvme_cli_pinf("Preparing and sending the command...");
	xnvme_prep_nvm(&ctx, XNVME_SPEC_NVM_OPC_WRITE, nsid, slba, nlb);
	ctx.cmd.nvm.dtype = dtype;
	ctx.cmd.nvm.cdw13.dspec = dspec;

	err = xnvme_cmd_pass(&ctx, dbuf, dbuf_nbytes, mbuf, mbuf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_cmd_pass()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, mbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"enum",
		"Enumerate Logical Block Namespaces on the system",
		"Enumerate Logical Block Namespaces on the system",
		sub_enumerate,
		{
			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SYS_URI, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_FLAGS, XNVME_CLI_LOPT},

			XNVME_CLI_CORE_OPTS,
		},
	},
	{
		"info",
		"Retrieve derived information for the given URI",
		"Retrieve derived information for the given URI",
		sub_info,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"idfy",
		"Identify the namespace for the given URI",
		"Identify the namespace for the given URI",
		sub_idfy,
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
		"dir-send",
		"Directive send for the given URI",
		"Directive send for the given URI",
		sub_dir_send,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DSPEC, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DTYPE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DOPER, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_ENDIR, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_TGTDIR, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"dir-receive",
		"Directive receive for the given URI",
		"Directive receive for the given URI",
		sub_dir_receive,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DTYPE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DOPER, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NSR, XNVME_CLI_LOPT},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"read",
		"Read data and optionally metadata",
		"Read data and optionally metadata",
		sub_read,
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
		"Writes data and optionally metadata",
		"Writes data and optionally metadata",
		sub_write,
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
		"write-zeros",
		"Set a range of logical blocks to zero",
		"Set a range of logical blocks to zero",
		sub_write_zeroes,
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
		"write-uncor",
		"Mark a range of logical blocks as invalid",
		"Mark a range of logical blocks as invalid",
		sub_write_uncor,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
	{
		"write-dir",
		"Writes directive specific data and optionally metadata",
		"Writes directive specific data and optionally metadata",
		sub_write_directive,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LREQ},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_META_INPUT, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DTYPE, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DSPEC, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Logical Block Namespace Utility",
	.descr_short = "Logical Block Namespace Utility",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

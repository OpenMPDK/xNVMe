#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_adm.h>
#include <libxnvme_nvm.h>
#include <libxnvme_3p.h>
#include <libxnvmec.h>

static int
sub_listing(struct xnvmec *cli)
{
	struct xnvme_enumeration *listing = NULL;
	int err;

	xnvmec_pinf("xnvme_enumerate()");

	err = xnvme_enumerate(&listing, cli->args.sys_uri, cli->args.flags);
	if (err) {
		xnvmec_perr("xnvme_enumerate()", err);
		goto exit;
	}

	xnvme_enumeration_pp(listing, XNVME_PR_DEF);

exit:
	free(listing);

	return err;
}

static int
sub_enumerate(struct xnvmec *cli)
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

	return err;
}

static int
sub_info(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;

	xnvme_dev_pr(dev, XNVME_PR_DEF);

	return 0;
}

static inline int
_sub_idfy(struct xnvmec *cli, uint8_t cns, uint16_t cntid, uint8_t nsid,
	  uint16_t nvmsetid, uint8_t uuid)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_idfy *dbuf = NULL;
	size_t dbuf_nbytes = sizeof(*dbuf);
	int err;

	// Setup command
	ctx.cmd.common.opcode = XNVME_SPEC_ADM_OPC_IDFY;
	ctx.cmd.common.nsid = nsid;
	ctx.cmd.idfy.cns = cns;
	ctx.cmd.idfy.cntid = cntid;
	ctx.cmd.idfy.nvmsetid = nvmsetid;
	ctx.cmd.idfy.uuid = uuid;

	dbuf = xnvme_buf_alloc(dev, dbuf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_cmd_pass_admin(&ctx, dbuf, dbuf_nbytes, NULL, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_cmd_pass_admin()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	switch (cns) {
	case XNVME_SPEC_IDFY_NS:
		xnvme_spec_idfy_ns_pr(&dbuf->ns, XNVME_PR_DEF);
		break;

	case XNVME_SPEC_IDFY_CTRLR:
		xnvme_spec_idfy_ctrl_pr(&dbuf->ctrlr, XNVME_PR_DEF);
		break;

	case XNVME_SPEC_IDFY_IOCS:
		xnvme_spec_idfy_cs_pr(&dbuf->cs, XNVME_PR_DEF);
		break;

	default:
		xnvmec_pinf("No pretty-printer for the given "
			    "cns(0%x0)\n"
			    "Use -o to dump the result to file",
			    cns);
		break;
	}

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file((char *) dbuf, dbuf_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, dbuf);

	return err;
}

static int
sub_idfy(struct xnvmec *cli)
{
	uint8_t cns = cli->args.cns;
	uint64_t cntid = cli->args.cntid;
	uint32_t nsid = cli->args.nsid;
	uint16_t nvmsetid = cli->args.setid;
	uint8_t uuid = cli->args.uuid;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	return _sub_idfy(cli, cns, cntid, nsid, nvmsetid, uuid);
}

static int
sub_idfy_ns(struct xnvmec *cli)
{
	uint8_t nsid = cli->args.nsid;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	return _sub_idfy(cli, XNVME_SPEC_IDFY_NS, 0x0, nsid, 0x0, 0x0);
}

static int
sub_idfy_ctrlr(struct xnvmec *cli)
{
	return _sub_idfy(cli, XNVME_SPEC_IDFY_CTRLR, 0x0, 0x0, 0x0, 0x0);
}

static int
sub_idfy_cs(struct xnvmec *cli)
{
	return _sub_idfy(cli, XNVME_SPEC_IDFY_IOCS, 0xFFFF, 0x0, 0x0, 0x0);
}

static int
sub_log_health(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_health_entry *log = NULL;
	const size_t log_nbytes = sizeof(*log);
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(log, 0, log_nbytes);

	xnvmec_pinf("Retrieving SMART / health log page ...");
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_HEALTH, 0x0, 0, nsid, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_log(XNVME_SPEC_LOG_HEALTH)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_spec_log_health_pr(log, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log_erri(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t limit = cli->args.limit;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_log_erri_entry *log = NULL;
	uint32_t log_nentries = 0;
	uint32_t log_nbytes = 0;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	// NOTE: The Error Log Page Entries (elpe) is a zero-based value
	log_nentries = (uint32_t) xnvme_dev_get_ctrlr(dev)->elpe + 1;
	if (limit) {
		log_nentries = limit;
	}
	log_nbytes = log_nentries * sizeof(*log);

	xnvmec_pinf("Allocating and clearing buffer...");
	log = xnvme_buf_alloc(dev, log_nbytes);
	if (!log) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(log, 0, log_nbytes);

	xnvmec_pinf("Retrieving error-info-log ...");
	err = xnvme_adm_log(&ctx, XNVME_SPEC_LOG_ERRI, 0x0, 0x0, nsid, 0, log, log_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_log(XNVME_SPEC_LOG_ERRI)", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	printf("# %d error log page entries:\n", log_nentries);
	xnvme_spec_log_erri_pr(log, log_nentries, XNVME_PR_DEF);

exit:
	xnvme_buf_free(dev, log);

	return err;
}

static int
sub_log(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

	uint8_t lid = cli->args.lid;
	uint8_t lsp = cli->args.lsp;
	uint64_t lpo_nbytes = cli->args.lpo_nbytes;
	uint32_t nsid = cli->args.nsid;
	uint8_t rae = cli->args.reset;
	uint32_t buf_nbytes = cli->args.data_nbytes;

	void *buf = NULL;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("xnvme_adm_log: {lid: 0x%x, lsp: 0x%x, "
		    "lpo_nbytes: %zu, nsid: 0x%x, rae: %d}",
		    lid, lsp, lpo_nbytes, nsid, rae);

	if (!buf_nbytes) {
		err = -EINVAL;
		xnvmec_perr("!arg:data_nbytes", err);
		goto exit;
	}

	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	err = xnvme_adm_log(&ctx, lid, lsp, lpo_nbytes, nsid, rae, buf, buf_nbytes);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_log()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("No printer for log-pages");

	if (!cli->args.data_output) {
		xnvmec_pinf("Use -o to dump output to file");
	}

	// Generic buf-print and/or dump to file
	if (cli->args.data_output) {
		xnvmec_pinf("Dumping to: '%s'", cli->args.data_output);
		err = xnvmec_buf_to_file(buf, buf_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, buf);

	return err;
}

static int
sub_gfeat(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t fid = cli->args.fid;
	uint8_t sel = cli->args.sel;
	uint32_t nsid = cli->args.nsid;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("cmd_gfeat: {nsid: 0x%x, fid: 0x%x, sel: 0x%x}",
		    nsid, fid, sel);

	err = xnvme_adm_gfeat(&ctx, nsid, fid, sel, NULL, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_gfeat()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	{
		struct xnvme_spec_feat feat = { .val = ctx.cpl.cdw0 };
		xnvme_spec_feat_pr(fid, feat, XNVME_PR_DEF);
	}

exit:
	return 0;
}

static int
sub_sfeat(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t fid = cli->args.fid;
	uint32_t feat = cli->args.feat;
	uint8_t save = cli->args.save;
	uint32_t nsid = cli->args.nsid;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("cmd_sfeat: {nsid: 0%x, fid: 0x%x, feat: 0x%x, 0x%x}",
		    nsid, fid, feat, save);

	err = xnvme_adm_sfeat(&ctx, 0, fid, feat, 0, NULL, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_sfeat()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_format(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint32_t nsid = cli->args.nsid;
	uint8_t lbaf = cli->args.lbaf;
	uint8_t zf = cli->args.zf;
	uint8_t mset = cli->args.mset;
	uint8_t ses = cli->args.ses;
	uint8_t pi = cli->args.pi;
	uint8_t pil = cli->args.pil;
	int err;

	if (!cli->given[XNVMEC_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.dev);
	}

	xnvmec_pinf("xnvme_adm_format: {nsid: 0x%08x, lbaf: 0x%x, zf: 0x%x, "
		    "mset: 0x%x, ses: 0x%x, pi: 0x%x, pil: 0x%x}", nsid,
		    lbaf, zf, mset, ses, pi, pil);

	err = xnvme_adm_format(&ctx, nsid, lbaf, zf, mset, ses, pi, pil);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_adm_format()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_sanitize(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t sanact = cli->args.action;
	uint8_t ause = cli->args.ause;
	uint32_t ovrpat = cli->args.ovrpat;
	uint8_t owpass = cli->args.owpass;
	uint8_t oipbp = cli->args.oipbp;
	uint8_t nodas = cli->args.nodas;
	int err;

	xnvmec_pinf("xnvme_nvm_sanitize: {sanact: 0x%x, ause: 0x%x, "
		    "ovrpat: 0x%x, owpass: 0x%x, oipbp: 0x%x, "
		    "nodas: 0x%x}", sanact, ause, ovrpat, owpass, oipbp,
		    nodas);

	err = xnvme_nvm_sanitize(&ctx, sanact, ause, ovrpat, owpass, oipbp, nodas);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_nvm_sanitize()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
	}

	return err;
}

static int
sub_pass(struct xnvmec *cli, int admin)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	void *data_buf = NULL;
	size_t data_nbytes = cli->args.data_nbytes;
	void *meta_buf = NULL;
	size_t meta_nbytes = cli->args.meta_nbytes;
	int err;

	xnvmec_pinf("xnvme_cmd_pass(...)");

	err = xnvmec_buf_from_file(&ctx.cmd, sizeof(ctx.cmd), cli->args.cmd_input);
	if (err) {
		xnvmec_perr("xnvmec_buf_from_file()", err);
		xnvmec_pinf("Error reading: '%s'", cli->args.cmd_input);
		goto exit;
	}

	if (data_nbytes) {
		data_buf = xnvme_buf_alloc(dev, data_nbytes);
		if (!data_buf) {
			err = -errno;
			xnvmec_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		if (cli->args.data_input) {
			xnvmec_pinf("Reading data(%s)", cli->args.data_input);
			xnvmec_buf_from_file(data_buf, data_nbytes,
					     cli->args.data_input);
		}
	}

	if (meta_nbytes) {
		meta_buf = xnvme_buf_alloc(dev, meta_nbytes);
		if (!meta_buf) {
			err = -errno;
			xnvmec_perr("xnvme_buf_alloc()", err);
			goto exit;
		}

		if (cli->args.meta_input) {
			xnvmec_pinf("Reading meta(%s)", cli->args.meta_input);
			xnvmec_buf_from_file(meta_buf, meta_nbytes,
					     cli->args.meta_input);
		}
	}

	if (cli->args.verbose) {
		xnvme_spec_cmd_pr(&ctx.cmd, XNVME_PR_DEF);
	}

	if (admin) {
		err = xnvme_cmd_pass_admin(&ctx, data_buf, data_nbytes, meta_buf, meta_nbytes);
	} else {
		err = xnvme_cmd_pass(&ctx, data_buf, data_nbytes, meta_buf, meta_nbytes);
	}
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_cmd_pass[_admin]()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	if (data_nbytes && cli->args.data_output) {
		xnvmec_pinf("Dumping data(%s)", cli->args.data_output);
		err = xnvmec_buf_to_file(data_buf, data_nbytes,
					 cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}
	if (meta_nbytes && cli->args.meta_output) {
		xnvmec_pinf("Dumping meta(%s)", cli->args.meta_output);
		err = xnvmec_buf_to_file(meta_buf, meta_nbytes,
					 cli->args.meta_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, data_buf);
	xnvme_buf_free(dev, meta_buf);

	return err;
}

static int
sub_pioc(struct xnvmec *cli)
{
	return sub_pass(cli, 0);
}

static int
sub_padc(struct xnvmec *cli)
{
	return sub_pass(cli, 1);
}

static int
sub_library_info(struct xnvmec *XNVME_UNUSED(cli))
{
	struct xnvme_be_attr_list *list = NULL;
	int err;

	xnvmec_pinf("xNVMe Library Information");
	xnvme_ver_pr(XNVME_PR_DEF);

	printf("\n");
	xnvme_3p_ver_pr(xnvme_3p_ver, XNVME_PR_DEF);

	err = xnvme_be_attr_list_bundled(&list);
	if (err) {
		xnvmec_perr("xnvme_be_list()", err);
		goto exit;
	}

	xnvme_be_attr_list_pr(list, XNVME_PR_DEF);

	free(list);

exit:
	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"list", "List devices on the system",
		"List devices on the system", sub_listing, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
			{XNVMEC_OPT_FLAGS, XNVMEC_LOPT},
		}
	},
	{
		"enum", "Enumerate devices on the system",
		"Enumerate devices on the system", sub_enumerate, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_LOPT},
			{XNVMEC_OPT_FLAGS, XNVMEC_LOPT},
		}
	},
	{
		"info", "Retrieve derived information for given device",
		"Retrieve derived information for given device", sub_info, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy", "Execute an User-defined Identify Command",
		"Execute an User-defined Identify Command", sub_idfy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CNS, XNVMEC_LREQ},
			{XNVMEC_OPT_CNTID, XNVMEC_LOPT},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SETID, XNVMEC_LOPT},
			{XNVMEC_OPT_UUID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"idfy-ns", "Identify the given Namespace",
		"Identify the given Namespace", sub_idfy_ns, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"idfy-ctrlr", "Identify the given Controller",
		"Identify the given Controller", sub_idfy_ctrlr, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"idfy-cs", "Identify the Command Sets supported by the controller",
		"Identify the Command Sets supported by the controller", sub_idfy_cs, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log", "Retrieve a User-defined Log",
		"Retrieve and print log", sub_log, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_LID, XNVMEC_LREQ},
			{XNVMEC_OPT_LSP, XNVMEC_LOPT},
			{XNVMEC_OPT_LPO_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_RAE, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log-erri", "Retrieve the error-information log",
		"Retrieve and print log", sub_log_erri, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_LIMIT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"log-health", "Retrieve the S.M.A.R.T. / Health information log",
		"Retrieve and print log", sub_log_health, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"feature-get", "Execute a Get-Features Command",
		"Execute a Get Features Command", sub_gfeat, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_FID, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SEL, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},
	{
		"feature-set", "Execute a Set-Features Command",
		"Execute a Set-Features Command", sub_sfeat, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_FID, XNVMEC_LREQ},
			{XNVMEC_OPT_FEAT, XNVMEC_LREQ},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
			{XNVMEC_OPT_SAVE, XNVMEC_LFLG},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
	{
		"format", "Format a NVM namespace",
		"Format a NVM namespace", sub_format, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_NSID, XNVMEC_LOPT},
		}
	},
	{
		"sanitize", "Sanitize...", "Sanitize...", sub_sanitize, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"pioc", "Pass a used-defined IO Command through",
		"Pass a used-defined IO Command through", sub_pioc, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CMD_INPUT, XNVMEC_LREQ},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_NBYTES, XNVMEC_LOPT},
		}
	},
	{
		"padc", "Pass a user-defined ADmin Command through",
		"Pass a user-defined ADmin Command through", sub_padc, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CMD_INPUT, XNVMEC_LREQ},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LOPT},
			{XNVMEC_OPT_META_INPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_META_NBYTES, XNVMEC_LOPT},
		}
	},
	{
		"library-info", "Produce information about the library",
		"Produce information about the library", sub_library_info, {
			{ 0 }
		}
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe - Cross-platform NVMe utility",
	.descr_short = "Construct and execute NVMe Commands",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
kvs_io(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	uint8_t nsid = cli->args.dev_nsid;
	const void *kv_key;
	const void *kv_val;
	void *dbuf = NULL;
	void *rbuf = NULL;
	uint8_t kv_key_nbytes = 0;
	size_t kv_val_nbytes = 0;
	int err;

	if (cli->given[XNVME_CLI_OPT_KV_KEY]) {
		kv_key = cli->args.kv_key;
	} else {
		kv_key = "marco";
	}

	if (cli->given[XNVME_CLI_OPT_KV_VAL]) {
		kv_val = cli->args.kv_val;
	} else {
		kv_val = "polo";
	}

	kv_key_nbytes = strlen(kv_key);
	kv_val_nbytes = strlen(kv_val) + 1;

	if (!cli->given[XNVME_CLI_OPT_DEV_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}

	xnvme_cli_pinf("Alloc/fill dbuf, kv_val_nbytes: %zu", kv_val_nbytes);

	dbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}

	memcpy(dbuf, kv_val, kv_val_nbytes);

	err = xnvme_kvs_store(&ctx, nsid, kv_key, kv_key_nbytes, dbuf, kv_val_nbytes, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_kvs_store()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("KV Value is written under a key: \"%s\"\n", kv_key);

	rbuf = xnvme_buf_alloc(dev, kv_val_nbytes);
	if (!rbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(rbuf, 0, kv_val_nbytes);

	xnvme_cli_pinf("Sending xnvme_kvs_retrieve command");
	err = xnvme_kvs_retrieve(&ctx, nsid, kv_key, kv_key_nbytes, rbuf, kv_val_nbytes, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_perr("xnvme_kvs_retrieve()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("KV Value retrieved: '%s'", rbuf);

	xnvme_cli_pinf("Comparing wbuf and rbuf");
	if (xnvme_buf_diff(dbuf, rbuf, kv_val_nbytes)) {
		xnvme_buf_diff_pr(dbuf, rbuf, kv_val_nbytes, XNVME_PR_DEF);
		goto exit;
	}
exit:
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, rbuf);

	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{"kvs_io",
	 "Basic Verification of being able to read, what was written",
	 "Basic Verification of being able to read, what was written",
	 kvs_io,
	 {
		 {XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
		 {XNVME_CLI_OPT_URI, XNVME_CLI_POSA},
		 {XNVME_CLI_OPT_KV_KEY, XNVME_CLI_LOPT},
		 {XNVME_CLI_OPT_KV_VAL, XNVME_CLI_LOPT},
		 XNVME_CLI_SYNC_OPTS,
	 }},
};

static struct xnvme_cli g_cli = {
	.title = "Basic KV Verification",
	.descr_short = "Basic KV Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

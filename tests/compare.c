// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 *  Test COMPARE command
 *
 *  a) Allocate buffer
 *  b) Populate with contiguous string af chars
 *  c) Write payload
 *  d) Compare payload with original
 */
static int
test_compare(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid;
	struct xnvme_cmd_ctx ctx = {0};

	char *buf = NULL;
	uint64_t nlb, nbytes, slba;
	int err;

	switch (geo->type) {
	case XNVME_GEO_CONVENTIONAL:
	case XNVME_GEO_ZONED:
		break;
	default:
		XNVME_DEBUG("FAILED: not nvm / zns, got; %d", geo->type);
		return -EINVAL;
	}

	nlb = cli->args.nlb + 1; // NOTE nlb is zero-based

	nbytes = nlb * geo->lba_nbytes;
	if (nbytes > geo->mdts_nbytes) {
		xnvme_cli_pinf("nbytes > mdts_nbytes -> limiting nlb to mdts_nbytes / lba_nbytes");
		nlb = geo->mdts_nbytes / geo->lba_nbytes;
		nbytes = geo->mdts_nbytes;
	}

	slba = cli->given[XNVME_CLI_OPT_SLBA] ? cli->args.slba : 0x0;

	buf = xnvme_buf_alloc(dev, nbytes);
	if (!buf) {
		err = errno;
		XNVME_DEBUG("Failed: allocating write buffer");
		goto exit;
	}
	xnvme_buf_fill(buf, nbytes, "anum");

	// Write data to the buffer
	nsid = xnvme_dev_get_nsid(dev);
	ctx = xnvme_cmd_ctx_from_dev(dev);

	err = xnvme_nvm_write(&ctx, nsid, slba, (nlb - 1), buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_write(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	// Compare LBA's
	err = xnvme_nvm_compare(&ctx, nsid, slba, (nlb - 1), buf, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_nvm_compare(): "
			       "{err: 0x%x, slba: 0x%016lx}",
			       err, slba);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("xnvme_nvm_compare(): Succeeded!");

exit:
	xnvme_buf_free(dev, buf);
	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"compare",
		"Basic test of compare functionality",
		"Basic test of compare functionality",
		test_compare,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_NLB, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Basic Compare Test Verification",
	.descr_short = "Basic Compare Test Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

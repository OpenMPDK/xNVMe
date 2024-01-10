// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

struct cb_args {
	uint64_t zslba;
	uint32_t ecount_offset;
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_lbacheck(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;
	uint64_t expected = cb_args->zslba + cb_args->completed;

	cb_args->completed += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	if (ctx->cpl.result != expected) {
		xnvme_cli_pinf("ERR: cpl.result: 0x%016lx != 0x%016lx", ctx->cpl.result, expected);
		cb_args->ecount_offset += 1;
	}
}

static int
cmd_verify(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;
	struct xnvme_spec_znd_descr zone = {0};

	struct cb_args cb_args = {0};
	struct xnvme_queue *queue = NULL;

	size_t buf_nbytes;
	void *dbuf = NULL, *vbuf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}
	if (cli->args.clear) {
		err = -EINVAL;
		xnvme_cli_perr("This test does not support XNVME_CMD_SYNC", -err);
		return err;
	}

	// Find an empty zone
	err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
	if (err) {
		xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
		goto exit;
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	// Buffers for verification
	buf_nbytes = zone.zcap * geo->lba_nbytes;
	xnvme_cli_pinf("Allocating buffers...");
	dbuf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	vbuf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!vbuf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	xnvme_buf_fill(dbuf, buf_nbytes, "anum");
	xnvme_buf_fill(vbuf, buf_nbytes, "zero");

	xnvme_cli_pinf("Using XNVME_CMD_ASYNC mode");

	xnvme_cli_pinf("Initializing async. context + alloc/init requests");
	err = xnvme_queue_init(dev, 2, 0, &queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_init()", -err);
		goto exit;
	}

	xnvme_cli_timer_start(cli);

	cb_args.zslba = zone.zslba;

	for (uint64_t sect = 0; (sect < zone.zcap) && !cb_args.ecount; ++sect) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		ctx.async.queue = queue;
		ctx.async.cb = cb_lbacheck;
		ctx.async.cb_arg = &cb_args;

		err = xnvme_znd_append(&ctx, nsid, zone.zslba, 0, dbuf + sect * geo->lba_nbytes,
				       NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			break;

		case -EBUSY:
		case -EAGAIN:
			xnvme_cli_perr("is busy, should not be possible!", err);
			goto exit;

		default:
			xnvme_cli_perr("submission-error", EIO);
			goto exit;
		}

		err = xnvme_queue_drain(queue);
		if (err < 0) {
			xnvme_cli_perr("xnvme_queue_drain()", err);
			goto exit;
		}
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "Wall-clock", geo->nsect * geo->nbytes);

	if (cb_args.ecount) {
		err = -EIO;
		xnvme_cli_perr("got completion errors", err);
		goto exit;
	}
	if (cb_args.ecount_offset) {
		err = -EIO;
		xnvme_cli_perr("got offset error in completion-result", err);
		goto exit;
	}

	// Read zone content into vbuf
	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		err = xnvme_nvm_read(&ctx, nsid, zone.zslba + sect, 0,
				     vbuf + sect * geo->lba_nbytes, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	// Verify
	{
		size_t diff;

		diff = xnvme_buf_diff(dbuf, vbuf, buf_nbytes);
		if (diff) {
			xnvme_cli_pinf("verification failed, diff: %zu", diff);
			xnvme_buf_diff_pr(dbuf, vbuf, buf_nbytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

	xnvme_cli_pinf("LGTM");

exit:
	xnvme_cli_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}", cb_args.submitted,
		       cb_args.completed, cb_args.ecount);

	{
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvme_cli_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, vbuf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"verify",
		"Fills a Zone one LBA at a time, checking addr on completion",
		"Fills a Zone one LBA at a time, checking addr on completion",
		cmd_verify,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_CLEAR, XNVME_CLI_LFLG},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Tests for Zone Append via Async interfaces",
	.descr_short = "Tests for Zone Append via Async interfaces",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

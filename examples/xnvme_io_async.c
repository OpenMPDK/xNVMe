// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>
#include <libxnvme_nvm.h>
#include <libxnvme_lba.h>
#include <libxnvmec.h>
#include <libxnvme_util.h>
#include <time.h>

#define DEFAULT_QD 8

struct cb_args {
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_pool(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;

	cb_args->completed += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * This example shows how to do asynchronous reads
 *
 * - Allocate buffers for Command Payload
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit read commands to read a range of LBAs continously
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Dumping buffers to file
 * - Reporting on IO-errors
 * - Teardown
 */
static int
sub_async_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_lba_range rng = { 0 };
	uint32_t nsid, qd;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	int err;

	qd = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(dev);

	rng = xnvme_lba_range_from_slba_elba(dev, cli->args.slba, cli->args.elba);
	if (!rng.attr.is_valid) {
		err = -EINVAL;
		xnvmec_perr("invalid range", err);
		xnvme_lba_range_pr(&rng, XNVME_PR_DEF);
		return err;
	}

	xnvmec_pinf("Allocating and filling buf of nbytes: %zu", rng.nbytes);
	buf = xnvme_buf_alloc(dev, rng.nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, rng.nbytes, "zero");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing queue and setting default callback function and arguments");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvmec_pinf("Read uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_lba_range_pr(&rng, XNVME_PR_DEF);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t curs = 0; (curs < rng.naddrs) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_nvm_read(ctx, nsid, rng.slba + curs, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++curs;
		payload += geo->nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "wall-clock", rng.naddrs * geo->nbytes);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping nbytes: %zu, to: '%s'", rng.nbytes, cli->args.data_output);

		err = xnvmec_buf_to_file(buf, rng.nbytes, cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

/**
 * This example shows how to do asynchronous write
 *
 * - Allocate command-payload buffers
 * * - Setup a Command Queue
 *   | Using asynchronous command-contexts
 * - Submit write commands
 * - Reap their completion
 * - Teardown
 */
static int
sub_async_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_lba_range rng = { 0 };
	uint32_t nsid, qd;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	int err;

	qd = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);

	rng = xnvme_lba_range_from_slba_elba(dev, cli->args.slba, cli->args.elba);
	if (!rng.attr.is_valid) {
		err = -EINVAL;
		xnvmec_perr("invalid range", err);
		xnvme_lba_range_pr(&rng, XNVME_PR_DEF);
		return err;
	}

	xnvmec_pinf("Allocating and filling buf of nbytes: %zu", rng.nbytes);
	buf = xnvme_buf_alloc(dev, rng.nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, rng.nbytes,
			      cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing queue and setting default callback function and arguments");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvmec_pinf("Write uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_lba_range_pr(&rng, XNVME_PR_DEF);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t sect = 0; (sect < rng.naddrs) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_nvm_write(ctx, nsid, rng.slba + sect, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}

next:
		++sect;
		payload += geo->nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "wall-clock", rng.nbytes);

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
	{
		"read",
		"Read the LBAs [SLBA,ELBA]",
		"Read the LBAs [SLBA,ELBA]",
		sub_async_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
			{XNVMEC_OPT_SEED, XNVMEC_LOPT},
		}
	},

	{
		"write",
		"Write the LBAs [SLBA,ELBA]",
		"Write the LBAs [SLBA,ELBA]",
		sub_async_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_ELBA, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe: Asynchronous IO Example",
	.descr_short = "xNVMe: Asynchronous IO Example: write, read",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

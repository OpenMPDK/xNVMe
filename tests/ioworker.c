// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

#define QDEPTH_MAX 16
#define QDEPTH_DEF 4

struct iowork_stats {
	uint64_t nerrors;
	uint64_t ncompletions;
	uint64_t nsubmissions;
};

static void
iowork_stats_reset(struct iowork_stats *stats)
{
	stats->nerrors = stats->ncompletions = stats->nsubmissions = 0;
	return;
}

struct iowork_range {
	uint32_t slba;   ///< First LBA of the range
	uint32_t elba;   ///< Last LBA of the range
	uint64_t nbytes; ///< Number of bytes in range
	uint64_t naddr;  ///< Number of addresses (count-from-1)
	uint64_t nlb;    ///< Number of LBAs (count-from-0)
};

/**
 * Per I/O life-time objects, for pre-allocation
 */
struct ioworker {
	int id;
	struct iowork *work;

	int vectored;      ///< Whether or not the vectored I/O path is used
	struct iovec *vec; ///< Array of iovecs for worker to utilize
	int vec_cnt;

	struct iowork_range range; ///< Range for the worker to do I/O within
};

/**
 * Argument setup for I/O work
 */
struct iowork {
	const struct xnvme_geo *geo;
	struct xnvme_dev *dev;
	struct xnvme_queue *queue;
	uint32_t qdepth; ///< Intended queue-pressure / out-standing I/O
	uint32_t nsid;
	uint32_t opc;

	struct iowork_range range; ///< Range to confine I/O inside of

	struct {
		size_t nlb;    ///< Number of LBAs per I/O (count-from-0)
		size_t nbytes; ///< Number of bytes pr. I/O
		size_t naddr;  ///< Number of addresses per I/O (count-from-1)
	} io;                  ///< Per I/O args

	struct iowork_stats stats;

	char *wbuf;
	char *rbuf;

	struct {
		char *data;
		uint64_t slba;
	} cur;

	int vectored; ///< Whether or not the vectored I/O path is used
	int vec_cnt;

	uint64_t nio; ///< Number of I/Os to complete (count-from-1)

	struct ioworker workers[QDEPTH_MAX];
	uint32_t nworkers;
};

int
iowork_pp(struct iowork *work)
{
	int wrtn = 0;

	wrtn += printf("iowork:");
	if (!work) {
		wrtn += printf(" ~\n");
		return wrtn;
	}

	printf("\n");
	printf("  io.nbytes: %zu\n", work->io.nbytes);
	printf("  io.naddr: %zu\n", work->io.naddr);
	printf("  vectored: %d\n", work->vectored);
	printf("  nworkers: %" PRIu32 "\n", work->nworkers);
	printf("  range.nbytes: %" PRIu64 "\n", work->range.nbytes);
	printf("  range.naddr: %" PRIu64 "\n", work->range.naddr);
	printf("  range.slba: %" PRIu32 "\n", work->range.slba);
	printf("  range.elba: %" PRIu32 "\n", work->range.elba);
	printf("  nio: %" PRIu64 "\n", work->nio);

	return 0;
}

static int
_submit(struct iowork *work, struct ioworker *worker, struct xnvme_cmd_ctx *ctx)
{
	int err;
	if (work->vectored) {
		for (int j = 0; j < work->vec_cnt; ++j) {
			work->cur.slba = ctx->cmd.nvm.slba + j;
			worker->vec[j].iov_base =
				work->cur.data + work->cur.slba * work->geo->lba_nbytes;
			worker->vec[j].iov_len = work->geo->lba_nbytes;
		}

submitv:
		err = xnvme_cmd_passv(ctx, worker->vec, worker->vec_cnt, work->io.nbytes, NULL, 0,
				      0);
		switch (err) {
		case 0:
			work->stats.nsubmissions += 1;
			return 0;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(work->queue, 0);
			goto submitv;

		default:
			XNVME_DEBUG("FAILED: xnvme_cmd_passv(), err: %d", err);
			work->stats.nerrors += 1;
			xnvme_queue_put_cmd_ctx(work->queue, ctx);
			return err;
		}
	} else {
		work->cur.slba = ctx->cmd.nvm.slba;
		void *dbuf = work->cur.data + work->cur.slba * work->io.nbytes;
		size_t dbuf_nbytes = work->io.nbytes;
submit:
		err = xnvme_cmd_pass(ctx, dbuf, dbuf_nbytes, NULL, 0);
		switch (err) {
		case 0:
			work->stats.nsubmissions += 1;
			return 0;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(work->queue, 0);
			goto submit;

		default:
			XNVME_DEBUG("FAILED: xnvme_cmd_pass(), err: %d", err);
			work->stats.nerrors += 1;
			xnvme_queue_put_cmd_ctx(work->queue, ctx);
			return err;
		}
	}
	return 0;
}

static int
_submit_sync(struct iowork *work)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(work->dev);
	struct ioworker *worker = &work->workers[0];
	int err;
	ctx.cmd.common.nsid = work->nsid;
	ctx.cmd.common.opcode = work->opc;
	ctx.cmd.nvm.nlb = work->io.nlb;

	if (work->vectored) {
		for (uint64_t i = 0; i < work->nio; ++i) {
			for (int j = 0; j < work->vec_cnt; ++j) {
				work->cur.slba = work->range.slba + j + i * work->vec_cnt;
				worker->vec[j].iov_base =
					work->cur.data + work->cur.slba * work->geo->lba_nbytes;
				worker->vec[j].iov_len = work->geo->lba_nbytes;
			}

			ctx.cmd.nvm.slba = work->range.slba + i * work->vec_cnt;
			err = xnvme_cmd_passv(&ctx, worker->vec, worker->vec_cnt, work->io.nbytes,
					      NULL, 0, 0);
			if (err) {
				XNVME_DEBUG("FAILED: xnvme_cmd_passv(), err: %d", err);
				return err;
			} else {
				work->stats.nsubmissions += 1;
				work->stats.ncompletions += 1;
			}
		}
	} else {
		for (uint64_t i = 0; i < work->nio; ++i) {
			work->cur.slba = work->range.slba + i;
			ctx.cmd.nvm.slba = work->cur.slba;
			void *dbuf = work->cur.data + work->cur.slba * work->io.nbytes;
			size_t dbuf_nbytes = work->io.nbytes;
			err = xnvme_cmd_pass(&ctx, dbuf, dbuf_nbytes, NULL, 0);
			if (err) {
				XNVME_DEBUG("FAILED: xnvme_cmd_pass(), err: %d", err);
				return err;
			} else {
				work->stats.nsubmissions += 1;
				work->stats.ncompletions += 1;
			}
		}
	}

	return 0;
}

/**
 * On processing of command completion; on success, a new command is submitted, re-using the given
 * 'ctx'. On error, further submissions are aborted and the given 'ctx' yielded back to the queue.
 * Regardless of success/error then stats are updated.
 */
static void
on_completion(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct ioworker *worker = cb_arg;
	struct iowork *work = worker->work;

	work->stats.ncompletions += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cli_perr("on_completion()", errno);
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		goto error;
	}

	if (work->cur.slba >= work->range.elba) {
		xnvme_queue_put_cmd_ctx(work->queue, ctx);
		return;
	}

	// Recreate context
	ctx->cmd.common.nsid = work->nsid;
	ctx->cmd.common.opcode = work->opc;
	ctx->cmd.nvm.nlb = work->io.nlb;
	ctx->cmd.nvm.slba = ++(work->cur.slba);

	_submit(work, worker, ctx);

	return;

error:
	work->stats.nerrors += 1;
	xnvme_queue_put_cmd_ctx(work->queue, ctx);
}

/**
 * Tear down function for I/O worker arguments
 */
static int
iowork_teardown(struct iowork *work)
{
	int err;

	for (uint32_t i = 0; i < work->nworkers; ++i) {
		xnvme_buf_free(work->dev, work->workers[i].vec);
	}

	xnvme_buf_free(work->dev, work->rbuf);
	xnvme_buf_free(work->dev, work->wbuf);

	err = xnvme_queue_term(work->queue);
	if (err) {
		xnvme_cli_perr("xnvme_queue_term()", err);
	}

	return 0;
}

static int
iowork_from_cli(struct xnvme_cli *cli, struct iowork *work)
{
	int err;

	xnvme_buf_fill(work, sizeof(*work), "zero");

	work->dev = cli->args.device;
	work->nsid = xnvme_dev_get_nsid(work->dev);
	work->geo = xnvme_dev_get_geo(work->dev);
	work->qdepth = cli->given[XNVME_CLI_OPT_QDEPTH] ? cli->args.qdepth : QDEPTH_DEF;
	work->vectored = cli->given[XNVME_CLI_OPT_VEC_CNT] && cli->args.vec_cnt;
	work->vec_cnt = work->vectored ? cli->args.vec_cnt : 1;

	if (((work->qdepth) < 1) || (work->qdepth > QDEPTH_MAX)) {
		XNVME_DEBUG("FAILED: invalid, work->qdepth: %d", work->qdepth);
		return -EINVAL;
	}

	work->io.nbytes = work->vec_cnt * work->geo->lba_nbytes;
	work->io.naddr = work->vec_cnt;
	work->io.nlb = work->io.naddr - 1;

	// work->range.nbytes = 1 << 27;
	work->range.nbytes = 1 << 24;
	work->range.naddr = work->range.nbytes / work->geo->lba_nbytes;
	work->range.slba = 0;
	work->range.elba = work->range.slba + work->range.naddr - 1;

	work->nio = work->range.naddr / work->io.naddr;
	work->nworkers = work->nio > work->qdepth ? work->qdepth : work->nio;

	work->wbuf = xnvme_buf_alloc(cli->args.device, work->range.nbytes);
	if (!work->wbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(data), err: %d", errno);
		goto failed;
	}
	xnvme_buf_fill(work->wbuf, work->range.nbytes, "rand-t");

	work->rbuf = xnvme_buf_alloc(cli->args.device, work->range.nbytes);
	if (!work->rbuf) {
		err = -errno;
		XNVME_DEBUG("FAILED: xnvme_buf_alloc(buf), err: %d", errno);
		goto failed;
	}
	xnvme_buf_fill(work->rbuf, work->io.nbytes, "zero");

	err = xnvme_queue_init(work->dev, work->qdepth * 2, 0, &work->queue);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_queue_init(), err: %d", err);
		goto failed;
	}
	xnvme_queue_set_cb(work->queue, on_completion, work);

	for (uint32_t i = 0; i < work->nworkers; ++i) {
		struct ioworker *worker = &work->workers[i];

		worker->id = i;
		worker->work = work;
		worker->vec_cnt = work->vec_cnt;
		worker->vectored = work->vectored;

		worker->range = work->range;

		worker->vec_cnt = work->vec_cnt;
		worker->vec = xnvme_buf_alloc(work->dev, sizeof(*worker->vec) * worker->vec_cnt);
		if (!worker->vec) {
			err = -errno;
			XNVME_DEBUG("FAILED: xnvme_buf_alloc(vec), err: %d", errno);
			goto failed;
		}
		xnvme_buf_fill(worker->vec, sizeof(*worker->vec) * worker->vec_cnt, "zero");
	}

	return 0;

failed:
	iowork_teardown(work);
	return err;
}

static int
start_workers(struct iowork *work)
{
	for (uint32_t i = 0; i < work->nworkers; ++i) {
		int err;
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(work->queue);
		struct ioworker *worker = &work->workers[i];
		ctx->cmd.common.nsid = work->nsid;
		ctx->cmd.common.opcode = work->opc;
		ctx->cmd.nvm.nlb = work->io.nlb;
		ctx->cmd.nvm.slba = work->range.slba + i * work->vec_cnt;
		ctx->async.cb_arg = worker;
		err = _submit(work, worker, ctx);
		if (err) {
			return err;
		}
	}
	return 0;
}

static int
final(struct iowork work, int err)
{
	size_t diff;
	xnvme_cli_pinf("exit-stats: {submitted: %u, completed: %u, ecount: %u}",
		       work.stats.nsubmissions, work.stats.ncompletions, work.stats.nerrors);

	diff = xnvme_buf_diff(work.wbuf, work.rbuf, work.range.nbytes);
	iowork_teardown(&work);

	if (err || (work.stats.nerrors) || diff) {
		xnvme_cli_pinf("ERR: {err: %d, nerrs: %" PRIu64 ", diff: %" PRIu64 "}", err,
			       work.stats.nerrors, diff);
		return EIO;
	}

	return 0;
}

static int
test_verify(struct xnvme_cli *cli)
{
	struct iowork work;
	int err;

	err = iowork_from_cli(cli, &work);
	if (err) {
		XNVME_DEBUG("FAILED: iowork_from_cli(), err: %d", err);
		return err;
	}

	iowork_pp(&work);
	xnvme_dev_pr(work.dev, XNVME_PR_DEF);

	for (int i = 0; i < 2; i++) {

		iowork_stats_reset(&work.stats);
		work.cur.data = i ? work.rbuf : work.wbuf;
		work.opc = i ? XNVME_SPEC_NVM_OPC_READ : XNVME_SPEC_NVM_OPC_WRITE;

		// Fill queue with commands
		err = start_workers(&work);
		if (err) {
			xnvme_cli_perr("_submit()", err);
			return final(work, err);
		}

		// Process completions, re-submitting upon completion, until done
		while (((!work.stats.nerrors) && (work.stats.ncompletions < work.nio))) {
			int res = xnvme_queue_poke(work.queue, 0);
			if (res < 0) {
				err = res;
				xnvme_cli_perr("xnvme_queue_poke()", err);
				return final(work, err);
			}
		}
		xnvme_cli_pinf("opc: %d, stats: {submitted: %u, completed: %u, ecount: %u}",
			       work.opc, work.stats.nsubmissions, work.stats.ncompletions,
			       work.stats.nerrors);

		if (work.stats.ncompletions != work.nio) {
			xnvme_cli_pinf("FAILED: logic-error; completions != submissions");
			err = EIO;
			return final(work, err);
		}
	}

	return final(work, err);
}

static int
test_verify_sync(struct xnvme_cli *cli)
{
	struct iowork work;
	int err;

	err = iowork_from_cli(cli, &work);
	if (err) {
		XNVME_DEBUG("FAILED: iowork_from_cli(), err: %d", err);
		return err;
	}

	iowork_pp(&work);
	xnvme_dev_pr(work.dev, XNVME_PR_DEF);

	for (int i = 0; i < 2; i++) {

		iowork_stats_reset(&work.stats);
		work.cur.data = i ? work.rbuf : work.wbuf;
		work.opc = i ? XNVME_SPEC_NVM_OPC_READ : XNVME_SPEC_NVM_OPC_WRITE;

		err = _submit_sync(&work);
		if (err) {
			xnvme_cli_perr("_submit()", err);

			return final(work, err);
		}

		xnvme_cli_pinf("opc: %d, stats: {submitted: %u, completed: %u, "
			       "ecount: %u}",
			       work.opc, work.stats.nsubmissions, work.stats.ncompletions,
			       work.stats.nerrors);

		if (work.stats.ncompletions != work.nio) {
			xnvme_cli_pinf("FAILED: logic-error; completions != submissions");
			err = EIO;
			return final(work, err);
		}
	}

	return final(work, err);
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"verify",
		"Write, then read and compare",
		"Write, then read and compare",
		test_verify,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_VEC_CNT, XNVME_CLI_LOPT},

			XNVME_CLI_ASYNC_OPTS,
		},
	},
	{
		"verify-sync",
		"Write, then read and compare",
		"Write, then read and compare",
		test_verify_sync,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_VEC_CNT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe basic buffer alloc/free",
	.descr_short = "Test xNVMe basic buffer alloc/free",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

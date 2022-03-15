// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Ankit Kumar <ankit.kumar@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
#include <errno.h>
#include <liburing.h>
#include <libxnvme_spec_fs.h>
#include <xnvme_queue.h>
#include <xnvme_dev.h>
#include <xnvme_be_linux_liburing.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>
#include <linux/nvme_ioctl.h>

#define IORING_OP_URING_CMD 40

static int g_linux_liburing_optional[] = {
	IORING_OP_URING_CMD,
};
static int g_linux_liburing_noptional =
	sizeof g_linux_liburing_optional / sizeof(*g_linux_liburing_optional);

static int
_linux_liburing_noptional_missing(void)
{
	struct io_uring_probe *probe;
	int missing = 0;

	probe = io_uring_get_probe();
	if (!probe) {
		XNVME_DEBUG("FAILED: io_uring_get_probe()");
		return -ENOSYS;
	}

	for (int i = 0; i < g_linux_liburing_noptional; ++i) {
		if (!io_uring_opcode_supported(probe, g_linux_liburing_optional[i])) {
			missing += 1;
		}
	}

	free(probe);

	return missing;
}

int
xnvme_be_linux_ucmd_init(struct xnvme_queue *q, int opts)
{
	if (_linux_liburing_noptional_missing()) {
		fprintf(stderr, "# FAILED: io_uring cmd, not supported by kernel!\n");
		return -ENOSYS;
	}

	opts |= XNVME_QUEUE_SQE128; // setting the sqe to 128 bytes

	return xnvme_be_linux_liburing_init(q, opts);
}

int
xnvme_be_linux_ucmd_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_liburing *queue = (void *)q;
	struct io_uring_cqe *cqes[XNVME_QUEUE_IOU_CQE_BATCH_MAX];
	unsigned completed;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;
	max = max > XNVME_QUEUE_IOU_CQE_BATCH_MAX ? XNVME_QUEUE_IOU_CQE_BATCH_MAX : max;

	if (queue->poll_io) {
		int ret;

		ret = io_uring_wait_cqe(&queue->ring, &cqes[0]);
		if (ret) {
			XNVME_DEBUG("FAILED: foo");
			return ret;
		}
		completed = 1;
	} else {
		if (!queue->poll_sq) {
			int ret;

			ret = io_uring_wait_cqe_nr(&queue->ring, cqes, max);
			if (ret) {
				XNVME_DEBUG("FAILED: foo");
				return ret;
			}
		}
		completed = io_uring_peek_batch_cqe(&queue->ring, cqes, max);
	}
	for (unsigned i = 0; i < completed; ++i) {
		struct io_uring_cqe *cqe = cqes[i];
		struct xnvme_cmd_ctx *ctx;

		ctx = io_uring_cqe_get_data(cqe);
		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("cqe->user_data is NULL! => NO REQ!");
			XNVME_DEBUG("cqe->res: %d", cqe->res);
			XNVME_DEBUG("cqe->flags: %u", cqe->flags);
			return -EIO;
		}
#ifdef NVME_IOCTL_IO64_CMD
		xnvme_be_linux_nvme_map_cpl(ctx, NVME_IOCTL_IO64_CMD);
#else
		xnvme_be_linux_nvme_map_cpl(ctx, NVME_IOCTL_IO_CMD);
#endif
		ctx->cpl.result = cqe->res;
		if (cqe->res < 0) {
			ctx->cpl.result = 0;
			ctx->cpl.status.sc = -cqe->res;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		ctx->async.cb(ctx, ctx->async.cb_arg);
	};

	if (completed) {
		if (queue->poll_io) {
			io_uring_cqe_seen(&queue->ring, cqes[0]);
		} else {
			io_uring_cq_advance(&queue->ring, completed);
		}
		queue->base.outstanding -= completed;
	}

	return completed;
}

int
xnvme_be_linux_ucmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		       size_t mbuf_nbytes)
{
	struct xnvme_queue_liburing *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	struct io_uring_sqe *sqe = NULL;
	int err = 0;
	struct xnvme_spec_cmd_passthru *cmd;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	sqe = io_uring_get_sqe(&queue->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->opcode = IORING_OP_URING_CMD;
	sqe->off = NVME_IOCTL_IO64_CMD;
	sqe->addr = sizeof(struct xnvme_spec_cmd_passthru);
	sqe->flags = queue->poll_sq ? IOSQE_FIXED_FILE : 0;
	sqe->ioprio = 0;
	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	sqe->fd = queue->poll_sq ? 0 : state->fd;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)ctx;

	cmd = (struct xnvme_spec_cmd_passthru *)&sqe->__pad[0];

	memset(cmd, 0, sizeof(struct xnvme_spec_cmd_passthru));

	cmd->opcode = ctx->cmd.common.opcode;
	cmd->nsid = ctx->cmd.common.nsid;

	/* cdw10 and cdw11 represent starting slba*/
	cmd->cdw10 = ctx->cmd.nvm.slba & 0xffffffff;
	cmd->cdw11 = ctx->cmd.nvm.slba >> 32;

	/* cdw12 represent number of lba to be read*/
	cmd->cdw12 = ctx->cmd.nvm.nlb;
	cmd->data = (uint64_t)dbuf;
	cmd->data_len = dbuf_nbytes;

	err = io_uring_submit(&queue->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", cmd->opcode, err);
		return err;
	}

	queue->base.outstanding += 1;

	return 0;
}
#endif

struct xnvme_be_async g_xnvme_be_linux_async_ucmd = {
	.id = "io_uring_cmd",
#ifdef XNVME_BE_LINUX_LIBURING_ENABLED
	.cmd_io = xnvme_be_linux_ucmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_linux_ucmd_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_linux_ucmd_init,
	.term = xnvme_be_linux_liburing_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};

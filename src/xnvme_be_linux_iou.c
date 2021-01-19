// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifdef XNVME_BE_LINUX_IOU_ENABLED
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <linux/nvme_ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <paths.h>
#include <liburing.h>

#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_iou.h>
#include <xnvme_dev.h>

// TODO: replace this with liburing 0.7 barriers
#define _linux_iou_barrier()  __asm__ __volatile__("":::"memory")

static int g_linux_iou_required[] = {
	IORING_OP_READV,
	IORING_OP_WRITEV,
	IORING_OP_READ_FIXED,
	IORING_OP_WRITE_FIXED,
	IORING_OP_READ,
	IORING_OP_WRITE,
};
int g_linux_iou_nrequired = sizeof g_linux_iou_required / sizeof(*g_linux_iou_required);

/**
 * Check whether the Kernel supports the io_uring features used by xNVMe
 *
 * @return On success, 0 is returned. On error, negative errno is returned,
 * specifically -ENOSYS;
 */
int
_linux_iou_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	struct io_uring_probe *probe;
	int err = 0;

	probe = io_uring_get_probe();
	if (!probe) {
		XNVME_DEBUG("FAILED: io_uring_get_probe()");
		err = -ENOSYS;
		goto exit;
	}

	for (int i = 0; i < g_linux_iou_nrequired; ++i) {
		if (!io_uring_opcode_supported(probe, g_linux_iou_required[i])) {
			err = -ENOSYS;
			XNVME_DEBUG("FAILED: Kernel does not support opc: %d",
				    g_linux_iou_required[i]);
			goto exit;
		}
	}

exit:
	free(probe);

	return err ? 0 : 1;
}

int
_linux_iou_init(struct xnvme_queue *q, int opts)
{
	struct xnvme_queue_iou *queue = (void *)q;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	int err = 0;
	int iou_flags = 0;

	if ((opts & XNVME_QUEUE_SQPOLL) || (state->poll_sq)) {
		queue->poll_sq = 1;
	}
	if ((opts & XNVME_QUEUE_IOPOLL) || (state->poll_io)) {
		queue->poll_io = 1;
	}

	// NOTE: Disabling IOPOLL, to avoid lock-up, until fixed in `_poke`
	if (queue->poll_io) {
		printf("ENOSYS: IORING_SETUP_IOPOLL\n");
		queue->poll_io = 0;
	}
	XNVME_DEBUG("queue->poll_sq: %d", queue->poll_sq);
	XNVME_DEBUG("queue->poll_io: %d", queue->poll_io);

	//
	// Ring-initialization
	//
	if (queue->poll_sq) {
		iou_flags |= IORING_SETUP_SQPOLL;
	}
	if (queue->poll_io) {
		iou_flags |= IORING_SETUP_IOPOLL;
	}

	err = io_uring_queue_init(queue->base.capacity, &queue->ring, iou_flags);
	if (err) {
		XNVME_DEBUG("FAILED: io_uring_queue_init(), err: %d", err);
		return err;
	}

	if (queue->poll_sq) {
		err = io_uring_register_files(&queue->ring, &(state->fd), 1);
		if (err) {
			XNVME_DEBUG("FAILED: io_uring_register_files, err: %d", err);
			return err;
		}
	}

	return 0;
}

int
_linux_iou_term(struct xnvme_queue *q)
{
	struct xnvme_queue_iou *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	if (queue->poll_sq) {
		io_uring_unregister_files(&queue->ring);
	}
	io_uring_queue_exit(&queue->ring);

	return 0;
}

int
_linux_iou_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_iou *queue = (void *)q;
	struct io_uring_cqe *cqes[XNVME_QUEUE_IOU_CQE_BATCH_MAX];
	unsigned completed;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;
	max = max > XNVME_QUEUE_IOU_CQE_BATCH_MAX ? XNVME_QUEUE_IOU_CQE_BATCH_MAX : max;

	completed = io_uring_peek_batch_cqe(&queue->ring, cqes, max);
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

		// Map cqe-result to cmd_ctx-completion
		ctx->cpl.status.sc = cqe->res ? cqe->res : ctx->cpl.status.sc;
		if (ctx->cpl.status.sc) {
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		ctx->async.cb(ctx, ctx->async.cb_arg);
	};

	if (completed) {
		io_uring_cq_advance(&queue->ring, completed);
		queue->base.outstanding -= completed;
	}

	return completed;
}

int
_linux_iou_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_iou_poke(queue, 0);
		if (err >= 0) {
			acc += err;
			continue;
		}

		switch (err) {
		case -EAGAIN:
		case -EBUSY:
			nanosleep(&ts1, NULL);
			continue;

		default:
			return err;
		}
	}

	return acc;
}

int
_linux_iou_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		  size_t mbuf_nbytes)
{
	struct xnvme_queue_iou *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->ssw;
	struct io_uring_sqe *sqe = NULL;

	int opcode = IORING_OP_NOP;
	int err = 0;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		opcode = IORING_OP_WRITE;
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		opcode = IORING_OP_READ;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d for async",
			    ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	sqe = io_uring_get_sqe(&queue->ring);
	if (!sqe) {
		return -EAGAIN;
	}

	sqe->opcode = opcode;
	sqe->addr = (unsigned long) dbuf;
	sqe->len = dbuf_nbytes;
	sqe->off = ctx->cmd.nvm.slba << ssw;
	sqe->flags = queue->poll_sq ? IOSQE_FIXED_FILE : 0;
	sqe->ioprio = 0;
	// NOTE: we only ever register a single file, the raw device, so the
	// provided index will always be 0
	sqe->fd = queue->poll_sq ? 0 : state->fd;
	sqe->rw_flags = 0;
	sqe->user_data = (unsigned long)ctx;
	sqe->__pad2[0] = sqe->__pad2[1] = sqe->__pad2[2] = 0;

	err = io_uring_submit(&queue->ring);
	if (err < 0) {
		XNVME_DEBUG("io_uring_submit(%d), err: %d", ctx->cmd.common.opcode,
			    err);
		return err;
	}

	queue->base.outstanding += 1;

	return 0;
}

struct xnvme_be_async g_linux_iou = {
	.id = "io_uring",
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	.enabled = 1,
	.cmd_io = _linux_iou_cmd_io,
	.poke = _linux_iou_poke,
	.wait = _linux_iou_wait,
	.init = _linux_iou_init,
	.term = _linux_iou_term,
	.supported = _linux_iou_supported,
#else
	.enabled = 0,
	.cmd_io = xnvme_be_nosys_async_cmd_io,
	.poke = xnvme_be_nosys_async_poke,
	.wait = xnvme_be_nosys_async_wait,
	.init = xnvme_be_nosys_async_init,
	.term = xnvme_be_nosys_async_term,
	.supported = xnvme_be_nosys_async_supported,
#endif
};

#endif

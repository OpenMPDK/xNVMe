// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#ifdef XNVME_BE_LINUX_AIO_ENABLED
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
#include <libaio.h>

#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_aio.h>
#include <xnvme_dev.h>

int
_linux_aio_term(struct xnvme_queue *q)
{
	struct xnvme_queue_aio *queue = (void *)q;

	if (!queue) {
		XNVME_DEBUG("FAILED: queue: %p", (void *)queue);
		return -EINVAL;
	}

	io_destroy(queue->aio_ctx);
	free(queue->aio_events);

	return 0;
}

int
_linux_aio_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_aio *queue = (void *)q;
	int err = 0;

	queue->aio_ctx = 0;
	queue->aio_events = calloc(queue->base.capacity, sizeof(struct io_event));

	err = io_queue_init(queue->base.capacity, &queue->aio_ctx);
	if (err) {
		XNVME_DEBUG("FAILED: io_queue_init(), err: %d", err);
		return err;
	}

	return 0;
}

int
_linux_aio_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_aio *queue = (void *)q;
	int completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	completed = io_getevents(queue->aio_ctx, 0, max, queue->aio_events, NULL);
	if (completed < 0) {
		XNVME_DEBUG("FAILED: completed: %d, errno: %d", completed, errno);
		return completed;
	}

	for (int event = 0; event < completed; event++) {
		struct io_event *ev = &queue->aio_events[event];
		struct xnvme_cmd_ctx *ctx = (struct xnvme_cmd_ctx *)(uintptr_t)ev->data;

		if (!ctx) {
			XNVME_DEBUG("-{[THIS SHOULD NOT HAPPEN]}-");
			XNVME_DEBUG("event->data is NULL! => NO REQ!");
			XNVME_DEBUG("event->res: %ld", ev->res);
			XNVME_DEBUG("unprocessed events might remain");

			queue->base.outstanding -= 1;

			return -EIO;
		}

		ctx->cpl.result = ev->res;
		if (((int64_t)ev->res) < 0) {
			XNVME_DEBUG("FAILED: res: %lu, res2: %lu", ev->res, ev->res2);
			ctx->cpl.result = 0;
			ctx->cpl.status.sc =  -ev->res;
			ctx->cpl.status.sct = XNVME_STATUS_CODE_TYPE_VENDOR;
		}

		ctx->async.cb(ctx, ctx->async.cb_arg);
	}

	queue->base.outstanding -= completed;
	return completed;
}

int
_linux_aio_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_aio_poke(queue, 0);
		if (err >= 0) {
			acc += 1;
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
_linux_aio_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		  size_t mbuf_nbytes)
{
	struct xnvme_queue_aio *queue = (void *)ctx->async.queue;
	struct xnvme_be_linux_state *state = (void *)queue->base.dev->be.state;
	const uint64_t ssw = queue->base.dev->ssw;
	struct iocb *iocb = (void *)&ctx->cmd;
	int err;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}
	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOSYS;
	}

	///< Literally convert the NVMe command / sqe memory to an io-control-block
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		io_prep_pwrite(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		io_prep_pread(iocb, state->fd, dbuf, dbuf_nbytes, ctx->cmd.nvm.slba << ssw);
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	iocb->data = (unsigned long *)ctx;

	err = io_submit(queue->aio_ctx, 1, &iocb);
	if (err == 1) {
		ctx->async.queue->base.outstanding += 1;
		return 0;
	}

	XNVME_DEBUG("FAILED: io_submit(), err: %d", err);

	return err;
}

int
_linux_aio_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_async g_linux_aio = {
	.id = "libaio",
#ifdef XNVME_BE_LINUX_AIO_ENABLED
	.enabled = 1,
	.cmd_io = _linux_aio_cmd_io,
	.poke = _linux_aio_poke,
	.wait = _linux_aio_wait,
	.init = _linux_aio_init,
	.term = _linux_aio_term,
	.supported = _linux_aio_supported,
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

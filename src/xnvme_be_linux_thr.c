// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>

#define XNVME_BE_LINUX_THR_NAME "thr"

#ifdef XNVME_BE_LINUX_THR_ENABLED
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

#include <xnvme_queue.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_thr.h>
#include <xnvme_dev.h>

int
_qp_term(struct _qp *qp)
{
	free(qp);

	return 0;
}

int
_qp_alloc(struct _qp **qp, uint32_t capacity)
{
	const size_t nbytes = capacity * sizeof(*(*qp)->elm) + sizeof(**qp);

	(*qp) = malloc(nbytes);
	if (!(*qp)) {
		return -errno;
	}
	memset((*qp), 0, nbytes);

	STAILQ_INIT(&(*qp)->sq);
	STAILQ_INIT(&(*qp)->cq);
	STAILQ_INIT(&(*qp)->rp);

	(*qp)->capacity = capacity;

	return 0;
}

int
_qp_init(struct _qp *qp)
{
	for (uint32_t i = 0; i < qp->capacity; ++i) {
		STAILQ_INSERT_HEAD(&qp->rp, &qp->elm[i], link);
	}

	return 0;
}

int
_linux_thr_term(struct xnvme_queue *q)
{
	struct xnvme_queue_thr *queue = (void *)q;

	_qp_term(queue->qp);

	return 0;
}

/**
 * TODO: add a thread-pool consuming the submission-queue, and populating the
 * completion-queue, replacing the submission in _poke consumption of entries in
 * the completion-queue. This should provide better throughput as commands will
 * leave xNVMe and go to the device as new ones are submitted. Additional
 * threads could further improve as overlapping threads can hide the round-trip
 * latency as well as the sw-stack overhead.
 * This should probably be done with an SPMC or something clever.
*/
int
_linux_thr_init(struct xnvme_queue *q, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_thr *queue = (void *)q;

	if (_qp_alloc(&(queue->qp), queue->base.capacity)) {
		XNVME_DEBUG("FAILED: _qp_alloc()");
		goto failed;
	}
	if (_qp_init(queue->qp)) {
		XNVME_DEBUG("FAILED: _qp_init()");
		goto failed;
	}

	return 0;

failed:
	_linux_thr_term(q);

	return 1;
}

int
_linux_thr_poke(struct xnvme_queue *q, uint32_t max)
{
	struct xnvme_queue_thr *queue = (void *)q;
	struct _qp *qp = queue->qp;
	unsigned completed = 0;

	max = max ? max : queue->base.outstanding;
	max = max > queue->base.outstanding ? queue->base.outstanding : max;

	while (completed < max) {
		struct _entry *entry = STAILQ_FIRST(&qp->sq);
		int err;

		STAILQ_REMOVE_HEAD(&qp->sq, link);

		err = queue->base.dev->be.sync.cmd_io(entry->ctx, entry->dbuf, entry->dbuf_nbytes,
						      entry->mbuf, entry->mbuf_nbytes);
		if (err) {
			XNVME_DEBUG("FAILED: err: %d", err);
		}

		entry->ctx->async.cb(entry->ctx, entry->ctx->async.cb_arg);
		STAILQ_INSERT_TAIL(&qp->rp, entry, link);

		++completed;
	};

	queue->base.outstanding -= completed;

	return completed;
}

int
_linux_thr_wait(struct xnvme_queue *queue)
{
	int acc = 0;

	while (queue->base.outstanding) {
		struct timespec ts1 = {.tv_sec = 0, .tv_nsec = 1000};
		int err;

		err = _linux_thr_poke(queue, 0);
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

static inline int
_linux_thr_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
		  size_t mbuf_nbytes)
{
	struct xnvme_queue_thr *queue = (void *)ctx->async.queue;
	struct _qp *qp = queue->qp;
	struct _entry *entry;

	if (queue->base.outstanding == queue->base.capacity) {
		XNVME_DEBUG("FAILED: queue is full");
		return -EBUSY;
	}

	// Grab entry from rp and push into sq
	entry = STAILQ_FIRST(&qp->rp);
	if (!entry) {
		XNVME_DEBUG("FAILED: should not happen");
		return -EIO;
	}
	STAILQ_REMOVE_HEAD(&qp->rp, link);

	entry->dev = ctx->dev;
	entry->ctx = ctx;
	entry->dbuf = dbuf;
	entry->dbuf_nbytes = dbuf_nbytes;
	entry->mbuf = mbuf;
	entry->mbuf_nbytes = mbuf_nbytes;

	STAILQ_INSERT_TAIL(&qp->sq, entry, link);

	ctx->async.queue->base.outstanding += 1;

	return 0;
}

int
_linux_thr_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

struct xnvme_be_async g_linux_thr = {
	.id = "thr",
#ifdef XNVME_BE_LINUX_IOU_ENABLED
	.enabled = 1,
	.cmd_io = _linux_thr_cmd_io,
	.poke = _linux_thr_poke,
	.wait = _linux_thr_wait,
	.init = _linux_thr_init,
	.term = _linux_thr_term,
	.supported = _linux_thr_supported,
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

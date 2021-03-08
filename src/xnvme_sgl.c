// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <libxnvme.h>
#include <libxnvme_sgl.h>
#include <xnvme_dev.h>
#include <xnvme_sgl.h>

struct xnvme_sgl_pool *xnvme_sgl_pool_create(struct xnvme_dev *dev)
{
	struct xnvme_sgl_pool *pool = calloc(1, sizeof(*pool));
	if (!pool) {
		return NULL;
	}

	pool->dev = dev;

	return pool;
}

void
xnvme_sgl_pool_destroy(struct xnvme_sgl_pool *pool)
{
	struct xnvme_dev *dev = pool->dev;
	struct xnvme_sgl *sgl;

	while (!SLIST_EMPTY(&pool->free_list)) {
		sgl = SLIST_FIRST(&pool->free_list);
		SLIST_REMOVE_HEAD(&pool->free_list, free_list);
		xnvme_sgl_destroy(dev, sgl);
	}

	free(pool);
}

struct xnvme_sgl *xnvme_sgl_create(struct xnvme_dev *dev, int hint)
{
	size_t dsize = sizeof(struct xnvme_spec_sgl_descriptor);

	struct xnvme_sgl *sgl;
	if (NULL == (sgl = calloc(1, sizeof(*sgl)))) {
		return NULL;
	}

	if (hint) {
		sgl->descriptors = xnvme_buf_alloc(dev, hint * dsize);
		if (!sgl->descriptors) {
			free(sgl);
			return NULL;
		}
	}

	sgl->nalloc = hint;
	sgl->indirect = xnvme_buf_alloc(dev, dsize);

	return sgl;
}

struct xnvme_sgl *xnvme_sgl_alloc(struct xnvme_sgl_pool *pool)
{
	struct xnvme_sgl *sgl;

	if (NULL != (sgl = SLIST_FIRST(&pool->free_list))) {
		SLIST_REMOVE_HEAD(&pool->free_list, free_list);
		return sgl;
	}

	return xnvme_sgl_create(pool->dev, 1);
}

void
xnvme_sgl_destroy(struct xnvme_dev *dev, struct xnvme_sgl *sgl)
{
	xnvme_buf_free(dev, sgl->descriptors);
	xnvme_buf_free(dev, sgl->indirect);
	free(sgl);
}

void
xnvme_sgl_reset(struct xnvme_sgl *sgl)
{
	sgl->ndescr = 0;
	sgl->len = 0;
}

void
xnvme_sgl_free(struct xnvme_sgl_pool *pool, struct xnvme_sgl *sgl)
{
	sgl->ndescr = 0;
	sgl->len = 0;
	SLIST_INSERT_HEAD(&pool->free_list, sgl, free_list);
}

int
xnvme_sgl_add(struct xnvme_dev *dev, struct xnvme_sgl *sgl, void *buf,
	      size_t nbytes)
{
	struct xnvme_spec_sgl_descriptor *d;

	/**
	 * TODO(klaus.jensen): currently we only support one Last Segment
	 * descriptor (one 4K page).
	 */
	if (sgl->ndescr == 256) {
		XNVME_DEBUG("max 256 SGL descriptors supported");
		errno = EINVAL;
		return -1;
	}

	if (sgl->ndescr == sgl->nalloc) {
		size_t desr_nbytes;
		sgl->nalloc = sgl->nalloc ? 2 * sgl->nalloc : 1;
		desr_nbytes = sgl->nalloc * sizeof(struct xnvme_spec_sgl_descriptor);
		sgl->descriptors = xnvme_buf_realloc(dev, sgl->descriptors,
						     desr_nbytes);
		if (!sgl->descriptors) {
			return -1;
		}
	}

	d = &sgl->descriptors[sgl->ndescr];
	d->unkeyed.type = XNVME_SPEC_SGL_DESCR_TYPE_DATA_BLOCK;
	d->unkeyed.len = nbytes;

	uint64_t phys;

	if (xnvme_buf_vtophys(dev, buf, &phys)) {
		return -1;
	}

	d->addr = phys;

	sgl->len += nbytes;
	++sgl->ndescr;

	return 0;
}

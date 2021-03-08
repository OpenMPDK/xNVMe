// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_dev.h>
#include <xnvme_be.h>

void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes)
{
	if (!nbytes) {
		errno = EINVAL;
		return NULL;
	}
#ifdef WIN32
	return _aligned_malloc(nbytes, alignment);
#else
	return aligned_alloc(alignment, nbytes);
#endif
}

void *
xnvme_buf_virt_realloc(void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(alignment),
		       size_t XNVME_UNUSED(nbytes))
{
	errno = ENOSYS;
	return NULL;
}

void
xnvme_buf_virt_free(void *buf)
{
#ifdef WIN32
	_aligned_free(buf);
#else
	free(buf);
#endif
}

void *
xnvme_buf_phys_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys)
{
	return dev->be.mem.buf_alloc(dev, nbytes, phys);
}

void *
xnvme_buf_phys_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes,
		       uint64_t *phys)
{
	return dev->be.mem.buf_realloc(dev, buf, nbytes, phys);
}

void
xnvme_buf_phys_free(const struct xnvme_dev *dev, void *buf)
{
	dev->be.mem.buf_free(dev, buf);
}

int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys)
{
	return dev->be.mem.buf_vtophys(dev, buf, phys);
}

void *
xnvme_buf_alloc(const struct xnvme_dev *dev, size_t nbytes)
{
	return dev->be.mem.buf_alloc(dev, nbytes, NULL);
}

void *
xnvme_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes)
{
	return dev->be.mem.buf_realloc(dev, buf, nbytes, NULL);
}

void
xnvme_buf_free(const struct xnvme_dev *dev, void *buf)
{
	xnvme_buf_phys_free(dev, buf);
}


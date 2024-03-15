// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_NOSYS_H
#define __INTERNAL_XNVME_BE_NOSYS_H

#ifdef __cplusplus
extern "C" {
#endif

extern struct xnvme_be xnvme_be_nosys;

int
xnvme_be_nosys_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			   size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			    size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			      void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_cmd_pseudo(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_nosys_sync_supported(struct xnvme_dev *dev, uint32_t opts);

int
xnvme_be_nosys_queue_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			    size_t mbuf_nbytes);

int
xnvme_be_nosys_queue_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			     size_t dvec_nbytes, void *mbuf, size_t mbuf_nbytes);

int
xnvme_be_nosys_queue_poke(struct xnvme_queue *queue, uint32_t max);

int
xnvme_be_nosys_queue_wait(struct xnvme_queue *queue);

int
xnvme_be_nosys_queue_init(struct xnvme_queue *queue, int opts);

int
xnvme_be_nosys_queue_term(struct xnvme_queue *queue);

int
xnvme_be_nosys_queue_get_completion_fd(struct xnvme_queue *queue);

int
xnvme_be_nosys_queue_supported(struct xnvme_dev *dev, uint32_t opts);

void *
xnvme_be_nosys_buf_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

int
xnvme_be_nosys_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

void *
xnvme_be_nosys_buf_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys);

void
xnvme_be_nosys_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_nosys_mem_map(const struct xnvme_dev *dev, void *vaddr, size_t nbytes, uint64_t *phys);

int
xnvme_be_nosys_mem_unmap(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_nosys_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args);

int
xnvme_be_nosys_dev_open(struct xnvme_dev *dev);

void
xnvme_be_nosys_dev_close(struct xnvme_dev *dev);

#define XNVME_BE_NOSYS_ADMIN                                                  \
	{                                                                     \
		.cmd_admin  = xnvme_be_nosys_sync_cmd_admin,                  \
		.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo, .id = "ENOSYS", \
	}

#define XNVME_BE_NOSYS_SYNC                                                                   \
	{                                                                                     \
		.cmd_iov = xnvme_be_nosys_sync_cmd_iov, .cmd_io = xnvme_be_nosys_sync_cmd_io, \
		.id = "ENOSYS",                                                               \
	}

#define XNVME_BE_NOSYS_QUEUE                                                                 \
	{                                                                                    \
		.cmd_io = xnvme_be_nosys_queue_cmd_io, .poke = xnvme_be_nosys_queue_poke,    \
		.wait = xnvme_be_nosys_queue_wait, .init = xnvme_be_nosys_queue_init,        \
		.term              = xnvme_be_nosys_queue_term,                              \
		.get_completion_fd = xnvme_be_nosys_queue_get_completion_fd, .id = "ENOSYS", \
	}

#define XNVME_BE_NOSYS_MEM                                                                        \
	{                                                                                         \
		.buf_alloc = xnvme_be_nosys_buf_alloc, .buf_vtophys = xnvme_be_nosys_buf_vtophys, \
		.buf_realloc = xnvme_be_nosys_buf_realloc, .buf_free = xnvme_be_nosys_buf_free,   \
	}

#define XNVME_BE_NOSYS_DEV                                                                  \
	{                                                                                   \
		.enumerate = xnvme_be_nosys_enumerate, .dev_open = xnvme_be_nosys_dev_open, \
		.dev_close = xnvme_be_nosys_dev_close,                                      \
	}

#define XNVME_BE_NOSYS                                                      \
	{                                                                   \
		.admin = XNVME_BE_NOSYS_ADMIN, .sync = XNVME_BE_NOSYS_SYNC, \
		.async = XNVME_BE_NOSYS_QUEUE, .mem = XNVME_BE_NOSYS_MEM,   \
		.dev  = XNVME_BE_NOSYS_DEV,                                 \
		.attr = {                                                   \
			.name = "ENOSYS",                                   \
		},                                                          \
	}

#endif /* __INTERNAL_XNVME_BE_NOSYS_H */

#ifdef __cplusplus
}
#endif
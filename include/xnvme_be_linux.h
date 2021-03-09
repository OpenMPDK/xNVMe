// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_H
#define __INTERNAL_XNVME_BE_LINUX_H
#include <xnvme_dev.h>

#ifndef LINUX_BLOCK_SSW
#define LINUX_BLOCK_SSW 9
#endif

/**
 * Internal representation of options for Linux backends
 */
struct xnvme_be_linux_opts {
	int flags;
	int mode;
};

/**
 * Internal representation of XNVME_BE_LINUX state
 */
struct xnvme_be_linux_state {
	int fd;

	uint8_t pseudo;
	uint8_t poll_io;
	uint8_t poll_sq;

	uint8_t _rsvd[121];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_linux_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

void *
xnvme_be_linux_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
			 uint64_t *phys);

void *
xnvme_be_linux_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev),
			   void *XNVME_UNUSED(buf), size_t XNVME_UNUSED(nbytes),
			   uint64_t *XNVME_UNUSED(phys));

void
xnvme_be_linux_buf_free(const struct xnvme_dev *dev, void *buf);

int
xnvme_be_linux_buf_vtophys(const struct xnvme_dev *dev, void *buf,
			   uint64_t *phys);

int
xnvme_be_linux_cmd_pass(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			void *dbuf, size_t dbuf_nbytes, void *mbuf,
			size_t mbuf_nbytes, int opts, struct xnvme_cmd_ctx *req);

int
xnvme_be_linux_cmd_pass_admin(struct xnvme_dev *dev, struct xnvme_spec_cmd *cmd,
			      void *dbuf, size_t dbuf_nbytes, void *mbuf,
			      size_t mbuf_nbytes, int opts,
			      struct xnvme_cmd_ctx *req);

int
xnvme_be_linux_dev_from_ident(const struct xnvme_ident *ident,
			      struct xnvme_dev **dev);

void
xnvme_be_linux_dev_close(struct xnvme_dev *dev);

int
xnvme_be_linux_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			 int opts);

static inline uint64_t
xnvme_lba2off(struct xnvme_dev *dev, uint64_t lba)
{
	return lba << dev->ssw;
}

static inline uint64_t
xnvme_off2lba(struct xnvme_dev *dev, uint64_t off)
{
	return off >> dev->ssw;
}

int
xnvme_be_linux_dev_idfy(struct xnvme_dev *dev);

int
xnvme_be_linux_sysfs_dev_attr_to_buf(struct xnvme_dev *dev, const char *attr,
				     char *buf, int buf_len);

int
xnvme_be_linux_sysfs_dev_attr_to_num(struct xnvme_dev *dev, const char *attr,
				     uint64_t *val);

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts);

#endif /* __INTERNAL_XNVME_BE_LINUX */

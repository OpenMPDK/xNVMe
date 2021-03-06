// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_LINUX_NIL_H
#define __INTERNAL_XNVME_BE_LINUX_NIL_H
#define XNVME_BE_LINUX_NIL_CTX_DEPTH_MAX 29

struct xnvme_queue_nil {
	struct xnvme_queue_base base;

	struct xnvme_cmd_ctx *ctx[XNVME_BE_LINUX_NIL_CTX_DEPTH_MAX];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_queue_nil) == XNVME_BE_QUEUE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_LINUX_NIL */

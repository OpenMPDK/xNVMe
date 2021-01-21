// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_SPDK_H
#define __INTERNAL_XNVME_BE_SPDK_H
#include <pthread.h>
#include <spdk/stdinc.h>
#include <spdk/env.h>
#include <spdk/nvme.h>

#define XNVME_BE_SPDK_QPAIR_MAX 64
#define XNVME_BE_SPDK_ALIGN 0x1000

struct xnvme_queue_spdk {
	struct xnvme_queue_base base;

	struct spdk_nvme_qpair *qpair;

	uint8_t rsvd[224];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_queue_spdk) == XNVME_BE_QUEUE_STATE_NBYTES,
	"Incorrect size"
)

XNVME_STATIC_ASSERT(
	sizeof(struct spdk_nvme_ctrlr_data) == sizeof(struct xnvme_spec_idfy_ctrlr),
	"Incorrect size"
)
XNVME_STATIC_ASSERT(
	sizeof(struct spdk_nvme_ns_data) == sizeof(struct xnvme_spec_idfy_ns),
	"Incorrect size"
)

/**
 * Wrapping the SPDK controller with reference count
 */
struct xnvme_be_spdk_ctrlr_ref {
	struct spdk_nvme_ctrlr *ctrlr;	///< Pointer to attached controller
	int refcount;			///< # of refs. to 'ctrlr'
	char trgt[XNVME_IDENT_TRGT_LEN + 1];
};

struct xnvme_be_spdk_state {
	union {
		pthread_mutex_t qpair_lock;	///< LOCK for SYNC IO commands
		uint8_t _fill[64];
	};
	struct spdk_nvme_qpair *qpair;	///< QPAIR for SYNC IO commands

	struct spdk_nvme_ctrlr *ctrlr;	///< Pointer to attached controller
	struct spdk_nvme_ns *ns;	///< Pointer to associated namespace

	uint8_t attached;

	// Options
	uint8_t cmb_sqs;
	uint8_t css;
	uint8_t shm_id;
	uint8_t cmask;

	uint8_t _rsvd[35];
};
XNVME_STATIC_ASSERT(
	sizeof(struct xnvme_be_spdk_state) == XNVME_BE_STATE_NBYTES,
	"Incorrect size"
)

#endif /* __INTERNAL_XNVME_BE_SPDK */

// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_vfio.h>

/*
 * Wrapper with reference count for the vfio controller
 */
struct xnvme_be_vfio_ctrlr_ref {
	struct nvme_ctrl *ctrlr; ///< Pointer to attached controller
	int refcount;            ///< # of refs. to 'ctrlr'
	char uri[XNVME_IDENT_URI_LEN + 1];
};

#define XNVME_BE_VFIO_CREFS_LEN 100

static struct xnvme_be_vfio_ctrlr_ref g_cref[XNVME_BE_VFIO_CREFS_LEN];

/**
 * Retrieve an already opened controller with the given ident.
 *
 * @return Returns a pointer to the cref on success. When no cref is found,
 * NULL is returned.
 */
static struct nvme_ctrl *
_vfio_cref_lookup(struct xnvme_ident *id)
{
	for (int i = 0; i < XNVME_BE_VFIO_CREFS_LEN; ++i) {
		if (strncmp(g_cref[i].uri, id->uri, XNVME_IDENT_URI_LEN - 1)) {
			continue;
		}
		if (!g_cref[i].ctrlr) {
			XNVME_DEBUG("FAILED: corrupted ctrlr in cref");
			return NULL;
		}
		if (g_cref[i].refcount < 1) {
			XNVME_DEBUG("FAILED: corrupted refcount");
			return NULL;
		}

		g_cref[i].refcount += 1;

		return g_cref[i].ctrlr;
	}

	return NULL;
}

/**
 * Insert the given controller. It must be opened before insertion.
 *
 * @return Returns 0 on success. On error, negated ``errno`` is returned.
 */
static int
_vfio_cref_insert(struct xnvme_ident *ident, struct nvme_ctrl *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_VFIO_CREFS_LEN; ++i) {
		if (g_cref[i].refcount) {
			continue;
		}

		g_cref[i].ctrlr = ctrlr;
		g_cref[i].refcount += 1;
		strncpy(g_cref[i].uri, ident->uri, XNVME_IDENT_URI_LEN);

		return 0;
	}

	return -ENOMEM;
}

/**
 * Decrease the refcount by one for the given controller.
 *
 * @return Returns 0 on success. On error, negated ``errno`` is returned.
 *
 */
int
_vfio_cref_deref(struct nvme_ctrl *ctrlr)
{
	if (!ctrlr) {
		XNVME_DEBUG("FAILED: !ctrlr");
		return -EINVAL;
	}

	for (int i = 0; i < XNVME_BE_VFIO_CREFS_LEN; ++i) {
		if (g_cref[i].ctrlr != ctrlr) {
			continue;
		}

		if (g_cref[i].refcount < 1) {
			XNVME_DEBUG("FAILED: invalid refcount: %d", g_cref[i].refcount);
			return -EINVAL;
		}

		g_cref[i].refcount -= 1;

		if (g_cref[i].refcount == 0) {
			XNVME_DEBUG("INFO: refcount: %d => detaching", g_cref[i].refcount);
			nvme_close(ctrlr);
			memset(&g_cref[i], 0, sizeof(g_cref[i]));
		}

		return 0;
	}

	XNVME_DEBUG("FAILED: no tracking for %p", (void *)ctrlr);
	return -EINVAL;
}

int
xnvme_be_vfio_enumerate(const char *XNVME_UNUSED(sys_uri), struct xnvme_opts *XNVME_UNUSED(opts),
			xnvme_enumerate_cb XNVME_UNUSED(cb_func), void *XNVME_UNUSED(cb_args))
{
	XNVME_DEBUG("xnvme_be_vfio_enumerate()");
	return -ENOSYS;
}

int
_xnvme_be_vfio_create_ioqpair(struct xnvme_be_vfio_state *state, int qd, int flags)
{
	unsigned int qid = __builtin_ffsll(state->qidmap);
	int err;

	if (qid == 0) {
		XNVME_DEBUG("no free queue identifiers\n");
		return -EBUSY;
	}

	qid--;

	XNVME_DEBUG("nvme_create_ioqpair(%p, %d, %d, %d)", state->ctrl, qid, qd + 1, flags);

	// nvme queue capacity must be one larger than the requested capacity
	// since only n-1 slots in an NVMe queue may be used
	err = nvme_create_ioqpair(state->ctrl, qid, qd + 1, -1, flags);
	if (err) {
		return -errno;
	}

	state->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_vfio_delete_ioqpair(struct xnvme_be_vfio_state *state, unsigned int qid)
{
	if (nvme_delete_ioqpair(state->ctrl, qid)) {
		return -errno;
	}

	state->qidmap |= 1ULL << qid;

	return 0;
}

int
xnvme_be_vfio_dev_open(struct xnvme_dev *dev)
{
	static const uint16_t nqueues = 63;

	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	struct nvme_ctrl *ctrl;
	int qpid;
	int err;

	ctrl = _vfio_cref_lookup(&dev->ident);
	if (!ctrl) {
		struct nvme_ctrl_opts ctrl_opts = {
			.nsqr = nqueues,
			.ncqr = nqueues,
		};

		ctrl = calloc(1, sizeof(struct nvme_ctrl));
		if (!ctrl) {
			XNVME_DEBUG("FAILED: calloc()");
			return -ENOMEM;
		}
		err = nvme_init(ctrl, dev->ident.uri, &ctrl_opts);
		if (err) {
			XNVME_DEBUG("FAILED: initializing nvme device: %d", errno);
			return -errno;
		}

		state->ctrl = ctrl;
		state->qidmap = -1 & ~0x1;

		// create queues for sync backend requests
		qpid = _xnvme_be_vfio_create_ioqpair(state, 31, 0x0);
		if (qpid < 0) {
			XNVME_DEBUG("FAILED: initializing qpairs for sync IO: %d", err);
			return -errno;
		}
		state->sq_sync = &state->ctrl->sq[qpid];
		state->cq_sync = &state->ctrl->cq[qpid];

		// TODO: ctrl->config will contain the actual number of queues
		// created (zero-based)

		err = _vfio_cref_insert(&dev->ident, ctrl);
		if (err) {
			XNVME_DEBUG("FAILED: _cref_insert()");
			return err;
		}
	}

	dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy()");
		// _be_linux_state_term((void *)dev->be.state);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		return err;
	}

	return 0;
}

void
xnvme_be_vfio_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_vfio_state *state = (void *)dev->be.state;
	// TODO: this should work, but I'm getting errors from libvfio's
	// nvme_init when attempting to initialize the device again. Not sure
	// if the error is in my or libvfio's code.
	if (_vfio_cref_deref(state->ctrl)) {
		XNVME_DEBUG("FAILED: _vfio_cref_deref()");
	}
}

#endif

struct xnvme_be_dev g_xnvme_be_vfio_dev = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_VFIO_ENABLED
	.enumerate = xnvme_be_vfio_enumerate,
	.dev_open = xnvme_be_vfio_dev_open,
	.dev_close = xnvme_be_vfio_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};

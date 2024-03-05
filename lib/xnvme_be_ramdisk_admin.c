// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_RAMDISK_ENABLED
#include <errno.h>
#include <unistd.h>
#include <xnvme_be_ramdisk.h>
#include <xnvme_dev.h>

static int
_idfy_ctrlr_iocs_fs(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_fs_idfy_ctrlr *ctrlr = dbuf;

	ctrlr->caps.direct = 1;

	ctrlr->iosizes.min = 1;
	ctrlr->iosizes.max = 1024 * 1024 * 128;
	ctrlr->iosizes.opt = 1024 * 64;

	ctrlr->limits.file_data_size = 1;

	ctrlr->ac = 0xAC;
	ctrlr->dc = 0xDC;

	return 0;
}

static int
_idfy_ns_iocs_fs(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_fs_idfy_ns *ns = dbuf;

	size_t ramdisk_size = xnvme_be_ramdisk_dev_get_size(dev);
	if (!ramdisk_size) {
		return -1;
	}

	ns->nsze = ramdisk_size;
	ns->ncap = ramdisk_size;
	ns->nuse = ramdisk_size;

	ns->ac = 0xAC;
	ns->dc = 0xDC;

	return 0;
}

static int
_idfy_ctrlr(struct xnvme_dev *XNVME_UNUSED(dev), void *dbuf)
{
	struct xnvme_spec_idfy_ctrlr *ctrlr = dbuf;
	ctrlr->mdts = 0;
	ctrlr->oncs.copy = 1;
	ctrlr->oncs.write_zeroes = 1;

	ctrlr->cdfs.format0 = 1;
	return 0;
}

static int
_idfy_ns(struct xnvme_dev *dev, void *dbuf)
{
	struct xnvme_spec_idfy_ns *ns = dbuf;
	size_t ramdisk_size;
	const size_t lba_size = 512;

	ramdisk_size = xnvme_be_ramdisk_dev_get_size(dev);
	if (!ramdisk_size) {
		return -1;
	}

	ns->nsze = ramdisk_size / lba_size;
	ns->ncap = ramdisk_size / lba_size;
	ns->nuse = ramdisk_size / lba_size;

	ns->nlbaf = 0;        ///< This means that there is only one
	ns->flbas.format = 0; ///< using the first one

	ns->lbaf[0].ms = 0;
	ns->lbaf[0].ds = XNVME_ILOG2(lba_size);
	ns->lbaf[0].rp = 0;

	ns->mcl = 128;
	ns->mssrl = 128;
	ns->msrc = 127;

	return 0;
}

static int
_idfy(struct xnvme_cmd_ctx *ctx, void *dbuf)
{
	switch (ctx->cmd.idfy.cns) {
	case XNVME_SPEC_IDFY_NS:
		return _idfy_ns(ctx->dev, dbuf);

	case XNVME_SPEC_IDFY_CTRLR:
		return _idfy_ctrlr(ctx->dev, dbuf);

	case XNVME_SPEC_IDFY_NS_IOCS:
		switch (ctx->cmd.idfy.csi) {
		case XNVME_SPEC_CSI_FS:
			return _idfy_ns_iocs_fs(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	case XNVME_SPEC_IDFY_CTRLR_IOCS:
		switch (ctx->cmd.idfy.csi) {
		case XNVME_SPEC_CSI_FS:
			return _idfy_ctrlr_iocs_fs(ctx->dev, dbuf);

		default:
			break;
		}
		break;

	default:
		break;
	}

	/* ///< TODO: set some appropriate status-code for other idfy-cmds */
	/* ctx->cpl.status.sc = 0x3; */
	/* ctx->cpl.status.sct = 0x3; */
	return 1;
}

static int
_ramdisk_gfeat(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(dbuf))
{
	struct xnvme_spec_feat feat = {0};

	switch (ctx->cmd.gfeat.cdw10.fid) {
	case XNVME_SPEC_FEAT_NQUEUES:
		feat.nqueues.nsqa = 63;
		feat.nqueues.ncqa = 63;
		ctx->cpl.cdw0 = feat.val;
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported fid: %d", ctx->cmd.gfeat.cdw10.fid);
		return -ENOSYS;
	}

	return 0;
}

static int
_xnvme_be_ramdisk_admin_cmd_admin(struct xnvme_cmd_ctx *ctx, void *dbuf,
				  size_t XNVME_UNUSED(dbuf_nbytes), void *XNVME_UNUSED(mbuf),
				  size_t XNVME_UNUSED(mbuf_nbytes))
{
	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_ADM_OPC_IDFY:
		return _idfy(ctx, dbuf);

	case XNVME_SPEC_ADM_OPC_GFEAT:
		return _ramdisk_gfeat(ctx, dbuf);

	default:
		XNVME_DEBUG("FAILED: ENOSYS opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}
}
#endif

struct xnvme_be_admin g_xnvme_be_ramdisk_admin = {
	.id = "ramdisk_as_ns",
#ifdef XNVME_BE_RAMDISK_ENABLED
	.cmd_admin = _xnvme_be_ramdisk_admin_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
	.cmd_pseudo = xnvme_be_nosys_sync_cmd_pseudo,
#endif
};

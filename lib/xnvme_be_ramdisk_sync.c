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
#include <xnvme_dev.h>
#include <xnvme_be_ramdisk.h>

int
xnvme_be_ramdisk_sync_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf,
			     size_t mbuf_nbytes)
{
	struct xnvme_be_ramdisk_state *state = (void *)ctx->dev->be.state;
	struct xnvme_spec_nvm_scopy_fmt_zero *ranges = dbuf;
	size_t sdlba_offset = 0;
	const uint64_t ssw = ctx->dev->geo.ssw;
	char *offset = state->ramdisk;
	int err = 0;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		memcpy(offset + (ctx->cmd.nvm.slba << ssw), dbuf, dbuf_nbytes);
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		memcpy(dbuf, offset + (ctx->cmd.nvm.slba << ssw), dbuf_nbytes);
		break;

	case XNVME_SPEC_NVM_OPC_WRITE_ZEROES:
		memset(offset + (ctx->cmd.nvm.slba << ssw), 0,
		       (ctx->cmd.nvm.nlb + 1) * ctx->dev->geo.lba_nbytes);
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		memcpy(offset + ctx->cmd.nvm.slba, dbuf, dbuf_nbytes);
		break;

	case XNVME_SPEC_FS_OPC_READ:
		memcpy(dbuf, offset + ctx->cmd.nvm.slba, dbuf_nbytes);
		break;

	case XNVME_SPEC_NVM_OPC_SCOPY:

		for (int i = 0; i <= ctx->cmd.scopy.nr; i++) {
			char *dest = offset + sdlba_offset + (ctx->cmd.scopy.sdlba << ssw);
			char *src = offset + (ranges[i].slba << ssw);
			size_t nbytes = (ranges[i].nlb + 1) * ctx->dev->geo.lba_nbytes;
			memcpy(dest, src, nbytes);
			sdlba_offset += nbytes;
		}
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		break;

	case XNVME_SPEC_NVM_OPC_DATASET_MANAGEMENT:
		// Just pass on the command, as it is just a hint to the controller.
		break;

	case XNVME_SPEC_NVM_OPC_COMPARE:
		err = memcmp(offset + (ctx->cmd.nvm.slba << ssw), dbuf, dbuf_nbytes);
		if (err) {
			return -EIO;
		}
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	return err;
}

int
xnvme_be_ramdisk_sync_cmd_iov(struct xnvme_cmd_ctx *ctx, struct iovec *dvec, size_t dvec_cnt,
			      size_t XNVME_UNUSED(dvec_nbytes), void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_be_ramdisk_state *state = (void *)ctx->dev->be.state;
	const uint64_t ssw = ctx->dev->geo.ssw;
	char *offset = state->ramdisk;

	if (mbuf || mbuf_nbytes) {
		XNVME_DEBUG("FAILED: mbuf or mbuf_nbytes provided");
		return -ENOTSUP;
	}

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_WRITE:
		for (size_t i = 0; i < dvec_cnt; ++i) {
			memcpy(offset + (ctx->cmd.nvm.slba << ssw), dvec[i].iov_base,
			       dvec[i].iov_len);
			offset += dvec[i].iov_len;
		}
		break;

	case XNVME_SPEC_NVM_OPC_READ:
		for (size_t i = 0; i < dvec_cnt; ++i) {
			memcpy(dvec[i].iov_base, offset + (ctx->cmd.nvm.slba << ssw),
			       dvec[i].iov_len);
			offset += dvec[i].iov_len;
		}
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		for (size_t i = 0; i < dvec_cnt; ++i) {
			memcpy(offset + ctx->cmd.nvm.slba, dvec[i].iov_base, dvec[i].iov_len);
			offset += dvec[i].iov_len;
		}
		break;

	case XNVME_SPEC_FS_OPC_READ:
		for (size_t i = 0; i < dvec_cnt; ++i) {
			memcpy(dvec[i].iov_base, offset + ctx->cmd.nvm.slba, dvec[i].iov_len);
			offset += dvec[i].iov_len;
		}
		break;

	case XNVME_SPEC_NVM_OPC_FLUSH:
	case XNVME_SPEC_FS_OPC_FLUSH:
		break;

	default:
		XNVME_DEBUG("FAILED: nosys opcode: %d", ctx->cmd.common.opcode);
		return -ENOSYS;
	}

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_ramdisk_sync = {
	.id = "ramdisk",
#ifdef XNVME_BE_RAMDISK_ENABLED
	.cmd_io = xnvme_be_ramdisk_sync_cmd_io,
	.cmd_iov = xnvme_be_ramdisk_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};

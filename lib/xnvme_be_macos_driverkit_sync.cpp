// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
#include <tuple>
#include <iostream>
extern "C" {
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <xnvme_dev.h>
#include <xnvme_be_macos_driverkit.h>

static int
cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes, void *mbuf, size_t mbuf_nbytes)
{
	NvmeSubmitCmd cmd = {0};
	NvmeSubmitCmd cmd_return = {0};
	uint64_t token;
	uint64_t _buf_base_offset;
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;
	cmd.queue_id = state->qid_sync;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_READ:
		break;

	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	if (dbuf){
		std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) dbuf);
		cmd.dbuf_nbytes = dbuf_nbytes;
		cmd.dbuf_offset = _buf_base_offset;
		cmd.dbuf_token = token;
	}
	if (mbuf){
		std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) mbuf);
		cmd.mbuf_nbytes = mbuf_nbytes;
		cmd.mbuf_offset = _buf_base_offset;
		cmd.mbuf_token = token;
	}
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));

	size_t output_cnt = sizeof(NvmeSubmitCmd);
	kern_return_t ret = IOConnectCallStructMethod(state->device_connection, NVME_ONESHOT, &cmd, sizeof(NvmeSubmitCmd), &cmd_return, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("xnvme_be_macos_driverkit_sync_io failed with error");
		return ret;
	}
	memcpy(cmd.cpl, &ctx->cpl, sizeof(struct xnvme_spec_cpl));

	return 0;
}
#endif

struct xnvme_be_sync g_xnvme_be_macos_driverkit_sync = {
	.id = "driverkit",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_io = cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#else
	.cmd_io = xnvme_be_nosys_sync_cmd_io,
	.cmd_iov = xnvme_be_nosys_sync_cmd_iov,
#endif
};
}
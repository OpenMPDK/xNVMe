// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
extern "C" {

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>

#include <xnvme_dev.h>
#include <xnvme_be_macos_driverkit.h>

std::map<uint64_t, std::tuple<uint64_t, uint64_t>> address_mapping;

int
xnvme_be_macos_driverkit_admin(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
			       void *XNVME_UNUSED(mbuf), size_t XNVME_UNUSED(mbuf_nbytes))
{
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;

	uint64_t token;
	uint64_t _buf_base_offset;
	std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) dbuf);
	NvmeSubmitCmd cmd = {
		.queue_id = 0,
		.dbuf_nbytes = dbuf_nbytes,
		.dbuf_offset = _buf_base_offset,
		.dbuf_token = token,
	};
	NvmeSubmitCmd cmd_return;
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));
	cmd.backend_opaque = (uint64_t)ctx;

	size_t output_cnt = sizeof(NvmeSubmitCmd);
	kern_return_t ret = IOConnectCallStructMethod(state->device_connection, NVME_ONESHOT, &cmd, sizeof(NvmeSubmitCmd), &cmd_return, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("xnvme_be_macos_driverkit_admin failed with error: 0x%08x, %s.\n", ret,
			    mach_error_string(ret));
		return ret;
	}

	memcpy(&ctx->cpl, cmd_return.cpl, sizeof(struct xnvme_spec_cpl));
	return 0;
}

#endif

struct xnvme_be_admin g_xnvme_be_macos_driverkit_admin = {
	.id = "driverkit",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_admin = xnvme_be_macos_driverkit_admin,
#else
	.cmd_admin = xnvme_be_nosys_sync_cmd_admin,
#endif
};

}
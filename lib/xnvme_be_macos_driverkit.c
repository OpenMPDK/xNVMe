// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED

#include <xnvme_be_cbi.h>
#include <xnvme_be_macos_driverkit.h>

static struct xnvme_be_mixin g_xnvme_be_mixin_macos_driverkit[] = {
	{
		.mtype = XNVME_BE_MEM,
		.name = "driverkit",
		.descr = "Use DriverKit malloc/free",
		.mem = &g_xnvme_be_macos_driverkit_mem,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ASYNC,
		.name = "driverkit",
		.descr = "Use DriverKit for Asynchronous I/O",
		.async = &g_xnvme_be_macos_driverkit_async,
		.check_support = xnvme_be_supported,
	},
	{
		.mtype = XNVME_BE_ASYNC,
		.name = "emu",
		.descr = "Use emu pool for Asynchronous I/O",
		.async = &g_xnvme_be_cbi_async_emu,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_SYNC,
		.name = "driverkit",
		.descr = "Use DriverKit for synchronous I/O",
		.sync = &g_xnvme_be_macos_driverkit_sync,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_ADMIN,
		.name = "driverkit",
		.descr = "Use DriverKit for admin commands",
		.admin = &g_xnvme_be_macos_driverkit_admin,
		.check_support = xnvme_be_supported,
	},

	{
		.mtype = XNVME_BE_DEV,
		.name = "driverkit",
		.descr = "Use DriverKit handles and enumerate NVMe devices",
		.dev = &g_xnvme_be_macos_driverkit_dev,
		.check_support = xnvme_be_supported,
	},
};
#endif

struct xnvme_be xnvme_be_macos_driverkit = {
	.admin = XNVME_BE_NOSYS_ADMIN,
	.async = XNVME_BE_NOSYS_QUEUE,
	.dev = XNVME_BE_NOSYS_DEV,
	.mem = XNVME_BE_NOSYS_MEM,
	.sync = XNVME_BE_NOSYS_SYNC,
	.attr =
		{
#ifdef XNVME_BE_MACOS_ENABLED
			.enabled = 1,
#endif
			.name = "driverkit",
		},
#ifdef XNVME_BE_MACOS_ENABLED
	.nobjs =
		sizeof g_xnvme_be_mixin_macos_driverkit / sizeof *g_xnvme_be_mixin_macos_driverkit,
	.objs = g_xnvme_be_mixin_macos_driverkit,
#endif
};

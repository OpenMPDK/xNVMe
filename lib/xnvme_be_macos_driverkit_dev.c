// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>

#include <xnvme_dev.h>
#include <xnvme_be_macos_driverkit.h>

int
xnvme_be_macos_driverkit_enumerate(const char *sys_uri, struct xnvme_opts *opts,
				   xnvme_enumerate_cb cb_func, void *cb_args)
{
	XNVME_DEBUG("xnvme_be_macos_driverkit_enumerate()");

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	io_iterator_t iterator;

	CFMutableDictionaryRef matching_dict = IOServiceNameMatching("MacVFN");
	kern_return_t ret =
		IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iterator);
	if (ret != kIOReturnSuccess || !iterator) {
		return -ENODEV;
	}

	struct xnvme_opts tmp_opts = *opts;
	tmp_opts.be = xnvme_be_macos_driverkit.attr.name;
	tmp_opts.nsid = 1;

	int i = 0;
	while (true) {
		struct xnvme_dev *dev = NULL;
		char uri[XNVME_IDENT_URI_LEN] = {0};
		io_service_t service = IOIteratorNext(iterator);

		if (!service) {
			break;
		}

		i += 1;
		sprintf(uri, "%d", i);

		dev = xnvme_dev_open(uri, &tmp_opts);
		if (!dev) {
			XNVME_DEBUG("xnvme_dev_open(): %d", errno);
			return -errno;
		}
		if (cb_func(dev, cb_args)) {
			xnvme_dev_close(dev);
		}
	}

	IOObjectRelease(iterator);

	return -ENOSYS;
}

void
xnvme_be_macos_driverkit_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state = (void *)dev->be.state;

	if (!state->device_service) {
		return;
	}

	IOServiceClose(state->device_service);
	IOServiceClose(state->device_connection);
}

int
_xnvme_be_driverkit_create_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qd, int flags)
{
	NvmeQueue ioqpair_return;
	kern_return_t ret;
	size_t output_cnt;
	unsigned int qid = __builtin_ffsll(state->qidmap);
	// int err;

	if (qid == 0) {
		XNVME_DEBUG("no free queue identifiers\n");
		return -EBUSY;
	}

	qid--;

	XNVME_DEBUG("nvme_create_ioqpair(%d, %d, %d)", qid, qd + 1, flags);

	// nvme queue capacity must be one larger than the requested capacity
	// since only n-1 slots in an NVMe queue may be used
	NvmeQueue ioqpair_create = {
		.id = qid,
		.depth = qd + 1,
		.vector = -1,
		.flags = flags,
	};
	output_cnt = sizeof(NvmeQueue);

	ret = IOConnectCallStructMethod(state->device_connection, NVME_CREATE_QUEUE_PAIR,
					&ioqpair_create, sizeof(NvmeQueue), &ioqpair_return,
					&output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("NVME_CREATE_QUEUE_PAIR failed with error: 0x%08x, %s.\n", ret,
			    mach_error_string(ret));
	}
	XNVME_DEBUG("NVME_CREATE_QUEUE_PAIR done\n");
	if (ret) {
		return -errno;
	}

	state->qid_sync = ioqpair_return.id;
	state->qidmap &= ~(1ULL << qid);

	return qid;
}

int
_xnvme_be_driverkit_delete_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qid)
{
	NvmeQueue ioqpair_return;
	size_t output_cnt = 0;
	NvmeQueue ioqpair_delete = {
		.id = qid,
	};

	kern_return_t ret =
		IOConnectCallStructMethod(state->device_connection, NVME_DELETE_QUEUE_PAIR,
					  &ioqpair_delete, sizeof(NvmeQueue), NULL, &output_cnt);
	if (ret != kIOReturnSuccess) {
		XNVME_DEBUG("NVME_DELETE_QUEUE_PAIR failed with error: 0x%08x, %s.\n", ret,
			    mach_error_string(ret));
	}
	XNVME_DEBUG("NVME_DELETE_QUEUE_PAIR done\n");

	state->qidmap |= 1ULL << qid;

	return 0;
}

int
xnvme_be_macos_driverkit_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_driverkit_state *state = (void *)dev->be.state;
	struct xnvme_opts *opts = &dev->opts;
	int qpid;

	kern_return_t err;
	io_service_t service;
	io_iterator_t iterator;

	CFMutableDictionaryRef matching_dict = IOServiceNameMatching("MacVFN");
	kern_return_t ret =
		IOServiceGetMatchingServices(kIOMasterPortDefault, matching_dict, &iterator);
	if (ret != kIOReturnSuccess || !iterator) {
		return -ENODEV;
	}

	int i = 0;
	while (true) {
		char uri[XNVME_IDENT_URI_LEN] = {0};
		service = IOIteratorNext(iterator);

		if (!service) {
			XNVME_DEBUG("Failed to discover MacVFN service");
			return -ENODEV;
		}

		i += 1;
		sprintf(uri, "%d", i);

		if (strcmp(uri, dev->ident.uri) == 0) {
			break;
		}
	}

	IOObjectRelease(iterator);

	int type = 0;
	io_connect_t connection;
	err = IOServiceOpen(service, mach_task_self_, type, &connection);
	XNVME_DEBUG("Connection: %u\n", connection);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("IOServiceOpen failed with error: %x:%s", err, mach_error_string(err));
		return 1;
	}
	XNVME_DEBUG("IOServiceOpen userclient: %u", connection);

	state->device_service = service;
	state->device_connection = connection;

	err = IOConnectCallScalarMethod(state->device_connection, NVME_INIT, NULL, 0, NULL, 0);
	if (err != kIOReturnSuccess) {
		XNVME_DEBUG("NVME_INIT failed with error: 0x%08x, %s.\n", err,
			    mach_error_string(err));
		return 1;
	}
	XNVME_DEBUG("NVME_INIT done\n");

	dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = dev->opts.nsid;

	state->qidmap = -1 & ~0x1;

	qpid = _xnvme_be_driverkit_create_ioqpair(state, 31, 0x0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: initializing qpairs for sync IO: %d", err);
		return -errno;
	}

	return err;
}
#endif

struct xnvme_be_dev g_xnvme_be_macos_driverkit_dev = {
#ifdef XNVME_BE_MACOS_ENABLED
	.enumerate = xnvme_be_macos_driverkit_enumerate,
	.dev_open = xnvme_be_macos_driverkit_dev_open,
	.dev_close = xnvme_be_macos_driverkit_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};

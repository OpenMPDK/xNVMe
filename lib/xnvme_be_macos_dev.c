// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED
#include <mach/mach_error.h>
#include <IOKit/IOTypes.h>
#include <IOKit/storage/nvme/NVMeSMARTLibExternal.h>
#include <CoreFoundation/CoreFoundation.h>
#include <libgen.h>

#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_macos.h>

#ifndef _PATH_DEV_DISK
#define _PATH_DEV_DISK "/dev/"
#define _DISK_PREFIX "disk"
#define _RDISK_PREFIX "rdisk"
#endif

io_object_t
_get_ioservice_device(char *devname)
{
	CFMutableDictionaryRef matching_dictionary;
	io_object_t ioservice_device;

	matching_dictionary = IOBSDNameMatching(kIOMasterPortDefault, 0, basename(devname));
	ioservice_device = IOServiceGetMatchingService(kIOMasterPortDefault, matching_dictionary);
	return ioservice_device;
}

int
_get_nvme_smart_interface(io_object_t ioservice_device,
			  IONVMeSMARTInterface ***nvme_smart_interface,
			  IOCFPlugInInterface ***plugin_interface)
{
	io_object_t device = ioservice_device;
	io_object_t parent_device;
	CFBooleanRef is_nvme;
	SInt32 score;
	kern_return_t ret;

	// Cannot use IORegistryEntrySearchCFProperty, as it won't return the parent having the key
	while (true) {
		is_nvme = IORegistryEntryCreateCFProperty(
			device, CFSTR(kIOPropertyNVMeSMARTCapableKey), kCFAllocatorDefault, 0);
		if (is_nvme) {
			CFRelease(is_nvme);
			break;
		}

		IOReturn err =
			IORegistryEntryGetParentEntry(device, kIOServicePlane, &parent_device);
		if (device !=
		    ioservice_device) { // Don't free initial device. We are just borrowing it.
			IOObjectRelease(device);
		}
		if (err) {
			XNVME_DEBUG("No parent device found");
			return -1;
		}

		device = parent_device;
	}

	ret = IOCreatePlugInInterfaceForService(device, kIONVMeSMARTUserClientTypeID,
						kIOCFPlugInInterfaceID, plugin_interface, &score);
	if (ret == kIOReturnSuccess) {
		(**plugin_interface)
			->QueryInterface(*plugin_interface,
					 CFUUIDGetUUIDBytes(kIONVMeSMARTInterfaceID),
					 (LPVOID *)nvme_smart_interface);
	} else {
		XNVME_DEBUG("IOCreatePlugInInterfaceForService failed: %s",
			    (char *)mach_error_string(ret));
		return -1;
	}
	return 0;
}

int
_get_nvme_ns(io_object_t ioservice_device)
{
	CFTypeRef cf_nsid = IORegistryEntrySearchCFProperty(
		ioservice_device, kIOServicePlane, CFSTR("NamespaceID"), kCFAllocatorDefault,
		kIORegistryIterateRecursively | kIORegistryIterateParents);
	if (!cf_nsid) {
		XNVME_DEBUG("Failed to get namespace id");
		return 0;
	}
	int nsid;
	if (!CFNumberGetValue(cf_nsid, kCFNumberIntType, &nsid)) {
		XNVME_DEBUG("Failed to convert namespace id to int");
		return 0;
	}
	return nsid;
}

int
_is_disk_nvme(io_object_t ioservice_device)
{
	CFBooleanRef is_nvme;

	is_nvme = IORegistryEntrySearchCFProperty(
		ioservice_device, kIOServicePlane, CFSTR(kIOPropertyNVMeSMARTCapableKey),
		kCFAllocatorDefault, kIORegistryIterateRecursively | kIORegistryIterateParents);

	if (is_nvme) {
		CFRelease(is_nvme);
		return 1;
	}
	return 0;
}

int
xnvme_be_macos_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args)
{
	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	struct xnvme_opts tmp_opts = *opts;
	tmp_opts.be = xnvme_be_macos.attr.name;

	for (int nid = 0; nid < 256; nid++) {
		char path[128] = {0};
		char disk_id[128] = {0};
		char uri[XNVME_IDENT_URI_LEN] = {0};
		struct xnvme_dev *dev;

		// Enumerate 'disk' devices
		snprintf(disk_id, 127, "%s%d", _DISK_PREFIX, nid);
		snprintf(path, 127, "%s%s", _PATH_DEV_DISK, disk_id);

		if (access(path, F_OK)) {
			continue;
		}

		snprintf(uri, XNVME_IDENT_URI_LEN - 1, "%s", path);

		dev = xnvme_dev_open(uri, &tmp_opts);
		if (!dev) {
			XNVME_DEBUG("xnvme_dev_open(): %d", errno);
			continue;
		}
		if (cb_func(dev, cb_args)) {
			xnvme_dev_close(dev);
		}
	}

	return 0;
}

void
xnvme_be_macos_state_term(struct xnvme_be_macos_state *state)
{
	if (!state) {
		return;
	}

	if (state->fd >= 0) {
		close(state->fd);
	}

	if (state->ioservice_device) {
		IOObjectRelease(state->ioservice_device);
	}

	if (state->nvme_smart_interface) {
		(*state->nvme_smart_interface)->Release(state->nvme_smart_interface);
	}

	if (state->plugin_interface) {
		IODestroyPlugInInterface(state->plugin_interface);
	}
}

void
xnvme_be_macos_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_macos_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_file_opts_to_macos(struct xnvme_opts *opts)
{
	int flags = 0;

	flags |= opts->create ? O_CREAT : 0x0;
	flags |= opts->rdonly ? O_RDONLY : 0x0;
	flags |= opts->wronly ? O_WRONLY : 0x0;
	flags |= opts->rdwr ? O_RDWR : 0x0;
	flags |= opts->truncate ? O_TRUNC : 0x0;

	return flags;
}

int
xnvme_be_macos_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_macos_state *state = (void *)dev->be.state;
	struct xnvme_opts *opts = &dev->opts;
	int flags = xnvme_file_opts_to_macos(opts);
	int err;
	io_object_t ioservice_device;
	IONVMeSMARTInterface **nvme_smart_interface;
	IOCFPlugInInterface **plugin_interface;

	ioservice_device = _get_ioservice_device(dev->ident.uri);
	if (!ioservice_device) {
		XNVME_DEBUG("No devices found");
		return -ENOTSUP;
	}

	if (_get_nvme_smart_interface(ioservice_device, &nvme_smart_interface,
				      &plugin_interface)) {
		XNVME_DEBUG("Failed to acquire NVMe SMART interface for %s", dev->ident.uri);
		return -ENOTSUP;
	}

	if (_is_disk_nvme(ioservice_device)) {
		state->fd = open(dev->ident.uri, flags);
		if (state->fd < 0) {
			XNVME_DEBUG("FAILED: Unable to open device, errno: %d, %s", errno,
				    strerror(errno));
			return -errno;
		}
		state->ioservice_device = ioservice_device;
		state->nvme_smart_interface = nvme_smart_interface;
		state->plugin_interface = plugin_interface;
		dev->ident.nsid = _get_nvme_ns(ioservice_device);
		dev->ident.dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->ident.csi = XNVME_SPEC_CSI_NVM;

		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_macos_admin;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_cbi_sync_psync;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
	} else {
		XNVME_DEBUG("non-nvme disks are currently unsupported.");
		return -ENOTSUP;

		state->fd = open(dev->ident.uri, flags, opts->create_mode);
		state->ioservice_device = 0;
		state->nvme_smart_interface = NULL;
		dev->ident.nsid = 1;
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
	}

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy");
		return -EINVAL;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		return err;
	}

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_macos_dev = {
	.id = "nvme",
#ifdef XNVME_BE_MACOS_ENABLED
	.enumerate = xnvme_be_macos_enumerate,
	.dev_open = xnvme_be_macos_dev_open,
	.dev_close = xnvme_be_macos_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};

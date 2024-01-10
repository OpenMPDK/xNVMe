// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_LINUX_ENABLED
#include <errno.h>
#include <dirent.h>
#include <paths.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>
#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>

static inline void
_be_linux_state_term(struct xnvme_be_linux_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
	state->fd = 0;
}

int
xnvme_be_linux_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	const struct xnvme_ident *ident = &dev->ident;
	struct xnvme_opts *opts = &dev->opts;
	struct stat dev_stat;
	int flags;
	int err;

	flags = xnvme_file_opts_to_linux(opts);

	XNVME_DEBUG("INFO: open() : flags: 0x%x, opts->create_mode: 0x%x", flags,
		    opts->create_mode);

	state->fd = open(ident->uri, flags, opts->create_mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d", ident->uri,
			    state->fd, errno);

		opts->direct = 0;
		flags = xnvme_file_opts_to_linux(opts);
		state->fd = open(ident->uri, flags, opts->create_mode);
		if (state->fd < 0) {
			XNVME_DEBUG("FAILED: open(uri: '%s') : state->fd: '%d', errno: %d",
				    ident->uri, state->fd, errno);
			return -errno;
		}
	}

	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: open() : fstat() : err: %d, errno: %d", err, errno);
		return -errno;
	}

	// Change {async,sync,admin} based on file unless one was explicitly requested
	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: open() : regular file");
		dev->ident.dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_cbi_admin_shim;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_cbi_sync_psync;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
		break;

	case S_IFBLK:
		dev->ident.dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->ident.csi = XNVME_SPEC_CSI_FS;
		dev->ident.nsid = 1;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_linux_admin_block;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_linux_sync_block;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
		XNVME_DEBUG("INFO: open() : block-device file");

		err = xnvme_be_linux_nvme_dev_nsid(dev);
		if (err < 1) {
			XNVME_DEBUG("INFO: open() : retrieving nsid, got: {nsid: %x, err: %d}",
				    err, err);
			break;
		}
		dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->ident.csi = XNVME_SPEC_CSI_NVM;
		dev->ident.nsid = err;
		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_linux_admin_nvme;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_linux_sync_nvme;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}
		XNVME_DEBUG("INFO: open() : block-device file (is a NVMe namespace)");

		break;

	case S_IFCHR:
		XNVME_DEBUG("INFO: open() : char-device-file");

		err = xnvme_be_linux_nvme_dev_nsid(dev);
		XNVME_DEBUG("INFO: open() : retrieving nsid, got: %x", err);
		if ((uint32_t)err ==
		    0xFFFFFFFF) { ///< Assuming a /dev/nvme0 -- nvme-controller handle
			dev->ident.dtype = XNVME_DEV_TYPE_NVME_CONTROLLER;
			dev->ident.csi = 0x0;
			dev->ident.nsid = err;
		} else if (err < 1) { ///< Assuming not a NVMe device
			return -EINVAL;
		} else { ///< Assuming a /dev/ngXnY -- nvme-namespace handle
			dev->ident.dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
			dev->ident.csi = XNVME_SPEC_CSI_NVM;
			dev->ident.nsid = err;
		}

		if (!opts->admin) {
			dev->be.admin = g_xnvme_be_linux_admin_nvme;
		}
		if (!opts->sync) {
			dev->be.sync = g_xnvme_be_linux_sync_nvme;
		}
		if (!opts->async) {
			dev->be.async = g_xnvme_be_cbi_async_emu;
		}

		XNVME_DEBUG("INFO: open() : char-device-file: NVMe ioctl() with async. emulation");

		break;

	default:
		XNVME_DEBUG("FAILED: open() : unsupported S_IFMT: %d", dev_stat.st_mode & S_IFMT);
		return -EINVAL;
	}

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy()");
		_be_linux_state_term((void *)dev->be.state);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		_be_linux_state_term((void *)dev->be.state);
		return err;
	}

	// TODO: consider this. Due to Kernel-segment constraint force mdts down
	if ((dev->geo.lba_nbytes) && ((dev->geo.mdts_nbytes / dev->geo.lba_nbytes) > 127)) {
		dev->geo.mdts_nbytes = dev->geo.lba_nbytes * 127;
	}

	state->poll_io = opts->poll_io;
	state->poll_sq = opts->poll_sq;

	XNVME_DEBUG("INFO: open() : dev->state.poll_io: %d", state->poll_io);
	XNVME_DEBUG("INFO: open() : dev->state.poll_sq: %d", state->poll_sq);

	XNVME_DEBUG("INFO: --- open() : OK ---");

	return 0;
}

void
xnvme_be_linux_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	_be_linux_state_term((void *)dev->be.state);
	memset(&dev->be, 0, sizeof(dev->be));
}

int
xnvme_path_nvme_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns, part;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, "nvme")) {
		snprintf(path, sizeof(path), "%s%s", "/dev/", d->d_name);
		if (stat(path, &bd)) {
			return 0;
		}
		if (!S_ISBLK(bd.st_mode)) {
			return 0;
		}
		if (sscanf(d->d_name, "nvme%dn%dp%d", &ctrl, &ns, &part) == 3) {
			// Do not want to use e.g. nvme0n1p2 etc.
			return 0;
		}
		return 1;
	}

	return 0;
}

int
xnvme_path_ng_filter(const struct dirent *d)
{
	char path[264];
	struct stat bd;
	int ctrl, ns;

	if (d->d_name[0] == '.') {
		return 0;
	}

	if (strstr(d->d_name, "ng")) {
		snprintf(path, sizeof(path), "%s%s", "/dev/", d->d_name);
		if (stat(path, &bd)) {
			return 0;
		}
		if (!S_ISCHR(bd.st_mode)) {
			return 0;
		}
		if (sscanf(d->d_name, "ng%dn%d", &ctrl, &ns) != 2) {
			return 0;
		}
		return 1;
	}

	return 0;
}

/**
 * Scanning /sys/class/nvme can give device names, such as "nvme0c65n1", which
 * are linked as virtual devices to the block device. So instead of scanning
 * that dir, then instead /sys/block/ is scanned under the assumption that
 * block-devices with "nvme" in them are NVMe devices with namespaces attached
 *
 * TODO: add enumeration of NS vs CTRLR, actually, replace this with the libnvme
 * topology functions
 */
int
xnvme_be_linux_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
			 void *cb_args)
{
	struct dirent **ns = NULL;
	int nns = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	struct xnvme_opts tmp_opts = *opts;
	tmp_opts.be = xnvme_be_linux.attr.name;

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = {0};
		struct xnvme_dev *dev = NULL;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1, _PATH_DEV "%s", ns[ni]->d_name);

		dev = xnvme_dev_open(uri, &tmp_opts);
		if (!dev) {
			XNVME_DEBUG("xnvme_dev_open(): %d", errno);
			return -errno;
		}
		if (cb_func(dev, cb_args)) {
			xnvme_dev_close(dev);
		}
	}
	nns = scandir("/dev", &ns, xnvme_path_ng_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = {0};
		struct xnvme_dev *dev = NULL;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1, _PATH_DEV "%s", ns[ni]->d_name);

		dev = xnvme_dev_open(uri, &tmp_opts);
		if (!dev) {
			XNVME_DEBUG("xnvme_dev_open(): %d", errno);
			return -errno;
		}
		if (cb_func(dev, cb_args)) {
			xnvme_dev_close(dev);
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}
#endif

struct xnvme_be_dev g_xnvme_be_dev_linux = {
	.id = "nvme",
#ifdef XNVME_BE_LINUX_ENABLED
	.enumerate = xnvme_be_linux_enumerate,
	.dev_open = xnvme_be_linux_dev_open,
	.dev_close = xnvme_be_linux_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};

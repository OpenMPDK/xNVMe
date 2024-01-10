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
#include <fcntl.h>
#include <xnvme_be_ramdisk.h>
#include <xnvme_dev.h>
#include <xnvme_be_cbi.h>

void
xnvme_be_ramdisk_dev_close(struct xnvme_dev *dev)
{
	struct xnvme_be_ramdisk_state *state;

	if (!dev) {
		return;
	}

	state = (void *)dev->be.state;
	if (state->ramdisk) {
		free(state->ramdisk);
	}

	memset(&dev->be, 0, sizeof(dev->be));
}

size_t
xnvme_be_ramdisk_dev_get_size(struct xnvme_dev *dev)
{
	const struct xnvme_ident *ident = &dev->ident;
	char *gb_marker;

	gb_marker = strrchr(ident->uri, 'G');
	if (!gb_marker || (gb_marker && strcmp(gb_marker, "GB"))) {
		XNVME_DEBUG("FAILED: Invalid URI. Only postfix of 'GB' allowed: %s", ident->uri);
		return 0;
	}

	return (size_t)atoi(ident->uri) * 1024 * 1024 * 1024;
}

int
xnvme_be_ramdisk_dev_open(struct xnvme_dev *dev)
{
	struct xnvme_be_ramdisk_state *state = (void *)dev->be.state;
	struct xnvme_opts *opts = &dev->opts;
	int err;

	size_t ramdisk_size = xnvme_be_ramdisk_dev_get_size(dev);
	if (!ramdisk_size) {
		return -EINVAL;
	}

	state->ramdisk = malloc(ramdisk_size);
	if (!state->ramdisk) {
		XNVME_DEBUG("FAILED: Unable to allocate ramdisk: uri=%s, state->ramdisk=%p",
			    dev->ident.uri, state->ramdisk);
		return -errno;
	}

	if (!opts->admin) {
		dev->be.admin = g_xnvme_be_ramdisk_admin;
	}
	if (!opts->sync) {
		dev->be.sync = g_xnvme_be_ramdisk_sync;
	}
	if (!opts->async) {
		dev->be.async = g_xnvme_be_cbi_async_thrpool;
	}

	dev->ident.dtype = XNVME_DEV_TYPE_RAMDISK;
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	dev->ident.nsid = 1;

	err = xnvme_be_dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_idfy()");
		xnvme_be_ramdisk_dev_close(dev);
		return -EINVAL;
	}
	err = xnvme_be_dev_derive_geometry(dev);
	if (err) {
		XNVME_DEBUG("FAILED: open() : xnvme_be_dev_derive_geometry()");
		xnvme_be_ramdisk_dev_close(dev);
		return err;
	}

	return 0;
}

#endif

struct xnvme_be_dev g_xnvme_be_ramdisk_dev = {
	.id = "nvme",
#ifdef XNVME_BE_RAMDISK_ENABLED
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_ramdisk_dev_open,
	.dev_close = xnvme_be_ramdisk_dev_close,
#else
	.enumerate = xnvme_be_nosys_enumerate,
	.dev_open = xnvme_be_nosys_dev_open,
	.dev_close = xnvme_be_nosys_dev_close,
#endif
};

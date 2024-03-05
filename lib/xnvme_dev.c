// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_cmd.h>
#include <xnvme_dev.h>
#include <xnvme_geo.h>

static int
_zoned_geometry(struct xnvme_dev *dev)
{
	struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_znd_idfy_lbafe *lbafe = &zns->lbafe[nvm->flbas.format];
	struct xnvme_geo *geo = &dev->geo;

	if (!zns->lbafe[0].zsze) {
		XNVME_DEBUG("FAILED: !zns.lbafe[0].zsze");
		return -EINVAL;
	}

	geo->type = XNVME_GEO_ZONED;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = nvm->nsze / lbafe->zsze;
	geo->nsect = lbafe->zsze;

	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended = nvm->flbas.extended && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	return 0;
}

// TODO: select LBAF correctly, instead of the first
static inline int
_conventional_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_idfy_ctrlr *ctrlr = (void *)xnvme_dev_get_ctrlr(dev);
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
	struct xnvme_spec_nvm_idfy_ns *nns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_elbaf *elbaf = &nns->elbaf[nvm->flbas.format];
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;

	geo->nsect = dev->id.ns.nsze;
	geo->nbytes = 2 << (lbaf->ds - 1);
	geo->nbytes_oob = lbaf->ms;

	geo->lba_nbytes = geo->nbytes;
	geo->lba_extended = nvm->flbas.extended && lbaf->ms;
	if (geo->lba_extended) {
		geo->lba_nbytes += geo->nbytes_oob;
	}

	geo->pi_type = nvm->dps.pit;

	if (geo->pi_type) {
		geo->pi_format = XNVME_SPEC_NVM_NS_16B_GUARD;
		geo->pi_loc = nvm->dps.md_start;
	}

	if (ctrlr->ctratt.extended_lba_formats && geo->pi_type) {
		geo->pi_format = elbaf->pif;
	}

	return 0;
}

static int
_fs_geometry(struct xnvme_dev *dev)
{
	const struct xnvme_spec_fs_idfy_ns *ns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_geo *geo = &dev->geo;

	geo->type = XNVME_GEO_CONVENTIONAL;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = 1;
	geo->nsect = 1;

	geo->nbytes = 1;
	geo->nbytes_oob = 0;

	geo->lba_nbytes = 512;
	geo->lba_extended = 0;

	geo->tbytes = ns->nuse;

	geo->mdts_nbytes = 1 << 20;
	geo->ssw = XNVME_ILOG2(geo->lba_nbytes);

	return 0;
}

static int
_kvs_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	geo->type = XNVME_GEO_KV;
	return 0;
}

static int
_controller_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	memset(geo, 0, sizeof(*geo));
	geo->type = XNVME_GEO_UNKNOWN;

	///< note: mdts is setup in derive-geometry, so this function does nothing

	return 0;
}

/**
 * Helper function for determining command set identifier
 */
static int
_dev_idfy_csi(struct xnvme_dev *dev, struct xnvme_spec_idfy *idfy_ns,
	      struct xnvme_spec_idfy *idfy_ctrlr)
{
	struct xnvme_cmd_ctx ctx;
	int err;

	// Attempt to identify ZONED

	struct xnvme_spec_znd_idfy_ns *zns = (void *)idfy_ns;

	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ctrlr_csi(CSI_ZONED)");
		goto not_zns;
	}

	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->ident.nsid, XNVME_SPEC_CSI_ZONED, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ns_csi(CSI_ZONED)");
		goto not_zns;
	}

	if (!zns->lbafe[0].zsze) {
		goto not_zns;
	}

	memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
	dev->ident.csi = XNVME_SPEC_CSI_ZONED;

	XNVME_DEBUG("INFO: looks like csi(ZNS)");
	return err;

not_zns:
	XNVME_DEBUG("INFO: no positive response to idfy(ZNS)");

	// Attempt to identify FS

	struct xnvme_spec_fs_idfy_ctrlr *fs_ctrlr = (void *)idfy_ctrlr;
	struct xnvme_spec_fs_idfy_ns *fs_ns = (void *)idfy_ns;

	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_FS, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ctrlr_csi(CSI_FS)");
		goto not_fs;
	}
	if (!(fs_ctrlr->ac == 0xAC && fs_ctrlr->dc == 0xDC)) {
		XNVME_DEBUG("INFO: invalid values in idfy-ctrlr(CSI_FS)");
		goto not_fs;
	}

	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->ident.nsid, XNVME_SPEC_CSI_FS, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ns_csi(CSI_FS)");
		goto not_fs;
	}
	if (!(fs_ns->nsze && fs_ns->ncap && fs_ns->ac == 0xAC && fs_ns->dc == 0xDC)) {
		XNVME_DEBUG("INFO: invalid values in idfy-ctrlr(CSI_FS)");
		goto not_fs;
	}

	memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

	XNVME_DEBUG("INFO: looks like csi(FS)");
	dev->ident.csi = XNVME_SPEC_CSI_FS;
	return err;

not_fs:
	XNVME_DEBUG("INFO: no positive response to idfy(FS)");

	// Attempt to identify NVM

	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_NVM, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ctrlr_csi(CSI_NVM)");
		goto not_nvm;
	}

	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->ident.nsid, XNVME_SPEC_CSI_NVM, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: !xnvme_adm_idfy_ns_csi(CSI_NVM)");
		goto not_nvm;
	}
	memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

	XNVME_DEBUG("INFO: looks like csi(NVM)");
	dev->ident.csi = XNVME_SPEC_CSI_NVM;
	return err;

not_nvm:
	XNVME_DEBUG("INFO: no positive response to idfy(NVM)");

	return err;
}

static int
_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = {0};
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	int err;

	dev->attempted_dev_idfy = true;
	//
	// Identify controller and namespace
	//

	// Allocate buffers for idfy
	idfy_ctrlr = xnvme_buf_alloc(dev, sizeof(*idfy_ctrlr));
	if (!idfy_ctrlr) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}
	idfy_ns = xnvme_buf_alloc(dev, sizeof(*idfy_ns));
	if (!idfy_ns) {
		XNVME_DEBUG("FAILED: xnvme_buf_alloc()");
		err = -errno;
		goto exit;
	}

	// Retrieve idfy-ctrlr
	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr(&ctx, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		XNVME_DEBUG("FAILED: xnvme_adm_idfy_ctrlr(), err: %d", err);
		goto exit;
	}
	// Store idfy-ctrlr in device instance
	memcpy(&dev->id.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	// Store subnqn in device-identifier
	memcpy(dev->ident.subnqn, idfy_ctrlr->ctrlr.subnqn, sizeof(dev->ident.subnqn));

	if (dev->ident.dtype == XNVME_DEV_TYPE_NVME_CONTROLLER) {
		goto exit;
	}

	// Retrieve idfy-ns
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns(&ctx, dev->ident.nsid, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		err = err ? err : -EIO;
		XNVME_DEBUG("FAILED: xnvme_adm_idfy_ns(), err: %d", err);
		goto exit;
	}
	// Store idfy-ns in device instance
	memcpy(&dev->id.ns, idfy_ns, sizeof(*idfy_ns));

	err = _dev_idfy_csi(dev, idfy_ns, idfy_ctrlr);
	if (err) {
		XNVME_DEBUG("INFO: unable to determine CSI - dev->idcss has not been set");
		// Failing _dev_idfy_csi is okay
		err = 0;
	}

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

int
xnvme_dev_derive_geo(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;
	int err;

	dev->attempted_derive_geo = true;

	err = _dev_idfy(dev);
	if (err) {
		XNVME_DEBUG("FAILED: identifying device; skipping derive-geo");
		return err;
	}

	switch (dev->ident.dtype) {
	case XNVME_DEV_TYPE_NVME_CONTROLLER:
		return _controller_geometry(dev);

	case XNVME_DEV_TYPE_FS_FILE:
		return _fs_geometry(dev);

	case XNVME_DEV_TYPE_RAMDISK:
	case XNVME_DEV_TYPE_BLOCK_DEVICE:
	case XNVME_DEV_TYPE_NVME_NAMESPACE:
		if (dev->ident.csi == XNVME_SPEC_CSI_FS) {
			if (_conventional_geometry(dev)) {
				XNVME_DEBUG("FAILED: _conventional_geometry");
				return -EINVAL;
			}
			break;
		}

		switch (dev->ident.csi) {
		case XNVME_SPEC_CSI_ZONED:
			if (_zoned_geometry(dev)) {
				XNVME_DEBUG("FAILED: _zoned_geometry");
				return -EINVAL;
			}
			break;

		case XNVME_SPEC_CSI_NVM:
			if (_conventional_geometry(dev)) {
				XNVME_DEBUG("FAILED: _conventional_geometry");
				return -EINVAL;
			}
			break;

		case XNVME_SPEC_CSI_KV:
			if (_kvs_geometry(dev)) {
				XNVME_DEBUG("FAILED: _kvs_geometry");
				return -EINVAL;
			}
			break;

		default:
			XNVME_DEBUG("FAILED: unhandled csi: 0x%x", dev->ident.csi);
			return -ENOSYS;
			break;
		}
		break;
	default:
		XNVME_DEBUG("FAILED: unhandled dtype: 0x%x", dev->ident.dtype);
		return -ENOSYS;
	}

	geo->tbytes =
		(unsigned long)(geo->npugrp) * geo->npunit * geo->nzone * geo->nsect * geo->nbytes;

	/* Derive the sector-shift-width for LBA mapping */
	geo->ssw = XNVME_ILOG2(dev->geo.nbytes);

	//
	// If the controller reports that MDTS is unbounded, that is, it can be
	// infinitely large then we cap it here to something that just might
	// exists in finite physical space and time withing this realm of
	// what we perceive as reality in our universe. Note, that this might
	// change with quantum devices...
	//
	// TODO: read the mpsmin register...
	{
		size_t mpsmin = 0;
		size_t mdts = dev->id.ctrlr.mdts;

		if (!mdts) {
			geo->mdts_nbytes = 1 << 20;
		} else {
			geo->mdts_nbytes = 1 << (mdts + 12 + mpsmin);
		}

		// Fabrics work-around
		if ((geo->mdts_nbytes > (16 * 1024)) && (dev->opts.spdk_fabrics)) {
			geo->mdts_nbytes = 16 * 1024;
		}
	}

	// TODO: add zamdts

	/** quirk: Linux kernel max-segments potentially less than mdts **/
	if ((!strncmp(dev->be.attr.name, "linux", 5)) && (!strncmp(dev->be.sync.id, "nvme", 4)) &&
	    (dev->geo.lba_nbytes) && ((dev->geo.mdts_nbytes / dev->geo.lba_nbytes) > 127)) {
		dev->geo.mdts_nbytes = dev->geo.lba_nbytes * 127;
	}

	return 0;
}

int
xnvme_dev_fpr(FILE *stream, const struct xnvme_dev *dev, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_dev:");

	if (!dev) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += xnvme_ident_yaml(stream, &dev->ident, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	wrtn += xnvme_be_yaml(stream, &dev->be, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "  xnvme_opts:\n");
	wrtn += fprintf(stream, "    be: '%s'\n", dev->opts.be);
	wrtn += fprintf(stream, "    mem: '%s'\n", dev->opts.mem);
	wrtn += fprintf(stream, "    dev: '%s'\n", dev->opts.dev);
	wrtn += fprintf(stream, "    admin: '%s'\n", dev->opts.admin);
	wrtn += fprintf(stream, "    sync: '%s'\n", dev->opts.sync);
	wrtn += fprintf(stream, "    async: '%s'\n", dev->opts.async);

	wrtn += xnvme_geo_yaml(stream, &dev->geo, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_dev_pr(const struct xnvme_dev *dev, int opts)
{
	return xnvme_dev_fpr(stdout, dev, opts);
}

const struct xnvme_geo *
xnvme_dev_get_geo(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_derive_geo) && xnvme_dev_derive_geo((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: xnvme_dev_derive_geo()");
	}

	return &dev->geo;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->id.ctrlr;
}

const struct xnvme_spec_idfy_ctrlr *
xnvme_dev_get_ctrlr_css(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->idcss.ctrlr;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->id.ns;
}

const struct xnvme_spec_idfy_ns *
xnvme_dev_get_ns_css(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->idcss.ns;
}

uint32_t
xnvme_dev_get_nsid(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return dev->ident.nsid;
}

uint8_t
xnvme_dev_get_csi(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return dev->ident.csi;
}

const struct xnvme_ident *
xnvme_dev_get_ident(const struct xnvme_dev *dev)
{
	if ((!dev->attempted_dev_idfy) && _dev_idfy((struct xnvme_dev *)dev)) {
		XNVME_DEBUG("FAILED: open() : _dev_idfy()");
	}
	return &dev->ident;
}

const struct xnvme_opts *
xnvme_dev_get_opts(const struct xnvme_dev *dev)
{
	return &dev->opts;
}

uint64_t
xnvme_dev_get_ssw(const struct xnvme_dev *dev)
{
	return dev->geo.ssw;
}

const void *
xnvme_dev_get_be_state(const struct xnvme_dev *dev)
{
	return dev->be.state;
}

struct xnvme_dev *
xnvme_dev_open(const char *dev_uri, struct xnvme_opts *opts)
{
	struct xnvme_opts opts_default = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	int err;

	if (!opts) { ///< Set defaults when none are given
		opts = &opts_default;
	}
	if (!(opts->rdonly | opts->wronly | opts->rdwr)) { ///< Set a default open-mode
		opts->rdwr = opts_default.rdwr;
	}
	if (opts->create && !opts->create_mode) { ///< Set a default umask/mode_t/create-mode
		opts->create_mode = opts_default.create_mode;
	}

	err = xnvme_dev_alloc(&dev);
	if (err) {
		XNVME_DEBUG("FAILED: failed xnvme_dev_alloc()");
		errno = -err;
		return NULL;
	}

	err = xnvme_ident_from_uri(dev_uri, &dev->ident);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_ident_from_uri(), err: %d", err);
		errno = -err;
		free(dev);
		return NULL;
	}

	err = xnvme_be_factory(dev, opts);
	if (err) {
		XNVME_DEBUG("FAILED: failed opening uri: %s", dev_uri);
		errno = -err;
		free(dev);
		return NULL;
	}

	return dev;
}

void
xnvme_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	dev->be.dev.dev_close(dev);
	free(dev);
}

int
xnvme_dev_alloc(struct xnvme_dev **dev)
{
	(*dev) = malloc(sizeof(**dev));
	if (!(*dev)) {
		XNVME_DEBUG("FAILED: allocating 'struct xnvme_dev'\n");
		return -errno;
	}
	memset(*dev, 0, sizeof(**dev));

	return 0;
}

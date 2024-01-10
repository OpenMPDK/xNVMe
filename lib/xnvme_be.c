// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <dirent.h>
#ifndef WIN32
#include <paths.h>
#endif
#include <ctype.h>
#include <errno.h>
#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_dev.h>

static struct xnvme_be *g_xnvme_be_registry[] = {
	&xnvme_be_spdk,    &xnvme_be_linux,   &xnvme_be_fbsd, &xnvme_be_macos,
	&xnvme_be_windows, &xnvme_be_ramdisk, &xnvme_be_vfio, NULL,
};
static int g_xnvme_be_count = sizeof g_xnvme_be_registry / sizeof *g_xnvme_be_registry - 1;

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent, const char *sep, int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_be:", indent, "");
		indent += 2;
	}
	if (!be) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}
	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	/*
	wrtn += fprintf(stream, "%*sdev: {id: '%s'}%s", indent, "", be->dev.id, sep);
	wrtn += fprintf(stream, "%*smem: {id: '%s'}%s", indent, "", be->mem.id, sep);
	*/
	wrtn += fprintf(stream, "%*sadmin: {id: '%s'}%s", indent, "", be->admin.id, sep);
	wrtn += fprintf(stream, "%*ssync: {id: '%s'}%s", indent, "", be->sync.id, sep);
	wrtn += fprintf(stream, "%*sasync: {id: '%s'}%s", indent, "", be->async.id, sep);
	wrtn += fprintf(stream, "%*sattr: {name: '%s'}", indent, "", be->attr.name);

	return wrtn;
}

int
xnvme_be_fpr(FILE *stream, const struct xnvme_be *be, enum xnvme_pr opts)
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

	wrtn += xnvme_be_yaml(stream, be, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_be_pr(const struct xnvme_be *be, enum xnvme_pr opts)
{
	return xnvme_be_fpr(stdout, be, opts);
}

int
xnvme_lba_fpr(FILE *stream, uint64_t lba, enum xnvme_pr opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "0x%016" PRIx64, lba);
		break;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		wrtn += fprintf(stream, "lba: 0x%016" PRIx64 "\n", lba);
		break;
	}

	return wrtn;
}

int
xnvme_lba_pr(uint64_t lba, enum xnvme_pr opts)
{
	return xnvme_lba_fpr(stdout, lba, opts);
}

int
xnvme_lba_fprn(FILE *stream, const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "lbas:");

	if (!lba) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "nlbas: %" PRIu16 "\n", nlb);
	wrtn += fprintf(stream, "lbas:\n");
	for (unsigned int i = 0; i < nlb; ++i) {
		wrtn += fprintf(stream, "  - ");
		xnvme_lba_pr(lba[i], XNVME_PR_TERSE);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
xnvme_lba_prn(const uint64_t *lba, uint16_t nlb, enum xnvme_pr opts)
{
	return xnvme_lba_fprn(stdout, lba, nlb, opts);
}

int
xnvme_be_attr_list_bundled(struct xnvme_be_attr_list **list)
{
	const size_t list_nbytes = sizeof(**list) + g_xnvme_be_count * sizeof((*list)->item[0]);

	*list = xnvme_buf_virt_alloc(512, list_nbytes);
	if (!*list) {
		return -1;
	}

	(*list)->count = g_xnvme_be_count;
	(*list)->capacity = g_xnvme_be_count;
	for (int i = 0; i < g_xnvme_be_count; ++i) {
		(*list)->item[i] = g_xnvme_be_registry[i]->attr;
	}

	return 0;
}

int
xnvme_be_attr_fpr(FILE *stream, const struct xnvme_be_attr *attr, int opts)
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

	wrtn += fprintf(stream, "name: '%s'\n", attr->name);
	wrtn += fprintf(stream, "    enabled: %" PRIu8 "\n", attr->enabled);

	return wrtn;
}

int
xnvme_be_attr_pr(const struct xnvme_be_attr *attr, int opts)
{
	return xnvme_be_attr_fpr(stdout, attr, opts);
}

int
xnvme_be_attr_list_fpr(FILE *stream, const struct xnvme_be_attr_list *list, int opts)
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

	wrtn += fprintf(stream, "xnvme_be_attr_list:\n");
	wrtn += fprintf(stream, "  count: %d\n", list->count);
	wrtn += fprintf(stream, "  capacity: %" PRIu32 "\n", list->capacity);
	wrtn += fprintf(stream, "  items:");

	if (!list->count) {
		wrtn += fprintf(stream, "~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	for (int i = 0; i < list->count; ++i) {
		wrtn += fprintf(stream, "  - ");
		wrtn += xnvme_be_attr_fpr(stream, &g_xnvme_be_registry[i]->attr, opts);
		wrtn += fprintf(stream, "\n");
	}

	return wrtn;
}

int
xnvme_be_attr_list_pr(const struct xnvme_be_attr_list *list, int opts)
{
	return xnvme_be_attr_list_fpr(stdout, list, opts);
}

static inline int
_zoned_geometry(struct xnvme_dev *dev)
{
	struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	struct xnvme_spec_znd_idfy_lbafe *lbafe = &zns->lbafe[nvm->flbas.format];
	struct xnvme_geo *geo = &dev->geo;
	uint64_t nzones;
	int err;

	if (!zns->lbafe[0].zsze) {
		XNVME_DEBUG("FAILED: !zns.lbafe[0].zsze");
		return -EINVAL;
	}

	err = xnvme_znd_stat(dev, XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL, &nzones);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_znd_mgmt_recv()");
		return err;
	}

	geo->type = XNVME_GEO_ZONED;

	geo->npugrp = 1;
	geo->npunit = 1;
	geo->nzone = nzones;
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
	const struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	const struct xnvme_spec_lbaf *lbaf = &nvm->lbaf[nvm->flbas.format];
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

	return 0;
}

static inline int
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

static inline int
_controller_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

	memset(geo, 0, sizeof(*geo));
	geo->type = XNVME_GEO_UNKNOWN;

	///< note: mdts is setup in derive-geometry, so this function does nothing

	return 0;
}

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev)
{
	struct xnvme_geo *geo = &dev->geo;

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

	return 0;
}

int
xnvme_be_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = {0};
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	int err;

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

	//
	// Command-set specific identify controller / namespace
	//

	// Attempt to identify Zoned Namespace
	{
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
		goto exit;

not_zns:
		XNVME_DEBUG("INFO: no positive response to idfy(ZNS)");
	}

	{
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
		goto exit;

not_fs:
		XNVME_DEBUG("INFO: no positive response to idfy(FS)");
	}

	// Attempt to identify LBLK Namespace
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->ident.nsid, XNVME_SPEC_CSI_NVM, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("INFO: not csi-specific id-NVM");
		XNVME_DEBUG("INFO: falling back to NVM assumption");
		err = 0;
		goto exit;
	}
	memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));

exit:
	xnvme_buf_free(dev, idfy_ctrlr);
	xnvme_buf_free(dev, idfy_ns);

	return err;
}

/**
 * Set up mixin of 'be' of the given 'mtype' and matching 'opts' when provided
 */
static int
be_setup(struct xnvme_be *be, enum xnvme_be_mixin_type mtype, struct xnvme_opts *opts)
{
	for (uint64_t j = 0; (j < be->nobjs); ++j) {
		const struct xnvme_be_mixin *mixin = &be->objs[j];

		if (mixin->mtype != mtype) {
			continue;
		}

		switch (mtype) {
		case XNVME_BE_ASYNC:
			if ((opts) && (opts->async) && strcmp(opts->async, mixin->name)) {
				XNVME_DEBUG("INFO: skipping async: '%s' != '%s'", mixin->name,
					    opts->async);
				continue;
			}
			be->async = *mixin->async;
			break;

		case XNVME_BE_SYNC:
			if ((opts) && (opts->sync) && strcmp(opts->sync, mixin->name)) {
				XNVME_DEBUG("INFO: skipping sync: '%s' != '%s'", mixin->name,
					    opts->sync);
				continue;
			}
			be->sync = *mixin->sync;
			break;

		case XNVME_BE_ADMIN:
			if ((opts) && (opts->admin) && strcmp(opts->admin, mixin->name)) {
				XNVME_DEBUG("INFO: skipping admin: '%s' != '%s'", mixin->name,
					    opts->admin);
				continue;
			}
			be->admin = *mixin->admin;
			break;

		case XNVME_BE_DEV:
			be->dev = *mixin->dev;
			break;

		case XNVME_BE_MEM:
			if ((opts) && (opts->mem) && strcmp(opts->mem, mixin->name)) {
				XNVME_DEBUG("INFO: skipping mem: '%s' != '%s'", mixin->name,
					    opts->mem);
				continue;
			}
			be->mem = *mixin->mem;
			break;

		case XNVME_BE_ATTR:
		case XNVME_BE_END:
			XNVME_DEBUG("FAILED: attr | end");
			return -EINVAL;
		}

		return mtype;
	}

	XNVME_DEBUG("FAILED: be: '%s' has no matching mixin of mtype: %x, mtype_key: '%s'",
		    be->attr.name, mtype, xnvme_be_mixin_key(mtype));

	return -ENOSYS;
}

int
xnvme_be_factory(struct xnvme_dev *dev, struct xnvme_opts *opts)
{
	int err = 0;

	for (int i = 0; (i < g_xnvme_be_count) && g_xnvme_be_registry[i]; ++i) {
		struct xnvme_be be = *g_xnvme_be_registry[i];
		int setup = 0;
		XNVME_DEBUG("INFO: checking be: '%s'", be.attr.name);

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}
		if (opts && (opts->be) && strcmp(opts->be, be.attr.name)) {
			XNVME_DEBUG("INFO: skipping be: '%s' != '%s'", be.attr.name, opts->be);
			continue;
		}

		for (int j = 0; j < 5; ++j) {
			int mtype = 1 << j;

			err = be_setup(&be, mtype, opts);
			if (err < 0) {
				XNVME_DEBUG("FAILED: be_setup(%s); err: %d", be.attr.name, err);
				continue;
			}

			setup |= err;
		}

		if (setup != XNVME_BE_CONFIGURED) {
			XNVME_DEBUG("INFO: !configured be(%s); err: %d", be.attr.name, err);
			continue;
		}

		dev->be = be;
		dev->opts = *opts;

		XNVME_DEBUG("INFO: obtained backend instance be: '%s'", be.attr.name);

		err = be.dev.dev_open(dev);
		switch (err < 0 ? -err : err) {
		case 0:
			XNVME_DEBUG("INFO: obtained device handle");
			dev->opts.be = be.attr.name;
			dev->opts.admin = dev->be.admin.id;
			dev->opts.sync = dev->be.sync.id;
			dev->opts.async = dev->be.async.id;
			dev->opts.mem = dev->be.mem.id;
			dev->opts.dev = dev->be.dev.id;
			return 0;

		case EPERM:
			XNVME_DEBUG("FAILED: permission-error; failed and stop trying");
			return err;

		default:
			XNVME_DEBUG("INFO: skipping backend due to err: %d", err);
			break;
		}
	}

	XNVME_DEBUG("FAILED: no backend for uri: '%s'", dev->ident.uri);

	return err ? err : -ENXIO;
}

int
xnvme_enumerate(const char *sys_uri, struct xnvme_opts *opts, xnvme_enumerate_cb cb_func,
		void *cb_args)
{
	struct xnvme_opts opts_default = xnvme_opts_default();
	int err;

	if (!opts) {
		opts = &opts_default;
	}

	for (int i = 0; g_xnvme_be_registry[i]; ++i) {
		struct xnvme_be be = *g_xnvme_be_registry[i];

		if (!be.attr.enabled) {
			XNVME_DEBUG("INFO: skipping be: '%s'; !enabled", be.attr.name);
			continue;
		}

		if (opts && (opts->be) && strcmp(opts->be, be.attr.name)) {
			// skip if opts->be != be.attr.name
			continue;
		}

		err = be_setup(&be, XNVME_BE_DEV, NULL);
		if (err < 0) {
			XNVME_DEBUG("FAILED: be_setup(); err: %d", err);
			continue;
		}

		err = be.dev.enumerate(sys_uri, opts, cb_func, cb_args);
		if (err) {
			XNVME_DEBUG("FAILED: %s->enumerate(...), err: '%s', i: %d",
				    g_xnvme_be_registry[i]->attr.name, strerror(-err), i);
		}
	}

	return 0;
}

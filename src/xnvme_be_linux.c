// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
// Copyright (C) Gurmeet Singh <gur.singh@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_be.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#include <libxnvme_file.h>

#define XNVME_BE_LINUX_SCHM "file"
#define XNVME_BE_LINUX_NAME "linux"

#ifdef XNVME_BE_LINUX_ENABLED
#include <fcntl.h>
#include <errno.h>
#include <linux/version.h>
#include <linux/fs.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include <xnvme_be_linux.h>
#include <xnvme_be_linux_nvme.h>

#include <xnvme_dev.h>
#include <libxnvme_adm.h>
#include <libxnvme_znd.h>

extern struct xnvme_be_sync g_linux_nvme;
extern struct xnvme_be_sync g_linux_block;
extern struct xnvme_be_sync g_linux_fs;

static struct xnvme_be_sync *g_linux_sync[] = {
	&g_linux_nvme,
	&g_linux_block,
	NULL
};
static int
g_linux_sync_count = sizeof g_linux_sync / sizeof * g_linux_sync - 1;

extern struct xnvme_be_async g_linux_thr;
extern struct xnvme_be_async g_linux_aio;
extern struct xnvme_be_async g_linux_iou;
extern struct xnvme_be_async g_linux_nil;

static struct xnvme_be_async *g_linux_async[] = {
	&g_linux_thr,
	&g_linux_iou,
	&g_linux_aio,
	&g_linux_nil,
	NULL
};
static int
g_linux_async_count = sizeof g_linux_async / sizeof * g_linux_async - 1;

int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr opts)
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

	wrtn += fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/%d-%d.%d.%d",
			LINUX_VERSION_CODE,
			(LINUX_VERSION_CODE & (0xff << 16)) >> 16,
			(LINUX_VERSION_CODE & (0xff << 8)) >> 8,
			LINUX_VERSION_CODE & 0xff);

	return wrtn;
}

int
_sysfs_path_to_buf(const char *path, char *buf, int buf_len)
{
	FILE *fp;
	int c;

	fp = fopen(path, "rb");
	if (!fp) {
		return -errno;
	}

	memset(buf, 0, sizeof(char) * buf_len);
	for (int i = 0; (((c = getc(fp)) != EOF) && i < buf_len); ++i) {
		buf[i] = c;
	}

	fclose(fp);

	return 0;
}

int
xnvme_be_linux_sysfs_dev_attr_to_buf(struct xnvme_dev *dev, const char *attr,
				     char *buf, int buf_len)
{
	const char *dev_name = basename(dev->ident.trgt);
	int path_len = 0x1000;
	char path[path_len];

	sprintf(path, "/sys/block/%s/%s", dev_name, attr);

	return _sysfs_path_to_buf(path, buf, buf_len);
}

int
xnvme_be_linux_sysfs_dev_attr_to_num(struct xnvme_dev *dev, const char *attr, uint64_t *num)
{
	const int buf_len = 0x1000;
	char buf[buf_len];
	int base = 10;
	int err;

	err = xnvme_be_linux_sysfs_dev_attr_to_buf(dev, attr, buf, buf_len);
	if (err) {
		XNVME_DEBUG("FAILED: _path_to_buf, err: %d", err);
		return err;
	}

	if ((strlen(buf) > 2) && (buf[0] == '0') && (buf[1] == 'x')) {
		base = 16;
	}

	*num = strtoll(buf, NULL, base);

	return 0;
}

void *
xnvme_be_linux_buf_alloc(const struct xnvme_dev *XNVME_UNUSED(dev), size_t nbytes,
			 uint64_t *XNVME_UNUSED(phys))
{
	// TODO: register buffer when async=iou

	// NOTE: Assign virt to phys?
	return xnvme_buf_virt_alloc(getpagesize(), nbytes);
}

void *
xnvme_be_linux_buf_realloc(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   size_t XNVME_UNUSED(nbytes), uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: xnvme_be_linux: does not support realloc");
	errno = ENOSYS;
	return NULL;
}

void
xnvme_be_linux_buf_free(const struct xnvme_dev *XNVME_UNUSED(dev), void *buf)
{
	// TODO: io_uring unregister buffer

	xnvme_buf_virt_free(buf);
}

int
xnvme_be_linux_buf_vtophys(const struct xnvme_dev *XNVME_UNUSED(dev), void *XNVME_UNUSED(buf),
			   uint64_t *XNVME_UNUSED(phys))
{
	XNVME_DEBUG("FAILED: xnvme_be_linux: does not support phys/DMA alloc");
	return -ENOSYS;
}

void
xnvme_be_linux_state_term(struct xnvme_be_linux_state *state)
{
	if (!state) {
		return;
	}

	close(state->fd);
}

int
xnvme_be_linux_state_init(struct xnvme_dev *dev, void *opts)
{
	struct xnvme_be_linux_opts *linux_opts = opts;
	struct xnvme_be_linux_state *state = (void *)dev->be.state;
	struct stat dev_stat;
	uint32_t opt_val;
	int err;

	if (xnvme_ident_opt_to_val(&dev->ident, "poll_io", &opt_val)) {
		state->poll_io = opt_val == 1;
	}
	if (xnvme_ident_opt_to_val(&dev->ident, "poll_sq", &opt_val)) {
		state->poll_sq = opt_val == 1;
	}
	XNVME_DEBUG("INFO: flags: 0x%x, mode: 0x%x", linux_opts->flags, linux_opts->mode);
	XNVME_DEBUG("state->poll_io: %d", state->poll_io);
	XNVME_DEBUG("state->poll_sq: %d", state->poll_sq);

	state->fd = open(dev->ident.trgt, linux_opts->flags, linux_opts->mode);
	if (state->fd < 0) {
		XNVME_DEBUG("FAILED: open(trgt: '%s'), state->fd: '%d', errno: %d",
			    dev->ident.trgt, state->fd, errno);
		return -errno;
	}
	err = fstat(state->fd, &dev_stat);
	if (err < 0) {
		XNVME_DEBUG("FAILED: err: %d, errno: %d", err, errno);
		return -errno;
	}

	// Determine sync-engine to use and setup func-pointers
	switch (dev_stat.st_mode & S_IFMT) {
	case S_IFREG:
		XNVME_DEBUG("INFO: regular file");
		dev->be.sync = g_linux_fs;
		break;

	case S_IFCHR:
		XNVME_DEBUG("INFO: char-device file");
		dev->be.sync = g_linux_nvme;
		break;

	case S_IFBLK:
		XNVME_DEBUG("INFO: block-device file");
		for (int i = 0; i < g_linux_sync_count; ++i) {
			struct xnvme_be_sync *sync = g_linux_sync[i];

			XNVME_DEBUG("id: %s, enabled: %zu", sync->id, sync->enabled);

			if (!(sync->enabled && sync->supported(dev, 0x0))) {
				XNVME_DEBUG("INFO: skipping: '%s'", sync->id);
				continue;
			}

			dev->be.sync = *sync;
			break;

		}
		break;

	default:
		XNVME_DEBUG("FAILED: unsupported file-type");
		return -EINVAL;
	}

	XNVME_DEBUG("INFO: selected sync: '%s'", dev->be.sync.id);
	if (!(dev->be.sync.enabled && dev->be.sync.supported(dev, 0x0))) {
		XNVME_DEBUG("FAILED: skipping, !enabled || !supported");
		return -ENOSYS;
	}

	// Determine async-engine to use and setup the func-pointers
	{
		char aname[10] = { 0 };
		uint8_t chosen;

		chosen = sscanf(dev->ident.opts, "?async=%9[a-z_]", aname) == 1;

		for (int i = 0; i < g_linux_async_count; ++i) {
			struct xnvme_be_async *queue = g_linux_async[i];

			if (chosen && strncmp(aname, queue->id,
					      XNVME_MIN(strnlen(queue->id, 9), 9))) {
				XNVME_DEBUG("chosen: %s != queue->id: %s",
					    aname, queue->id);
				continue;
			}

			if (queue->enabled && queue->supported(dev, 0x0)) {
				dev->be.async = *queue;
				XNVME_DEBUG("got: %s", queue->id);
				break;
			}
		}
	}

	if (!(dev->be.async.enabled && dev->be.async.supported(dev, 0x0))) {
		XNVME_DEBUG("FAILED: no async-interface");
		return -ENOSYS;
	}

	XNVME_DEBUG("INFO: selected async: '%s'", dev->be.async.id);

	return 0;
}

/**
 * Determine the following:
 *
 * - Determine device-type	(setup: dev->dtype)
 *
 * - Identify controller	(setup: dev->ctrlr)
 *
 * - Identify namespace		(setup: dev->ns)
 * - Identify namespace-id	(setup: dev->nsid)
 * - Determine Command Set	(setup: dev->csi)
 *
 * TODO: fixup for XNVME_DEV_TYPE_NVME_NAMESPACE
 * TODO: fixup for XNVME_DEV_TYPE_BLOCK_DEVICE
 */
int
xnvme_be_linux_dev_idfy(struct xnvme_dev *dev)
{
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_idfy *idfy_ctrlr = NULL, *idfy_ns = NULL;
	int err;

	if (strncmp(dev->be.sync.id, "block_ioctl", 11) == 0) {
		dev->dtype = XNVME_DEV_TYPE_BLOCK_DEVICE;
		dev->csi = XNVME_SPEC_CSI_NVM;
		dev->nsid = 1;
	} else if (strncmp(dev->be.sync.id, g_linux_fs.id, 11) == 0) {
		dev->dtype = XNVME_DEV_TYPE_FS_FILE;
		dev->csi = XNVME_SPEC_CSI_NVM;
		dev->nsid = 1;
	} else {
		dev->dtype = XNVME_DEV_TYPE_NVME_NAMESPACE;
		dev->csi = XNVME_SPEC_CSI_NVM;
		err = xnvme_be_linux_nvme_dev_nsid(dev);
		if (err < 1) {
			XNVME_DEBUG("FAILED: retrieving nsid, got: %x", err);
			// Propagate errno from ioctl
			goto exit;
		}
		dev->nsid = err;

		XNVME_DEBUG("INFO: dev->nsid: %d", dev->nsid);
	}

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

	// Retrieve and store ctrl and ns
	memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ctrlr(&ctx, idfy_ctrlr);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: identify controller");
		err = err ? err : -EIO;
		goto exit;
	}

	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns(&ctx, dev->nsid, idfy_ns);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		XNVME_DEBUG("FAILED: identify namespace, err: %d", err);
		goto exit;
	}
	memcpy(&dev->id.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
	memcpy(&dev->id.ns, idfy_ns, sizeof(*idfy_ns));

	//
	// Determine command-set / namespace type by probing
	//
	dev->csi = XNVME_SPEC_CSI_NVM;		// Assume NVM

	// Attempt to identify Zoned Namespace
	{
		struct xnvme_spec_znd_idfy_ns *zns = (void *)idfy_ns;

		memset(idfy_ctrlr, 0, sizeof(*idfy_ctrlr));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ctrlr_csi(&ctx, XNVME_SPEC_CSI_ZONED, idfy_ctrlr);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ctrlr-zns");
			goto not_zns;
		}

		memset(idfy_ns, 0, sizeof(*idfy_ns));
		ctx = xnvme_cmd_ctx_from_dev(dev);
		err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_ZONED, idfy_ns);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			XNVME_DEBUG("INFO: !id-ns-zns");
			goto not_zns;
		}

		if (!zns->lbafe[0].zsze) {
			goto not_zns;
		}

		memcpy(&dev->idcss.ctrlr, idfy_ctrlr, sizeof(*idfy_ctrlr));
		memcpy(&dev->idcss.ns, idfy_ns, sizeof(*idfy_ns));
		dev->csi = XNVME_SPEC_CSI_ZONED;

		XNVME_DEBUG("INFO: looks like csi(ZNS)");
		goto exit;

not_zns:
		XNVME_DEBUG("INFO: failed idfy with csi(ZNS)");
	}

	// Attempt to identify LBLK Namespace
	memset(idfy_ns, 0, sizeof(*idfy_ns));
	ctx = xnvme_cmd_ctx_from_dev(dev);
	err = xnvme_adm_idfy_ns_csi(&ctx, dev->nsid, XNVME_SPEC_CSI_NVM, idfy_ns);
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

struct xnvme_be_linux_opts
xnvme_be_linux_parse_opts(const struct xnvme_ident *ident)
{
	struct xnvme_be_linux_opts opts = { 0 };

	uint32_t create, direct, rdonly, wronly, rdwr = 0;
	char cmask[5];

	int xnvme_flags = 0;
	if (xnvme_ident_opt_to_val(ident, "create", &create) && create) {
		xnvme_flags |= XNVME_FILE_OFLG_CREATE;
		opts.mode = 0755; // default value
	}

	if (xnvme_ident_opt_to_val(ident, "direct", &direct)) {
		if (direct) {
			xnvme_flags |= XNVME_FILE_OFLG_DIRECT_ON;
		} else {
			xnvme_flags |= XNVME_FILE_OFLG_DIRECT_OFF;
		}
	} else {
		xnvme_flags |= XNVME_FILE_OFLG_DIRECT_ON;
	}

	if (xnvme_ident_opt_to_val(ident, "rdonly", &rdonly)) {
		if (rdonly) {
			xnvme_flags |= XNVME_FILE_OFLG_RDONLY;
		}
	} else if (xnvme_ident_opt_to_val(ident, "wronly", &wronly)) {
		if (wronly) {
			xnvme_flags |= XNVME_FILE_OFLG_WRONLY;
		}
	} else if (xnvme_ident_opt_to_val(ident, "rdwr", &rdwr) && rdwr) {
		if (rdwr) {
			xnvme_flags |= XNVME_FILE_OFLG_RDWR;
		}
	} else {	// None explicitly chosen, set a default
		xnvme_flags |= XNVME_FILE_OFLG_RDWR;
	}

	if (xnvme_ident_opt_to_char_val(ident, "cmask", cmask)) {
		opts.mode = strtol(cmask, NULL, 8);
	}

	if (xnvme_flags & XNVME_FILE_OFLG_CREATE) {
		opts.flags |= O_CREAT;
	}
	if (xnvme_flags & XNVME_FILE_OFLG_DIRECT_ON) {
		opts.flags |= O_DIRECT;
	}
	if (xnvme_flags & XNVME_FILE_OFLG_RDONLY) {
		opts.flags |= O_RDONLY;
	}
	if (xnvme_flags & XNVME_FILE_OFLG_WRONLY) {
		opts.flags |= O_WRONLY;
	}
	if (xnvme_flags & XNVME_FILE_OFLG_RDWR) {
		opts.flags |= O_RDWR;
	}

	return opts;
}

int
xnvme_be_linux_dev_from_ident(const struct xnvme_ident *ident, struct xnvme_dev **dev)
{
	struct xnvme_be_linux_opts opts = xnvme_be_linux_parse_opts(ident);
	int err;

	err = xnvme_dev_alloc(dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_dev_alloc()");
		return err;
	}
	(*dev)->ident = *ident;
	(*dev)->be = xnvme_be_linux;

	// TODO: interpret `ident->opts` and init `xnvme_flags` properly

	// TODO: determine nsid, csi, and device-type
	err = xnvme_be_linux_state_init(*dev, &opts);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_state_init()");
		free(*dev);
		return err;
	}
	err = xnvme_be_linux_dev_idfy(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_linux_dev_idfy()");
		xnvme_be_linux_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}
	err = xnvme_be_dev_derive_geometry(*dev);
	if (err) {
		XNVME_DEBUG("FAILED: xnvme_be_dev_derive_geometry()");
		xnvme_be_linux_state_term((void *)(*dev)->be.state);
		free(*dev);
		return err;
	}

	// TODO: consider this. Due to Kernel-segment constraint force mdts down
	if (((*dev)->geo.mdts_nbytes / (*dev)->geo.lba_nbytes) > 127) {
		(*dev)->geo.mdts_nbytes = (*dev)->geo.lba_nbytes * 127;
	}

	return 0;
}

void
xnvme_be_linux_dev_close(struct xnvme_dev *dev)
{
	if (!dev) {
		return;
	}

	xnvme_be_linux_state_term((void *)dev->be.state);
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
xnvme_be_linux_enumerate(struct xnvme_enumeration *list, const char *sys_uri,
			 int XNVME_UNUSED(opts))
{
	struct dirent **ns = NULL;
	int nns = 0;

	if (sys_uri) {
		XNVME_DEBUG("FAILED: sys_uri: %s is not supported", sys_uri);
		return -ENOSYS;
	}

	nns = scandir("/sys/block", &ns, xnvme_path_nvme_filter, alphasort);
	for (int ni = 0; ni < nns; ++ni) {
		char uri[XNVME_IDENT_URI_LEN] = { 0 };
		struct xnvme_ident ident = { 0 };
		struct xnvme_dev *dev;

		snprintf(uri, XNVME_IDENT_URI_LEN - 1,
			 XNVME_BE_LINUX_SCHM ":" _PATH_DEV "%s",
			 ns[ni]->d_name);
		if (xnvme_ident_from_uri(uri, &ident)) {
			continue;
		}

		if (xnvme_be_linux_dev_from_ident(&ident, &dev)) {
			XNVME_DEBUG("FAILED: xnvme_be_linux_dev_from_ident()");
			continue;
		}
		xnvme_be_linux_dev_close(dev);
		free(dev);

		if (xnvme_enumeration_append(list, &ident)) {
			XNVME_DEBUG("FAILED: adding ident");
		}
	}

	for (int ni = 0; ni < nns; ++ni) {
		free(ns[ni]);
	}
	free(ns);

	return 0;
}
#else
int
xnvme_be_linux_uapi_ver_fpr(FILE *stream, enum xnvme_pr XNVME_UNUSED(opts))
{
	return fprintf(stream, "linux;LINUX_VERSION_CODE-UAPI/NOSYS\n",
}
#endif

static const char *g_schemes[] = {
	XNVME_BE_LINUX_SCHM,
};

struct xnvme_be xnvme_be_linux = {
#ifdef XNVME_BE_LINUX_ENABLED
	.async = XNVME_BE_NOSYS_QUEUE,	///< Selected at runtime
	.sync = XNVME_BE_NOSYS_SYNC,	///< Selected at runtime
	.mem = {
		.buf_alloc = xnvme_be_linux_buf_alloc,
		.buf_realloc = xnvme_be_linux_buf_realloc,
		.buf_free = xnvme_be_linux_buf_free,
		.buf_vtophys = xnvme_be_linux_buf_vtophys,
	},
	.dev = {
		.enumerate = xnvme_be_linux_enumerate,

		.dev_from_ident = xnvme_be_linux_dev_from_ident,
		.dev_close = xnvme_be_linux_dev_close,
	},
#else
	.async = XNVME_BE_NOSYS_QUEUE,
	.sync = XNVME_BE_NOSYS_SYNC,
	.mem = XNVME_BE_NOSYS_MEM,
	.dev = XNVME_BE_NOSYS_DEV,
#endif
	.attr = {
		.name = XNVME_BE_LINUX_NAME,
#ifdef XNVME_BE_LINUX_ENABLED
		.enabled = 1,
#else
		.enabled = 0,
#endif
		.schemes = g_schemes,
		.nschemes = sizeof g_schemes / sizeof(*g_schemes),
	},
	.state = { 0 },
};


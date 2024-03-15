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
	&xnvme_be_spdk,
	&xnvme_be_linux,
	&xnvme_be_fbsd,
	&xnvme_be_macos,
	&xnvme_be_macos_driverkit,
	&xnvme_be_windows,
	&xnvme_be_ramdisk,
	&xnvme_be_vfio,
	NULL,
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

			dev->opts.dev = "FIX-ID-VS-MIXIN-NAME";
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
		XNVME_DEBUG("INFO: found be: '%s'; enabled", be.attr.name);

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

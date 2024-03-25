// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

void
xnvme_opts_set_defaults(struct xnvme_opts *opts)
{
	opts->rdwr = 1;
	opts->nsid = 1;
	opts->admin_timeout = 60000000;
	opts->command_timeout = 30000000;

	// Value is only applicable if the user also sets opts->create = 1
	opts->create_mode = S_IRUSR | S_IWUSR;
}

struct xnvme_opts
xnvme_opts_default(void)
{
	struct xnvme_opts opts = {0};

	xnvme_opts_set_defaults(&opts);

	return opts;
}

int
xnvme_opts_yaml(FILE *stream, const struct xnvme_opts *opts, int indent, const char *sep, int head)
{
	int wrtn = 0;

	if (head) {
		wrtn += fprintf(stream, "%*sxnvme_opts:", indent, "");
		indent += 2;
	}
	if (!opts) {
		wrtn += fprintf(stream, " ~");
		return wrtn;
	}
	if (head) {
		wrtn += fprintf(stream, "\n");
	}

	wrtn += fprintf(stream, "\n");

	wrtn += fprintf(stream, "%*sbe: '%s'%s", indent, "", opts->be, sep);
	wrtn += fprintf(stream, "%*sdev: '%s'%s", indent, "", opts->dev, sep);
	wrtn += fprintf(stream, "%*smem: '%s'%s", indent, "", opts->mem, sep);
	wrtn += fprintf(stream, "%*ssync: '%s'%s", indent, "", opts->sync, sep);
	wrtn += fprintf(stream, "%*sasync: '%s'%s", indent, "", opts->async, sep);
	wrtn += fprintf(stream, "%*sadmin: '%s'%s", indent, "", opts->admin, sep);

	wrtn += fprintf(stream, "%*snsid: 0x%" PRIx32 "%s", indent, "", opts->nsid, sep);

	wrtn += fprintf(stream, "%*srdonly: %" PRIu32 "%s", indent, "", opts->rdonly, sep);
	wrtn += fprintf(stream, "%*swronly: %" PRIu32 "%s", indent, "", opts->wronly, sep);
	wrtn += fprintf(stream, "%*srdwr: %" PRIu32 "%s", indent, "", opts->rdwr, sep);
	wrtn += fprintf(stream, "%*screate: %" PRIu32 "%s", indent, "", opts->create, sep);
	wrtn += fprintf(stream, "%*struncate: %" PRIu32 "%s", indent, "", opts->truncate, sep);
	wrtn += fprintf(stream, "%*sdirect: %" PRIu32 "%s", indent, "", opts->direct, sep);

	wrtn += fprintf(stream, "%*screate_mode: 0x%" PRIx32 "%s", indent, "", opts->create_mode,
			sep);

	wrtn += fprintf(stream, "%*spoll_io: %" PRIu8 "%s", indent, "", opts->poll_io, sep);
	wrtn += fprintf(stream, "%*spoll_sq: %" PRIu8 "%s", indent, "", opts->poll_sq, sep);
	wrtn += fprintf(stream, "%*sregister_files: %" PRIu8 "%s", indent, "",
			opts->register_files, sep);
	wrtn += fprintf(stream, "%*sregister_buffers: %" PRIu8 "%s", indent, "",
			opts->register_buffers, sep);

	wrtn += fprintf(stream, "%*suse_cmb_sqs: 0x%" PRIx32 "%s", indent, "", opts->use_cmb_sqs,
			sep);
	wrtn += fprintf(stream, "%*sshm_id: 0x%" PRIx32 "%s", indent, "", opts->shm_id, sep);
	wrtn += fprintf(stream, "%*smain_core: 0x%" PRIx32 "%s", indent, "", opts->main_core, sep);

	wrtn += fprintf(stream, "%*score_mask: '%s'%s", indent, "", opts->core_mask, sep);
	wrtn += fprintf(stream, "%*sadrfam: '%s'%s", indent, "", opts->adrfam, sep);

	wrtn += fprintf(stream, "%*sspdk_fabrics: 0x%" PRIx32 "%s", indent, "", opts->spdk_fabrics,
			sep);

	return wrtn;
}

int
xnvme_opts_fpr(FILE *stream, const struct xnvme_opts *opts, enum xnvme_pr pr_opts)
{
	int wrtn = 0;

	switch (pr_opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: pr_opts(%x)", pr_opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += xnvme_opts_yaml(stream, opts, 2, "\n", 1);
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_opts_pr(const struct xnvme_opts *opts, int pr_opts)
{
	return xnvme_opts_fpr(stdout, opts, pr_opts);
}

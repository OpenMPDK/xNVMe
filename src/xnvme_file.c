// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <libxnvme_file.h>
#include <xnvme_be_linux.h>


ssize_t
xnvme_file_pread(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

ssize_t
xnvme_file_pwrite(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset)
{
	// TODO: consider returning -EINVAL when mbuf is provided and namespace
	// have extended-lba in effect

	ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
	ctx->cmd.common.nsid = xnvme_dev_get_nsid(ctx->dev);
	ctx->cmd.nvm.slba = offset;

	return xnvme_cmd_pass(ctx, buf, count, NULL, 0);
}

int
xnvme_file_close(struct xnvme_dev *fh)
{
	// TODO: the return value of `close` is not passed all the way back up the
	// stack
	xnvme_dev_close(fh);
	return 0;
}

int
xnvme_file_sync(struct xnvme_dev *fh)
{
	// TODO: I expect that we want to send this through xnvme_cmd_pass
	// or at least have some other level of indirection, allowing us to
	// fsync on multiple OSes
	struct xnvme_be_linux_state *state = (void *)fh->be.state;
	return fsync(state->fd);
}

struct xnvme_cmd_ctx
xnvme_file_get_cmd_ctx(struct xnvme_dev *fh)
{
	return xnvme_cmd_ctx_from_dev(fh);
}


struct flag_opt {
	int flag;
	const char *opt;
};

static struct flag_opt g_flag_opts[] = {
	{ XNVME_FILE_OFLG_CREATE,	"?cmask=0755" },
	{ XNVME_FILE_OFLG_CREATE,	"?create=1" },
	{ XNVME_FILE_OFLG_DIRECT_ON,	"?direct=1" },
	{ XNVME_FILE_OFLG_DIRECT_OFF,	"?direct=0" },
	{ XNVME_FILE_OFLG_RDONLY,	"?rdonly=1" },
	{ XNVME_FILE_OFLG_WRONLY,	"?wronly=1" },
	{ XNVME_FILE_OFLG_RDWR,		"?rdwr=1" },
};
static int g_nflag_opts = sizeof g_flag_opts / sizeof(*g_flag_opts);

struct xnvme_dev *
xnvme_file_open(const char *pathname, int flags)
{
	size_t left = XNVME_IDENT_URI_LEN;
	int wrote = 0;
	char uri[XNVME_IDENT_URI_LEN] = { 0 };

	if (strlen(pathname) >= XNVME_IDENT_URI_LEN) {
		XNVME_DEBUG("FAILED: constructing uri");
		errno = EINVAL;
		return NULL;
	}

	if (!(flags & (XNVME_FILE_OFLG_DIRECT_ON | XNVME_FILE_OFLG_DIRECT_OFF))) {
		flags |= XNVME_FILE_OFLG_DIRECT_OFF;
	}

	XNVME_DEBUG("INFO: strlen(pathname): %zu", strlen(pathname));

	wrote = snprintf(uri, left - 1, "%s", pathname);
	if (wrote < 0 || (wrote >= (int)left - 1)) {
		XNVME_DEBUG("FAILED: constructing uri");
		return NULL;
	}
	left -= wrote;

	XNVME_DEBUG("INFO: left: %zu, wrote: %d", left, wrote);

	for (int i = 0; i < g_nflag_opts; ++i) {
		if (g_flag_opts[i].flag && (!(flags & g_flag_opts[i].flag))) {
			continue;
		}

		wrote = snprintf(uri + XNVME_IDENT_URI_LEN - left, left - 1, "%s",
				 g_flag_opts[i].opt);
		if (wrote < 0 || (wrote >= ((int)left - 1))) {
			XNVME_DEBUG("FAILED: constructing uri");
			return NULL;
		}

		left -= wrote;
	}

	XNVME_DEBUG("INFO: uri: %s, len: %zu", uri, strlen(uri));

	return xnvme_dev_openf(uri, 0x0);
}

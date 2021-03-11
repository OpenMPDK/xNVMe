#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_file.h>

#define IOSIZE_DEF 4096

int
read_write(struct xnvmec *cli)
{
	int size = cli->args.data_nbytes;
	const char *uri = cli->args.sys_uri;
	struct xnvme_dev *dev;
	ssize_t err;

	xnvmec_pinf("opening '%s'\n", uri);
	dev = xnvme_file_open(uri, XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_RDWR);
	if (dev == NULL) {
		xnvmec_perr("no xnvme device. abort mission!\n", errno);
		return errno;
	}
	xnvmec_pinf("opened nvme device %s", uri);

	char *buf;
	struct xnvme_cmd_ctx ctx;

	buf = xnvme_buf_alloc(dev, size);
	for (int u = 0; u < size; u++) {
		buf[u] = 'A';
	}

	ctx = xnvme_file_get_cmd_ctx(dev);
	err = xnvme_file_pwrite(&ctx, buf, size, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_file_pwrite()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	err = xnvme_file_pread(&ctx, buf, size, 0);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_perr("xnvme_file_pread()", err);
		xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
		goto exit;
	}

	xnvmec_pinf("Successfully wrote and read %d bytes to/from %s!", size, uri);

exit:
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);
	return 0;
}

int
dump_sync(struct xnvmec *cli)
{
	const char *fpath;
	struct xnvme_dev *fh;
	int flags;
	size_t buf_nbytes, tbytes, iosize;
	char *buf;

	fpath = cli->args.data_output;
	flags = XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_WRONLY;
	flags |= cli->given[XNVMEC_OPT_DIRECT] ? XNVME_FILE_OFLG_DIRECT_ON : 0x0;
	iosize = cli->given[XNVMEC_OPT_IOSIZE] ? cli->args.iosize : IOSIZE_DEF;

	fh = xnvme_file_open(fpath, flags);
	if (!fh) {
		xnvmec_perr("xnvme_file_open(fh)", errno);
		return errno;
	}
	tbytes = cli->args.data_nbytes;

	buf_nbytes = tbytes;
	buf = xnvme_buf_alloc(fh, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	xnvmec_buf_fill(buf, buf_nbytes, "anum");

	xnvmec_pinf("dump-sync: {fpath: %s, tbytes: %zu, buf_nbytes: %zu iosize: %zu}",
		    fpath, tbytes, buf_nbytes, iosize);

	xnvmec_timer_start(cli);

	for (size_t ofz = 0; ofz < tbytes; ofz += iosize) {
		struct xnvme_cmd_ctx ctx = xnvme_file_get_cmd_ctx(fh);
		size_t nbytes = XNVME_MIN_U64(iosize, tbytes - ofz);
		ssize_t res;

		res = xnvme_file_pwrite(&ctx, buf + ofz, nbytes, ofz);
		if (res || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_file_pwrite(fh)", res);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			goto exit;
		}
	}

	xnvmec_timer_stop(cli);
	xnvmec_timer_bw_pr(cli, "wall-clock", tbytes);

exit:
	xnvme_buf_free(fh, buf);
	xnvme_file_close(fh);
	return 0;
}

static struct xnvmec_sub g_subs[] = {
	{
		"write-read", "Write and read a file",
		"Write and read a file", read_write, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_POSA},
		}
	},
	{
		"dump-sync", "Write a buffer of 'data-nbytes' to file",
		"Write a buffer of 'data-nbytes' to file", dump_sync, {
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_LREQ},
			{XNVMEC_OPT_IOSIZE, XNVMEC_LOPT},
			{XNVMEC_OPT_DIRECT, XNVMEC_LFLG},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "xNVMe file - Exercise the xnvme_file API",
	.descr_short = "Exercise the xnvme_file API",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};


int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_NONE);
}

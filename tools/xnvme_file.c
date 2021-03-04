#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <libxnvme.h>
#include <libxnvmec.h>
#include <libxnvme_file.h>


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

static struct xnvmec_sub g_subs[] = {
	{
		"write-read", "Write and read a file",
		"Write and read a file", read_write, {
			{XNVMEC_OPT_SYS_URI, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_NBYTES, XNVMEC_POSA},
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

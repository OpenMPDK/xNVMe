#include <errno.h>
#include <libxnvme_file.h>
#include <libxnvmec.h>

int
xnvmec_copy_io(struct xnvmec *cli)
{
	const char *input_path = cli->args.data_input, *output_path = cli->args.data_output;
	struct xnvme_dev *input_fh = NULL, *output_fh = NULL;
	size_t file_size = 0;
	void *buf = NULL;
	int err = 0;

	input_fh = xnvme_file_open(input_path, XNVME_FILE_OFLG_RDONLY);
	if (input_fh == NULL) {
		xnvmec_perr("xnvme_file_open()", errno);
		return -errno;
	}
	file_size = xnvme_dev_get_geo(input_fh)->tbytes;

	buf = xnvme_buf_alloc(input_fh, file_size);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		xnvme_file_close(input_fh);
		return -errno;
	}

	output_fh = xnvme_file_open(output_path, XNVME_FILE_OFLG_CREATE | XNVME_FILE_OFLG_WRONLY);
	if (output_fh == NULL) {
		xnvmec_perr("xnvme_file_open()", errno);
		xnvme_buf_free(input_fh, buf);
		xnvme_file_close(input_fh);
		return -errno;
	}

	err = xnvmec_buf_from_file(buf, file_size, input_path);
	if (err) {
		xnvmec_perr("xnvme_buf_from_file()", errno);
		goto exit;
	}

	err = xnvmec_buf_to_file(buf, file_size, output_path);
	if (err) {
		xnvmec_perr("xnvmec_buf_to_file()", errno);
		goto exit;
	}

exit:
	xnvme_buf_free(input_fh, buf);
	xnvme_file_close(input_fh);
	xnvme_file_close(output_fh);
	return err;
}

static struct xnvmec_sub g_subs[] = {
	{
		"copy-xnvmec", "Copy a file using xnvmec_buf_{to,from}_file",
		"Copy a file using xnvmec_buf_{to,from}_file", xnvmec_copy_io, {
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_POSA},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_POSA},
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

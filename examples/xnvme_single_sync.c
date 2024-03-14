// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

/**
 * This example shows how send a single synchronous command
 *
 * - Allocate a buffer for the command payload
 * - Submit the asynchronous command
 * - Reporting on IO-errors
 * - Teardown
 */
int
main(int argc, char **argv)
{
	struct xnvme_opts opts = xnvme_opts_default();
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	int err = 0;

	if (argc < 2) {
		xnvme_cli_perr("Usage: %s <ident>", EINVAL);
		return 1;
	}

	dev = xnvme_dev_open(argv[1], &opts);
	if (!dev) {
		xnvme_cli_perr("xnvme_dev_open()", errno);
		return 1;
	}
	nsid = xnvme_dev_get_nsid(dev);

	buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	xnvme_cli_pinf("Allocate a payload-buffer of nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvme_cli_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	memset(buf, 0, buf_nbytes);

	// This block would typically be a loop as more than a single command would be submitted
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		// Submit and wait for the completion of a read command
		err = xnvme_nvm_read(&ctx, nsid, 0x0, 0, buf, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
		xnvme_cli_pinf("Submitted and completed command succesfully");
	}

exit:
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);

	return err;
}

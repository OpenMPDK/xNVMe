// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>
#include <libxnvme_nvm.h>
#include <libxnvme_lba.h>
#include <libxnvme_util.h>
#include <libxnvmec.h>
#include <time.h>

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
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	int err = 0;

	if (argc < 2) {
		xnvmec_perr("Usage: %s <ident>", EINVAL);
		return 1;
	}

	dev = xnvme_dev_open(argv[1]);
	if (!dev) {
		xnvmec_perr("xnvme_dev_open()", errno);
		return 1;
	}
	nsid = xnvme_dev_get_nsid(dev);

	buf_nbytes = xnvme_dev_get_geo(dev)->nbytes;

	xnvmec_pinf("Allocate a payload-buffer of nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		xnvmec_perr("xnvme_buf_alloc()", errno);
		goto exit;
	}
	memset(buf, 0, buf_nbytes);

	// This block would typically be a loop as more than a single command would be submitted
	{
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		// Submit and wait for the completion of a read command
		err = xnvme_nvm_read(&ctx, nsid, 0x0, 0, buf, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	xnvmec_pinf("Dumping the first 64 bytes of payload-buffer");
	printf("buf[0-63]: '");
	for (size_t i = 0; i < 64; ++i) {
		printf("%c", buf[i]);
	}
	xnvmec_pinf("'");

exit:
	xnvme_buf_free(dev, buf);
	xnvme_dev_close(dev);

	return err;
}

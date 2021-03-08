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

struct cb_args {
	uint32_t ecount;
	uint32_t completed;
	uint32_t submitted;
};

static void
cb_pool(struct xnvme_cmd_ctx *ctx, void *cb_arg)
{
	struct cb_args *cb_args = cb_arg;

	cb_args->completed += 1;

	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
		cb_args->ecount += 1;
	}

	// Completed: Put the command-context back in the queue
	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * This example shows how send a single asynchronous command
 *
 * - Allocate a buffer for the command payload
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using command-contexts
 * - Submit the asynchronous command
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Reporting on IO-errors
 * - Teardown
 */
int
main(int argc, char **argv)
{
	struct cb_args cb_args = { 0 };
	struct xnvme_dev *dev = NULL;
	uint32_t nsid;
	char *buf = NULL;
	size_t buf_nbytes;
	struct xnvme_queue *queue = NULL;
	const int qd = 16;
	int ret = 0, err = 0;

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

	// Initialize a command-queue
	ret = xnvme_queue_init(dev, qd, 0, &queue);
	if (ret) {
		xnvmec_perr("xnvme_queue_init()", ret);
		xnvme_dev_close(dev);
		return 1;
	}

	// Set the default callback function and callback function-argument
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

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
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		// Submit a read command
		err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
		switch (err) {
		// Submission success!
		case 0:
			cb_args.submitted += 1;
			goto next;

		// Submission failed: queue is full => process completions and try again
		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		// Submission failed: unexpected error, put the command-context back in the queue
		default:
			xnvmec_perr("xnvme_nvm_read()", err);

			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}
	}

next:

	// Done submitting, wait for all outstanding I/O to complete as we are about to exit
	ret = xnvme_queue_wait(queue);
	if (ret < 0) {
		xnvmec_perr("xnvme_queue_wait()", ret);
		goto exit;
	}

	// Report if the callback function observed completion-errors
	if (cb_args.ecount) {
		ret = ret ? ret : EIO;
		xnvmec_perr("got completion errors", EIO);
		goto exit;
	}

	xnvmec_pinf("Dumping the first 64 bytes of payload-buffer");
	printf("buf[0-63]: '");
	for (size_t i = 0; i < 64; ++i) {
		printf("%c", buf[i]);
	}
	xnvmec_pinf("'");

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed, cb_args.ecount);

	xnvme_buf_free(dev, buf);
	xnvme_queue_term(queue);
	xnvme_dev_close(dev);

	return ret || err;
}

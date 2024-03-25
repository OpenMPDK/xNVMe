// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static void
cb_fn(struct xnvme_cmd_ctx *ctx, void *XNVME_UNUSED(cb_arg))
{
	if (xnvme_cmd_ctx_cpl_status(ctx)) {
		xnvme_cli_pinf("Command did not complete successfully");
		xnvme_cmd_ctx_pr(ctx, XNVME_PR_DEF);
	} else {
		xnvme_cli_pinf("Command completed succesfully");
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
	struct xnvme_queue *queue = NULL;
	const int qdepth = 16;
	int ret = 0, err = 0;

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

	// Initialize a command-queue
	ret = xnvme_queue_init(dev, qdepth, 0, &queue);
	if (ret) {
		xnvme_cli_perr("xnvme_queue_init()", ret);
		xnvme_dev_close(dev);
		return 1;
	}

	// Set the callback function
	// Note: for this example the cb_arg is NULL
	// However, it would typically be a pointer to a struct
	xnvme_queue_set_cb(queue, cb_fn, NULL);

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
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		// Submit a read command
		err = xnvme_nvm_read(ctx, nsid, 0x0, 0, buf, NULL);
		switch (err) {
		case 0:
			xnvme_cli_pinf("Submission succeeded");
			break;

		// Submission failed: queue is full => process completions and try again
		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		// Submission failed: unexpected error, put the command-context back in the queue
		default:
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_queue_put_cmd_ctx(queue, ctx);
			goto exit;
		}
	}

	// Done submitting, wait for all outstanding I/O to complete as we are about to exit
	ret = xnvme_queue_drain(queue);
	if (ret < 0) {
		xnvme_cli_perr("xnvme_queue_drain()", ret);
		goto exit;
	}

exit:
	xnvme_buf_free(dev, buf);
	xnvme_queue_term(queue);
	xnvme_dev_close(dev);

	return ret || err;
}

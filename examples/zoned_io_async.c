// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme.h>
#include <libxnvme_pp.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_nvm.h>
#include <libxnvme_util.h>
#include <libxnvme_znd.h>
#include <libxnvmec.h>
#include <time.h>

#define DEFAULT_QD 8

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

	xnvme_queue_put_cmd_ctx(ctx->async.queue, ctx);
}

/**
 * This example shows how to read a entire Zone using asynchronous read commands
 *
 * - Allocate buffers for Command Payload
 * - Retrieve a zone-descriptor
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit read commands to read a range of LBAs continously
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Dumping buffers to file
 * - Reporting on IO-errors
 * - Teardown
 */
static int
sub_async_read(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_spec_znd_descr zone = { 0 };
	uint32_t nsid, qd;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	size_t buf_nbytes;
	int err;

	qd = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_FULL, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvmec_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, buf_nbytes, "zero");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing queue and setting default callback function and arguments");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvmec_pinf("Read uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t curs = 0; (curs < zone.zcap) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_nvm_read(ctx, nsid, zone.zslba + curs, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

next:
		++curs;
		payload += geo->lba_nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

	if (cli->args.data_output) {
		xnvmec_pinf("Dumping nbytes: %zu, to: '%s'", buf_nbytes, cli->args.data_output);
		err = xnvmec_buf_to_file(buf, buf_nbytes, cli->args.data_output);
		if (err) {
			xnvmec_perr("xnvmec_buf_to_file()", err);
		}
	}

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

/**
 * This example shows how to write a entire Zone using asynchronous write commands
 *
 * - Allocate buffers for Command Payload
 * - Retrieve a zone-descriptor
 * - Setup a Command Queue
 *   | Callback functions and callback arguments
 *   | Using asynchronous command-contexts
 * - Submit write commands to fill up a zone contiguously under the zone io-constraints
 *   | Re-submission when busy, reap completion, waiting till empty
 * - Reporting on IO-errors
 * - Teardown
 */
static int
sub_async_write(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_spec_znd_descr zone = { 0 };
	uint32_t nsid, qd;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	size_t buf_nbytes;
	int err;

	qd = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvmec_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, buf_nbytes, cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing async. context + alloc/init requests");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvmec_pinf("Writed uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t curs = 0; (curs < zone.zcap) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_nvm_write(ctx, nsid, zone.zslba + curs, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

next:
		// Wait for completion to avoid racing zone.wp
		err = xnvme_queue_wait(queue);
		if (err < 0) {
			xnvmec_perr("xnvme_queue_wait()", err);
			goto exit;
		}

		++curs;
		payload += geo->lba_nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed,
		    cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

/**
 * This function provides an example of writing a Zone in an asynchronous
 * manner, it has all the boiler-plate code taking care of:
 *
 * - Command-payload buffers
 * - Context allocation
 * - Request-pool allocation
 * - As well as sending the append commands and consuming their completion
 */
static int
sub_async_append(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = cli->args.geo;
	struct xnvme_spec_znd_descr zone = { 0 };
	uint32_t nsid, qd;

	struct cb_args cb_args = { 0 };
	struct xnvme_queue *queue = NULL;

	char *buf = NULL, *payload = NULL;
	size_t buf_nbytes;
	int err;

	qd = cli->given[XNVMEC_OPT_QDEPTH] ? cli->args.qdepth : DEFAULT_QD;
	nsid = cli->given[XNVMEC_OPT_NSID] ? cli->args.nsid : xnvme_dev_get_nsid(cli->args.dev);

	if (cli->given[XNVMEC_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvmec_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvmec_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvmec_buf_fill(buf, buf_nbytes, cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Initializing async. context + alloc/init requests");
	err = xnvme_queue_init(dev, qd, 0, &queue);
	if (err) {
		xnvmec_perr("xnvme_queue_init()", err);
		goto exit;
	}
	xnvme_queue_set_cb(queue, cb_pool, &cb_args);

	xnvmec_pinf("Append uri: '%s', qd: %d", cli->args.uri, qd);
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	xnvmec_timer_start(cli);

	payload = buf;
	for (uint64_t curs = 0; (curs < zone.zcap) && !cb_args.ecount;) {
		struct xnvme_cmd_ctx *ctx = xnvme_queue_get_cmd_ctx(queue);

submit:
		err = xnvme_znd_append(ctx, nsid, zone.zslba, 0, payload, NULL);
		switch (err) {
		case 0:
			cb_args.submitted += 1;
			goto next;

		case -EBUSY:
		case -EAGAIN:
			xnvme_queue_poke(queue, 0);
			goto submit;

		default:
			xnvmec_perr("submission-error", EIO);
			goto exit;
		}

next:
		++curs;
		payload += geo->lba_nbytes;
	}

	err = xnvme_queue_wait(queue);
	if (err < 0) {
		xnvmec_perr("xnvme_queue_wait()", err);
		goto exit;
	}

	xnvmec_timer_stop(cli);

	if (cb_args.ecount) {
		err = -EIO;
		xnvmec_perr("got completion errors", err);
		goto exit;
	}

	xnvmec_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

exit:
	xnvmec_pinf("cb_args: {submitted: %u, completed: %u, ecount: %u}",
		    cb_args.submitted, cb_args.completed, cb_args.ecount);

	if (queue) {
		int err_exit = xnvme_queue_term(queue);
		if (err_exit) {
			xnvmec_perr("xnvme_queue_term()", err_exit);
		}
	}
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvmec_sub g_subs[] = {
	{
		"read", "Asynchronous Zone Read of an entire Zone",
		"Asynchronous Zone Read of an entire Zone", sub_async_read, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_OUTPUT, XNVMEC_LOPT},
		}
	},

	{
		"write", "Asynchronous Zone Write until full",
		"Zone asynchronous Write until full", sub_async_write, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},

	{
		"append", "Asynchronous Zone Append until full",
		"Zone asynchronous Append until full", sub_async_append, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
			{XNVMEC_OPT_QDEPTH, XNVMEC_LOPT},
			{XNVMEC_OPT_DATA_INPUT, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Zoned Synchronous IO Example",
	.descr_short =	"Synchronous IO: read / write / append, "
	"using 4k payload at QD1",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

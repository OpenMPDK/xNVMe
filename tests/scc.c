// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>
#include <libxnvme_nvm.h>
#include <libxnvme_spec_pp.h>

/**
 * TODO: add a test, scopy-failure, verifying the following completion-behavior:
 *
 * If the command completes with failure, then Dword 0 of the completion queue
 * entry contains the number of the lowest numbered Source Range entry that was
 * not successfully copied (e.g., if Source Range 0, Source Range 1, Source
 * Range 2, and Source Range 5 are copied successfully and Source Range 3 and
 * Source Range 4 are not copied successfully, then Dword 0 is set to 3). If no
 * data was written to the destination LBAs, then Dword 0 of the completion
 * queue entry shall be set to 0h.
 */

static int
sub_support(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_nvm_idfy_ctrlr *ctrlr;
	struct xnvme_spec_nvm_idfy_ns *ns;
	int err = 0;

	ctrlr = (void *)xnvme_dev_get_ctrlr(dev);
	if (!ctrlr) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ctrlr()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns(dev);
	if (!ns) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ns()", -err);
		return err;
	}
	xnvme_spec_nvm_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_nvm_idfy_ns_pr(ns, XNVME_PR_DEF);

	if (!ctrlr->oncs.copy) {
		err = -ENOSYS;
		xnvmec_perr("!ctrlr->oncs.copy", -err);
	}
	if (!ctrlr->ocfs.copy_fmt0) {
		err = -ENOSYS;
		xnvmec_perr("!ctrlr->ocfs.copy_fmt0", -err);
	}

	if (!ns->mcl) {
		err = -ENOSYS;
		xnvmec_perr("!ns->mcl", -err);
	}
	if (!ns->mssrl) {
		err = -ENOSYS;
		xnvmec_perr("!ns->mssrl", -err);
	}

	xnvmec_pinf(err ? "SCC: is NOT supported" : "SCC: LGTM");

	return err;
}

static int
sub_idfy(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_spec_nvm_idfy_ctrlr *ctrlr;
	struct xnvme_spec_nvm_idfy_ns *ns;
	int err;

	ctrlr = (void *)xnvme_dev_get_ctrlr(dev);
	if (!ctrlr) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ctrlr()", -err);
		return err;
	}
	ns = (void *)xnvme_dev_get_ns(dev);
	if (!ns) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ns()", -err);
		return err;
	}

	xnvme_spec_nvm_idfy_ctrlr_pr(ctrlr, XNVME_PR_DEF);
	xnvme_spec_nvm_idfy_ns_pr(ns, XNVME_PR_DEF);

	xnvmec_pinf("LGTM");

	return 0;
}

static void
cb_noop(struct xnvme_cmd_ctx *XNVME_UNUSED(ctx), void *XNVME_UNUSED(cb_arg)) { }

/**
 * Constructs a 'range' and decides on a 'sdlba' for the Simple-Copy-Command
 * Constructs buffers and writes buffers for verification
 *  - Initialize 'dbuf' with a sequence of alphanumeric chars
 *  - Write 'dbuf' to source LBAs using 'tlbas' # of NVMe write commands
 *  - Initialize 'vbuf' with zeroes
 *  - Write 'vbuf' to destination LBAs using 'tlbas' # of NVMe Write commands
 * Copie source LBAs to destination LBAs using the Simple-Copy-Command
 * Reads destination LBAs into 'vbuf' using 'tlbas' # of NVMe read commands
 * Compares the content of 'dbuf' and 'vbuf' to each other for verification
 *
 * NOTE: When 'tlbas' = 0, then assume max.
 */
static int
_scopy_helper(struct xnvmec *cli, uint64_t tlbas)
{
	struct xnvme_dev *dev = cli->args.dev;
	const struct xnvme_geo *geo = xnvme_dev_get_geo(dev);
	uint32_t nsid = cli->args.nsid;
	struct xnvme_spec_nvm_idfy_ns *ns;
	enum xnvme_nvm_scopy_fmt copy_fmt;
	char *dbuf = NULL, *vbuf = NULL;
	size_t buf_nbytes;
	uint64_t sdlba;
	struct xnvme_spec_nvm_scopy_source_range *range = NULL;
	uint8_t nr;
	int err;

	nsid = cli->given[XNVMEC_OPT_NSID] ? nsid : xnvme_dev_get_nsid(cli->args.dev);

	// Retrieve SCC parameters via idfy-namespace
	ns = (void *)xnvme_dev_get_ns(dev);
	if (!ns) {
		err = -errno;
		xnvmec_perr("xnvme_dev_get_ns()", -err);
		return err;
	}

	xnvmec_pinf("tlbas: %zu, mcl: %u, msrc: %d, mssrl: %u",
		    tlbas, ns->mcl, ns->msrc + 1, ns->mssrl);

	if (!tlbas) {
		err = -EINVAL;
		xnvmec_perr("!tlbas", -err);
		goto exit;
	}
	if (tlbas > (uint64_t)ns->mcl) {
		err = -EINVAL;
		xnvmec_perr("tlbas > ns.mcl", -err);
		goto exit;
	}
	if (tlbas > ((uint64_t)ns->msrc) + 1) {
		err = -EINVAL;
		xnvmec_perr("tlbas > ns.msrc", -err);
		goto exit;
	}

	// Buffer for source-range
	range = xnvme_buf_alloc(dev, sizeof(*range));
	if (!range) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	memset(range, 0, sizeof(*range));

	// Buffers for verification
	buf_nbytes = tlbas * geo->lba_nbytes;
	dbuf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!dbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	vbuf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!vbuf) {
		err = -errno;
		xnvmec_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	xnvmec_buf_fill(dbuf, buf_nbytes, "anum");
	xnvmec_buf_fill(vbuf, buf_nbytes, "zero");

	// TODO: Currently, only a single entry is added per row, construct the
	// range in fancier ways, that is, distributed different over the
	// device, in variations of entries, in variations of entry-lengths.
	for (uint64_t ridx = 0; ridx < tlbas; ++ridx) {
		range->entry[ridx].slba = ridx;
		range->entry[ridx].nlb = 0;
	}
	nr = tlbas - 1;
	sdlba = range->entry[nr].slba + range->entry[nr].nlb + 1;

	// NVMe-struct copy format
	copy_fmt = XNVME_NVM_SCOPY_FMT_ZERO;

	// Write the dbuf to source LBAs
	for (uint64_t i = 0; i < tlbas; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t ofz = i * geo->lba_nbytes;

		err = xnvme_nvm_write(&ctx, nsid, range->entry[i].slba, range->entry[i].nlb,
				      dbuf + ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_nvm_write()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	// Write the vbuf to destination LBAs
	for (uint64_t i = 0; i < tlbas; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t ofz = i * geo->lba_nbytes;

		err = xnvme_nvm_write(&ctx, nsid, sdlba + i, 0, vbuf + ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	xnvmec_pinf("Copying:");
	xnvme_spec_nvm_scopy_source_range_pr(range, nr, XNVME_PR_DEF);

	xnvmec_pinf("To:");
	printf("sdlba: 0x%016lx\n", sdlba);

	if (cli->args.clear) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);

		xnvmec_pinf("Using XNVME_CMD_SYNC mode");
		err = xnvme_nvm_scopy(
			      &ctx, nsid, sdlba, range->entry, nr, copy_fmt);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_nvm_scopy()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	} else {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		struct xnvme_queue *queue = NULL;

		xnvmec_pinf("Using XNVME_CMD_ASYNC mode");

		err = xnvme_queue_init(dev, 2, 0, &queue);
		if (err) {
			xnvmec_perr("xnvme_queue_init()", err);
			goto exit;
		}
		ctx.async.queue = queue;
		ctx.async.cb = cb_noop;

		err = xnvme_nvm_scopy(
			      &ctx, nsid, sdlba, range->entry, nr, copy_fmt);
		if (err) {
			xnvmec_perr("xnvme_nvm_scopy()", err);
			xnvme_queue_term(queue);
			goto exit;
		}
		err = xnvme_queue_wait(queue);
		if (err < 0) {
			xnvmec_perr("xnvme_queue_wait()", err);
			xnvme_queue_term(queue);
			goto exit;
		}
	}

	// Read destination LBAs into vbuf
	for (uint64_t i = 0; i < tlbas; ++i) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		size_t ofz = i * geo->lba_nbytes;

		err = xnvme_nvm_read(&ctx, nsid, sdlba + i, 0, vbuf + ofz, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvmec_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	{
		size_t diff;

		diff = xnvmec_buf_diff(dbuf, vbuf, buf_nbytes);
		if (diff) {
			xnvmec_pinf("verification failed, diff: %zu", diff);
			xnvmec_buf_diff_pr(dbuf, vbuf, buf_nbytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}
	}

	xnvmec_pinf("LGTM");

exit:
	xnvme_buf_free(dev, range);
	xnvme_buf_free(dev, dbuf);
	xnvme_buf_free(dev, vbuf);

	return err;
}

static int
sub_scopy(struct xnvmec *cli)
{
	return _scopy_helper(cli, 8);
}

static int
sub_scopy_msrc(struct xnvmec *cli)
{
	struct xnvme_spec_nvm_idfy_ns *ns;

	ns = (void *)xnvme_dev_get_ns(cli->args.dev);
	if (!ns) {
		xnvmec_perr("xnvme_dev_get_ns()", errno);
		return -errno;
	}

	return _scopy_helper(cli, XNVME_MIN(ns->msrc + 1, ns->mcl));
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"support",
		"Print id-ctrlr-ONCS bits and check for SCC support",
		"Print id-ctrlr-ONCS bits and check for SCC support",
		sub_support, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"idfy",
		"Print SCC related Controller and Namespace identification",
		"Print SCC related Controller and Namespace identification",
		sub_idfy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
		}
	},
	{
		"scopy",
		"Copy one LBA using one Simple-Copy-Command",
		"Copy one LBA using one Simple-Copy-Command"
		"\n\n**NOTE** --clear toggles sync/async command mode",
		sub_scopy, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CLEAR, XNVMEC_LFLG},
		}
	},
	{
		"scopy-msrc",
		"Copy MSRC # of LBAs using one Simple-Copy-Command",
		"Copy one LBA using one Simple-Copy-Command"
		"\n\n**NOTE** --clear toggles sync/async command mode",
		sub_scopy_msrc, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_CLEAR, XNVMEC_LFLG},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Simple-Copy-Command Verification",
	.descr_short = "Simple-Copy-Command Verification",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvme_spec.h>
#include <libxnvme_spec_pp.h>
#include <libxnvme_znd.h>
#include <libxnvmec.h>

static int
test_open_zdptr(struct xnvmec *cli)
{
	struct xnvme_dev *dev = cli->args.dev;
	struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
	struct xnvme_spec_idfy_ns *nvm = (void *)xnvme_dev_get_ns(dev);
	struct xnvme_spec_znd_idfy_ns *zns = (void *)xnvme_dev_get_ns_css(dev);
	uint32_t nsid = xnvme_dev_get_nsid(dev);

	uint32_t zde_nbytes = zns->lbafe[nvm->flbas.format].zdes * 64;
	uint8_t *zde = NULL;

	uint64_t zslba = cli->args.lba;
	uint64_t zidx = 0;

	struct xnvme_znd_report *before = NULL, *after = NULL;

	int err;

	if (!zde_nbytes) {
		xnvmec_pinf("Invalid device: zde_nbytes: %d", zde_nbytes);
		err = -EINVAL;
		goto exit;
	}

	// Allocate and fill Zone Descriptor Extension
	zde = xnvme_buf_alloc(dev, zde_nbytes);
	if (!zde) {
		xnvmec_pinf("xnvme_buf_alloc(zde_nbytes)");
		err = -errno;
		goto exit;
	}
	err = xnvmec_buf_fill(zde, zde_nbytes, "anum");
	if (err) {
		xnvmec_perr("xnvmec_buf_fill()", err);
		goto exit;
	}

	xnvmec_pinf("Retrieving state before EOPEN");

	before = xnvme_znd_report_from_dev(dev, 0x0, 0, 0);
	if (!before) {
		err = -errno;
		xnvmec_perr("xnvme_znd_report_from_dev()", err);
		goto exit;
	}

	xnvmec_pinf("Scan for empty and sequential write ctx. zone");

	for (uint64_t idx = 0; idx < before->nentries; ++idx) {
		struct xnvme_spec_znd_descr *descr = XNVME_ZND_REPORT_DESCR(before, idx);

		if (cli->given[XNVMEC_OPT_SLBA] && \
		    (cli->args.lba != descr->zslba)) {
			continue;
		}

		if ((descr->zs == XNVME_SPEC_ZND_STATE_EMPTY) && \
		    (descr->zt == XNVME_SPEC_ZND_TYPE_SEQWR) && \
		    (descr->zcap)) {
			zslba = descr->zslba;
			zidx = idx;
			break;
		}
	}
	if (!zslba) {
		xnvmec_pinf("Could not find a usable zslba...");
		err = -ENOSPC;
		goto exit;
	}

	xnvmec_pinf("Using: {zslba: 0x%016lx, zidx: %zu}", zslba, zidx);

	xnvmec_pinf("Before");
	xnvme_spec_znd_descr_pr(XNVME_ZND_REPORT_DESCR(before, zidx), XNVME_PR_DEF);

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET, 0x0, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_pinf("xnvme_znd_mgmt_send(RESET)");
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("Sending MGMT-EOPEN");

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN, 0, zde);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvmec_pinf("xnvme_cmd_zone_mgmt(OPEN)");
		err = err ? err : -EIO;
		goto exit;
	}

	xnvmec_pinf("After");

	after = xnvme_znd_report_from_dev(dev, 0x0, 0, 0);
	if (!after) {
		err = -errno;
		xnvmec_perr("xnvme_znd_report_from_dev", err);
		goto exit;
	}

	xnvme_spec_znd_descr_pr(XNVME_ZND_REPORT_DESCR(before, zidx), XNVME_PR_DEF);

	{
		// Verification
		struct xnvme_spec_znd_descr *descr = XNVME_ZND_REPORT_DESCR(after, zidx);
		uint8_t *zde_after = XNVME_ZND_REPORT_DEXT(after, zidx);

		if (xnvmec_buf_diff(zde, zde_after, zde_nbytes)) {
			xnvmec_buf_diff_pr(zde, descr, zde_nbytes,
					   XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}

		if (!descr->za.zdev) {
			xnvmec_pinf("ERR: !descr->za.zdev");
			err = -EIO;
			goto exit;
		}

		if (XNVME_SPEC_ZND_STATE_EOPEN != descr->zs) {
			xnvmec_pinf("ERR: invalid zc: 0x%x", descr->zs);
			err = -EIO;
			goto exit;
		}
	}

exit:
	xnvme_buf_virt_free(before);
	xnvme_buf_virt_free(after);
	xnvme_buf_free(dev, zde);
	return err;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"test_open_zdptr", "jazz", "jazz", test_open_zdptr, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_SLBA, XNVMEC_LOPT},
		}
	},
};

static struct xnvmec g_cli = {
	.title = "Test Zoned Reporting and Management",
	.descr_short = "Test Zoned Reporting and Management",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

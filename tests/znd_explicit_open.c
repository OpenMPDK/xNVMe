// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
test_open_zdptr(struct xnvme_cli *cli)
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
	struct xnvme_spec_znd_descr *descr;

	int err;

	if (!zde_nbytes) {
		xnvme_cli_pinf("Invalid device: zde_nbytes: %d", zde_nbytes);
		err = -EINVAL;
		goto exit;
	}

	// Allocate and fill Zone Descriptor Extension
	zde = xnvme_buf_alloc(dev, zde_nbytes);
	if (!zde) {
		xnvme_cli_pinf("xnvme_buf_alloc(zde_nbytes)");
		err = -errno;
		goto exit;
	}
	err = xnvme_buf_fill(zde, zde_nbytes, "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Retrieving state before EOPEN");

	before = xnvme_znd_report_from_dev(dev, 0x0, 0, 0);
	if (!before) {
		err = -errno;
		xnvme_cli_perr("xnvme_znd_report_from_dev()", err);
		goto exit;
	}

	xnvme_cli_pinf("Scan for empty and sequential write ctx. zone");

	for (uint64_t idx = 0; idx < before->nentries; ++idx) {
		err = xnvme_znd_report_get_descr(before, idx, &descr, NULL);
		if (err) {
			XNVME_DEBUG("xnvme_znd_report_get_descr(), err: %d", err);
			return err;
		}

		if (cli->given[XNVME_CLI_OPT_SLBA] && (cli->args.lba != descr->zslba)) {
			continue;
		}

		if ((descr->zs == XNVME_SPEC_ZND_STATE_EMPTY) &&
		    (descr->zt == XNVME_SPEC_ZND_TYPE_SEQWR) && (descr->zcap)) {
			zslba = descr->zslba;
			zidx = idx;
			break;
		}
	}
	if (!zslba) {
		xnvme_cli_pinf("Could not find a usable zslba...");
		err = -ENOSPC;
		goto exit;
	}

	err = xnvme_znd_report_get_descr(before, zidx, &descr, NULL);
	if (err) {
		XNVME_DEBUG("xnvme_znd_report_get_descr(), zidx: %" PRIu64 " err: %d", zidx err);
		return err;
	}

	xnvme_cli_pinf("Using: {zslba: 0x%016lx, zidx: %zu}", zslba, zidx);
	xnvme_cli_pinf("Before");
	xnvme_spec_znd_descr_pr(descr, XNVME_PR_DEF);

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, false, XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET,
				  0x0, NULL);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_znd_mgmt_send(RESET)");
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("Sending MGMT-EOPEN");

	err = xnvme_znd_mgmt_send(&ctx, nsid, zslba, false, XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN, 0x0,
				  zde);
	if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
		xnvme_cli_pinf("xnvme_cmd_zone_mgmt(OPEN)");
		err = err ? err : -EIO;
		goto exit;
	}

	xnvme_cli_pinf("After");

	after = xnvme_znd_report_from_dev(dev, 0x0, 0, 0);
	if (!after) {
		err = -errno;
		xnvme_cli_perr("xnvme_znd_report_from_dev", err);
		goto exit;
	}

	err = xnvme_znd_report_get_descr(before, zidx, &descr, NULL);
	if (err) {
		XNVME_DEBUG("xnvme_znd_report_get_descr(), zidx: %" PRIu64 " err: %d", zidx err);
		return err;
	}
	xnvme_spec_znd_descr_pr(descr, XNVME_PR_DEF);

	{
		void *zde_after;

		// Verification
		err = xnvme_znd_report_get_descr(after, zidx, &descr, &zde_after);
		if (err) {
			XNVME_DEBUG("xnvme_znd_report_get_descr(), zidx: %" PRIu64 " err: %d",
				    zidx, err);
			return err;
		}

		if (xnvme_buf_diff(zde, zde_after, zde_nbytes)) {
			xnvme_buf_diff_pr(zde, descr, zde_nbytes, XNVME_PR_DEF);
			err = -EIO;
			goto exit;
		}

		if (!descr->za.zdev) {
			xnvme_cli_pinf("ERR: !descr->za.zdev");
			err = -EIO;
			goto exit;
		}

		if (XNVME_SPEC_ZND_STATE_EOPEN != descr->zs) {
			xnvme_cli_pinf("ERR: invalid zc: 0x%x", descr->zs);
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
static struct xnvme_cli_sub g_subs[] = {
	{
		"test_open_zdptr",
		"jazz",
		"jazz",
		test_open_zdptr,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test Zoned Reporting and Management",
	.descr_short = "Test Zoned Reporting and Management",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

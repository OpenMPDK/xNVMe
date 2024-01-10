// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
sub_sync_read(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_znd_descr zone = {0};

	size_t buf_nbytes;
	char *buf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}
	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvme_cli_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvme_buf_fill(buf, buf_nbytes, "zero");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Read from uri: '%s'", cli->args.uri);
	xnvme_cli_timer_start(cli);

	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		void *payload = buf + sect * geo->lba_nbytes;

		err = xnvme_nvm_read(&ctx, nsid, zone.zslba + sect, 0, payload, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_nvm_read()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

	if (cli->args.data_output) {
		xnvme_cli_pinf("Dumping nbytes: %zu, to: '%s'", buf_nbytes, cli->args.data_output);
		err = xnvme_buf_to_file(buf, buf_nbytes, cli->args.data_output);
		if (err) {
			xnvme_cli_perr("xnvme_buf_to_file()", err);
		}
	}

exit:
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

static int
sub_sync_write(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_znd_descr zone = {0};

	size_t buf_nbytes;
	char *buf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}
	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvme_cli_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvme_buf_fill(buf, buf_nbytes,
			     cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Write to uri: '%s'", cli->args.uri);
	xnvme_cli_timer_start(cli);

	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		void *payload = buf + sect * geo->lba_nbytes;

		err = xnvme_nvm_write(&ctx, nsid, zone.zslba + sect, 0, payload, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_cmd_append()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

exit:
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

static int
sub_sync_append(struct xnvme_cli *cli)
{
	struct xnvme_dev *dev = cli->args.device;
	const struct xnvme_geo *geo = cli->args.geo;
	uint32_t nsid = cli->args.nsid;

	struct xnvme_spec_znd_descr zone = {0};

	size_t buf_nbytes;
	char *buf = NULL;
	int err;

	if (!cli->given[XNVME_CLI_OPT_NSID]) {
		nsid = xnvme_dev_get_nsid(cli->args.device);
	}
	if (cli->given[XNVME_CLI_OPT_SLBA]) {
		err = xnvme_znd_descr_from_dev(dev, cli->args.slba, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	} else {
		err = xnvme_znd_descr_from_dev_in_state(dev, XNVME_SPEC_ZND_STATE_EMPTY, &zone);
		if (err) {
			xnvme_cli_perr("xnvme_znd_descr_from_dev()", -err);
			goto exit;
		}
	}
	xnvme_cli_pinf("Using the following zone:");
	xnvme_spec_znd_descr_pr(&zone, XNVME_PR_DEF);

	buf_nbytes = zone.zcap * geo->lba_nbytes;

	xnvme_cli_pinf("Allocating and filling buf_nbytes: %zu", buf_nbytes);
	buf = xnvme_buf_alloc(dev, buf_nbytes);
	if (!buf) {
		err = -errno;
		xnvme_cli_perr("xnvme_buf_alloc()", err);
		goto exit;
	}
	err = xnvme_buf_fill(buf, buf_nbytes,
			     cli->args.data_input ? cli->args.data_input : "anum");
	if (err) {
		xnvme_cli_perr("xnvme_buf_fill()", err);
		goto exit;
	}

	xnvme_cli_pinf("Append to uri: '%s'", cli->args.uri);
	xnvme_cli_timer_start(cli);

	for (uint64_t sect = 0; sect < zone.zcap; ++sect) {
		struct xnvme_cmd_ctx ctx = xnvme_cmd_ctx_from_dev(dev);
		void *payload = buf + sect * geo->lba_nbytes;

		err = xnvme_znd_append(&ctx, nsid, zone.zslba, 0, payload, NULL);
		if (err || xnvme_cmd_ctx_cpl_status(&ctx)) {
			xnvme_cli_perr("xnvme_cmd_append()", err);
			xnvme_cmd_ctx_pr(&ctx, XNVME_PR_DEF);
			err = err ? err : -EIO;
			goto exit;
		}
	}

	xnvme_cli_timer_stop(cli);
	xnvme_cli_timer_bw_pr(cli, "Wall-clock", zone.zcap * geo->lba_nbytes);

exit:
	xnvme_buf_free(dev, buf);

	return err < 0 ? err : 0;
}

//
// Command-Line Interface (CLI) definition
//

static struct xnvme_cli_sub g_subs[] = {
	{
		"read",
		"Zone Read of a full zone",
		"Zone Read of a full zone",
		sub_sync_read,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_OUTPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},

	{
		"write",
		"Zone Write sync. until full",
		"Zone Write sync. until full",
		sub_sync_write,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},

	{
		"append",
		"Zone Append sync. until full",
		"Zone Append sync. until full",
		sub_sync_append,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_NSID, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_SLBA, XNVME_CLI_LOPT},
			{XNVME_CLI_OPT_DATA_INPUT, XNVME_CLI_LOPT},

			XNVME_CLI_SYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Zoned Synchronous IO Example",
	.descr_short = "Synchronous IO: read / write / append, "
		       "using 4k payload at QD1",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
sub_optional(struct xnvme_cli *cli)
{
	int err = 0;

	xnvme_cli_pinf("be: '%s', given: %d", cli->args.be, cli->given[XNVME_CLI_OPT_BE]);
	xnvme_cli_pinf("mem: '%s', given: %d", cli->args.mem, cli->given[XNVME_CLI_OPT_MEM]);
	xnvme_cli_pinf("sync: '%s', given: %d", cli->args.sync, cli->given[XNVME_CLI_OPT_SYNC]);
	xnvme_cli_pinf("async: '%s', given: %d", cli->args.async, cli->given[XNVME_CLI_OPT_ASYNC]);
	xnvme_cli_pinf("admin: '%s', given: %d", cli->args.admin, cli->given[XNVME_CLI_OPT_ADMIN]);
	xnvme_cli_pinf("dev: '%s', given: %d", cli->args.dev, cli->given[XNVME_CLI_OPT_DEV]);

	return err;
}

static struct xnvme_cli_sub g_subs[] = {
	{
		"optional",
		"Optional command-line arguments",
		"Optional command-line arguments",
		sub_optional,
		{
			XNVME_CLI_ASYNC_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Exercise the diffent parsers ",
	.descr_short = "Exercise the different parsers",
	.descr_long = "",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_NONE);
}

// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>

static int
test_buf_alloc_free(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvme_cli_pinf("[alloc/free] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		buf = xnvme_buf_alloc(cli->args.device, buf_nbytes);
		if (!buf) {
			xnvme_cli_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvme_buf_free(cli->args.device, buf);

		xnvme_cli_pinf("buf: %p", buf);
	}

	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGMT: xnvme_buf_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

static int
test_virt_buf_alloc_free(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvme_cli_pinf("[alloc/free] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		buf = xnvme_buf_virt_alloc(0x1000, buf_nbytes);
		if (!buf) {
			xnvme_cli_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvme_cli_pinf("buf: %p", buf);

		xnvme_buf_virt_free(buf);
	}

	printf("\n");
	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGMT: xnvme_buf_virt_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"buf_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_buf_alloc_free,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
	{
		"buf_virt_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_virt_buf_alloc_free,
		{
			{XNVME_CLI_OPT_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_URI, XNVME_CLI_POSA},

			{XNVME_CLI_OPT_NON_POSA_TITLE, XNVME_CLI_SKIP},
			{XNVME_CLI_OPT_COUNT, XNVME_CLI_LREQ},

			XNVME_CLI_ADMIN_OPTS,
		},
	},
};

static struct xnvme_cli g_cli = {
	.title = "Test xNVMe basic buffer alloc/free",
	.descr_short = "Test xNVMe basic buffer alloc/free",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

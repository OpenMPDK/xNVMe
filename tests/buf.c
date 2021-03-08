// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#include <stdio.h>
#include <errno.h>
#include <libxnvmec.h>

static int
test_buf_alloc_free(struct xnvmec *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvmec_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvmec_pinf("[alloc/free] i: %zu, buf_nbytes: %zu",
			    i + 1, buf_nbytes);

		buf = xnvme_buf_alloc(cli->args.dev, buf_nbytes);
		if (!buf) {
			xnvmec_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvme_buf_free(cli->args.dev, buf);

		xnvmec_pinf("buf: %p", buf);
	}

	printf("\n");
	if (nerr) {
		xnvmec_pinf("--={[ Got Errors - see details above ]}=--");
		xnvmec_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvmec_pinf("LGMT: xnvme_buf_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

static int
test_virt_buf_alloc_free(struct xnvmec *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvmec_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;

		printf("\n");
		xnvmec_pinf("[alloc/free] i: %zu, buf_nbytes: %zu",
			    i + 1, buf_nbytes);

		buf = xnvme_buf_virt_alloc(0x1000, buf_nbytes);
		if (!buf) {
			xnvmec_perr("xnvme_buf_alloc()", -errno);
			nerr += 1;
			continue;
		}
		xnvmec_pinf("buf: %p", buf);

		xnvme_buf_virt_free(buf);
	}

	printf("\n");
	if (nerr) {
		xnvmec_pinf("--={[ Got Errors - see details above ]}=--");
		xnvmec_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvmec_pinf("LGMT: xnvme_buf_virt_{alloc,free}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvmec_sub g_subs[] = {
	{
		"buf_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_buf_alloc_free, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_COUNT, XNVMEC_LREQ},
		},
	},
	{
		"buf_virt_alloc_free",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		"Allocate and free a buffer 'count' times of size [1, 2^count]",
		test_virt_buf_alloc_free, {
			{XNVMEC_OPT_URI, XNVMEC_POSA},
			{XNVMEC_OPT_COUNT, XNVMEC_LREQ},
		},
	},
};

static struct xnvmec g_cli = {
	.title = "Test xNVMe basic buffer alloc/free",
	.descr_short = "Test xNVMe basic buffer alloc/free",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvmec(&g_cli, argc, argv, XNVMEC_INIT_DEV_OPEN);
}

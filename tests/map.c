// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <errno.h>
#include <libxnvme.h>
#include <sys/mman.h>
#include <unistd.h>

#define ALIGN_UP(x, a) (((x) + (a)-1) & ~((a)-1))

static int
test_mem_map_unmap(struct xnvme_cli *cli)
{
	uint64_t count = cli->args.count;
	int nerr = 0;

	xnvme_cli_pinf("count: %zu", count);

	for (uint64_t i = 0; i < count; ++i) {
		size_t buf_nbytes = 1 << i;
		void *buf;
		int err;
		long page_size;

		printf("\n");
		xnvme_cli_pinf("[map/unmap] i: %zu, buf_nbytes: %zu", i + 1, buf_nbytes);

		page_size = sysconf(_SC_PAGESIZE);
		buf_nbytes = ALIGN_UP(buf_nbytes, page_size);

		buf = mmap(NULL, buf_nbytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS,
			   0, 0);
		if (buf == MAP_FAILED) {
			xnvme_cli_perr("mmap()", -errno);
			continue;
		}

		err = xnvme_mem_map(cli->args.device, buf, buf_nbytes);
		if (err) {
			xnvme_cli_perr("xnvme_mem_map()", -errno);
			nerr += 1;
			continue;
		}

		err = xnvme_mem_unmap(cli->args.device, buf);
		if (err) {
			xnvme_cli_perr("xnvme_mem_unmap()", -errno);
			nerr += 1;
			continue;
		}
	}

	if (nerr) {
		xnvme_cli_pinf("--={[ Got Errors - see details above ]}=--");
		xnvme_cli_pinf("nerr: %d out of count: %zu", nerr, count);
	} else {
		xnvme_cli_pinf("LGTM: xnvme_mem_{map,unmap}");
	}
	printf("\n");

	return nerr ? -ENOMEM : 0;
}

//
// Command-Line Interface (CLI) definition
//
static struct xnvme_cli_sub g_subs[] = {
	{
		"mem_map_unmap",
		"Map and unmap a buffer 'count' times of size [1, 2^count]",
		"Map and unmap a buffer 'count' times of size [1, 2^count]",
		test_mem_map_unmap,
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
	.title = "Test xNVMe basic memory map/unmap",
	.descr_short = "Test xNVMe basic memory map/unmap",
	.subs = g_subs,
	.nsubs = sizeof g_subs / sizeof(*g_subs),
};

int
main(int argc, char **argv)
{
	return xnvme_cli_run(&g_cli, argc, argv, XNVME_CLI_INIT_DEV_OPEN);
}

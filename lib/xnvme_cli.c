// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#define _XOPEN_SOURCE 700
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <unistd.h>
#include <libxnvme.h>
#include <xnvme_be.h>

#define XNVME_CLI_SUB_MAXOPTS 64

const char *
xnvme_cli_opt_type_str(enum xnvme_cli_opt_type otype)
{
	switch (otype) {
	case XNVME_CLI_POSA:
		return "XNVME_CLI_POSA";
	case XNVME_CLI_LFLG:
		return "XNVME_CLI_LFLG";
	case XNVME_CLI_LOPT:
		return "XNVME_CLI_LOPT";
	case XNVME_CLI_LREQ:
		return "XNVME_CLI_LREQ";
	case XNVME_CLI_SKIP:
		return "XNVME_CLI_SKIP";
	}

	return "XNVME_CLI_ENOSYS";
}

/**
 * Returns the minimum width required for sub-name, should be in the range
 * [0, XNVME_CLI_SUB_NAME_LEN_MAX]
 */
static inline size_t
sub_name_wmin(struct xnvme_cli *cli)
{
	size_t wmin = 0;

	for (int i = 0; i < cli->nsubs; ++i) {
		size_t len = 0;

		len = strnlen(cli->subs[i].name, XNVME_CLI_SUB_NAME_LEN_MAX);
		wmin = len > wmin ? len : wmin;
	}

	return wmin;
}

static struct xnvme_cli_sub *
sub_by_name(struct xnvme_cli *cli, const char *name)
{
	for (int i = 0; i < cli->nsubs; ++i) {
		if (!cli->subs[i].name) {
			break;
		}
		if (!strcmp(cli->subs[i].name, name)) {
			return &cli->subs[i];
		}
	}

	return NULL;
}

// TODO: finish this printer function
void
xnvme_cli_args_pr(struct xnvme_cli_args *args, int opts)
{
	xnvme_cli_pinf("opts: %d", opts);

	for (int i = 0; i < 16; ++i) {
		printf("cdw%i: 0x%" PRIx32 "\n", i, args->cdw[i]);
	}

	printf("uri: '%s'\n", args->uri);
	printf("sys_uri: '%s'\n", args->sys_uri);

	printf("fid: 0x%" PRIx32 "\n", args->fid);
	printf("feat: 0x%" PRIx32 "\n", args->feat);

	printf("status: %" PRIu32 "\n", args->status);
	printf("save: %" PRIu32 "\n", args->save);
	printf("reset: %" PRIu32 "\n", args->reset);
	printf("verbose: %" PRIu32 "\n", args->verbose);
	printf("help: %" PRIu32 "\n", args->help);
}

const char *
xnvme_cli_opt_value_type_str(int vtype)
{
	switch (vtype) {
	case XNVME_CLI_OPT_VTYPE_URI:
		return "uri";
	case XNVME_CLI_OPT_VTYPE_NUM:
		return "NUM";
	case XNVME_CLI_OPT_VTYPE_HEX:
		return "0xNUM";
	case XNVME_CLI_OPT_VTYPE_FILE:
		return "FILE";
	case XNVME_CLI_OPT_VTYPE_STR:
		return "STRING";
	}

	return "ENOSYS";
}

static struct xnvme_cli_opt_attr xnvme_cli_opts[] = {
	{
		.opt = XNVME_CLI_OPT_CDW00,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw0",
		.descr = "Command Dword0",
	},
	{
		.opt = XNVME_CLI_OPT_CDW01,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw1",
		.descr = "Command Dword1",
	},
	{
		.opt = XNVME_CLI_OPT_CDW02,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw2",
		.descr = "Command Dword2",
	},
	{
		.opt = XNVME_CLI_OPT_CDW03,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw3",
		.descr = "Command Dword3",
	},
	{
		.opt = XNVME_CLI_OPT_CDW04,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw4",
		.descr = "Command Dword4",
	},
	{
		.opt = XNVME_CLI_OPT_CDW05,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw5",
		.descr = "Command Dword5",
	},
	{
		.opt = XNVME_CLI_OPT_CDW06,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw6",
		.descr = "Command Dword6",
	},
	{
		.opt = XNVME_CLI_OPT_CDW07,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw7",
		.descr = "Command Dword7",
	},
	{
		.opt = XNVME_CLI_OPT_CDW08,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw8",
		.descr = "Command Dword8",
	},
	{
		.opt = XNVME_CLI_OPT_CDW09,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw9",
		.descr = "Command Dword9",
	},
	{
		.opt = XNVME_CLI_OPT_CDW10,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw10",
		.descr = "Command Dword10",
	},
	{
		.opt = XNVME_CLI_OPT_CDW11,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw11",
		.descr = "Command Dword11",
	},
	{
		.opt = XNVME_CLI_OPT_CDW12,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw12",
		.descr = "Command Dword12",
	},
	{
		.opt = XNVME_CLI_OPT_CDW13,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw13",
		.descr = "Command Dword13",
	},
	{
		.opt = XNVME_CLI_OPT_CDW14,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw14",
		.descr = "Command Dword14",
	},
	{
		.opt = XNVME_CLI_OPT_CDW15,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cdw15",
		.descr = "Command Dword15",
	},
	{
		.opt = XNVME_CLI_OPT_CMD_INPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "cmd-input",
		.descr = "Path to command input-file",
	},
	{
		.opt = XNVME_CLI_OPT_CMD_OUTPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "cmd-output",
		.descr = "Path to command output-file",
	},
	{
		.opt = XNVME_CLI_OPT_DATA_NBYTES,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "data-nbytes",
		.descr = "Data size in bytes",
	},
	{
		.opt = XNVME_CLI_OPT_DATA_INPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "data-input",
		.descr = "Path to data input-file",
	},
	{
		.opt = XNVME_CLI_OPT_DATA_OUTPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "data-output",
		.descr = "Path to data output-file",
	},
	{
		.opt = XNVME_CLI_OPT_META_NBYTES,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "meta-nbytes",
		.descr = "Meta size in bytes",
	},
	{
		.opt = XNVME_CLI_OPT_META_INPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "meta-input",
		.descr = "Path to meta input-file",
	},
	{
		.opt = XNVME_CLI_OPT_META_OUTPUT,
		.vtype = XNVME_CLI_OPT_VTYPE_FILE,
		.name = "meta-output",
		.descr = "Path to meta output-file",
	},
	{
		.opt = XNVME_CLI_OPT_LBAF,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "lbaf",
		.descr = "LBA Format",
	},
	{
		.opt = XNVME_CLI_OPT_SLBA,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "slba",
		.descr = "Start Logical Block Address",
	},
	{
		.opt = XNVME_CLI_OPT_ELBA,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "elba",
		.descr = "End Logical Block Address",
	},
	{
		.opt = XNVME_CLI_OPT_LBA,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "lba",
		.descr = "Logical Block Address",
	},
	{
		.opt = XNVME_CLI_OPT_NLB,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "nlb",
		.descr = "Number of LBAs (NOTE: zero-based value)",
	},
	{
		.opt = XNVME_CLI_OPT_LLB,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "llb",
		.descr = "Length in LBAs (NOTE: one-based value)",
	},
	{
		.opt = XNVME_CLI_OPT_URI,
		.vtype = XNVME_CLI_OPT_VTYPE_URI,
		.name = "uri",
		.descr = "Device URI e.g. '/dev/nvme0n1', '0000:01:00.1', '10.9.8.1.8888', "
			 "'\\\\.\\PhysicalDrive1'",
	},
	{
		.opt = XNVME_CLI_OPT_SYS_URI,
		.vtype = XNVME_CLI_OPT_VTYPE_URI,
		.name = "uri",
		.descr = "System URI e.g. '10.9.8.1:8888'",
	},
	{
		.opt = XNVME_CLI_OPT_SUBNQN,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "subnqn",
		.descr = "Subsystem NQN of the NVMe over Fabrics endpoint e.g. "
			 "'nqn.2022-06.io.xnvme:ctrlnode1'",
	},
	{
		.opt = XNVME_CLI_OPT_HOSTNQN,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "hostnqn",
		.descr = "The host NQN to use when connecting to NVMe over Fabrics "
			 "controllers",
	},
	{
		.opt = XNVME_CLI_OPT_CNTID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cntid",
		.descr = "Controller Identifier",
	},
	{
		.opt = XNVME_CLI_OPT_NSID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "nsid",
		.descr = "Namespace Identifier for Command Construction",
	},
	{
		.opt = XNVME_CLI_OPT_UUID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "uuid",
		.descr = "Universally Unique Identifier",
	},
	{
		.opt = XNVME_CLI_OPT_CNS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "cns",
		.descr = "Controller or Namespace Struct",
	},
	{
		.opt = XNVME_CLI_OPT_CSI,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "csi",
		.descr = "Command Set Identifier",
	},
	{
		.opt = XNVME_CLI_OPT_INDEX,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "index",
		.descr = "Index",
	},
	{
		.opt = XNVME_CLI_OPT_SETID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "setid",
		.descr = "NVM Set Identifier",
	},
	{
		.opt = XNVME_CLI_OPT_LPO_NBYTES,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "lpo-nbytes",
		.descr = "Log-Page Offset (in bytes)",
	},
	{
		.opt = XNVME_CLI_OPT_LID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "lid",
		.descr = "Log-page IDentifier",
	},
	{
		.opt = XNVME_CLI_OPT_LSP,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "lsp",
		.descr = "Log-SPecific parameters",
	},
	{
		.opt = XNVME_CLI_OPT_RAE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "rae",
		.descr = "Reset Async. Events",
	},
	{
		.opt = XNVME_CLI_OPT_ZF,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "zf",
		.descr = "Zone Format",
	},
	{
		.opt = XNVME_CLI_OPT_SES,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "ses",
		.descr = "Secure Erase setting. No=0x0, UserData=0x1, Cryptographic=0x2",
	},
	{
		.opt = XNVME_CLI_OPT_SEL,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "sel",
		.descr = "current=0x0, default=0x1, saved=0x2, supported=0x3",
	},
	{
		.opt = XNVME_CLI_OPT_MSET,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "mset",
		.descr = "Metadata settings. Off=0x0, On=0x1",
	},
	{
		.opt = XNVME_CLI_OPT_AUSE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "ause",
		.descr = "AUSE?",
	},
	{
		.opt = XNVME_CLI_OPT_OVRPAT,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "ovrpat",
		.descr = "Overwrite Pattern",
	},
	{
		.opt = XNVME_CLI_OPT_OWPASS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "owpass",
		.descr = "Overwrite Passes",
	},
	{
		.opt = XNVME_CLI_OPT_OIPBP,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "oipbp",
		.descr = "Overwrite Inverse Bit Pattern",
	},
	{
		.opt = XNVME_CLI_OPT_NODAS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "nodas",
		.descr = "Nodas?",
	},
	{
		.opt = XNVME_CLI_OPT_ACTION,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "action",
		.descr = "Command action",
	},
	{
		.opt = XNVME_CLI_OPT_ZRMS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "zrms",
		.descr = "Zone Resource Management",
	},
	{
		.opt = XNVME_CLI_OPT_PI,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "pi",
		.descr = "Protection Information. Off=0x0, Type1/2/3=0x1/0x2/0x3",
	},
	{
		.opt = XNVME_CLI_OPT_PIL,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "pil",
		.descr = "Protection Information Location. Last=0x0, First=0x1",
	},
	{
		.opt = XNVME_CLI_OPT_FID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "fid",
		.descr = "Feature Identifier",
	},
	{
		.opt = XNVME_CLI_OPT_FEAT,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "feat",
		.descr = "Feature e.g. cdw12 content",
	},
	{
		.opt = XNVME_CLI_OPT_OPCODE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "opcode",
		.descr = "Command opcode",
	},
	{
		.opt = XNVME_CLI_OPT_FLAGS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "flags",
		.descr = "Command flags",
	},
	{
		.opt = XNVME_CLI_OPT_ALL,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "all",
		.descr = "Select / Affect all",
	},
	{
		.opt = XNVME_CLI_OPT_SEED,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "seed",
		.descr = "Use given 'NUM' as random seed",
	},
	{
		.opt = XNVME_CLI_OPT_LIMIT,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "limit",
		.descr = "Restrict amount to 'NUM'",
	},
	{
		.opt = XNVME_CLI_OPT_IOSIZE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "iosize",
		.descr = "Use given 'NUM' as bs/iosize",
	},
	{
		.opt = XNVME_CLI_OPT_QDEPTH,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "qdepth",
		.descr = "Use given 'NUM' as queue max capacity",
	},
	{
		.opt = XNVME_CLI_OPT_DIRECT,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "direct",
		.descr = "Bypass layers",
	},
	{
		.opt = XNVME_CLI_OPT_COUNT,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "count",
		.descr = "Use given 'NUM' as count",
	},
	{
		.opt = XNVME_CLI_OPT_OFFSET,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "offset",
		.descr = "Use given 'NUM' as offset",
	},
	{
		.opt = XNVME_CLI_OPT_CLEAR,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "clear",
		.descr = "Clear something...",
	},
	{
		.opt = XNVME_CLI_OPT_STATUS,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "status",
		.descr = "Provide command state",
	},
	{
		.opt = XNVME_CLI_OPT_SAVE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "save",
		.descr = "Save",
	},
	{
		.opt = XNVME_CLI_OPT_RESET,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "reset",
		.descr = "Reset controller",
	},
	{
		.opt = XNVME_CLI_OPT_VERBOSE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "verbose",
		.descr = "Increase output info",
	},
	{
		.opt = XNVME_CLI_OPT_HELP,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "help",
		.descr = "Show usage / help",
	},
	{
		.opt = XNVME_CLI_OPT_DEV_NSID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "dev-nsid",
		.descr = "Namespace Identifier for Device Handle",
	},
	{
		.opt = XNVME_CLI_OPT_BE,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "be",
		.descr = "xNVMe backend, e.g. 'linux', 'spdk', 'fbsd', 'macos', "
			 "'posix', 'windows'",
	},
	{
		.opt = XNVME_CLI_OPT_MEM,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "mem",
		.descr = "xNVMe buffer/memory manager",
	},
	{
		.opt = XNVME_CLI_OPT_SYNC,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "sync",
		.descr = "xNVMe sync. command-interface, e.g. 'nvme', 'block'",
	},
	{
		.opt = XNVME_CLI_OPT_ASYNC,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "async",
		.descr = "xNVMe async. command-interface, e.g. 'io_uring', 'emu'",
	},
	{
		.opt = XNVME_CLI_OPT_ADMIN,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "admin",
		.descr = "xNVMe admin. command-interface, e.g. 'nvme', 'block'",
	},
	{
		.opt = XNVME_CLI_OPT_DEV,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "dev",
		.descr = "xNVMe dev interface, e.g. 'nvme'",
	},
	{
		.opt = XNVME_CLI_OPT_SHM_ID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "shm_id",
		.descr = "For be=spdk, multi-process shared-memory-id",
	},
	{
		.opt = XNVME_CLI_OPT_MAIN_CORE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "main_core",
		.descr = "For be=spdk, multi-process main core",
	},
	{
		.opt = XNVME_CLI_OPT_CORE_MASK,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "core_mask",
		.descr = "For be=spdk, multi-process core_mask/cpus",
	},
	{
		.opt = XNVME_CLI_OPT_USE_CMB_SQS,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "use_cmb_sqs",
		.descr = "For be=spdk, use controller-memory-buffer for sqs",
	},
	{
		.opt = XNVME_CLI_OPT_CSS,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "css",
		.descr = "For be=spdk, ctrl.config. command-set-selection",
	},
	{
		.opt = XNVME_CLI_OPT_ADRFAM,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "adrfam",
		.descr = "For be=spdk, Fabrics Address-Family e.g. IPv4/IPv6",
	},

	{
		.opt = XNVME_CLI_OPT_POLL_IO,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "poll_io",
		.descr = "For async=io_uring, enable hipri/io-compl.polling",
	},
	{
		.opt = XNVME_CLI_OPT_POLL_SQ,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "poll_sq",
		.descr = "For async=io_uring, enable kernel-side sqthread-poll",
	},
	{
		.opt = XNVME_CLI_OPT_REGISTER_FILES,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "register_files",
		.descr = "For async=io_uring, register files",
	},
	{
		.opt = XNVME_CLI_OPT_REGISTER_BUFFERS,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "register_buffers",
		.descr = "For async=io_uring, register buffers",
	},
	{
		.opt = XNVME_CLI_OPT_TRUNCATE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "truncate",
		.descr = "For files; on-open truncate contents",
	},
	{
		.opt = XNVME_CLI_OPT_RDONLY,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "rdonly",
		.descr = "For files; open read-only",
	},
	{
		.opt = XNVME_CLI_OPT_WRONLY,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "wronly",
		.descr = "For files; open write-only",
	},
	{
		.opt = XNVME_CLI_OPT_RDWR,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "rdwr",
		.descr = "For files; open for read and write",
	},
	{
		.opt = XNVME_CLI_OPT_CREATE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "create",
		.descr = "For files; on-open create",
	},

	{
		.opt = XNVME_CLI_OPT_CREATE_MODE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "create_mode",
		.descr = "For files; create-mode / umask / mode_t in HEX",
	},

	{
		.opt = XNVME_CLI_OPT_VEC_CNT,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "vec-cnt",
		.descr = "Number of elements in vectors when doing vectored IOs",
	},

	{
		.opt = XNVME_CLI_OPT_DTYPE,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "dtype",
		.descr = "Directive type; Identify 0x0, Streams 0x1",
	},
	{
		.opt = XNVME_CLI_OPT_DSPEC,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "dspec",
		.descr = "Directive specification associated with directive type",
	},
	{
		.opt = XNVME_CLI_OPT_DOPER,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "doper",
		.descr = "Directive operation to perform",
	},
	{
		.opt = XNVME_CLI_OPT_ENDIR,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "endir",
		.descr = "Directive enable/disable; Enable 0x1, Disable 0x0",
	},
	{
		.opt = XNVME_CLI_OPT_TGTDIR,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "tgtdir",
		.descr = "Target directive to enable/disable",
	},
	{
		.opt = XNVME_CLI_OPT_NSR,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "nsr",
		.descr = "Namespace streams requested",
	},
	{
		.opt = XNVME_CLI_OPT_POSA_TITLE,
		.vtype = XNVME_CLI_OPT_VTYPE_SKIP,
		.name = "\nPositional arguments:\n",
		.descr = "",
	},
	{
		.opt = XNVME_CLI_OPT_NON_POSA_TITLE,
		.vtype = XNVME_CLI_OPT_VTYPE_SKIP,
		.name = "\nWhere <args> include:\n",
		.descr = "",
	},
	{
		.opt = XNVME_CLI_OPT_ORCH_TITLE,
		.vtype = XNVME_CLI_OPT_VTYPE_SKIP,
		.name = "\nWith <args> for backend:\n",
		.descr = "",
	},

	{
		.opt = XNVME_CLI_OPT_AD,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "ad",
		.descr = "If set, deallocate ranges",
	},
	{
		.opt = XNVME_CLI_OPT_IDW,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "idw",
		.descr = "If set, hint to use range as an integral unit when writing.",
	},
	{
		.opt = XNVME_CLI_OPT_IDR,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "idr",
		.descr = "If set, hint to use range as an integral unit when reading.",
	},
	{
		.opt = XNVME_CLI_OPT_LSI,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "lsi",
		.descr = "Log specific identifier",
	},
	{
		.opt = XNVME_CLI_OPT_PID,
		.vtype = XNVME_CLI_OPT_VTYPE_HEX,
		.name = "pid",
		.descr = "Placement identifier",
	},
	{
		.opt = XNVME_CLI_OPT_KV_KEY,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "key",
		.descr = "KV Pair Key",
	},
	{
		.opt = XNVME_CLI_OPT_KV_VAL,
		.vtype = XNVME_CLI_OPT_VTYPE_STR,
		.name = "value",
		.descr = "KV Pair Value",
	},
	{
		.opt = XNVME_CLI_OPT_KV_STORE_ADD,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "only-add",
		.descr = "KV Store Option to only add new KV-Pairs",
	},
	{
		.opt = XNVME_CLI_OPT_KV_STORE_UPDATE,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "only-update",
		.descr = "KV Store Option to only update existing",
	},

	{
		.opt = XNVME_CLI_OPT_KV_STORE_COMPRESS,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "compress",
		.descr = "KV Store Option to compress value on device",
	},

	{
		.opt = XNVME_CLI_OPT_END,
		.vtype = XNVME_CLI_OPT_VTYPE_NUM,
		.name = "",
		.descr = "",
	},
};

struct xnvme_cli_opt_attr *
xnvme_cli_opt_attr_by_opt(enum xnvme_cli_opt opt, struct xnvme_cli_opt_attr *attrs)
{
	for (int idx = 0; attrs[idx].opt; ++idx) {
		if (attrs[idx].opt != opt) {
			continue;
		}

		return &attrs[idx];
	}

	return NULL;
}

const struct xnvme_cli_opt_attr *
xnvme_cli_get_opt_attr(enum xnvme_cli_opt opt)
{
	return xnvme_cli_opt_attr_by_opt(opt, xnvme_cli_opts);
}

struct xnvme_cli_opt_attr *
xnvme_cli_opt_attr_by_getoptval(int getoptval, struct xnvme_cli_opt_attr *attrs)
{
	for (int idx = 0; attrs[idx].getoptval; ++idx) {
		if (attrs[idx].getoptval != getoptval) {
			continue;
		}

		return &attrs[idx];
	}

	return NULL;
}

void
xnvme_cli_pinf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("# ");
	vprintf(format, args);
	printf("\n");

	va_end(args);

	fflush(stdout);
}

#ifdef WIN32
#include <windows.h>
void
xnvme_cli_perr(const char *msg, int err)
{
	if (err < 0) {
		err = err * -1;
	}
	char err_msg_buff[1024];
	int count;

	count = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err, 0, (LPTSTR)&err_msg_buff,
			      sizeof(err_msg_buff), NULL);

	if (count != 0) {
		fprintf(stderr, "# ERR: '%s': {error code: %d, msg: '%s'}\n", msg, err,
			err_msg_buff);
	} else {
		fprintf(stderr, "# ERR: Format message failed:  {error code: %ld}\n",
			GetLastError());
	}
}
#else
void
xnvme_cli_perr(const char *msg, int err)
{
	fprintf(stderr, "# ERR: '%s': {errno: %d, msg: '%s'}\n", msg, err,
		strerror(err < 0 ? -err : err));

	fflush(stderr);
}
#endif

void
xnvme_cli_usage_sub_long(struct xnvme_cli *cli, struct xnvme_cli_sub *sub)
{
	printf("Usage: %s %s ", cli->argv[0], sub->name);
	for (int oi = 0; oi < XNVME_CLI_SUB_OPTS_LEN; ++oi) {
		struct xnvme_cli_sub_opt *opt = &sub->opts[oi];
		struct xnvme_cli_opt_attr *attr = NULL;

		if ((opt->opt == XNVME_CLI_OPT_END) || (opt->opt == XNVME_CLI_OPT_NONE)) {
			break;
		}

		attr = xnvme_cli_opt_attr_by_opt(opt->opt, xnvme_cli_opts);
		if (!attr) {
			break;
		}

		if (opt->type && opt->type == XNVME_CLI_SKIP) {
			continue;
		}
		if (opt->type && (opt->type != XNVME_CLI_POSA)) {
			break;
		}

		printf("<%s> ", attr->name);
	}

	printf("[<args>]\n");

	if (strnlen(sub->descr_long, 2) > 1) {
		printf("\n%s\n", sub->descr_long);
	}

	for (int oi = 0; oi < XNVME_CLI_SUB_OPTS_LEN; ++oi) {
		struct xnvme_cli_sub_opt *opt = &sub->opts[oi];
		struct xnvme_cli_opt_attr *attr = NULL;
		int width = 0;

		if ((opt->opt == XNVME_CLI_OPT_END) || (opt->opt == XNVME_CLI_OPT_NONE)) {
			break;
		}

		attr = xnvme_cli_opt_attr_by_opt(opt->opt, xnvme_cli_opts);
		if (!attr) {
			break;
		}

		printf("  ");
		switch (opt->type) {
		case XNVME_CLI_POSA:
			width = printf("%s", attr->name);
			break;
		case XNVME_CLI_LREQ:
			width = printf("--%s %s", attr->name,
				       xnvme_cli_opt_value_type_str(attr->vtype));
			break;
		case XNVME_CLI_LOPT:
			width = printf("[ --%s %s ]", attr->name,
				       xnvme_cli_opt_value_type_str(attr->vtype));
			break;
		case XNVME_CLI_LFLG:
			width = printf("[ --%s ]", attr->name);
			break;
		case XNVME_CLI_SKIP:
			width = printf("%s", attr->name);
			break;
		}

		if (opt->type != XNVME_CLI_SKIP) {
			printf("%*s; %s", 30 - width, "", attr->descr);
		}
		printf("\n");
	}

	printf("\n");
	printf("See '%s --help' for other commands\n", cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr(XNVME_PR_DEF);
		printf("\n");
	}
}

void
xnvme_cli_usage_sub_short(struct xnvme_cli_sub *sub, size_t name_width)
{
	int name_len = strnlen(sub->name, XNVME_CLI_SUB_NAME_LEN_MAX);
	int fill = name_width - name_len;

	printf("  %s%*s | ", sub->name, fill + 1, "");

	if (strnlen(sub->descr_short, 2) > 1) {
		printf("%s", sub->descr_short);
	} else {
		printf("Undocumented");
	}

	printf("\n");
}

void
xnvme_cli_usage(struct xnvme_cli *cli)
{
	if (!cli) {
		return;
	}

	printf("Usage: %s <command> [<args>]\n", cli->argv[0]);

	printf("\n");
	printf("Where <command> is one of:\n");
	printf("\n");
	{
		size_t name_width = sub_name_wmin(cli);

		if (name_width < 15) {
			name_width = 15;
		}

		for (int si = 0; si < cli->nsubs; ++si) {
			xnvme_cli_usage_sub_short(&cli->subs[si], name_width);
		}
	}

	printf("\n");
	printf("See '%s <command> --help' for the description of [<args>]\n", cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr(XNVME_PR_DEF);
		printf("\n");
	}
}

int
xnvme_cli_assign_arg(struct xnvme_cli *cli, struct xnvme_cli_opt_attr *opt_attr, char *arg,
		     enum xnvme_cli_opt_type opt_type)
{
	struct xnvme_cli_args *args = &cli->args;
	char *endptr = NULL;
	uint64_t num = 0;

	// Check numerical args
	if (arg && (opt_type != XNVME_CLI_LFLG)) {
		int num_base = opt_attr->vtype == XNVME_CLI_OPT_VTYPE_NUM ? 10 : 16;

		switch (opt_attr->vtype) {
		case XNVME_CLI_OPT_VTYPE_URI:
		case XNVME_CLI_OPT_VTYPE_FILE:
		case XNVME_CLI_OPT_VTYPE_STR:
		case XNVME_CLI_OPT_VTYPE_SKIP:
			break;

		case XNVME_CLI_OPT_VTYPE_NUM:
		case XNVME_CLI_OPT_VTYPE_HEX:
			errno = 0;
			num = strtoll(arg, &endptr, num_base);
			if (errno) {
				XNVME_DEBUG("FAILED: strtoll(), errno: %d", errno);
				errno = EINVAL;
				return -1;
			}
			if (arg == endptr) {
				XNVME_DEBUG("FAILED: strtoll(), no num. !");
				errno = EINVAL;
				return -1;
			}
			break;
		}
	}

	switch (opt_attr->opt) {
	case XNVME_CLI_OPT_CDW00:
		args->cdw[0] = num;
		break;
	case XNVME_CLI_OPT_CDW01:
		args->cdw[1] = num;
		break;
	case XNVME_CLI_OPT_CDW02:
		args->cdw[2] = num;
		break;
	case XNVME_CLI_OPT_CDW03:
		args->cdw[3] = num;
		break;
	case XNVME_CLI_OPT_CDW04:
		args->cdw[4] = num;
		break;
	case XNVME_CLI_OPT_CDW05:
		args->cdw[5] = num;
		break;
	case XNVME_CLI_OPT_CDW06:
		args->cdw[6] = num;
		break;
	case XNVME_CLI_OPT_CDW07:
		args->cdw[7] = num;
		break;
	case XNVME_CLI_OPT_CDW08:
		args->cdw[8] = num;
		break;
	case XNVME_CLI_OPT_CDW09:
		args->cdw[9] = num;
		break;
	case XNVME_CLI_OPT_CDW10:
		args->cdw[10] = num;
		break;
	case XNVME_CLI_OPT_CDW11:
		args->cdw[11] = num;
		break;
	case XNVME_CLI_OPT_CDW12:
		args->cdw[12] = num;
		break;
	case XNVME_CLI_OPT_CDW13:
		args->cdw[13] = num;
		break;
	case XNVME_CLI_OPT_CDW14:
		args->cdw[14] = num;
		break;
	case XNVME_CLI_OPT_CDW15:
		args->cdw[15] = num;
		break;
	case XNVME_CLI_OPT_CMD_INPUT:
		args->cmd_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_CMD_OUTPUT:
		args->cmd_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_DATA_NBYTES:
		args->data_nbytes = num;
		break;
	case XNVME_CLI_OPT_DATA_INPUT:
		args->data_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_DATA_OUTPUT:
		args->data_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_META_NBYTES:
		args->meta_nbytes = num;
		break;
	case XNVME_CLI_OPT_META_INPUT:
		args->meta_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_META_OUTPUT:
		args->meta_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_LBAF:
		args->lbaf = num;
		break;
	case XNVME_CLI_OPT_SLBA:
		args->slba = num;
		break;
	case XNVME_CLI_OPT_ELBA:
		args->elba = num;
		break;
	case XNVME_CLI_OPT_LBA:
		args->lba = num;
		break;
	case XNVME_CLI_OPT_NLB:
		args->nlb = num;
		break;
	case XNVME_CLI_OPT_LLB:
		args->llb = num;
		break;
	case XNVME_CLI_OPT_URI:
		args->uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_SYS_URI:
		args->sys_uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_SUBNQN:
		args->subnqn = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_HOSTNQN:
		args->hostnqn = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_UUID:
		args->uuid = num;
		break;
	case XNVME_CLI_OPT_NSID:
		args->nsid = num;
		break;
	case XNVME_CLI_OPT_DEV_NSID:
		args->dev_nsid = num;
		break;
	case XNVME_CLI_OPT_CNS:
		args->cns = num;
		break;
	case XNVME_CLI_OPT_CSI:
		args->csi = num;
		break;
	case XNVME_CLI_OPT_INDEX:
		args->index = num;
		break;
	case XNVME_CLI_OPT_SETID:
		args->setid = num;
		break;

	case XNVME_CLI_OPT_CNTID:
		args->cntid = num;
		break;
	case XNVME_CLI_OPT_LID:
		args->lid = num;
		break;
	case XNVME_CLI_OPT_LSP:
		args->lsp = num;
		break;
	case XNVME_CLI_OPT_LPO_NBYTES:
		args->action = num;
		break;
	case XNVME_CLI_OPT_RAE:
		args->rae = num;
		break;
	case XNVME_CLI_OPT_CLEAR:
		args->clear = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_ZF:
		args->zf = num;
		break;
	case XNVME_CLI_OPT_SES:
		args->ses = num;
		break;
	case XNVME_CLI_OPT_SEL:
		args->sel = num;
		break;
	case XNVME_CLI_OPT_MSET:
		args->mset = num;
		break;
	case XNVME_CLI_OPT_AUSE:
		args->ause = num;
		break;
	case XNVME_CLI_OPT_OVRPAT:
		args->ovrpat = num;
		break;
	case XNVME_CLI_OPT_OWPASS:
		args->owpass = num;
		break;
	case XNVME_CLI_OPT_OIPBP:
		args->oipbp = num;
		break;
	case XNVME_CLI_OPT_NODAS:
		args->nodas = num;
		break;

	case XNVME_CLI_OPT_ACTION:
		args->action = num;
		break;
	case XNVME_CLI_OPT_ZRMS:
		args->action = num;
		break;
	case XNVME_CLI_OPT_PI:
		args->pi = num;
		break;
	case XNVME_CLI_OPT_PIL:
		args->pil = num;
		break;
	case XNVME_CLI_OPT_FID:
		args->fid = num;
		break;
	case XNVME_CLI_OPT_FEAT:
		args->feat = num;
		break;
	case XNVME_CLI_OPT_SEED:
		args->seed = num;
		break;
	case XNVME_CLI_OPT_LIMIT:
		args->limit = num;
		break;
	case XNVME_CLI_OPT_IOSIZE:
		args->iosize = num;
		break;
	case XNVME_CLI_OPT_QDEPTH:
		args->qdepth = num;
		break;
	case XNVME_CLI_OPT_DIRECT:
		args->direct = arg ? num : 1;
		break;

	case XNVME_CLI_OPT_OPCODE:
		args->opcode = num;
		break;
	case XNVME_CLI_OPT_FLAGS:
		args->flags = num;
		break;

	case XNVME_CLI_OPT_ALL:
		args->all = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_STATUS:
		args->status = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_SAVE:
		args->save = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_RESET:
		args->reset = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_VERBOSE:
		args->verbose = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_HELP:
		args->help = arg ? num : 1;
		break;

	case XNVME_CLI_OPT_COUNT:
		args->count = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_OFFSET:
		args->offset = arg ? num : 1;
		break;

	case XNVME_CLI_OPT_BE:
		args->be = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_MEM:
		args->mem = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_SYNC:
		args->sync = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_ASYNC:
		args->async = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_ADMIN:
		args->admin = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_DEV:
		args->dev = arg ? arg : "INVALID_INPUT";
		break;

	case XNVME_CLI_OPT_SHM_ID:
		args->shm_id = num;
		break;
	case XNVME_CLI_OPT_MAIN_CORE:
		args->main_core = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_CORE_MASK:
		args->core_mask = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_USE_CMB_SQS:
		args->use_cmb_sqs = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_CSS:
		args->css.value = arg ? num : 0;
		args->css.given = arg ? 1 : 0;
		break;

	case XNVME_CLI_OPT_POLL_IO:
		args->poll_io = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_POLL_SQ:
		args->poll_sq = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_REGISTER_FILES:
		args->register_files = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_REGISTER_BUFFERS:
		args->register_buffers = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_TRUNCATE:
		args->truncate = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_RDONLY:
		args->rdonly = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_WRONLY:
		args->wronly = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_RDWR:
		args->rdwr = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_CREATE:
		args->create = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_CREATE_MODE:
		args->create_mode = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_ADRFAM:
		args->adrfam = arg ? arg : "IPv4";
		break;
	case XNVME_CLI_OPT_VEC_CNT:
		args->vec_cnt = arg ? num : 0;
		break;
	case XNVME_CLI_OPT_DTYPE:
		args->dtype = num;
		break;
	case XNVME_CLI_OPT_DSPEC:
		args->dspec = num;
		break;
	case XNVME_CLI_OPT_DOPER:
		args->doper = num;
		break;
	case XNVME_CLI_OPT_ENDIR:
		args->endir = num;
		break;
	case XNVME_CLI_OPT_TGTDIR:
		args->tgtdir = num;
		break;
	case XNVME_CLI_OPT_NSR:
		args->nsr = num;
		break;
	case XNVME_CLI_OPT_AD:
		args->ad = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_IDW:
		args->idw = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_IDR:
		args->idr = arg ? num : 1;
		break;
	case XNVME_CLI_OPT_KV_KEY:
		if (strlen(arg) > 16) {
			xnvme_cli_pinf("KV Key longer than 16 bytes is not supported");
			errno = EINVAL;
			return -1;
		}
		args->kv_key = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_KV_VAL:
		if (strlen(arg) > 4096) {
			errno = EINVAL;
			return -1;
		}
		args->kv_val = arg ? arg : "INVALID_INPUT";
		break;
	case XNVME_CLI_OPT_KV_STORE_UPDATE:
		if (args->kv_store_add) {
			errno = EINVAL;
			xnvme_cli_perr("--update-only and --add-only are mutually exclusive",
				       errno);
			return -1;
		}
		args->kv_store_update = true;
		break;
	case XNVME_CLI_OPT_KV_STORE_ADD:
		if (args->kv_store_update) {
			errno = EINVAL;
			xnvme_cli_perr("--update-only and --add-only are mutually exclusive",
				       errno);
			return -1;
		}
		args->kv_store_add = true;
		break;
	case XNVME_CLI_OPT_KV_STORE_COMPRESS:
		args->kv_store_compress = true;
		break;
	case XNVME_CLI_OPT_LSI:
		args->lsi = num;
		break;
	case XNVME_CLI_OPT_PID:
		args->pid = num;
		break;
	case XNVME_CLI_OPT_POSA_TITLE:
	case XNVME_CLI_OPT_NON_POSA_TITLE:
	case XNVME_CLI_OPT_ORCH_TITLE:
	case XNVME_CLI_OPT_END:
	case XNVME_CLI_OPT_NONE:
		errno = EINVAL;
		XNVME_DEBUG("opt_attr->opt: 0x%x", opt_attr->opt);
		return -1;
	}

	cli->given[opt_attr->opt] = 1;

	return 0;
}

struct xnvme_cli_counts {
	int posa;
	int lreq;
	int lopt;
	int lflg;

	int total_long;
	int total_req;
	int total;
};

void
xnvme_cli_counts_pr(const struct xnvme_cli_counts *counts)
{
	printf("xnvme_cli_counts:");

	if (!counts) {
		printf(" ~\n");
		return;
	}

	printf("\n");
	printf("  posa: %d", counts->posa);
	printf("  lreq: %d", counts->lreq);
	printf("  lopt: %d", counts->lopt);
	printf("  lflg: %d", counts->lflg);

	printf("  total_long: %d", counts->total_long);
	printf("  total_req: %d", counts->total_req);
	printf("  total: %d", counts->total);
	printf("\n");
}

/**
 * Parse at most XNVME_CLI_SUB_OPTS_LEN arguments
 */
int
xnvme_cli_parse(struct xnvme_cli *cli)
{
	struct xnvme_cli_args *args = &cli->args;
	struct option long_options[XNVME_CLI_SUB_OPTS_LEN] = {0};
	struct xnvme_cli_opt_attr *pos_args[XNVME_CLI_SUB_OPTS_LEN] = {0};
	struct xnvme_cli_sub *sub;
	struct xnvme_cli_counts signature = {0};
	struct xnvme_cli_counts parsed = {0};

	const char *sub_name = NULL;

	if (cli->argc < 2) {
		xnvme_cli_pinf("Insufficient arguments: no <command> given");
		errno = EINVAL;
		return -1;
	}

	// Parse the sub-command so we know which PSAs and OPTs to scan for
	sub_name = cli->argv[1];

	sub = sub_by_name(cli, sub_name);
	if (!sub) {
		xnvme_cli_pinf("%s: invalid command: '%s'", cli->argv[0], sub_name);
		errno = EINVAL;
		return -1;
	}

	cli->sub = sub; // Boom!

	// Convert from XNVME_CLI_OPT to getopt long_options
	for (int oi = 0; oi < XNVME_CLI_SUB_OPTS_LEN; ++oi) {
		struct xnvme_cli_sub_opt *sub_opt = &sub->opts[oi];
		struct xnvme_cli_opt_attr *opt_attr = NULL;
		struct option *lopt = NULL;

		if ((oi + 1) >= (XNVME_CLI_SUB_MAXOPTS)) {
			xnvme_cli_pinf("Invalid arguments: nargs-exceeding '%d'",
				       XNVME_CLI_SUB_MAXOPTS);
			errno = EINVAL;
			return -1;
		}

		if ((sub_opt->opt == XNVME_CLI_OPT_END) || (sub_opt->opt == XNVME_CLI_OPT_NONE)) {
			break;
		}

		opt_attr = xnvme_cli_opt_attr_by_opt(sub_opt->opt, xnvme_cli_opts);
		if (!opt_attr) {
			xnvme_cli_pinf("Invalid arguments: cannot parse value");
			errno = EINVAL;
			return -1;
		}

		// "Dynamically" create the option-character, starting at 'a'
		opt_attr->getoptval = 97 + oi;

		switch (sub_opt->type) {
		case XNVME_CLI_LFLG:
			++signature.lflg;
			++signature.total_long;
			break;
		case XNVME_CLI_LOPT:
			++signature.lopt;
			++signature.total_long;
			break;

		case XNVME_CLI_LREQ:
			++signature.lreq;
			++signature.total_long;
			++signature.total_req;
			break;

		case XNVME_CLI_POSA:
			++signature.posa;
			++signature.total_req;
			break;
		case XNVME_CLI_SKIP:
			continue;
		}
		++signature.total;

		switch (sub_opt->type) {
		case XNVME_CLI_LFLG:
		case XNVME_CLI_LREQ:
		case XNVME_CLI_LOPT:

			lopt = &long_options[signature.total_long - 1];
			lopt->name = opt_attr->name;
			lopt->flag = NULL;
			lopt->val = opt_attr->getoptval;

			lopt->has_arg = required_argument;
			if (sub_opt->type == XNVME_CLI_LFLG) {
				lopt->has_arg = no_argument;
			}

			break;

		case XNVME_CLI_POSA:
			pos_args[signature.posa - 1] = opt_attr;
			break;
		case XNVME_CLI_SKIP:
			break;
		}
	}

	// Parse the long-opts
	for (int count = 0; count < signature.total_long; ++count) {
		struct xnvme_cli_sub_opt *sub_opt = NULL;
		struct xnvme_cli_opt_attr *opt_attr = NULL;
		int optidx = 0;
		int ret = 0;
		int found = 0;

		ret = getopt_long(cli->argc, cli->argv, "", long_options, &optidx);
		if (ret == -1) {
			break;
		}

		// Find the option, and the option-attributes matching the getopt-optionchar/ret
		for (int oi = 0; oi < XNVME_CLI_SUB_OPTS_LEN; ++oi) {
			sub_opt = &sub->opts[oi];

			opt_attr = xnvme_cli_opt_attr_by_opt(sub_opt->opt, xnvme_cli_opts);
			if (!opt_attr) {
				xnvme_cli_pinf("no joy");
				errno = EINVAL;
				return -1;
			}

			if (opt_attr->getoptval != ret) {
				continue;
			}

			switch (sub_opt->type) {
			case XNVME_CLI_LFLG:
				++parsed.lflg;
				break;

			case XNVME_CLI_LOPT:
				++parsed.lopt;
				break;

			case XNVME_CLI_LREQ:
				++parsed.lreq;
				++parsed.total_req;
				break;

			case XNVME_CLI_POSA:
				XNVME_DEBUG("Positional out of place");
				errno = EINVAL;
				return -1;

			case XNVME_CLI_SKIP:
				break;
			}

			++parsed.total_long;
			++parsed.total;
			found = 1;
			break;
		}

		if (!found) {
			XNVME_DEBUG("Chaos");
			errno = EINVAL;
			return -1;
		}

		if (xnvme_cli_assign_arg(cli, opt_attr, optarg, sub_opt->type)) {
			XNVME_DEBUG("FAILED: xnvme_cli_assign_arg()");
			xnvme_cli_pinf("invalid argument value(%s)", optarg);
			errno = EINVAL;
			return -1;
		}
	}

	if (args->help) {
		return 0;
	}

	// Should now be able to parse PCAs as they are now shuffled to the end
	// of argv

	// Parse positional arguments
	if (signature.posa) {
		int npos_given;

		if (cli->argc < 3) {
			xnvme_cli_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		npos_given = cli->argc - (optind + 1);

		if (npos_given != signature.posa) {
			xnvme_cli_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		// Skip the sub-command and parse positional arguments
		for (int pos = 0; pos < signature.posa; ++pos) {
			struct xnvme_cli_opt_attr *attr = NULL;
			int idx = pos + (optind + 1);

			if (idx > cli->argc) {
				break;
			}

			attr = pos_args[pos];

			if (xnvme_cli_assign_arg(cli, attr, cli->argv[idx], XNVME_CLI_POSA)) {
				XNVME_DEBUG("FAILED: xnvme_cli_assign_arg()");
				xnvme_cli_pinf("invalid argument value(%s)", cli->argv[idx]);
				errno = EINVAL;
				return -1;
			}

			++parsed.posa;
			++parsed.total_req;
			++parsed.total;
		}
	}

	// Didn't ask for help and insufficient arguments provided
	if ((!args->help) && (parsed.total_req < signature.total_req)) {
		xnvme_cli_pinf("Insufficient required arguments, see: --help");
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int
xnvme_cli_run(struct xnvme_cli *cli, int argc, char **argv, int opts)
{
	int err = 0;

	if (!cli) {
		err = -EINVAL;
		xnvme_cli_perr("xnvme_cli_run(!cli)", err);
		return err;
	}

	cli->argc = argc;
	cli->argv = argv;

	if (!cli->ver_pr) {
		cli->ver_pr = xnvme_ver_pr;
	}

	if ((argc < 2) || (!strcmp(argv[1], "--help")) || (!strcmp(argv[1], "-h"))) {
		xnvme_cli_usage(cli);
		return 0;
	}

	for (int i = 0; i < cli->nsubs; ++i) { // We all need help! ;)
		struct xnvme_cli_sub *sub = &cli->subs[i];

		if (!sub->name) {
			break;
		}

		for (int oi = 0; oi < XNVME_CLI_SUB_OPTS_LEN; ++oi) {
			struct xnvme_cli_sub_opt *sopt = &sub->opts[oi];

			if (sopt->opt == XNVME_CLI_OPT_NONE) {
				sopt->opt = XNVME_CLI_OPT_HELP;
				sopt->type = XNVME_CLI_LFLG;
				break;
			}
		}
	}

	err = xnvme_cli_parse(cli);
	if (err) {
		xnvme_cli_perr("xnvme_cli_run()", errno);
		return err;
	}

	// Arguments parsed without any errors
	if (cli->args.help) {
		xnvme_cli_usage_sub_long(cli, cli->sub);
		return 0;
	}

	if ((opts & XNVME_CLI_INIT_DEV_OPEN) && cli->args.uri) {
		struct xnvme_opts opts = xnvme_opts_default();

		if (xnvme_cli_to_opts(cli, &opts)) {
			xnvme_cli_perr("xnvme_cli_to_opts()", errno);
			return -1;
		}

		cli->args.device = xnvme_dev_open(cli->args.uri, &opts);
		if (!cli->args.device) {
			err = -errno;
			xnvme_cli_perr("xnvme_dev_open()", err);
			return -1;
		}
		cli->args.geo = xnvme_dev_get_geo(cli->args.device);
	}

	err = cli->sub->command(cli);
	if (err) {
		xnvme_cli_perr(cli->sub->name, err);
	}

	if (cli->args.verbose) {
		xnvme_cli_args_pr(&cli->args, 0x0);
	}

	if ((opts & XNVME_CLI_INIT_DEV_OPEN) && cli->args.device) {
		xnvme_dev_close(cli->args.device);
	}

	return err ? 1 : 0;
}

int
xnvme_cli_to_opts(const struct xnvme_cli *cli, struct xnvme_opts *opts)
{
	opts->be = cli->given[XNVME_CLI_OPT_BE] ? cli->args.be : opts->be;
	opts->mem = cli->given[XNVME_CLI_OPT_MEM] ? cli->args.mem : opts->mem;
	opts->sync = cli->given[XNVME_CLI_OPT_SYNC] ? cli->args.sync : opts->sync;
	opts->async = cli->given[XNVME_CLI_OPT_ASYNC] ? cli->args.async : opts->async;
	opts->admin = cli->given[XNVME_CLI_OPT_ADMIN] ? cli->args.admin : opts->admin;
	opts->dev = cli->given[XNVME_CLI_OPT_DEV] ? cli->args.dev : opts->dev;

	opts->nsid = cli->given[XNVME_CLI_OPT_DEV_NSID] ? cli->args.dev_nsid : opts->nsid;

	opts->rdonly = cli->given[XNVME_CLI_OPT_RDONLY] ? cli->args.rdonly : opts->rdonly;
	opts->wronly = cli->given[XNVME_CLI_OPT_WRONLY] ? cli->args.wronly : opts->wronly;
	opts->rdwr = cli->given[XNVME_CLI_OPT_RDWR] ? cli->args.rdwr : opts->rdwr;
	opts->create = cli->given[XNVME_CLI_OPT_CREATE] ? cli->args.create : opts->create;
	opts->truncate = cli->given[XNVME_CLI_OPT_TRUNCATE] ? cli->args.truncate : opts->truncate;
	opts->direct = cli->given[XNVME_CLI_OPT_DIRECT] ? cli->args.direct : opts->direct;

	opts->create_mode =
		cli->given[XNVME_CLI_OPT_CREATE_MODE] ? cli->args.create_mode : opts->create_mode;

	opts->poll_io = cli->given[XNVME_CLI_OPT_POLL_IO] ? cli->args.poll_io : opts->poll_io;
	opts->poll_sq = cli->given[XNVME_CLI_OPT_POLL_SQ] ? cli->args.poll_sq : opts->poll_sq;
	opts->register_files = cli->given[XNVME_CLI_OPT_REGISTER_FILES] ? cli->args.register_files
									: opts->register_files;
	opts->register_buffers = cli->given[XNVME_CLI_OPT_REGISTER_BUFFERS]
					 ? cli->args.register_buffers
					 : opts->register_buffers;

	opts->css.value = cli->given[XNVME_CLI_OPT_CSS] ? cli->args.css.value : opts->css.value;
	opts->css.given = cli->given[XNVME_CLI_OPT_CSS] ? cli->args.css.given : opts->css.given;

	opts->use_cmb_sqs =
		cli->given[XNVME_CLI_OPT_USE_CMB_SQS] ? cli->args.use_cmb_sqs : opts->use_cmb_sqs;
	opts->shm_id = cli->given[XNVME_CLI_OPT_SHM_ID] ? cli->args.shm_id : opts->shm_id;
	opts->main_core =
		cli->given[XNVME_CLI_OPT_MAIN_CORE] ? cli->args.main_core : opts->main_core;
	opts->core_mask =
		cli->given[XNVME_CLI_OPT_CORE_MASK] ? cli->args.core_mask : opts->core_mask;
	opts->adrfam = cli->given[XNVME_CLI_OPT_ADRFAM] ? cli->args.adrfam : opts->adrfam;
	opts->subnqn = cli->given[XNVME_CLI_OPT_SUBNQN] ? cli->args.subnqn : opts->subnqn;
	opts->hostnqn = cli->given[XNVME_CLI_OPT_HOSTNQN] ? cli->args.hostnqn : opts->hostnqn;

	return 0;
}

void
xnvme_cli_enumeration_free(struct xnvme_cli_enumeration *list)
{
	xnvme_buf_virt_free(list);
}

int
xnvme_cli_enumeration_alloc(struct xnvme_cli_enumeration **list, uint32_t capacity)
{
	*list = xnvme_buf_virt_alloc(512, sizeof(**list) + sizeof(*(*list)->entries) * capacity);
	if (!(*list)) {
		XNVME_DEBUG("FAILED: malloc(list + entry * cap(%" PRIu32 "))", capacity);
		return -errno;
	}
	(*list)->capacity = capacity;
	(*list)->nentries = 0;

	return 0;
}

int
xnvme_cli_enumeration_append(struct xnvme_cli_enumeration *list, const struct xnvme_ident *entry)
{
	if (!list->capacity) {
		XNVME_DEBUG("FAILED: syslist->capacity: %" PRIu32, list->capacity);
		return -ENOMEM;
	}
	list->entries[(list->nentries)++] = *entry;
	list->capacity--;

	return 0;
}

int
xnvme_cli_enumeration_fpr(FILE *stream, struct xnvme_cli_enumeration *list, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_cli_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	wrtn += fprintf(stream, "\n");
	wrtn += fprintf(stream, "  capacity: %" PRIu32 "\n", list->capacity);
	wrtn += fprintf(stream, "  nentries: %" PRIu32 "\n", list->nentries);
	wrtn += fprintf(stream, "  entries:");

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint64_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *entry = &list->entries[idx];

		wrtn += fprintf(stream, "\n  - {");
		wrtn += xnvme_ident_yaml(stream, entry, 0, ", ", 0);
		wrtn += fprintf(stream, "}");
	}
	wrtn += fprintf(stream, "\n");

	return wrtn;
}

int
xnvme_cli_enumeration_pr(struct xnvme_cli_enumeration *list, int opts)
{
	return xnvme_cli_enumeration_fpr(stdout, list, opts);
}

/**
 * Check whether the given list has the trgt as associated with the given ident
 *
 * @return Returns 1 it is exist, 0 otherwise.
 */
static int
enumeration_has_ident(struct xnvme_cli_enumeration *list, struct xnvme_ident *ident, uint32_t idx)
{
	uint32_t bound = XNVME_MIN(list->nentries, idx);

	for (uint32_t i = 0; i < bound; ++i) {
		struct xnvme_ident *id = &list->entries[i];

		if (id->nsid != ident->nsid) {
			continue;
		}
		if (id->csi != ident->csi) {
			continue;
		}
		if (id->dtype != ident->dtype) {
			continue;
		}
		if (!strncmp(ident->uri, id->uri, XNVME_IDENT_URI_LEN - 1)) {
			return 1;
		}
	}

	return 0;
}

int
xnvme_cli_enumeration_fpp(FILE *stream, struct xnvme_cli_enumeration *list, int opts)
{
	int wrtn = 0;

	switch (opts) {
	case XNVME_PR_TERSE:
		wrtn += fprintf(stream, "# ENOSYS: opts(%x)", opts);
		return wrtn;

	case XNVME_PR_DEF:
	case XNVME_PR_YAML:
		break;
	}

	wrtn += fprintf(stream, "xnvme_cli_enumeration:");

	if (!list) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	if (!list->nentries) {
		wrtn += fprintf(stream, " ~\n");
		return wrtn;
	}

	for (uint32_t idx = 0; idx < list->nentries; ++idx) {
		struct xnvme_ident *ident = &list->entries[idx];

		if (enumeration_has_ident(list, ident, idx)) {
			continue;
		}

		wrtn += fprintf(stream, "\n");
		wrtn += fprintf(stream, "  - uri: %s\n", ident->uri);
	}

	return wrtn;
}

int
xnvme_cli_enumeration_pp(struct xnvme_cli_enumeration *list, int opts)
{
	return xnvme_cli_enumeration_fpp(stdout, list, opts);
}

uint64_t
xnvme_cli_timer_start(struct xnvme_cli *cli)
{
	cli->timer.start = _xnvme_timer_clock_sample();
	return cli->timer.start;
}

uint64_t
xnvme_cli_timer_stop(struct xnvme_cli *cli)
{
	cli->timer.stop = _xnvme_timer_clock_sample();
	return cli->timer.stop;
}

void
xnvme_cli_timer_bw_pr(struct xnvme_cli *cli, const char *prefix, size_t nbytes)
{
	xnvme_timer_bw_pr(&cli->timer, prefix, nbytes);
}

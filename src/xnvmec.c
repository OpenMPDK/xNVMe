#define _XOPEN_SOURCE 700
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <libxnvmec.h>

const char *
xnvmec_opt_type_str(enum xnvmec_opt_type otype)
{
	switch (otype) {
	case XNVMEC_POSA:
		return "XNVMEC_POSA";
	case XNVMEC_LFLG:
		return "XNVMEC_LFLG";
	case XNVMEC_LOPT:
		return "XNVMEC_LOPT";
	case XNVMEC_LREQ:
		return "XNVMEC_LREQ";
	}

	return "XNVMEC_ENOSYS";
}

void *
xnvmec_buf_clear(void *buf, size_t nbytes)
{
	return memset(buf, 0, nbytes);
}

/**
 * Helper function writing/reading given buf to/from file. When mode has
 * O_WRONLY, the given buffer is written to file, otherwise, it is read.
 *
 * NOTE: this depends on POSIX syscalls
 */
static int
fdio_func(void *buf, size_t nbytes, const char *path, int flags, mode_t mode)
{
	const int do_write = flags & O_WRONLY;
	size_t transferred = 0;
	int hnd, err;

	hnd = open(path, flags, mode);
	if (hnd == -1) {
		XNVME_DEBUG("FAILED: open(), errno: %d", errno);
		return -errno;
	}

	while (transferred < nbytes) {
		const size_t remain = nbytes - transferred;
		size_t nbytes_call = remain < SSIZE_MAX ? remain : SSIZE_MAX;
		ssize_t ret;

		ret = do_write ? write(hnd, buf + transferred, nbytes_call) : \
		      read(hnd, buf + transferred, nbytes_call);
		if (do_write ? ret <= 0 : ret < 0) {
			XNVME_DEBUG("FAILED: do_write: %d, ret: %ld, errno: %d",
				    do_write, ret, errno);
			close(hnd);
			return -errno;
		}

		// when reading, break at end of file
		if (!do_write && ret == 0) {
			break;
		}

		transferred += ret;
	}

	err = close(hnd);
	if (err) {
		XNVME_DEBUG("FAILED: do_write: %d, err: %d, errno: %d",
			    do_write, err, errno);
		return -errno;
	}

	return 0;
}

int
xnvmec_buf_from_file(void *buf, size_t nbytes, const char *path)
{
	return fdio_func(buf, nbytes, path, O_RDONLY, 0x0);
}

int
xnvmec_buf_to_file(void *buf, size_t nbytes, const char *path)
{
	return fdio_func(buf, nbytes, path, O_CREAT | O_TRUNC | O_WRONLY,
			 S_IWUSR | S_IRUSR);
}

int
xnvmec_buf_fill(void *buf, size_t nbytes, const char *content)
{
	uint8_t *cbuf = buf;

	if (!strncmp(content, "anum", 4)) {
		for (size_t i = 0; i < nbytes; ++i) {
			cbuf[i] = (i % 26) + 65;
		}

		return 0;
	}

	if (!strncmp(content, "zero", 4)) {
		xnvmec_buf_clear(buf, nbytes);

		return 0;
	}

	return xnvmec_buf_from_file(buf, nbytes, content);
}

size_t
xnvmec_buf_diff(const void *expected, const void *actual, size_t nbytes)
{
	const uint8_t *exp = expected;
	const uint8_t *act = actual;
	size_t diff = 0;

	for (size_t i = 0; i < nbytes; ++i) {
		if (exp[i] == act[i]) {
			continue;
		}

		++diff;
	}

	return diff;
}

void
xnvmec_buf_diff_pr(const void *expected, const void *actual, size_t nbytes,
		   int XNVME_UNUSED(opts))
{
	const uint8_t *exp = expected;
	const uint8_t *act = actual;
	size_t diff = 0;

	printf("comparison:\n");
	printf("  diffs:\n");
	for (size_t i = 0; i < nbytes; ++i) {
		if (exp[i] == act[i]) {
			continue;
		}

		++diff;
		printf("    - {byte: '%06lu', expected: 0x%x, actual: 0x%x)\n",
		       i, exp[i], act[i]);
	}
	printf("  nbytes: %zu\n", nbytes);
	printf("  nbytes_diff: %zu\n", diff);
}

/**
 * Returns the minimum width required for sub-name, should be in the range
 * [0, XNVMEC_SUB_NAME_LEN_MAX]
 */
static inline size_t
sub_name_wmin(struct xnvmec *cli)
{
	size_t wmin = 0;

	for (int i = 0; i < cli->nsubs; ++i) {
		size_t len = 0;

		len = strnlen(cli->subs[i].name, XNVMEC_SUB_NAME_LEN_MAX);
		wmin = len > wmin ? len : wmin;
	}

	return wmin;
}

struct xnvmec_sub *
sub_by_name(struct xnvmec *cli, const char *name)
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
xnvmec_args_pr(struct xnvmec_args *args, int opts)
{
	xnvmec_pinf("opts: %d", opts);

	for (int i = 0; i < 16; ++i) {
		printf("cdw%i: 0x%x\n", i, args->cdw[i]);
	}

	printf("uri: '%s'\n", args->uri);
	printf("sys_uri: '%s'\n", args->sys_uri);

	printf("fid: 0x%x\n", args->fid);
	printf("feat: 0x%x\n", args->feat);

	printf("status: %d\n", args->status);
	printf("save: %d\n", args->save);
	printf("reset: %d\n", args->reset);
	printf("verbose: %d\n", args->verbose);
	printf("help: %d\n", args->help);
}

enum xnvmec_opt
xnvmec_int2opt(int opt) {
	enum xnvmec_opt val = opt;

	switch (val)
	{
	case XNVMEC_OPT_NONE:
	case XNVMEC_OPT_CDW00:
	case XNVMEC_OPT_CDW01:
	case XNVMEC_OPT_CDW02:
	case XNVMEC_OPT_CDW03:
	case XNVMEC_OPT_CDW04:
	case XNVMEC_OPT_CDW05:
	case XNVMEC_OPT_CDW06:
	case XNVMEC_OPT_CDW07:
	case XNVMEC_OPT_CDW08:
	case XNVMEC_OPT_CDW09:
	case XNVMEC_OPT_CDW10:
	case XNVMEC_OPT_CDW11:
	case XNVMEC_OPT_CDW12:
	case XNVMEC_OPT_CDW13:
	case XNVMEC_OPT_CDW14:
	case XNVMEC_OPT_CDW15:
	case XNVMEC_OPT_CMD_INPUT:
	case XNVMEC_OPT_CMD_OUTPUT:
	case XNVMEC_OPT_DATA_NBYTES:
	case XNVMEC_OPT_DATA_INPUT:
	case XNVMEC_OPT_DATA_OUTPUT:
	case XNVMEC_OPT_META_NBYTES:
	case XNVMEC_OPT_META_INPUT:
	case XNVMEC_OPT_META_OUTPUT:
	case XNVMEC_OPT_LBAF:
	case XNVMEC_OPT_LBA:
	case XNVMEC_OPT_SLBA:
	case XNVMEC_OPT_ELBA:
	case XNVMEC_OPT_NLB:
	case XNVMEC_OPT_URI:
	case XNVMEC_OPT_SYS_URI:
	case XNVMEC_OPT_UUID:
	case XNVMEC_OPT_NSID:
	case XNVMEC_OPT_CNS:
	case XNVMEC_OPT_CSI:
	case XNVMEC_OPT_INDEX:
	case XNVMEC_OPT_SETID:

	case XNVMEC_OPT_CNTID:
	case XNVMEC_OPT_LID:
	case XNVMEC_OPT_LSP:
	case XNVMEC_OPT_LPO_NBYTES:
	case XNVMEC_OPT_RAE:
	case XNVMEC_OPT_CLEAR:
	case XNVMEC_OPT_ZF:
	case XNVMEC_OPT_SES:
	case XNVMEC_OPT_SEL:
	case XNVMEC_OPT_MSET:
	case XNVMEC_OPT_AUSE:
	case XNVMEC_OPT_OVRPAT:
	case XNVMEC_OPT_OWPASS:
	case XNVMEC_OPT_OIPBP:
	case XNVMEC_OPT_NODAS:

	case XNVMEC_OPT_ACTION:
	case XNVMEC_OPT_ZRMS:
	case XNVMEC_OPT_PI:
	case XNVMEC_OPT_PIL:
	case XNVMEC_OPT_FID:
	case XNVMEC_OPT_FEAT:
	case XNVMEC_OPT_SEED:
	case XNVMEC_OPT_LIMIT:
	case XNVMEC_OPT_QDEPTH:
	case XNVMEC_OPT_STATUS:
	case XNVMEC_OPT_SAVE:
	case XNVMEC_OPT_RESET:
	case XNVMEC_OPT_VERBOSE:
	case XNVMEC_OPT_HELP:

	case XNVMEC_OPT_COUNT:
	case XNVMEC_OPT_OFFSET:

	case XNVMEC_OPT_OPCODE:
	case XNVMEC_OPT_FLAGS:
	case XNVMEC_OPT_ALL:
		return val;

	case XNVMEC_OPT_UNUSED01:
	case XNVMEC_OPT_UNUSED02:
	case XNVMEC_OPT_UNUSED03:
	case XNVMEC_OPT_UNUSED04:
	case XNVMEC_OPT_UNUSED05:
	case XNVMEC_OPT_UNUSED06:
	case XNVMEC_OPT_UNUSED07:
	case XNVMEC_OPT_UNUSED08:

	case XNVMEC_OPT_UNKNOWN:
	case XNVMEC_OPT_WEIRD:
	case XNVMEC_OPT_END:
		return XNVMEC_OPT_END;
	}

	return XNVMEC_OPT_END;
}

enum xnvmec_opt_value_type {
	XNVMEC_OPT_VTYPE_URI = 0x1,
	XNVMEC_OPT_VTYPE_NUM = 0x2,
	XNVMEC_OPT_VTYPE_HEX = 0x3,
	XNVMEC_OPT_VTYPE_FILE = 0x4,
};

const char *
xnvmec_opt_value_type_str(int vtype)
{
	switch (vtype) {
	case XNVMEC_OPT_VTYPE_URI:
		return "uri";
	case XNVMEC_OPT_VTYPE_NUM:
		return "NUM";
	case XNVMEC_OPT_VTYPE_HEX:
		return "0xNUM";
	case XNVMEC_OPT_VTYPE_FILE:
		return "FILE";
	}

	return "ENOSYS";
}

struct xnvmec_opt_attr {
	enum xnvmec_opt opt;
	enum xnvmec_opt_value_type vtype;
	const char *name;
	const char *descr;
};

static struct xnvmec_opt_attr xnvmec_opts[] = {
	{XNVMEC_OPT_CDW00,	XNVMEC_OPT_VTYPE_HEX,	"cdw0",		"Command Dword0"},
	{XNVMEC_OPT_CDW01,	XNVMEC_OPT_VTYPE_HEX,	"cdw1",		"Command Dword1"},
	{XNVMEC_OPT_CDW02,	XNVMEC_OPT_VTYPE_HEX,	"cdw2",		"Command Dword2"},
	{XNVMEC_OPT_CDW03,	XNVMEC_OPT_VTYPE_HEX,	"cdw3",		"Command Dword3"},
	{XNVMEC_OPT_CDW04,	XNVMEC_OPT_VTYPE_HEX,	"cdw4",		"Command Dword4"},
	{XNVMEC_OPT_CDW05,	XNVMEC_OPT_VTYPE_HEX,	"cdw5",		"Command Dword5"},
	{XNVMEC_OPT_CDW06,	XNVMEC_OPT_VTYPE_HEX,	"cdw6",		"Command Dword6"},
	{XNVMEC_OPT_CDW07,	XNVMEC_OPT_VTYPE_HEX,	"cdw7",		"Command Dword7"},
	{XNVMEC_OPT_CDW08,	XNVMEC_OPT_VTYPE_HEX,	"cdw8",		"Command Dword8"},
	{XNVMEC_OPT_CDW09,	XNVMEC_OPT_VTYPE_HEX,	"cdw9",		"Command Dword9"},
	{XNVMEC_OPT_CDW10,	XNVMEC_OPT_VTYPE_HEX,	"cdw10",	"Command Dword10"},
	{XNVMEC_OPT_CDW11,	XNVMEC_OPT_VTYPE_HEX,	"cdw11",	"Command Dword11"},
	{XNVMEC_OPT_CDW12,	XNVMEC_OPT_VTYPE_HEX,	"cdw12",	"Command Dword12"},
	{XNVMEC_OPT_CDW13,	XNVMEC_OPT_VTYPE_HEX,	"cdw13",	"Command Dword13"},
	{XNVMEC_OPT_CDW14,	XNVMEC_OPT_VTYPE_HEX,	"cdw14",	"Command Dword14"},
	{XNVMEC_OPT_CDW15,	XNVMEC_OPT_VTYPE_HEX,	"cdw15",	"Command Dword15"},

	{XNVMEC_OPT_CMD_INPUT,		XNVMEC_OPT_VTYPE_FILE,	"cmd-input",	"Path to command input-file"},
	{XNVMEC_OPT_CMD_OUTPUT,		XNVMEC_OPT_VTYPE_FILE,	"cmd-output",	"Path to command output-file"},

	{XNVMEC_OPT_DATA_NBYTES,	XNVMEC_OPT_VTYPE_NUM,	"data-nbytes",	"Data size in bytes"},
	{XNVMEC_OPT_DATA_INPUT,		XNVMEC_OPT_VTYPE_FILE,	"data-input",	"Path to data input-file"},
	{XNVMEC_OPT_DATA_OUTPUT,	XNVMEC_OPT_VTYPE_FILE,	"data-output",	"Path to data output-file"},
	{XNVMEC_OPT_META_NBYTES,	XNVMEC_OPT_VTYPE_NUM,	"meta-nbytes",	"Meta size in bytes"},
	{XNVMEC_OPT_META_INPUT,		XNVMEC_OPT_VTYPE_FILE,	"meta-input",	"Path to meta input-file"},
	{XNVMEC_OPT_META_OUTPUT,	XNVMEC_OPT_VTYPE_FILE,	"meta-output",	"Path to meta output-file"},

	{XNVMEC_OPT_LBAF,		XNVMEC_OPT_VTYPE_HEX,	"lbaf",	"LBA Format"},
	{XNVMEC_OPT_SLBA,		XNVMEC_OPT_VTYPE_HEX,	"slba",	"Start Logical Block Address"},
	{XNVMEC_OPT_ELBA,		XNVMEC_OPT_VTYPE_HEX,	"elba",	"End Logical Block Address"},
	{XNVMEC_OPT_LBA,		XNVMEC_OPT_VTYPE_HEX,	"lba",	"Logical Block Address"},
	{XNVMEC_OPT_NLB,		XNVMEC_OPT_VTYPE_NUM,	"nlb",	"Number of LBAs (NOTE: zero-based value)"},

	{XNVMEC_OPT_URI,		XNVMEC_OPT_VTYPE_URI,	"uri",		"Device URI e.g. /dev/nvme0n1, liou:/dev/nvme0n1 or pci:0000:01:00.1"},
	{XNVMEC_OPT_SYS_URI,		XNVMEC_OPT_VTYPE_URI,	"uri",		"System URI e.g. fab:10.9.8.1:8888"},
	{XNVMEC_OPT_CNTID,		XNVMEC_OPT_VTYPE_HEX,	"cntid",	"Controller Identifier"},
	{XNVMEC_OPT_NSID,		XNVMEC_OPT_VTYPE_HEX,	"nsid",		"Namespace Identifier"},
	{XNVMEC_OPT_UUID,		XNVMEC_OPT_VTYPE_HEX,	"uuid",		"Universally Unique Identifier"},
	{XNVMEC_OPT_CNS,		XNVMEC_OPT_VTYPE_HEX,	"cns",		"Controller or Namespace Struct"},
	{XNVMEC_OPT_CSI,		XNVMEC_OPT_VTYPE_HEX,	"csi",		"Command Set Identifier"},
	{XNVMEC_OPT_INDEX,		XNVMEC_OPT_VTYPE_HEX,	"index",	"Index"},
	{XNVMEC_OPT_SETID,		XNVMEC_OPT_VTYPE_HEX,	"setid",	"NVM Set Identifier"},

	{XNVMEC_OPT_LPO_NBYTES,		XNVMEC_OPT_VTYPE_NUM,	"lpo-nbytes",	"Log-Page Offset (in bytes)"},
	{XNVMEC_OPT_LID,		XNVMEC_OPT_VTYPE_HEX,	"lid",		"Log-page IDentifier"},
	{XNVMEC_OPT_LSP,		XNVMEC_OPT_VTYPE_HEX,	"lsp",		"Log-SPecific parameters"},
	{XNVMEC_OPT_RAE,		XNVMEC_OPT_VTYPE_HEX,	"rae",		"Reset Async. Events"},

	{XNVMEC_OPT_ZF,		XNVMEC_OPT_VTYPE_HEX,	"zf",		"ZOne Format"},
	{XNVMEC_OPT_SES,	XNVMEC_OPT_VTYPE_HEX,	"ses",		"Ses?"},
	{XNVMEC_OPT_SEL,	XNVMEC_OPT_VTYPE_HEX,	"sel",		"current=0x0, default=0x1, saved=0x2, supported=0x3"},
	{XNVMEC_OPT_MSET,	XNVMEC_OPT_VTYPE_HEX,	"mset",		"Mset?"},
	{XNVMEC_OPT_AUSE,	XNVMEC_OPT_VTYPE_HEX,	"ause",		"AUSE?"},
	{XNVMEC_OPT_OVRPAT,	XNVMEC_OPT_VTYPE_HEX,	"ovrpat",	"Overwrite Pattern"},
	{XNVMEC_OPT_OWPASS,	XNVMEC_OPT_VTYPE_HEX,	"owpass",	"Overwrite Passes"},
	{XNVMEC_OPT_OIPBP,	XNVMEC_OPT_VTYPE_HEX,	"oipbp",	"Overwrite Inverse Bit Pattern"},
	{XNVMEC_OPT_NODAS,	XNVMEC_OPT_VTYPE_HEX,	"nodas",	"Nodas?"},

	{XNVMEC_OPT_ACTION,	XNVMEC_OPT_VTYPE_HEX,	"action",	"Command action"},
	{XNVMEC_OPT_ZRMS,	XNVMEC_OPT_VTYPE_HEX,	"zrms",		"Zone Resource Management"},
	{XNVMEC_OPT_PI,		XNVMEC_OPT_VTYPE_HEX,	"pi",		"Protection Information"},
	{XNVMEC_OPT_PIL,	XNVMEC_OPT_VTYPE_HEX,	"pil",		"Protection Information Location"},
	{XNVMEC_OPT_FID,	XNVMEC_OPT_VTYPE_HEX,	"fid",		"Feature Identifier"},
	{XNVMEC_OPT_FEAT,	XNVMEC_OPT_VTYPE_HEX,	"feat",		"Feature e.g. cdw12 content"},

	{XNVMEC_OPT_OPCODE,	XNVMEC_OPT_VTYPE_HEX,	"opcode",	"Command opcode"},
	{XNVMEC_OPT_FLAGS,	XNVMEC_OPT_VTYPE_HEX,	"flags",	"Command flags"},
	{XNVMEC_OPT_ALL,	XNVMEC_OPT_VTYPE_HEX,	"all",		"Select / Affect all"},

	{XNVMEC_OPT_SEED,	XNVMEC_OPT_VTYPE_NUM,	"seed",		"Use given 'NUM' as random seed"},
	{XNVMEC_OPT_LIMIT,	XNVMEC_OPT_VTYPE_NUM,	"limit",	"Restrict amount to 'NUM'"},
	{XNVMEC_OPT_QDEPTH,	XNVMEC_OPT_VTYPE_NUM,	"qdepth",	"Use given 'NUM' as queue max capacity"},

	{XNVMEC_OPT_COUNT,	XNVMEC_OPT_VTYPE_NUM,	"count",	"Use given 'NUM' as count"},
	{XNVMEC_OPT_OFFSET,	XNVMEC_OPT_VTYPE_NUM,	"offset",	"Use given 'NUM' as offset"},

	{XNVMEC_OPT_CLEAR,	XNVMEC_OPT_VTYPE_HEX,	"clear",	"Clear something..."},

	{XNVMEC_OPT_STATUS,	XNVMEC_OPT_VTYPE_NUM,	"status",	"Provide command state"},
	{XNVMEC_OPT_SAVE,	XNVMEC_OPT_VTYPE_NUM,	"save",		"Save"},
	{XNVMEC_OPT_RESET,	XNVMEC_OPT_VTYPE_NUM,	"reset",	"Reset controller"},
	{XNVMEC_OPT_VERBOSE,	XNVMEC_OPT_VTYPE_NUM,	"verbose",	"Increase output info"},
	{XNVMEC_OPT_HELP,	XNVMEC_OPT_VTYPE_NUM,	"help",		"Show usage / help"},

	{XNVMEC_OPT_END, XNVMEC_OPT_VTYPE_NUM, "", ""},
};

struct xnvmec_opt_attr *
xnvmec_opt_attr_by_opt(enum xnvmec_opt opt, struct xnvmec_opt_attr *attrs)
{
	for (int idx = 0; attrs[idx].opt; ++idx) {
		if (attrs[idx].opt != opt) {
			continue;
		}

		return &attrs[idx];
	}

	return NULL;
}

void
xnvmec_pinf(const char *format, ...)
{
	va_list args;
	va_start(args, format);

	printf("# ");
	vprintf(format, args);
	printf("\n");

	va_end(args);

	fflush(stdout);
}

void
xnvmec_perr(const char *msg, int err)
{
	fprintf(stderr, "# ERR: '%s': {errno: %d, msg: '%s'}\n",
		msg, err, strerror(err < 0 ? -err : err));

	fflush(stderr);
}

void
xnvmec_usage_sub_long(struct xnvmec *cli, struct xnvmec_sub *sub)
{
	int nopts = 0;

	printf("Usage: %s %s ", cli->argv[0], sub->name);
	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *opt = &sub->opts[oi];
		struct xnvmec_opt_attr *attr = NULL;

		if ((opt->opt == XNVMEC_OPT_END) || \
		    (opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		attr = xnvmec_opt_attr_by_opt(opt->opt, xnvmec_opts);
		if (!attr) {
			break;
		}
		if (opt->type && (opt->type != XNVMEC_POSA)) {
			++nopts;
			break;
		}

		printf("<%s> ", attr->name);
	}

	printf("[<args>]\n");

	if (strnlen(sub->descr_long, 2) > 1) {
		printf("\n%s\n", sub->descr_long);
	}

	printf("\n");
	printf("Where <args> include:\n\n");

	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *opt = &sub->opts[oi];
		struct xnvmec_opt_attr *attr = NULL;
		int width = 0;

		if ((opt->opt == XNVMEC_OPT_END) || \
		    (opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		attr = xnvmec_opt_attr_by_opt(opt->opt, xnvmec_opts);
		if (!attr) {
			break;
		}

		printf("  ");
		switch (opt->type) {
		case XNVMEC_POSA:
			width = printf("%s", attr->name);
			break;
		case XNVMEC_LREQ:
			width = printf("--%s %s", attr->name,
				       xnvmec_opt_value_type_str(attr->vtype));
			break;
		case XNVMEC_LOPT:
			width = printf("[ --%s %s ]", attr->name,
				       xnvmec_opt_value_type_str(attr->vtype));
			break;
		case XNVMEC_LFLG:
			width = printf("[ --%s ]", attr->name);
			break;
		}

		printf("%*s; %s", 30 - width, "", attr->descr);
		printf("\n");
	}

	printf("\n");
	printf("See '%s --help' for other commands\n", cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr();
		printf("\n");
	}
}

void
xnvmec_usage_sub_short(struct xnvmec_sub *sub, size_t name_width)
{
	int name_len = strnlen(sub->name, XNVMEC_SUB_NAME_LEN_MAX);
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
xnvmec_usage(struct xnvmec *cli)
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
			xnvmec_usage_sub_short(&cli->subs[si], name_width);
		}
	}

	printf("\n");
	printf(
		"See '%s <command> --help' for the description of [<args>]\n",
		cli->argv[0]);

	if (cli->title) {
		printf("\n");
		printf("%s -- ", cli->title);
		cli->ver_pr();
		printf("\n");
	}
}

int
xnvmec_assign_arg(struct xnvmec *cli, int optval, char *arg,
		  enum xnvmec_opt_type opt_type)
{
	struct xnvmec_opt_attr *attr = NULL;
	struct xnvmec_args *args = &cli->args;
	enum xnvmec_opt subopt;
	char *endptr = NULL;
	uint64_t num = 0;

	subopt = xnvmec_int2opt(optval);

	attr = xnvmec_opt_attr_by_opt(subopt, xnvmec_opts);
	if (!attr) {
		XNVME_DEBUG("FAILED: xnvmec_opt_attr_by_opt()");
		errno = EINVAL;
		return -1;
	}

	// Check numerical args
	if (arg && (opt_type != XNVMEC_LFLG)) {
		int num_base = attr->vtype == XNVMEC_OPT_VTYPE_NUM ? 10 : 16;

		switch (attr->vtype) {
		case XNVMEC_OPT_VTYPE_URI:
		case XNVMEC_OPT_VTYPE_FILE:
			break;

		case XNVMEC_OPT_VTYPE_NUM:
		case XNVMEC_OPT_VTYPE_HEX:
			errno = 0;
			num = strtoll(arg, &endptr, num_base);
			if (errno) {
				XNVME_DEBUG("FAILED: strtoll(), errno: %d",
					    errno);
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

	switch (subopt) {
	case XNVMEC_OPT_CDW00:
		args->cdw[0] = num;
		break;
	case XNVMEC_OPT_CDW01:
		args->cdw[1] = num;
		break;
	case XNVMEC_OPT_CDW02:
		args->cdw[2] = num;
		break;
	case XNVMEC_OPT_CDW03:
		args->cdw[3] = num;
		break;
	case XNVMEC_OPT_CDW04:
		args->cdw[4] = num;
		break;
	case XNVMEC_OPT_CDW05:
		args->cdw[5] = num;
		break;
	case XNVMEC_OPT_CDW06:
		args->cdw[6] = num;
		break;
	case XNVMEC_OPT_CDW07:
		args->cdw[7] = num;
		break;
	case XNVMEC_OPT_CDW08:
		args->cdw[8] = num;
		break;
	case XNVMEC_OPT_CDW09:
		args->cdw[9] = num;
		break;
	case XNVMEC_OPT_CDW10:
		args->cdw[10] = num;
		break;
	case XNVMEC_OPT_CDW11:
		args->cdw[11] = num;
		break;
	case XNVMEC_OPT_CDW12:
		args->cdw[12] = num;
		break;
	case XNVMEC_OPT_CDW13:
		args->cdw[13] = num;
		break;
	case XNVMEC_OPT_CDW14:
		args->cdw[14] = num;
		break;
	case XNVMEC_OPT_CDW15:
		args->cdw[15] = num;
		break;
	case XNVMEC_OPT_CMD_INPUT:
		args->cmd_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_CMD_OUTPUT:
		args->cmd_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_DATA_NBYTES:
		args->data_nbytes = num;
		break;
	case XNVMEC_OPT_DATA_INPUT:
		args->data_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_DATA_OUTPUT:
		args->data_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_META_NBYTES:
		args->meta_nbytes = num;
		break;
	case XNVMEC_OPT_META_INPUT:
		args->meta_input = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_META_OUTPUT:
		args->meta_output = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_LBAF:
		args->lbaf = num;
		break;
	case XNVMEC_OPT_SLBA:
		args->slba = num;
		break;
	case XNVMEC_OPT_ELBA:
		args->elba = num;
		break;
	case XNVMEC_OPT_LBA:
		args->lba = num;
		break;
	case XNVMEC_OPT_NLB:
		args->nlb = num;
		break;
	case XNVMEC_OPT_URI:
		args->uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_SYS_URI:
		args->sys_uri = arg ? arg : "INVALID_INPUT";
		break;
	case XNVMEC_OPT_UUID:
		args->uuid = num;
		break;
	case XNVMEC_OPT_NSID:
		args->nsid = num;
		break;
	case XNVMEC_OPT_CNS:
		args->cns = num;
		break;
	case XNVMEC_OPT_CSI:
		args->csi = num;
		break;
	case XNVMEC_OPT_INDEX:
		args->index = num;
		break;
	case XNVMEC_OPT_SETID:
		args->setid = num;
		break;

	case XNVMEC_OPT_CNTID:
		args->cntid = num;
		break;
	case XNVMEC_OPT_LID:
		args->lid = num;
		break;
	case XNVMEC_OPT_LSP:
		args->lsp = num;
		break;
	case XNVMEC_OPT_LPO_NBYTES:
		args->action = num;
		break;
	case XNVMEC_OPT_RAE:
		args->rae = num;
		break;
	case XNVMEC_OPT_CLEAR:
		args->clear = arg ? num : 1;
		break;
	case XNVMEC_OPT_ZF:
		args->zf = num;
		break;
	case XNVMEC_OPT_SES:
		args->ses = num;
		break;
	case XNVMEC_OPT_SEL:
		args->sel = num;
		break;
	case XNVMEC_OPT_MSET:
		args->mset = num;
		break;
	case XNVMEC_OPT_AUSE:
		args->ause = num;
		break;
	case XNVMEC_OPT_OVRPAT:
		args->ovrpat = num;
		break;
	case XNVMEC_OPT_OWPASS:
		args->owpass = num;
		break;
	case XNVMEC_OPT_OIPBP:
		args->oipbp = num;
		break;
	case XNVMEC_OPT_NODAS:
		args->nodas = num;
		break;

	case XNVMEC_OPT_ACTION:
		args->action = num;
		break;
	case XNVMEC_OPT_ZRMS:
		args->action = num;
		break;
	case XNVMEC_OPT_PI:
		args->pi = num;
		break;
	case XNVMEC_OPT_PIL:
		args->pil = num;
		break;
	case XNVMEC_OPT_FID:
		args->fid = num;
		break;
	case XNVMEC_OPT_FEAT:
		args->feat = num;
		break;
	case XNVMEC_OPT_SEED:
		args->seed = num;
		break;
	case XNVMEC_OPT_LIMIT:
		args->limit = num;
		break;
	case XNVMEC_OPT_QDEPTH:
		args->qdepth = num;
		break;

	case XNVMEC_OPT_OPCODE:
		args->opcode = num;
		break;
	case XNVMEC_OPT_FLAGS:
		args->flags = num;
		break;

	case XNVMEC_OPT_ALL:
		args->all = arg ? num : 1;
		break;
	case XNVMEC_OPT_STATUS:
		args->status = arg ? num : 1;
		break;
	case XNVMEC_OPT_SAVE:
		args->save = arg ? num : 1;
		break;
	case XNVMEC_OPT_RESET:
		args->reset = arg ? num : 1;
		break;
	case XNVMEC_OPT_VERBOSE:
		args->verbose = arg ? num : 1;
		break;
	case XNVMEC_OPT_HELP:
		args->help = arg ? num : 1;
		break;

	case XNVMEC_OPT_COUNT:
		args->count = arg ? num : 1;
		break;
	case XNVMEC_OPT_OFFSET:
		args->offset = arg ? num : 1;
		break;

	case XNVMEC_OPT_UNUSED01:
	case XNVMEC_OPT_UNUSED02:
	case XNVMEC_OPT_UNUSED03:
	case XNVMEC_OPT_UNUSED04:
	case XNVMEC_OPT_UNUSED05:
	case XNVMEC_OPT_UNUSED06:
	case XNVMEC_OPT_UNUSED07:
	case XNVMEC_OPT_UNUSED08:
		errno = EINVAL;
		XNVME_DEBUG("subopt: 0x%x", subopt);
		return -1;

	case XNVMEC_OPT_WEIRD:
	case XNVMEC_OPT_UNKNOWN:
	case XNVMEC_OPT_END:
	case XNVMEC_OPT_NONE:
		errno = EINVAL;
		XNVME_DEBUG("subopt: 0x%x", subopt);
		return -1;
	}

	cli->given[subopt] = 1;

	return 0;
}

struct xnvmec_counts {
	int posa;
	int lreq;
	int lopt;
	int lflg;

	int total_long;
	int total_req;
	int total;
};

void
xnvmec_counts_pr(const struct xnvmec_counts *counts)
{
	printf("xnvmec_counts:");

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
 * Parse at most XNVMEC_SUB_OPTS_LEN arguments
 */
int
xnvmec_parse(struct xnvmec *cli)
{
	struct xnvmec_args *args = &cli->args;
	struct option long_options[XNVMEC_SUB_OPTS_LEN] = { 0 };
	struct xnvmec_opt_attr *pos_args[XNVMEC_SUB_OPTS_LEN] = { 0 };
	struct xnvmec_sub *sub;
	struct xnvmec_counts signature = { 0 };
	struct xnvmec_counts parsed = { 0 };

	const char *sub_name = NULL;

	if (cli->argc < 2) {
		xnvmec_pinf("Insufficient arguments: no <command> given");
		errno = EINVAL;
		return -1;
	}

	// Parse the sub-command so we know which PSAs and OPTs to scan for
	sub_name = cli->argv[1];

	sub = sub_by_name(cli, sub_name);
	if (!sub) {
		xnvmec_pinf("%s: invalid command: '%s'", cli->argv[0],
			    sub_name);
		errno = EINVAL;
		return -1;
	}

	cli->sub = sub;		// Boom!

	// Convert from XNVMEC_OPT to getopt long_options
	for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
		struct xnvmec_sub_opt *opt = &sub->opts[oi];
		struct xnvmec_opt_attr *attr = NULL;
		struct option *lopt = NULL;

		if ((opt->opt == XNVMEC_OPT_END) || \
		    (opt->opt == XNVMEC_OPT_NONE)) {
			break;
		}

		attr = xnvmec_opt_attr_by_opt(opt->opt, xnvmec_opts);
		if (!attr) {
			xnvmec_pinf("Invalid arguments: cannot parse value");
			errno = EINVAL;
			return -1;
		}

		switch (opt->type) {
		case XNVMEC_LFLG:
			++signature.lflg;
			++signature.total_long;
			break;
		case XNVMEC_LOPT:
			++signature.lopt;
			++signature.total_long;
			break;

		case XNVMEC_LREQ:
			++signature.lreq;
			++signature.total_long;
			++signature.total_req;
			break;

		case XNVMEC_POSA:
			++signature.posa;
			++signature.total_req;
			break;
		}
		++signature.total;

		switch (opt->type) {
		case XNVMEC_LFLG:
		case XNVMEC_LREQ:
		case XNVMEC_LOPT:

			lopt = &long_options[signature.total_long - 1];
			lopt->name = attr->name;
			lopt->flag = NULL;
			lopt->val = opt->opt;

			lopt->has_arg = required_argument;
			if (opt->type == XNVMEC_LFLG) {
				lopt->has_arg = no_argument;
			}

			break;

		case XNVMEC_POSA:
			pos_args[signature.posa - 1] = attr;
			break;
		}
	}

	// Parse the long-opts
	for (int count = 0; count < signature.total_long; ++count) {
		struct xnvmec_sub_opt *opti = NULL;
		int optidx = 0;
		int ret = 0;
		int found = 0;

		ret = getopt_long(cli->argc, cli->argv, "", long_options,
				  &optidx);
		if (ret == -1) {
			break;
		}

		// scan for the corresponding option
		for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
			opti = &sub->opts[oi];

			if ((int)opti->opt != ret) {
				continue;
			}

			switch (opti->type) {
			case XNVMEC_LFLG:
				++parsed.lflg;
				break;

			case XNVMEC_LOPT:
				++parsed.lopt;
				break;

			case XNVMEC_LREQ:
				++parsed.lreq;
				++parsed.total_req;
				break;

			case XNVMEC_POSA:
				XNVME_DEBUG("Positional out of place");
				errno = EINVAL;
				return -1;
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

		if (xnvmec_assign_arg(cli, ret, optarg, opti->type)) {
			XNVME_DEBUG("FAILED: xnvmec_assign_arg()");
			xnvmec_pinf("invalid argument value(%s)", optarg);
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
			xnvmec_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		npos_given = cli->argc - (optind + 1);

		if (npos_given != signature.posa) {
			xnvmec_pinf("Insufficient arguments, see: --help");
			errno = EINVAL;
			return -1;
		}

		// Skip the sub-command and parse positional arguments
		for (int pos = 0; pos < signature.posa; ++pos) {
			struct xnvmec_opt_attr *attr = NULL;
			int idx = pos + (optind + 1);

			if (idx > cli->argc) {
				break;
			}

			attr = pos_args[pos];

			if (xnvmec_assign_arg(cli, attr->opt, cli->argv[idx],
					      XNVMEC_POSA)) {
				XNVME_DEBUG("FAILED: xnvmec_assign_arg()");
				xnvmec_pinf("invalid argument value(%s)",
					    cli->argv[idx]);
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
		xnvmec_pinf("Insufficient required arguments, see: --help");
		errno = EINVAL;
		return -1;
	}

	return 0;
}

int
xnvmec(struct xnvmec *cli, int argc, char **argv, int opts)
{
	int err = 0;

	if (!cli) {
		err = -EINVAL;
		xnvmec_perr("xnvmec(!cli)", err);
		return err;
	}

	cli->argc = argc;
	cli->argv = argv;

	if (!cli->ver_pr) {
		cli->ver_pr = xnvme_ver_pr;
	}

	if ((argc < 2) || (!strcmp(argv[1], "--help")) || \
	    (!strcmp(argv[1], "-h"))) {
		xnvmec_usage(cli);
		return 0;
	}

	for (int i = 0; i < cli->nsubs; ++i) {	// We all need help! ;)
		struct xnvmec_sub *sub = &cli->subs[i];

		if (!sub->name) {
			break;
		}

		for (int oi = 0; oi < XNVMEC_SUB_OPTS_LEN; ++oi) {
			struct xnvmec_sub_opt *sopt = &sub->opts[oi];

			if (sopt->opt == XNVMEC_OPT_NONE) {
				sopt->opt = XNVMEC_OPT_HELP;
				sopt->type = XNVMEC_LFLG;
				break;
			}
		}
	}

	err = xnvmec_parse(cli);
	if (err) {
		xnvmec_perr("xnvmec()", errno);
		return err;
	}

	// Arguments parsed without any errors
	if (cli->args.help) {
		xnvmec_usage_sub_long(cli, cli->sub);
		return 0;
	}

	if ((opts & XNVMEC_INIT_DEV_OPEN) && cli->args.uri) {
		cli->args.dev = xnvme_dev_open(cli->args.uri);
		if (!cli->args.dev) {
			err = -errno;
			xnvmec_perr("xnvme_dev_open()", err);
			return -1;
		}
		cli->args.geo = xnvme_dev_get_geo(cli->args.dev);
	}

	err = cli->sub->command(cli);
	if (err) {
		xnvmec_perr(cli->sub->name, err);
	}

	if (cli->args.verbose) {
		xnvmec_args_pr(&cli->args, 0x0);
	}

	if ((opts & XNVMEC_INIT_DEV_OPEN) && cli->args.dev) {
		xnvme_dev_close(cli->args.dev);
	}

	return err ? 1 : 0;
}

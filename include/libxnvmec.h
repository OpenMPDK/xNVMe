/**
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvmec.h
 */
#ifndef __LIBXNVMEC_H
#define __LIBXNVMEC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>
#include <libxnvme_util.h>
#include <libxnvme_ver.h>
#include <libxnvme_pp.h>

/**
 * Fills `buf` with content `nbytes` of content
 *
 * @param buf Pointer to the buffer to fill
 * @param content Name of a file, or special "zero", or "anum"
 * @param nbytes Amount of bytes to fill in buf
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_fill(void *buf, size_t nbytes, const char *content);

void *
xnvmec_buf_clear(void *buf, size_t nbytes);

/**
 * Returns the number of bytes where expected is different from actual
 *
 * @param expected
 * @param actual
 * @param nbytes
 *
 * @return On success, returns number of bytes that differ
 */
size_t
xnvmec_buf_diff(const void *expected, const void *actual, size_t nbytes);

/**
 * Prints the number and value of bytes where expected is different from actual
 *
 * @param expected
 * @param actual
 * @param nbytes
 * @param opts
 */
void
xnvmec_buf_diff_pr(const void *expected, const void *actual, size_t nbytes,
		   int opts);

/**
 * Write content of buffer into file
 *
 * - If file exists, then it is truncated / overwritten
 * - If file does NOT exist, then it is created
 * - When file is created, permissions are set to user WRITE + READ
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Destination where buffer will be dumped to
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_to_file(void *buf, size_t nbytes, const char *path);

/**
 * Read content of file into buffer
 *
 * @param buf Pointer to the buffer
 * @param nbytes Size of buf
 * @param path Source to read from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_buf_from_file(void *buf, size_t nbytes, const char *path);

/**
 * Fill the the given 'cmd' with content of file at 'fpath'
 *
 * @param cmd the command to fill
 * @param fpath path to file
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_cmd_from_file(struct xnvme_spec_cmd *cmd, const char *fpath);

/**
 * Stores the given `cmd` in `fpath`
 *
 * @param cmd the command to store
 * @param fpath path to store content of cmd in
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvmec_cmd_to_file(const struct xnvme_spec_cmd *cmd, const char *fpath);

/**
 * Options are stored in an instance of this structure
 *
 * @struct xnvmec_args
 */
struct xnvmec_args {
	struct xnvme_dev *dev;				///< Created
	const struct xnvme_geo *geo;			///< Created

	const char *uri;
	const char *sys_uri;

	const char *cmd_input;
	const char *cmd_output;

	size_t data_nbytes;
	const char *data_input;
	const char *data_output;
	size_t meta_nbytes;
	const char *meta_input;
	const char *meta_output;

	uint32_t cdw[16];

	uint64_t lbaf;
	uint64_t lba;
	uint32_t nlb;
	uint64_t slba;
	uint64_t elba;

	uint32_t uuid;
	uint32_t nsid;
	uint32_t cns;
	uint32_t csi;
	uint64_t index;
	uint32_t setid;

	uint64_t cntid;
	uint32_t lid;
	uint32_t lsp;
	uint64_t lpo_nbytes;
	uint32_t rae;
	uint32_t clear;
	uint32_t zf;
	uint32_t ses;
	uint32_t sel;
	uint32_t mset;
	uint32_t ause;
	uint32_t ovrpat;
	uint32_t owpass;
	uint32_t oipbp;
	uint32_t nodas;

	uint32_t action;
	uint32_t zrms;

	uint32_t pi;
	uint32_t pil;

	uint32_t fid;
	uint32_t feat;

	uint32_t seed;
	uint32_t iosize;
	uint32_t qdepth;
	uint32_t direct;
	uint32_t limit;

	uint64_t count;
	uint64_t offset;

	uint64_t opcode;
	uint64_t flags;
	uint64_t all;

	uint32_t status;
	uint32_t save;
	uint32_t reset;
	uint32_t verbose;
	uint32_t help;
};

void xnvmec_args_pr(struct xnvmec_args *args, int opts);

/**
 * Command-Line Options, each command-line option can be one of ::xnvmec_opt_type
 *
 * @enum xnvmec_opt
 */
enum xnvmec_opt {
	XNVMEC_OPT_NONE = 0x0, ///< XNVMEC_OPT_NONE

	XNVMEC_OPT_CDW00 = 0x1, ///< XNVMEC_OPT_CDW00
	XNVMEC_OPT_CDW01 = 0x2, ///< XNVMEC_OPT_CDW01
	XNVMEC_OPT_CDW02 = 0x3, ///< XNVMEC_OPT_CDW02
	XNVMEC_OPT_CDW03 = 0x4, ///< XNVMEC_OPT_CDW03
	XNVMEC_OPT_CDW04 = 0x5, ///< XNVMEC_OPT_CDW04
	XNVMEC_OPT_CDW05 = 0x6, ///< XNVMEC_OPT_CDW05
	XNVMEC_OPT_CDW06 = 0x7, ///< XNVMEC_OPT_CDW06
	XNVMEC_OPT_CDW07 = 0x8, ///< XNVMEC_OPT_CDW07
	XNVMEC_OPT_CDW08 = 0x9, ///< XNVMEC_OPT_CDW08
	XNVMEC_OPT_CDW09 = 0xA, ///< XNVMEC_OPT_CDW09
	XNVMEC_OPT_CDW10 = 0xB, ///< XNVMEC_OPT_CDW10
	XNVMEC_OPT_CDW11 = 0xC, ///< XNVMEC_OPT_CDW11
	XNVMEC_OPT_CDW12 = 0xD, ///< XNVMEC_OPT_CDW12
	XNVMEC_OPT_CDW13 = 0xE, ///< XNVMEC_OPT_CDW13
	XNVMEC_OPT_CDW14 = 0xF, ///< XNVMEC_OPT_CDW14
	XNVMEC_OPT_CDW15 = 0x1F, ///< XNVMEC_OPT_CDW15

	XNVMEC_OPT_CMD_INPUT = 'G', ///< XNVMEC_OPT_CMD_INPUT
	XNVMEC_OPT_CMD_OUTPUT = 'H', ///< XNVMEC_OPT_CMD_OUTPUT

	XNVMEC_OPT_DATA_NBYTES = 'I', ///< XNVMEC_OPT_DATA_NBYTES
	XNVMEC_OPT_DATA_INPUT = 'J', ///< XNVMEC_OPT_DATA_INPUT
	XNVMEC_OPT_DATA_OUTPUT = 'K', ///< XNVMEC_OPT_DATA_OUTPUT

	XNVMEC_OPT_META_NBYTES = 'L', ///< XNVMEC_OPT_META_NBYTES
	XNVMEC_OPT_META_INPUT = 'M', ///< XNVMEC_OPT_META_INPUT
	XNVMEC_OPT_META_OUTPUT = 'N', ///< XNVMEC_OPT_META_OUTPUT

	XNVMEC_OPT_LBAF = 'O', ///< XNVMEC_OPT_LBAF
	XNVMEC_OPT_SLBA = '(', ///< XNVMEC_OPT_SLBA
	XNVMEC_OPT_ELBA = ')', ///< XNVMEC_OPT_ELBA
	XNVMEC_OPT_LBA = 'P', ///< XNVMEC_OPT_LBA
	XNVMEC_OPT_NLB = 'Q', ///< XNVMEC_OPT_NLB

	XNVMEC_OPT_URI = 'R', ///< XNVMEC_OPT_URI
	XNVMEC_OPT_SYS_URI = ',', ///< XNVMEC_OPT_SYS_URI
	XNVMEC_OPT_UUID = 'S', ///< XNVMEC_OPT_UUID
	XNVMEC_OPT_NSID = 'T', ///< XNVMEC_OPT_NSID
	XNVMEC_OPT_CNS = 'U', ///< XNVMEC_OPT_CNS
	XNVMEC_OPT_CSI = '/', ///< XNVMEC_OPT_CSI
	XNVMEC_OPT_INDEX = '-', ///< XNVMEC_OPT_INDEX
	XNVMEC_OPT_SETID = 'V', ///< XNVMEC_OPT_SETID

	XNVMEC_OPT_CNTID = 'W', ///< XNVMEC_OPT_CNTID
	XNVMEC_OPT_LID = 'X', ///< XNVMEC_OPT_LID
	XNVMEC_OPT_LSP = 'Y', ///< XNVMEC_OPT_LSP
	XNVMEC_OPT_LPO_NBYTES = 'Z', ///< XNVMEC_OPT_LPO_NBYTES
	XNVMEC_OPT_RAE = 'a', ///< XNVMEC_OPT_RAE
	XNVMEC_OPT_CLEAR = 'b', ///< XNVMEC_OPT_CLEAR
	XNVMEC_OPT_ZF = 'c', ///< XNVMEC_OPT_ZF
	XNVMEC_OPT_SES = 'd', ///< XNVMEC_OPT_SES
	XNVMEC_OPT_SEL = 'e', ///< XNVMEC_OPT_SEL
	XNVMEC_OPT_MSET = 'f', ///< XNVMEC_OPT_MSET
	XNVMEC_OPT_AUSE = 'g', ///< XNVMEC_OPT_AUSE
	XNVMEC_OPT_OVRPAT = 'h', ///< XNVMEC_OPT_OVRPAT
	XNVMEC_OPT_OWPASS = 'i', ///< XNVMEC_OPT_OWPASS
	XNVMEC_OPT_OIPBP = 'j', ///< XNVMEC_OPT_OIPBP
	XNVMEC_OPT_NODAS = 'k', ///< XNVMEC_OPT_NODAS

	XNVMEC_OPT_ACTION = 'l', ///< XNVMEC_OPT_ACTION
	XNVMEC_OPT_ZRMS = 'm', ///< XNVMEC_OPT_ZRMS

	XNVMEC_OPT_PI = 'n', ///< XNVMEC_OPT_PI
	XNVMEC_OPT_PIL = 'o', ///< XNVMEC_OPT_PIL

	XNVMEC_OPT_FID = 'p', ///< XNVMEC_OPT_FID
	XNVMEC_OPT_FEAT = 'q', ///< XNVMEC_OPT_FEAT

	XNVMEC_OPT_SEED = 'r', ///< XNVMEC_OPT_SEED
	XNVMEC_OPT_LIMIT = 's', ///< XNVMEC_OPT_LIMIT
	XNVMEC_OPT_IOSIZE = '}', ///< XNVMEC_OPT_IOSIZE
	XNVMEC_OPT_QDEPTH = 't', ///< XNVMEC_OPT_QDEPTH
	XNVMEC_OPT_DIRECT = '~', ///< XNVMEC_OPT_DIRECT

	XNVMEC_OPT_STATUS = 'u', ///< XNVMEC_OPT_STATUS
	XNVMEC_OPT_SAVE = 'v', ///< XNVMEC_OPT_SAVE
	XNVMEC_OPT_RESET = 'w', ///< XNVMEC_OPT_RESET
	XNVMEC_OPT_VERBOSE = 'x', ///< XNVMEC_OPT_VERBOSE
	XNVMEC_OPT_HELP = 'y', ///< XNVMEC_OPT_HELP

	XNVMEC_OPT_COUNT = 'z', ///< XNVMEC_OPT_COUNT
	XNVMEC_OPT_OFFSET = '{', ///< XNVMEC_OPT_OFFSET

	XNVMEC_OPT_OPCODE = '#', ///< XNVMEC_OPT_OPCODE
	XNVMEC_OPT_FLAGS = '*', ///< XNVMEC_OPT_FLAGS

	XNVMEC_OPT_ALL = '.', ///< XNVMEC_OPT_ALL

	XNVMEC_OPT_UNUSED03 = '!',
	XNVMEC_OPT_UNUSED04 = '"',
	XNVMEC_OPT_UNUSED05 = '$',
	XNVMEC_OPT_UNUSED06 = '%',
	XNVMEC_OPT_UNUSED07 = '&',
	XNVMEC_OPT_UNUSED08 = '+',

	XNVMEC_OPT_UNKNOWN = '?',	///< XNVMEC_OPT_UNKNOWN: do not use getopt special char
	XNVMEC_OPT_WEIRD = ':',		///< XNVMEC_OPT_WEIRD: do not use getopt special char
	XNVMEC_OPT_END = 255,  ///< XNVMEC_OPT_END: do not use
};

/**
 * Converts the given integer to opt
 *
 * @param opt The integer to convert
 *
 * @return On success, a valid OP is returned. On error, XNVME_OPT_END is
 * returned.
 */
enum xnvmec_opt xnvmec_int2opt(int opt);

/**
 * XNVMEC_LFLG: ./cli --verbose; always optional
 * XNVMEC_LOPT: ./cli --arg 0x0; optionally provide a value
 * XNVMEC_LREQ: ./cli --arg 0x0; require providing a value
 * XNVMEC_POSA: ./cli arg1 arg2; required, ordered, and provides a value
 *
 * @enum xnvmec_opt_type
 */
enum xnvmec_opt_type {
	XNVMEC_POSA = 0x1, ///< XNVMEC_POSA
	XNVMEC_LFLG = 0x2, ///< XNVMEC_LFLG
	XNVMEC_LOPT = 0x3, ///< XNVMEC_LOPT
	XNVMEC_LREQ = 0x4, ///< XNVMEC_LREQ
};

/**
 * @struct xnvmec_sub_opt
 */
struct xnvmec_sub_opt {
	enum xnvmec_opt opt;
	enum xnvmec_opt_type type;
};

/**
 * Forward declaration
 */
struct xnvmec;

typedef int (*xnvmec_subfunc)(struct xnvmec *);

#define XNVMEC_SUB_OPTS_LEN 200

#define XNVMEC_SUB_NAME_LEN_MAX 30

/**
 * Filled by the user of libxnvmc, usually statically right above main
 */
struct xnvmec_sub {
	const char *name;		///< Short name of the sub-command
	const char *descr_short;	///< Short sub-command description
	const char *descr_long;		///< Long sub-command description

	xnvmec_subfunc command;		///< Function to execute

	///< Options to parse into args and pass onto func
	struct xnvmec_sub_opt opts[XNVMEC_SUB_OPTS_LEN];
};

struct xnvmec {
	const char *title;		///< Setup by user
	const char *descr_short;	///< Setup by user
	const char *descr_long;		///< Setup by user

	int nsubs;			///< Setup by user
	struct xnvmec_sub *subs;	///< Setup by user

	int (*ver_pr)();		///< Setup by library if unset

	int argc;			///< Setup by library
	char **argv;			///< Setup by library
	struct xnvmec_args args;	///< Setup by library

	int given[255];			///< Setup by library

	struct xnvmec_sub *sub;		///< Setup by library

	struct xnvme_timer timer;	///< Used by xnvmec_timer_*
};

enum xnvmec_opts {
	XNVMEC_INIT_NONE = 0x0,
	XNVMEC_INIT_DEV_OPEN = 0x1,
};

static inline uint64_t xnvmec_timer_start(struct xnvmec *cli)
{
	cli->timer.start = _xnvme_timer_clock_sample();
	return cli->timer.start;
}

static inline uint64_t xnvmec_timer_stop(struct xnvmec *cli)
{
	cli->timer.stop = _xnvme_timer_clock_sample();
	return cli->timer.stop;
}

static inline void xnvmec_timer_bw_pr(struct xnvmec *cli, const char *prefix,
				      size_t nbytes)
{
	double secs = xnvme_timer_elapsed_secs(&cli->timer);
	double mb = nbytes / (double)1048576;

	printf("%s: {elapsed: %.4f, mb: %.2f, mbsec: %.2f}\n",
	       prefix, secs, mb, mb / secs);
}

void xnvmec_pinf(const char *format, ...);

void xnvmec_perr(const char *msg, int err);

int xnvmec(struct xnvmec *cli, int argc, char **argv, int opts);

#ifdef __cplusplus
}
#endif

#endif /* LIBXNVMEC_H */

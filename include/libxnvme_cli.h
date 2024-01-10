/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_cli.h
 */

#define XNVME_CLI_CORE_OPTS                                                                 \
	{XNVME_CLI_OPT_ORCH_TITLE, XNVME_CLI_SKIP}, {XNVME_CLI_OPT_SUBNQN, XNVME_CLI_LOPT}, \
		{XNVME_CLI_OPT_HOSTNQN, XNVME_CLI_LOPT},                                    \
	{                                                                                   \
		XNVME_CLI_OPT_BE, XNVME_CLI_LOPT                                            \
	}

#define XNVME_CLI_ADMIN_OPTS                                                                \
	XNVME_CLI_CORE_OPTS, {XNVME_CLI_OPT_DEV_NSID, XNVME_CLI_LOPT},                      \
		{XNVME_CLI_OPT_ADMIN, XNVME_CLI_LOPT}, {XNVME_CLI_OPT_MEM, XNVME_CLI_LOPT}, \
		{XNVME_CLI_OPT_DEV, XNVME_CLI_LOPT},                                        \
	{                                                                                   \
		XNVME_CLI_OPT_DIRECT, XNVME_CLI_LOPT                                        \
	}

#define XNVME_CLI_SYNC_OPTS \
	XNVME_CLI_ADMIN_OPTS, { XNVME_CLI_OPT_SYNC, XNVME_CLI_LOPT }

#define XNVME_CLI_ASYNC_OPTS \
	XNVME_CLI_SYNC_OPTS, { XNVME_CLI_OPT_ASYNC, XNVME_CLI_LOPT }

/**
 * Options are stored in an instance of this structure
 *
 * @struct xnvme_cli_args
 */
struct xnvme_cli_args {
	struct xnvme_dev *device;    ///< Created
	const struct xnvme_geo *geo; ///< Created

	const char *uri;
	const char *sys_uri;

	const char *subnqn;
	const char *hostnqn;

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
	uint32_t llb;
	uint32_t nlb;
	uint64_t slba;
	uint64_t elba;

	uint32_t uuid;
	uint32_t nsid;
	uint32_t dev_nsid;
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

	const char *be;
	const char *mem;
	const char *sync;
	const char *async;
	const char *admin;
	const char *dev;

	uint64_t shm_id;
	uint32_t main_core;
	const char *core_mask;

	struct xnvme_opts_css css; ///< SPDK controller-setup: do command-set-selection

	uint32_t use_cmb_sqs;
	const char *adrfam;

	uint32_t poll_io;
	uint32_t poll_sq;
	uint32_t register_files;
	uint32_t register_buffers;

	uint32_t truncate;
	uint32_t rdonly;
	uint32_t wronly;
	uint32_t rdwr;
	uint32_t create;
	uint32_t create_mode;

	uint32_t nr;
	uint32_t ad;
	uint32_t idw;
	uint32_t idr;

	uint32_t vec_cnt;

	uint32_t dtype;
	uint32_t dspec;
	uint32_t doper;
	uint32_t endir;
	uint32_t tgtdir;
	uint32_t nsr;

	uint32_t lsi;
	uint32_t pid;

	const char *kv_key;
	const char *kv_val;
	bool kv_store_add;
	bool kv_store_update;
	bool kv_store_compress;
};

void
xnvme_cli_args_pr(struct xnvme_cli_args *args, int opts);

/**
 * Command-Line Options, each command-line option can be one of ::xnvme_cli_opt_type
 *
 * @enum xnvme_cli_opt
 */
enum xnvme_cli_opt {
	XNVME_CLI_OPT_NONE = 0, ///< XNVME_CLI_OPT_NONE

	XNVME_CLI_OPT_CDW00 = 1,  ///< XNVME_CLI_OPT_CDW00
	XNVME_CLI_OPT_CDW01 = 2,  ///< XNVME_CLI_OPT_CDW01
	XNVME_CLI_OPT_CDW02 = 3,  ///< XNVME_CLI_OPT_CDW02
	XNVME_CLI_OPT_CDW03 = 4,  ///< XNVME_CLI_OPT_CDW03
	XNVME_CLI_OPT_CDW04 = 5,  ///< XNVME_CLI_OPT_CDW04
	XNVME_CLI_OPT_CDW05 = 6,  ///< XNVME_CLI_OPT_CDW05
	XNVME_CLI_OPT_CDW06 = 7,  ///< XNVME_CLI_OPT_CDW06
	XNVME_CLI_OPT_CDW07 = 8,  ///< XNVME_CLI_OPT_CDW07
	XNVME_CLI_OPT_CDW08 = 9,  ///< XNVME_CLI_OPT_CDW08
	XNVME_CLI_OPT_CDW09 = 10, ///< XNVME_CLI_OPT_CDW09
	XNVME_CLI_OPT_CDW10 = 11, ///< XNVME_CLI_OPT_CDW10
	XNVME_CLI_OPT_CDW11 = 12, ///< XNVME_CLI_OPT_CDW11
	XNVME_CLI_OPT_CDW12 = 13, ///< XNVME_CLI_OPT_CDW12
	XNVME_CLI_OPT_CDW13 = 14, ///< XNVME_CLI_OPT_CDW13
	XNVME_CLI_OPT_CDW14 = 15, ///< XNVME_CLI_OPT_CDW14
	XNVME_CLI_OPT_CDW15 = 16, ///< XNVME_CLI_OPT_CDW15

	XNVME_CLI_OPT_CMD_INPUT  = 17, ///< XNVME_CLI_OPT_CMD_INPUT
	XNVME_CLI_OPT_CMD_OUTPUT = 18, ///< XNVME_CLI_OPT_CMD_OUTPUT

	XNVME_CLI_OPT_DATA_NBYTES = 19, ///< XNVME_CLI_OPT_DATA_NBYTES
	XNVME_CLI_OPT_DATA_INPUT  = 20, ///< XNVME_CLI_OPT_DATA_INPUT
	XNVME_CLI_OPT_DATA_OUTPUT = 21, ///< XNVME_CLI_OPT_DATA_OUTPUT

	XNVME_CLI_OPT_META_NBYTES = 22, ///< XNVME_CLI_OPT_META_NBYTES
	XNVME_CLI_OPT_META_INPUT  = 23, ///< XNVME_CLI_OPT_META_INPUT
	XNVME_CLI_OPT_META_OUTPUT = 24, ///< XNVME_CLI_OPT_META_OUTPUT

	XNVME_CLI_OPT_LBAF = 25, ///< XNVME_CLI_OPT_LBAF
	XNVME_CLI_OPT_SLBA = 26, ///< XNVME_CLI_OPT_SLBA
	XNVME_CLI_OPT_ELBA = 27, ///< XNVME_CLI_OPT_ELBA
	XNVME_CLI_OPT_LBA  = 28, ///< XNVME_CLI_OPT_LBA
	XNVME_CLI_OPT_NLB  = 29, ///< XNVME_CLI_OPT_NLB

	XNVME_CLI_OPT_URI     = 30, ///< XNVME_CLI_OPT_URI
	XNVME_CLI_OPT_SYS_URI = 31, ///< XNVME_CLI_OPT_SYS_URI
	XNVME_CLI_OPT_UUID    = 32, ///< XNVME_CLI_OPT_UUID
	XNVME_CLI_OPT_NSID    = 33, ///< XNVME_CLI_OPT_NSID
	XNVME_CLI_OPT_CNS     = 34, ///< XNVME_CLI_OPT_CNS
	XNVME_CLI_OPT_CSI     = 35, ///< XNVME_CLI_OPT_CSI
	XNVME_CLI_OPT_INDEX   = 36, ///< XNVME_CLI_OPT_INDEX
	XNVME_CLI_OPT_SETID   = 37, ///< XNVME_CLI_OPT_SETID

	XNVME_CLI_OPT_CNTID      = 38, ///< XNVME_CLI_OPT_CNTID
	XNVME_CLI_OPT_LID        = 39, ///< XNVME_CLI_OPT_LID
	XNVME_CLI_OPT_LSP        = 40, ///< XNVME_CLI_OPT_LSP
	XNVME_CLI_OPT_LPO_NBYTES = 41, ///< XNVME_CLI_OPT_LPO_NBYTES
	XNVME_CLI_OPT_RAE        = 42, ///< XNVME_CLI_OPT_RAE
	XNVME_CLI_OPT_CLEAR      = 43, ///< XNVME_CLI_OPT_CLEAR
	XNVME_CLI_OPT_ZF         = 44, ///< XNVME_CLI_OPT_ZF
	XNVME_CLI_OPT_SES        = 45, ///< XNVME_CLI_OPT_SES
	XNVME_CLI_OPT_SEL        = 46, ///< XNVME_CLI_OPT_SEL
	XNVME_CLI_OPT_MSET       = 47, ///< XNVME_CLI_OPT_MSET
	XNVME_CLI_OPT_AUSE       = 48, ///< XNVME_CLI_OPT_AUSE
	XNVME_CLI_OPT_OVRPAT     = 49, ///< XNVME_CLI_OPT_OVRPAT
	XNVME_CLI_OPT_OWPASS     = 50, ///< XNVME_CLI_OPT_OWPASS
	XNVME_CLI_OPT_OIPBP      = 51, ///< XNVME_CLI_OPT_OIPBP
	XNVME_CLI_OPT_NODAS      = 52, ///< XNVME_CLI_OPT_NODAS

	XNVME_CLI_OPT_ACTION = 53, ///< XNVME_CLI_OPT_ACTION
	XNVME_CLI_OPT_ZRMS   = 54, ///< XNVME_CLI_OPT_ZRMS

	XNVME_CLI_OPT_PI  = 55, ///< XNVME_CLI_OPT_PI
	XNVME_CLI_OPT_PIL = 56, ///< XNVME_CLI_OPT_PIL

	XNVME_CLI_OPT_FID  = 57, ///< XNVME_CLI_OPT_FID
	XNVME_CLI_OPT_FEAT = 58, ///< XNVME_CLI_OPT_FEAT

	XNVME_CLI_OPT_SEED   = 59, ///< XNVME_CLI_OPT_SEED
	XNVME_CLI_OPT_LIMIT  = 60, ///< XNVME_CLI_OPT_LIMIT
	XNVME_CLI_OPT_IOSIZE = 61, ///< XNVME_CLI_OPT_IOSIZE
	XNVME_CLI_OPT_QDEPTH = 62, ///< XNVME_CLI_OPT_QDEPTH
	XNVME_CLI_OPT_DIRECT = 63, ///< XNVME_CLI_OPT_DIRECT

	XNVME_CLI_OPT_STATUS  = 64, ///< XNVME_CLI_OPT_STATUS
	XNVME_CLI_OPT_SAVE    = 65, ///< XNVME_CLI_OPT_SAVE
	XNVME_CLI_OPT_RESET   = 66, ///< XNVME_CLI_OPT_RESET
	XNVME_CLI_OPT_VERBOSE = 67, ///< XNVME_CLI_OPT_VERBOSE
	XNVME_CLI_OPT_HELP    = 68, ///< XNVME_CLI_OPT_HELP

	XNVME_CLI_OPT_COUNT  = 69, ///< XNVME_CLI_OPT_COUNT
	XNVME_CLI_OPT_OFFSET = 70, ///< XNVME_CLI_OPT_OFFSET

	XNVME_CLI_OPT_OPCODE = 71, ///< XNVME_CLI_OPT_OPCODE
	XNVME_CLI_OPT_FLAGS  = 72, ///< XNVME_CLI_OPT_FLAGS

	XNVME_CLI_OPT_ALL = 73, ///< XNVME_CLI_OPT_ALL

	XNVME_CLI_OPT_BE    = 74, ///< XNVME_CLI_OPT_BE
	XNVME_CLI_OPT_MEM   = 75, ///< XNVME_CLI_OPT_MEM
	XNVME_CLI_OPT_SYNC  = 76, ///< XNVME_CLI_OPT_SYNC
	XNVME_CLI_OPT_ASYNC = 77, ///< XNVME_CLI_OPT_ASYNC
	XNVME_CLI_OPT_ADMIN = 78, ///< XNVME_CLI_OPT_ADMIN
	XNVME_CLI_OPT_DEV   = 79, ///< XNVME_CLI_OPT_DEV

	XNVME_CLI_OPT_SHM_ID    = 80, ///< XNVME_CLI_OPT_SHM_ID
	XNVME_CLI_OPT_MAIN_CORE = 81, ///< XNVME_CLI_OPT_MAIN_CORE
	XNVME_CLI_OPT_CORE_MASK = 82, ///< XNVME_CLI_OPT_CORE_MASK

	XNVME_CLI_OPT_USE_CMB_SQS = 83, ///< XNVME_CLI_OPT_USE_CMB_SQS
	XNVME_CLI_OPT_CSS         = 84, ///< XNVME_CLI_OPT_CSS

	XNVME_CLI_OPT_POLL_IO          = 85, ///< XNVME_CLI_OPT_POLL_IO
	XNVME_CLI_OPT_POLL_SQ          = 86, ///< XNVME_CLI_OPT_POLL_SQ
	XNVME_CLI_OPT_REGISTER_FILES   = 87, ///< XNVME_CLI_OPT_REGISTER_FILES
	XNVME_CLI_OPT_REGISTER_BUFFERS = 88, ///< XNVME_CLI_OPT_REGISTER_BUFFERS

	XNVME_CLI_OPT_TRUNCATE = 89, ///< XNVME_CLI_OPT_TRUNCATE
	XNVME_CLI_OPT_RDONLY   = 90, ///< XNVME_CLI_OPT_RDONLY
	XNVME_CLI_OPT_WRONLY   = 91, ///< XNVME_CLI_OPT_WRONLY
	XNVME_CLI_OPT_RDWR     = 92, ///< XNVME_CLI_OPT_RDWR

	XNVME_CLI_OPT_CREATE      = 93, ///< XNVME_CLI_OPT_CREATE
	XNVME_CLI_OPT_CREATE_MODE = 94, ///< XNVME_CLI_OPT_CREATE_MODE

	XNVME_CLI_OPT_ADRFAM   = 96, ///< XNVME_CLI_OPT_ADRFAM
	XNVME_CLI_OPT_DEV_NSID = 97, ///< XNVME_CLI_OPT_DEV_NSID

	XNVME_CLI_OPT_VEC_CNT = 98, ///< XNVME_CLI_OPT_VEC_CNT

	XNVME_CLI_OPT_SUBNQN  = 99,  ///< XNVME_OPT_SUBNQN
	XNVME_CLI_OPT_HOSTNQN = 100, ///< XNVME_CLI_OPT_HOSTNQN

	XNVME_CLI_OPT_DTYPE  = 101, ///< XNVME_CLI_OPT_DTYPE
	XNVME_CLI_OPT_DSPEC  = 102, ///< XNVME_CLI_OPT_DSPEC
	XNVME_CLI_OPT_DOPER  = 103, ///< XNVME_CLI_OPT_DOPER
	XNVME_CLI_OPT_ENDIR  = 104, ///< XNVME_CLI_OPT_ENDIR
	XNVME_CLI_OPT_TGTDIR = 105, ///< XNVME_CLI_OPT_TGTDIR
	XNVME_CLI_OPT_NSR    = 106, ///< XNVME_CLI_OPT_NSR

	XNVME_CLI_OPT_POSA_TITLE     = 107, ///< XNVME_CLI_OPT_POSA_TITLE
	XNVME_CLI_OPT_NON_POSA_TITLE = 108, ///< XNVME_CLI_OPT_NON_POSA_TITLE

	XNVME_CLI_OPT_ORCH_TITLE = 109, ///< XNVME_CLI_OPT_ORCH_TITLE

	XNVME_CLI_OPT_AD  = 110, ///< XNVME_CLI_OPT_AD
	XNVME_CLI_OPT_IDW = 111, ///< XNVME_CLI_OPT_IDW
	XNVME_CLI_OPT_IDR = 112, ///< XNVME_CLI_OPT_IDR
	XNVME_CLI_OPT_LLB = 113, ///< XNVME_CLI_OPT_LLB

	XNVME_CLI_OPT_LSI = 114, ///< XNVME_CLI_OPT_LSI
	XNVME_CLI_OPT_PID = 115, ///< XNVME_CLI_OPT_PID

	XNVME_CLI_OPT_KV_KEY = 116, ///< XNVME_CLI_OPT_KV_KEY
	XNVME_CLI_OPT_KV_VAL = 117, ///< XNVME_CLI_OPT_KV_VAL

	XNVME_CLI_OPT_KV_STORE_UPDATE   = 118, ///< XNVME_CLI_OPT_KV_STORE_UPDATE
	XNVME_CLI_OPT_KV_STORE_ADD      = 119, ///< XNVME_CLI_OPT_KV_STORE_ADD
	XNVME_CLI_OPT_KV_STORE_COMPRESS = 120, ///< XNVME_CLI_OPT_KV_STORE_COMPRESS

	XNVME_CLI_OPT_END = 121, ///< XNVME_CLI_OPT_END
};

/**
 * XNVME_CLI_LFLG: ./cli --verbose; always optional
 * XNVME_CLI_LOPT: ./cli --arg 0x0; optionally provide a value
 * XNVME_CLI_LREQ: ./cli --arg 0x0; require providing a value
 * XNVME_CLI_POSA: ./cli arg1 arg2; required, ordered, and provides a value
 * XNVME_CLI_SKIP: ; This argument is used for formatting etc.
 *
 * @enum xnvme_cli_opt_type
 */
enum xnvme_cli_opt_type {
	XNVME_CLI_POSA = 0x1, ///< XNVME_CLI_POSA
	XNVME_CLI_LFLG = 0x2, ///< XNVME_CLI_LFLG
	XNVME_CLI_LOPT = 0x3, ///< XNVME_CLI_LOPT
	XNVME_CLI_LREQ = 0x4, ///< XNVME_CLI_LREQ
	XNVME_CLI_SKIP = 0x5, ///< XNVME_CLI_SKIP
};

enum xnvme_cli_opt_value_type {
	XNVME_CLI_OPT_VTYPE_URI  = 0x1,
	XNVME_CLI_OPT_VTYPE_NUM  = 0x2,
	XNVME_CLI_OPT_VTYPE_HEX  = 0x3,
	XNVME_CLI_OPT_VTYPE_FILE = 0x4,
	XNVME_CLI_OPT_VTYPE_STR  = 0x5,
	XNVME_CLI_OPT_VTYPE_SKIP = 0x6,
};

struct xnvme_cli_opt_attr {
	enum xnvme_cli_opt opt;
	enum xnvme_cli_opt_value_type vtype;
	const char *name;
	const char *descr;

	char getoptval; // character returned by getopt_log() when found
};

const struct xnvme_cli_opt_attr *
xnvme_cli_get_opt_attr(enum xnvme_cli_opt opt);

/**
 * @struct xnvme_cli_sub_opt
 */
struct xnvme_cli_sub_opt {
	enum xnvme_cli_opt opt;
	enum xnvme_cli_opt_type type;
};

/**
 * Forward declaration
 */
struct xnvme_cli;

typedef int (*xnvme_cli_subfunc)(struct xnvme_cli *);

#define XNVME_CLI_SUB_OPTS_LEN 200

#define XNVME_CLI_SUB_NAME_LEN_MAX 30

/**
 * Filled by the user of libxnvmc, usually statically right above main
 */
struct xnvme_cli_sub {
	const char *name;        ///< Short name of the sub-command
	const char *descr_short; ///< Short sub-command description
	const char *descr_long;  ///< Long sub-command description

	xnvme_cli_subfunc command; ///< Function to execute

	///< Options to parse into args and pass onto func
	struct xnvme_cli_sub_opt opts[XNVME_CLI_SUB_OPTS_LEN];
};

struct xnvme_cli {
	const char *title;       ///< Setup by user
	const char *descr_short; ///< Setup by user
	const char *descr_long;  ///< Setup by user

	int nsubs;                  ///< Setup by user
	struct xnvme_cli_sub *subs; ///< Setup by user

	int (*ver_pr)(int); ///< Setup by library if unset

	int argc;                   ///< Setup by library
	char **argv;                ///< Setup by library
	struct xnvme_cli_args args; ///< Setup by library

	int given[XNVME_CLI_OPT_END]; ///< Setup by library

	struct xnvme_cli_sub *sub; ///< Setup by library

	struct xnvme_timer timer; ///< Used by xnvme_cli_timer_*
};

enum xnvme_cli_opts {
	XNVME_CLI_INIT_NONE     = 0x0,
	XNVME_CLI_INIT_DEV_OPEN = 0x1,
};

/**
 * List of devices found on the system usable with xNVMe
 *
 * @struct xnvme_cli_enumeration
 */
struct xnvme_cli_enumeration {
	uint32_t capacity;            ///< Remaining unused entries
	uint32_t nentries;            ///< Used entries
	struct xnvme_ident entries[]; ///< Device entries
};

int
xnvme_cli_enumeration_alloc(struct xnvme_cli_enumeration **list, uint32_t capacity);

void
xnvme_cli_enumeration_free(struct xnvme_cli_enumeration *list);

int
xnvme_cli_enumeration_append(struct xnvme_cli_enumeration *list, const struct xnvme_ident *entry);

/**
 * Prints the given ::xnvme_cli_enumeration to the given output stream
 *
 * @param stream output stream used for printing
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_cli_enumeration_fpr(FILE *stream, struct xnvme_cli_enumeration *list, int opts);

int
xnvme_cli_enumeration_fpp(FILE *stream, struct xnvme_cli_enumeration *list, int opts);

int
xnvme_cli_enumeration_pp(struct xnvme_cli_enumeration *list, int opts);

/**
 * Prints the given ::xnvme_cli_enumeration to stdout
 *
 * @param list pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_cli_enumeration_pr(struct xnvme_cli_enumeration *list, int opts);

uint64_t
xnvme_cli_timer_start(struct xnvme_cli *cli);

uint64_t
xnvme_cli_timer_stop(struct xnvme_cli *cli);

void
xnvme_cli_timer_bw_pr(struct xnvme_cli *cli, const char *prefix, size_t nbytes);

void
xnvme_cli_pinf(const char *format, ...);

void
xnvme_cli_perr(const char *msg, int err);

int
xnvme_cli_run(struct xnvme_cli *cli, int argc, char **argv, int opts);

/**
 * Fill the given 'opts' with values parsed in the given 'cli'
 *
 * @param cli The command-line-interface to parse arguments for
 * @param opts The device-options to fill
 *
 * @return On success, 0 is returned.
 */
int
xnvme_cli_to_opts(const struct xnvme_cli *cli, struct xnvme_opts *opts);

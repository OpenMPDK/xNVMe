/**
 * libxnvme_spec - NVMe structs, enum, values and helper function declarations
 *
 * If an entity is defined in the NVMe specification, then an enum or struct should exist in this
 * header file. The purpose of this is to provide a single point of entry for spec. definitions for
 * the purpose of providing structs with accessors, possibly bit-fields, and enums for values such
 * as command opcodes.
 *
 * Auxilary header files are provided for the different IO command-sets, e.g. ``libxnvme_nvm.h``,
 * providing utility functions such as ``xnvme_nvm_read``, ``xnvme_nvm_write``, and similarly
 * ``libxnvme_znd.h`` with utilities such as ``xnvme_znd_append``, ``xnvme_znd_mgmt_send``.
 * These can also contain enums and structs, however, these are not based on definitions in the NVMe
 * specification, rather, these build on the spec. definition in order to provide something sligthly
 * more convenient to the user.
 *
 * A special class of utility functions are pretty-printers, all spec. defintions have two
 * associated pretty-printers named by the type, suffixed by ``_fpr`` and ``_pr``, for example:
 *
 * - xnvme_spec_cpl_fpr(), prints the given ``struct xnvme_spec_cpl`` struct to the given fstream
 * - xnvme_spec_cpl_pr(),  prints the given ``struct xnvme_spec_cpl`` struct to stdout
 *
 * These functions are auto-generated and available by importing ``libxnvme_spec_pp.h``. Thus, when
 * you see a definition in ``libxnvme_spec.h`` then you can count on always having a textual
 * representation available for that definition by importing ``libxnvme_spec_pp.h``.
 * You can also choose to add the ``libxnvme_pp.h`` which includes pretty-printers for all enums and
 * struct known by xNVMe.
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_spec.h
 */
#ifndef __LIBXNVME_SPEC_H
#define __LIBXNVME_SPEC_H

#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <libxnvme_util.h>

/**
 * NVMe Command Status Code Types
 *
 * @enum xnvme_spec_status_code_type
 */
enum xnvme_spec_status_code_type {
	XNVME_STATUS_CODE_TYPE_GENERIC	= 0x0,	///< Generic
	XNVME_STATUS_CODE_TYPE_CMDSPEC	= 0x1,	///< Command Specific
	XNVME_STATUS_CODE_TYPE_MEDIA	= 0x2,	///< Media and data integrity error
	XNVME_STATUS_CODE_TYPE_PATH	= 0x3,	///< Path related status
	XNVME_STATUS_CODE_TYPE_VENDOR	= 0x7,	///< Vendor specific
};

/**
 * NVMe Command Completion Status field
 *
 * @struct xnvme_spec_status
 */
struct xnvme_spec_status {
	union {
		struct {
			uint16_t p	: 1;	///< Phase tag
			uint16_t sc	: 8;	///< Status codes
			uint16_t sct	: 3;	///< Status code type
			uint16_t rsvd2	: 2;
			uint16_t m	: 1;	///< More
			uint16_t dnr	: 1;	///< Do not retry
		};
		uint16_t val;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_status) == 2, "Incorrect size")

/**
 * Completion Queue Entry
 *
 * @struct xnvme_spec_cpl
 */
struct xnvme_spec_cpl {
	union {
		struct {
			/* dword 0 */
			uint32_t	cdw0;	///< command-specific

			/* dword 1 */
			uint32_t	rsvd1;
		};

		uint64_t result;	/* Combined result of cdw 0 and cdw 1 */
	};

	/* dword 2 */
	uint16_t		sqhd;	///< submission queue head pointer
	uint16_t		sqid;	///< submission queue identifier

	/* dword 3 */
	uint16_t		cid;	///< command identifier
	struct xnvme_spec_status	status;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cpl) == 16, "Incorrect size")

/**
 * NVMe get-log-page entry for error information
 *
 * NVMe 1.4: Figure 194
 *
 * NOTE: using __attribute__((packed))__ as GCC does not like "uint16_t ct"
 * for some reason. Without the packing it becomes 2 bytes larger
 *
 * @struct xnvme_spec_log_health_entry
 */
struct __attribute__((packed)) xnvme_spec_log_health_entry {
	uint8_t crit_warn;		///< Critical Warning
	uint16_t comp_temp;		///< Composite Temperature (Temp.)
	uint8_t avail_spare;		///< Available Spare (pct)
	uint8_t avail_spare_thresh;	///< Available Spare Threshold (pct)
	uint8_t pct_used;		///< Percentage used (pct), can exceed 100
	uint8_t eg_crit_warn_sum;	///< Endurance Group Critical Warning Summary
	uint8_t rsvd8[25];

	uint8_t data_units_read[16];	///< Data Units Read
	uint8_t data_units_written[16];	///< Data Units Written

	uint8_t host_read_cmds[16];	///< Host Read Commands
	uint8_t host_write_cmds[16];	///< Host Write Commands

	uint8_t ctrlr_busy_time[16];	///< Controller Busy Time
	uint8_t pwr_cycles[16];		///< Power Cycles
	uint8_t pwr_on_hours[16];	///< Power On Hours
	uint8_t unsafe_shutdowns[16];	///< Unsafe Shutdowns
	uint8_t mdi_errs[16];		///< Media and Data Integrity Errors
	uint8_t nr_err_logs[16];	///< Nr. of Error Information Log Entries (life)

	uint32_t warn_comp_temp_time;	///< Warning Composite Temp. Time
	uint32_t crit_comp_temp_time;	///< Critical Composite Temp. Time
	uint16_t temp_sens[8];	///< Temp. Sensor Temp. 1-8
	uint32_t tmt1tc;	///< Thermal Management Temp. 1 Trans. Count
	uint32_t tmt2tc;	///< Thermal Management Temp. 2 Trans. Count
	uint32_t tttmt1;	///< Total Time for Thermal Mgmt. Temp. 1
	uint32_t tttmt2;	///< Total Time for Thermal Mgmt. Temp. 2
	uint8_t rsvd[280];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_health_entry) == 512, "Incorrect size")

/**
 * NVMe get-log-page entry for error information
 *
 * NVMe 1.4 - Figure ?
 *
 * @struct xnvme_spec_log_erri_entry
 */
struct xnvme_spec_log_erri_entry {
	uint64_t ecnt;
	uint16_t sqid;
	uint16_t cid;
	struct xnvme_spec_status status;
	uint16_t eloc;
	uint64_t lba;
	uint32_t nsid;
	uint8_t ven_si;
	uint8_t trtype;
	uint8_t reserved30[2];
	uint64_t cmd_si;
	uint16_t trtype_si;
	uint8_t reserved42[22];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_log_erri_entry) == 64, "Incorrect size")

/**
 * Identifiers (lpi) for NVMe get-log-page
 *
 * NVMe 1.4 - Figure ?
 *
 * @enum xnvme_spec_log_lpi
 */
enum xnvme_spec_log_lpi {
	XNVME_SPEC_LOG_RSVD = 0x0,	///< XNVME_SPEC_LOG_RSVD
	XNVME_SPEC_LOG_ERRI = 0x1,	///< XNVME_SPEC_LOG_ERRI
	XNVME_SPEC_LOG_HEALTH = 0x2,	///< XNVME_SPEC_LOG_HEALTH
	XNVME_SPEC_LOG_FW = 0x3,	///< XNVME_SPEC_LOG_FW
	XNVME_SPEC_LOG_CHNS = 0x4,	///< XNVME_SPEC_LOG_CHNS
	XNVME_SPEC_LOG_CSAE = 0x5,	///< XNVME_SPEC_LOG_CSAE
	XNVME_SPEC_LOG_SELFTEST = 0x6,	///< XNVME_SPEC_LOG_SELFTEST
	XNVME_SPEC_LOG_TELEHOST = 0x7,	///< XNVME_SPEC_LOG_TELEHOST
	XNVME_SPEC_LOG_TELECTRLR = 0x8,	///< XNVME_SPEC_LOG_TELECTRLR
};

/**
 * @enum xnvme_spec_idfy_cns
 */
enum xnvme_spec_idfy_cns {
	XNVME_SPEC_IDFY_NS = 0x0,	///< XNVME_SPEC_IDFY_NS
	XNVME_SPEC_IDFY_CTRLR = 0x1,	///< XNVME_SPEC_IDFY_CTRLR
	XNVME_SPEC_IDFY_NSLIST = 0x2,	///< XNVME_SPEC_IDFY_NSLIST
	XNVME_SPEC_IDFY_NSDSCR = 0x3,	///< XNVME_SPEC_IDFY_NSDSCR
	XNVME_SPEC_IDFY_SETL = 0x4,	///< XNVME_SPEC_IDFY_SETL

	XNVME_SPEC_IDFY_NS_IOCS = 0x05,		///< XNVME_SPEC_IDFY_NS_IOCS
	XNVME_SPEC_IDFY_CTRLR_IOCS = 0x6,	///< XNVME_SPEC_IDFY_CTRLR_IOCS
	XNVME_SPEC_IDFY_NSLIST_IOCS = 0x7,	///< XNVME_SPEC_IDFY_NSLIST_IOCS

	XNVME_SPEC_IDFY_NSLIST_ALLOC = 0x10,	///< XNVME_SPEC_IDFY_NSLIST_ALLOC
	XNVME_SPEC_IDFY_NS_ALLOC = 0x11,	///< XNVME_SPEC_IDFY_NS_ALLOC
	XNVME_SPEC_IDFY_CTRLR_NS = 0x12,	///< XNVME_SPEC_IDFY_CTRLR_NS
	XNVME_SPEC_IDFY_CTRLR_SUB = 0x13,	///< XNVME_SPEC_IDFY_CTRLR_SUB
	XNVME_SPEC_IDFY_CTRLR_PRI = 0x14,	///< XNVME_SPEC_IDFY_CTRLR_PRI
	XNVME_SPEC_IDFY_CTRLR_SEC = 0x15,	///< XNVME_SPEC_IDFY_CTRLR_SEC
	XNVME_SPEC_IDFY_NSGRAN = 0x16,		///< XNVME_SPEC_IDFY_NSGRAN
	XNVME_SPEC_IDFY_UUIDL = 0x17,		///< XNVME_SPEC_IDFY_UUIDL

	XNVME_SPEC_IDFY_NSLIST_ALLOC_IOCS = 0x1A,	///< XNVME_SPEC_IDFY_NSLIST_ALLOC_IOCS
	XNVME_SPEC_IDFY_NS_ALLOC_IOCS = 0x1B,		///< XNVME_SPEC_IDFY_NS_ALLOC_IOCS
	XNVME_SPEC_IDFY_IOCS = 0x1C,			///< XNVME_SPEC_IDFY_IOCS
};

/**
 * @struct xnvme_spec_lbaf
 */
struct xnvme_spec_lbaf {
	uint16_t ms;	///< metadata size
	uint8_t  ds;	///< lba data size
	uint8_t  rp: 2;	///< relative performance
	uint8_t	 rsvd : 6;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_lbaf) == 4, "Incorrect size")

/**
 * Command Set Identifiers
 *
 * @see Specification Section 5.15.2.1, figure X1
 *
 * @enum xnvme_spec_csi
 */
enum xnvme_spec_csi {
	XNVME_SPEC_CSI_NVM	= 0x0,	///< XNVME_SPEC_CSI_NVM
	XNVME_SPEC_CSI_ZONED	= 0x2,	///< XNVME_SPEC_CSI_ZONED
};

/**
 * Representation of NVMe completion result Identify Namespace
 *
 * That is, for opcode XNVME_SPEC_OPC_IDFY(0x06) with XNVME_SPEC_IDFY_NS(0x0)
 *
 * @struct xnvme_spec_idfy_ns
 */
struct xnvme_spec_idfy_ns {
	uint64_t	nsze;	///< namespace size
	uint64_t	ncap;	///< namespace capacity
	uint64_t	nuse;	///< namespace utilization

	/** namespace features */
	struct {
		uint8_t	thin_prov : 1;			///< thin provisioning
		uint8_t	ns_atomic_write_unit : 1;	///< NAWUN, NAWUPF, and NACWU
		uint8_t	dealloc_or_unwritten_error : 1;
		uint8_t	guid_never_reused : 1;		////< Non-zero NGUID and EUI64

		uint8_t	reserved1 : 4;
	} nsfeat;

	uint8_t		nlbaf; ///< number of lba formats

	/** formatted lba size */
	struct {
		uint8_t	format    : 4;
		uint8_t	extended  : 1;
		uint8_t	reserved2 : 3;
	} flbas;

	/** metadata capabilities */
	struct {
		/** metadata can be transferred as part of data prp list */
		uint8_t		extended  : 1;

		/** metadata can be transferred with separate metadata pointer */
		uint8_t		pointer   : 1;

		/** reserved */
		uint8_t		reserved3 : 6;
	} mc;

	/** end-to-end data protection capabilities */
	union {
		struct {
			/** protection information type 1 */
			uint8_t		pit1     : 1;

			/** protection information type 2 */
			uint8_t		pit2     : 1;

			/** protection information type 3 */
			uint8_t		pit3     : 1;

			/** first eight bytes of metadata */
			uint8_t		md_start : 1;

			/** last eight bytes of metadata */
			uint8_t		md_end   : 1;
		};
		uint8_t val;
	} dpc;

	/** end-to-end data protection type settings */
	union {
		struct {
			/** protection information type */
			uint8_t		pit       : 3;

			/** 1 == protection info transferred at start of metadata */
			/** 0 == protection info transferred at end of metadata */
			uint8_t		md_start  : 1;

			uint8_t		reserved4 : 4;
		};
		uint8_t val;
	} dps;

	/** namespace multi-path I/O and namespace sharing capabilities */
	struct {
		uint8_t		can_share : 1;
		uint8_t		reserved : 7;
	} nmic;

	/** reservation capabilities */
	union {
		struct {
			/** supports persist through power loss */
			uint8_t		persist : 1;

			/** supports write exclusive */
			uint8_t		write_exclusive : 1;

			/** supports exclusive access */
			uint8_t		exclusive_access : 1;

			/** supports write exclusive - registrants only */
			uint8_t		write_exclusive_reg_only : 1;

			/** supports exclusive access - registrants only */
			uint8_t		exclusive_access_reg_only : 1;

			/** supports write exclusive - all registrants */
			uint8_t		write_exclusive_all_reg : 1;

			/** supports exclusive access - all registrants */
			uint8_t		exclusive_access_all_reg : 1;

			/** supports ignore existing key */
			uint8_t		ignore_existing_key : 1;
		};
		uint8_t	 val;
	} nsrescap;

	/** format progress indicator */
	union {
		struct {
			uint8_t		percentage_remaining : 7;
			uint8_t		fpi_supported : 1;
		};
		uint8_t val;
	} fpi;

	/** deallocate logical features */
	union {
		struct {
			/**
			 * Value read from deallocated blocks
			 *
			 * 000b = not reported
			 * 001b = all bytes 0x00
			 * 010b = all bytes 0xFF
			 *
			 */
			uint8_t read_value : 3;

			/** Supports Deallocate bit in Write Zeroes */
			uint8_t write_zero_deallocate : 1;

			/**
			 * Guard field behavior for deallocated logical blocks
			 * 0: contains 0xFFFF
			 * 1: contains CRC for read value
			 */
			uint8_t guard_value : 1;

			uint8_t reserved : 3;
		} bits;

		uint8_t val;
	} dlfeat;

	/** namespace atomic write unit normal */
	uint16_t nawun;

	/** namespace atomic write unit power fail */
	uint16_t nawupf;

	/** namespace atomic compare & write unit */
	uint16_t nacwu;

	/** namespace atomic boundary size normal */
	uint16_t nabsn;

	/** namespace atomic boundary offset */
	uint16_t nabo;

	/** namespace atomic boundary size power fail */
	uint16_t nabspf;

	/** namespace optimal I/O boundary in logical blocks */
	uint16_t noiob;

	uint64_t nvmcap[2];	///< NVM Capacity

	uint8_t reserved64[40];

	/** namespace globally unique identifier */
	uint8_t nguid[16];

	/** IEEE extended unique identifier */
	uint64_t eui64;

	struct xnvme_spec_lbaf	lbaf[16];	/// LBA format support

	uint8_t rsvd3776[3648];

	uint8_t vendor_specific[256];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ns) == 4096, "Incorrect size")

#define XNVME_SPEC_CTRLR_SN_LEN 20
#define XNVME_SPEC_CTRLR_MN_LEN 40
#define XNVME_SPEC_CTRLR_FR_LEN 8

/**
 * @struct xnvme_spec_power_state
 */
struct xnvme_spec_power_state {
	uint16_t mp;			/* bits 15:00: maximum power */

	uint8_t reserved1;

	uint8_t mps		: 1;	/* bit 24: max power scale */
	uint8_t nops		: 1;	/* bit 25: non-operational state */
	uint8_t reserved2	: 6;

	uint32_t enlat;			/* bits 63:32: entry latency in microseconds */
	uint32_t exlat;			/* bits 95:64: exit latency in microseconds */

	uint8_t rrt		: 5;	/* bits 100:96: relative read throughput */
	uint8_t reserved3	: 3;

	uint8_t rrl		: 5;	/* bits 108:104: relative read latency */
	uint8_t reserved4	: 3;

	uint8_t rwt		: 5;	/* bits 116:112: relative write throughput */
	uint8_t reserved5	: 3;

	uint8_t rwl		: 5;	/* bits 124:120: relative write latency */
	uint8_t reserved6	: 3;

	uint8_t reserved7[16];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_power_state) == 32, "Incorrect size")

/**
 * @struct xnvme_spec_vs_register
 */
union xnvme_spec_vs_register {
	struct {
		uint32_t ter : 8;	///< Tertiary version
		uint32_t mnr : 8;	///< Indicated Minor Version
		uint32_t mjr : 16;	///< Indicated Major Version
	} bits;
	uint32_t val;
};
XNVME_STATIC_ASSERT(sizeof(union xnvme_spec_vs_register) == 4, "Incorrect size")

/**
 * @struct xnvme_spec_idfy_ctrlr
 */
struct xnvme_spec_idfy_ctrlr {
	/* bytes 0-255: controller capabilities and features */

	uint16_t vid;				///< PCI Vendor ID
	uint16_t ssvid;				///< PCI Subsystem Vendor ID
	int8_t sn[XNVME_SPEC_CTRLR_SN_LEN];	///< SerialNumber
	int8_t mn[XNVME_SPEC_CTRLR_MN_LEN];	///< Model Number
	uint8_t fr[XNVME_SPEC_CTRLR_FR_LEN];	///< Firmware Revision
	uint8_t rab;				///< Recomm. Arbitration Burst
	uint8_t ieee[3];			///< IEEE OUI Identifier

	/** controller multi-path I/O and namespace sharing capabilities */
	union {
		struct {
			uint8_t multi_port	: 1;
			uint8_t multi_host	: 1;
			uint8_t sr_iov		: 1;
			uint8_t reserved	: 5;
		};
		uint8_t val;
	} cmic;

	uint8_t mdts;				///< Maximum Data Transfer Size
	uint16_t cntlid;			///< Controller ID
	union xnvme_spec_vs_register ver;		///< Version

	uint32_t rtd3r;				///< RTD3 Resume Latency
	uint32_t rtd3e;				///< RTD3 Resume Latency

	/** optional asynchronous events supported */
	union {
		struct {
			uint32_t reserved1		: 8;

			/** Supports sending Namespace Attribute Notices. */
			uint32_t ns_attribute_notices	: 1;

			/** Supports sending Firmware Activation Notices. */
			uint32_t fw_activation_notices	: 1;

			uint32_t reserved2		: 22;
		};
		uint32_t val;
	} oaes;

	/** controller attributes */
	union {
		struct {
			/** Supports 128-bit host identifier */
			uint32_t host_id_exhid_supported: 1;

			/** Supports non-operational power state permissive mode */
			uint32_t non_operational_power_state_permissive_mode: 1;

			uint32_t reserved: 30;
		};
		uint32_t val;
	} ctratt;

	uint8_t reserved_100[12];

	uint8_t fguid[16];	///< FRU Globally Unique Ident.

	uint8_t reserved_128[128];

	/* bytes 256-511: admin command set attributes */

	/** optional admin command support */
	union {
		struct {
			/* supports security send/receive commands */
			uint16_t security			: 1;

			/* supports format nvm command */
			uint16_t format				: 1;

			/* supports firmware activate/download commands */
			uint16_t firmware			: 1;

			/* supports ns manage/ns attach commands */
			uint16_t ns_manage			: 1;
			uint16_t device_self_test		: 1;
			uint16_t directives			: 1;
			uint16_t nvme_mi			: 1;

			/** Supports SPDK_NVME_OPC_VIRTUALIZATION_MANAGEMENT */
			uint16_t virtualization_management	: 1;

			/** Supports SPDK_NVME_OPC_DOORBELL_BUFFER_CONFIG */
			uint16_t doorbell_buffer_config		: 1;
			uint16_t oacs_rsvd			: 7;
		};
		uint16_t val;
	} oacs;

	/** abort command limit */
	uint8_t acl;

	/** asynchronous event request limit */
	uint8_t aerl;

	/** firmware updates */
	union {
		struct {
			/* first slot is read-only */
			uint8_t slot1_ro			: 1;

			/* number of firmware slots */
			uint8_t num_slots			: 3;

			/* support activation without reset */
			uint8_t activation_without_reset	: 1;

			uint8_t frmw_rsvd			: 3;
		};
		uint8_t val;
	} frmw;

	/** Log Page Attributes */
	union {
		struct {
			/* per namespace smart/health log page */
			uint8_t ns_smart	: 1;
			/* command effects log page */
			uint8_t celp		: 1;
			/* extended data (32bit vs 12bit) for get log page */
			uint8_t edlp		: 1;
			/* telemetry log pages and notices */
			uint8_t telemetry	: 1;
			/* Persistent event log */
			uint8_t pel		: 1;

			uint8_t lpa_rsvd	: 3;
		};
		uint8_t val;
	} lpa;

	uint8_t elpe;		///< Error Log Page Entries
	uint8_t npss;		///< Number of Power States Supported

	/** admin vendor specific command configuration */
	union {
		struct {
			/* admin vendor specific commands use disk format */
			uint8_t spec_format	: 1;
			uint8_t avscc_rsvd	: 7;
		};
		uint8_t val;
	} avscc;

	/** autonomous power state transition attributes */
	union {
		struct {
			uint8_t supported	: 1;
			uint8_t apsta_rsvd	: 7;
		};
		uint8_t val;
	} apsta;

	uint16_t wctemp;	///< Warning Composite Temperature threshold
	uint16_t cctemp;	///< Critical Composite Temperature threshold
	uint16_t mtfa;		///< Maximum Time for Firmware Activation
	uint32_t hmpre;		///< Host Memory Buffer Preferred size
	uint32_t hmmin;		///< Host Memory Buffer Minimum size */

	/** total NVM capacity */
	uint64_t tnvmcap[2];

	/** unallocated NVM capacity */
	uint64_t unvmcap[2];

	/** replay protected memory block support */
	union {
		struct {
			uint8_t		num_rpmb_units	: 3;
			uint8_t		auth_method	: 3;
			uint8_t		reserved1	: 2;

			uint8_t		reserved2;

			uint8_t		total_size;
			uint8_t		access_size;
		};
		uint32_t val;
	} rpmbs;

	/** extended device self-test time (in minutes) */
	uint16_t edstt;

	/** device self-test options */
	union {
		struct {
			/** Device supports only one device self-test operation at a time */
			uint8_t one_only	: 1;

			uint8_t reserved	: 7;
		} bits;
		uint8_t val;
	} dsto;

	/**
	 * Firmware update granularity
	 *
	 * 4KB units
	 * 0x00 = no information provided
	 * 0xFF = no restriction
	 */
	uint8_t fwug;

	/**
	 * Keep Alive Support
	 *
	 * Granularity of keep alive timer in 100 ms units
	 * 0 = keep alive not supported
	 */
	uint16_t kas;

	/** Host controlled thermal management attributes */
	union {
		struct {
			uint16_t	supported : 1;
			uint16_t	reserved : 15;
		} bits;
		uint16_t val;
	} hctma;

	uint16_t mntmt;	///< Minimum Thermal Management Temperature
	uint16_t mxtmt;	///< Maximum Thermal Management Temperature

	/** Sanitize capabilities */
	union {
		struct {
			uint32_t crypto_erase	: 1;
			uint32_t block_erase	: 1;
			uint32_t overwrite	: 1;
			uint32_t reserved	: 29;
		} bits;
		uint32_t val;
	} sanicap;

	uint8_t reserved3[180];

	/* bytes 512-703: nvm command set attributes */

	/** submission queue entry size */
	union {
		struct {
			uint8_t		min : 4;
			uint8_t		max : 4;
		};
		uint8_t val;
	} sqes;

	/** completion queue entry size */
	union {
		struct {
			uint8_t		min : 4;
			uint8_t		max : 4;
		};
		uint8_t val;
	} cqes;

	uint16_t maxcmd;

	uint32_t nn;	///< Number of Namespaces

	/** optional nvm command support */
	union {
		struct {
			uint16_t compare		: 1;
			uint16_t write_unc		: 1;
			uint16_t dsm			: 1;
			uint16_t write_zeroes		: 1;
			uint16_t set_features_save	: 1;
			uint16_t reservations		: 1;
			uint16_t timestamp		: 1;
			uint16_t reserved		: 9;
		};
		uint16_t val;
	} oncs;

	uint16_t fuses; ///< Fused Operation Support

	/** format nvm attributes */
	union {
		struct {
			uint8_t format_all_ns		: 1;
			uint8_t erase_all_ns		: 1;
			uint8_t crypto_erase_supported	: 1;
			uint8_t reserved		: 5;
		};
		uint8_t val;
	} fna;

	/** volatile write cache */
	union {
		struct {
			uint8_t present		: 1;
			uint8_t flush_broadcast	: 2;
			uint8_t reserved	: 5;
		};
		uint8_t val;
	} vwc;

	uint16_t awun;		///< Atomic Write Unit Normal
	uint16_t awupf;		///< Atomic Write Unit Power Fail
	uint8_t nvscc;		///< NVM vendor specific command configuration

	uint8_t reserved531;

	uint16_t acwu;		///< atomic compare & write unit

	uint16_t reserved534;

	/** SGL support */
	union {
		struct {
			uint32_t supported		: 2;
			uint32_t keyed_sgl		: 1;
			uint32_t reserved1		: 13;
			uint32_t bit_bucket_descriptor	: 1;
			uint32_t metadata_pointer	: 1;
			uint32_t oversized_sgl		: 1;
			uint32_t metadata_address	: 1;
			uint32_t sgl_offset		: 1;
			uint32_t transport_sgl		: 1;
			uint32_t reserved2		: 10;
		};
		uint32_t val;
	} sgls;

	uint32_t mnan;		///< Maximum Number of Allowed Namespaces

	uint8_t reserved4[224];

	uint8_t subnqn[256];

	uint8_t reserved5[768];

	/** NVMe over Fabrics-specific fields */
	struct {
		/** I/O queue command capsule supported size (16-byte units) */
		uint32_t ioccsz;

		/** I/O queue response capsule supported size (16-byte units) */
		uint32_t iorcsz;

		/** In-capsule data offset (16-byte units) */
		uint16_t icdoff;

		/** Controller attributes */
		struct {
			uint8_t ctrlr_model	: 1;
			uint8_t reserved	: 7;
		} ctrattr;

		/** Maximum SGL block descriptors (0 = no limit) */
		uint8_t msdbd;

		uint8_t reserved[244];
	} nvmf_specific;

	/* bytes 2048-3071: power state descriptors */
	struct xnvme_spec_power_state psd[32];

	/* bytes 3072-4095: vendor specific */
	uint8_t vs[1024];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * Representation of I/O Command Set Vector
 *
 * See NVMe spec tbd, section xyz, for details
 *
 * @struct xnvme_spec_cs_vector
 */
struct xnvme_spec_cs_vector {
	union {
		struct {
			uint64_t nvm : 1;
			uint64_t rsvd1 : 1;
			uint64_t zns : 1;
			uint64_t rsvd : 61;
		};
		uint64_t val;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cs_vector) == 8, "Incorrect size")

#define XNVME_SPEC_IDFY_CS_IOCSC_LEN 512

/**
 * Representation of I/O Command Set data structure
 *
 * See NVMe spec tbd, section xyz, for details
 *
 * @struct xnvme_spec_idfy_cs
 */
struct xnvme_spec_idfy_cs {
	// I/O Command Set Combinations
	struct xnvme_spec_cs_vector iocsc[XNVME_SPEC_IDFY_CS_IOCSC_LEN];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy_cs) == 4096, "Incorrect size")

/**
 * NVMe completion result accessor
 *
 * TODO: clarify
 *
 * @struct xnvme_spec_idfy
 */
struct xnvme_spec_idfy {
	union {
		struct xnvme_spec_idfy_ctrlr ctrlr;
		struct xnvme_spec_idfy_ns ns;
		struct xnvme_spec_idfy_cs cs;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_idfy) == 4096, "Incorrect size")

/**
 * NVMe Admin Command Opcodes
 *
 * @see specification Section xx, figure yy
 *
 * @enum xnvme_spec_adm_opc
 */
enum xnvme_spec_adm_opc {
	XNVME_SPEC_ADM_OPC_LOG = 0x02,	///< XNVME_SPEC_ADM_OPC_LOG
	XNVME_SPEC_ADM_OPC_IDFY = 0x06,	///< XNVME_SPEC_ADM_OPC_IDFY

	XNVME_SPEC_ADM_OPC_SFEAT = 0x09,	///< XNVME_SPEC_ADM_OPC_SFEAT
	XNVME_SPEC_ADM_OPC_GFEAT = 0x0A,	///< XNVME_SPEC_ADM_OPC_GFEAT
};

/**
 * NVMe Non-Volatile Memory Command Opcodes
 *
 * @see specification Section xx, figure yy
 *
 * @enum xnvme_spec_nvm_opc
 */
enum xnvme_spec_nvm_opc {
	XNVME_SPEC_NVM_OPC_WRITE = 0x01,	///< XNVME_SPEC_NVM_OPC_WRITE
	XNVME_SPEC_NVM_OPC_READ = 0x02,		///< XNVME_SPEC_NVM_OPC_READ

	XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE = 0x04,	///< XNVME_SPEC_NVM_OPC_WRITE_UNCORRECTABLE
	XNVME_SPEC_NVM_OPC_WRITE_ZEROES = 0x08,		///< XNVME_SPEC_NVM_OPC_WRITE_ZEROES

	XNVME_SPEC_NVM_OPC_SCOPY	= 0x19,	///< XNVME_SPEC_NVM_OPC_SCOPY

	XNVME_SPEC_NVM_OPC_FMT = 0x80,		///< XNVME_SPEC_NVM_OPC_FMT
	XNVME_SPEC_NVM_OPC_SANITIZE = 0x84,	///< XNVME_SPEC_NVM_OPC_SANITIZE
};

/**
 * NVMe Feature Identifiers
 *
 * TODO: expand these
 *
 * @enum xnvme_spec_feat_id
 */
enum xnvme_spec_feat_id {
	XNVME_SPEC_FEAT_ARBITRATION = 0x1, ///< XNVME_SPEC_FEAT_ARBITRATION
	XNVME_SPEC_FEAT_PWR_MGMT = 0x2, ///< XNVME_SPEC_FEAT_PWR_MGMT
	XNVME_SPEC_FEAT_LBA_RANGETYPE = 0x3, ///< XNVME_SPEC_FEAT_LBA_RANGETYPE
	XNVME_SPEC_FEAT_TEMP_THRESHOLD = 0x4, ///< XNVME_SPEC_FEAT_TEMP_THRESHOLD
	XNVME_SPEC_FEAT_ERROR_RECOVERY = 0x5, ///< XNVME_SPEC_FEAT_ERROR_RECOVERY
	XNVME_SPEC_FEAT_VWCACHE = 0x6, ///< XNVME_SPEC_FEAT_VWCACHE
	XNVME_SPEC_FEAT_NQUEUES = 0x7, ///< XNVME_SPEC_FEAT_NQUEUES
};

/**
 * @enum xnvme_spec_feat_sel
 */
enum xnvme_spec_feat_sel {
	XNVME_SPEC_FEAT_SEL_CURRENT = 0x0, ///< XNVME_SPEC_FEAT_SEL_CURRENT
	XNVME_SPEC_FEAT_SEL_DEFAULT = 0x1, ///< XNVME_SPEC_FEAT_SEL_DEFAULT
	XNVME_SPEC_FEAT_SEL_SAVED = 0x2, ///< XNVME_SPEC_FEAT_SEL_SAVED
	XNVME_SPEC_FEAT_SEL_SUPPORTED = 0x3 ///< XNVME_SPEC_FEAT_SEL_SUPPORTED
};

/**
 * Encapsulation of NVMe/NVM features
 *
 * This structure is an accessor for Command Dword 11 of an Get Features Cmd.
 * and encapsulation of values provided to the `xnvme_cmd_gfeat`.
 *
 * @struct xnvme_spec_feat
 */
struct xnvme_spec_feat {
	union {

		struct {
			uint32_t tmpth	: 16;	///< Temperature threshold
			uint32_t tmpsel	: 4;	///< Threshold Temp. Select
			uint32_t thsel	: 3;	///< Threshold Type Select
		} temp_threshold;

		struct {
			uint32_t tler  : 16;
			uint32_t dulbe :  1;
			uint32_t rsvd  : 15;
		} error_recovery;	// Error recovery attributes

		struct {
			uint32_t nsqa	: 16;
			uint32_t ncqa	: 16;
		} nqueues;

		uint32_t val;	///< For constructing feature without accessors
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_feat) == 4, "Incorrect size")

#define XNVME_SPEC_FEAT_ERROR_RECOVERY_DULBE(feat) (feat & (1 << 16))
#define XNVME_SPEC_FEAT_ERROR_RECOVERY_TLER(feat)  (feat & 0xffff)

#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_HECC(feat)  (feat & 0x1)
#define XNVME_SPEC_FEAT_MEDIA_FEEDBACK_VHECC(feat) (feat & 0x2)

/**
 * Representation of DSM ranges as defined in NVM Express 1.3 Figure 207 Dataset
 * Management Range Definition Figure 207
 *
 * @struct xnvme_spec_dsm_range
 */
struct xnvme_spec_dsm_range {
	uint32_t	cattr;	///< Context attributes
	uint32_t	nlb;	///< Length in logical blocks
	uint64_t	slba;	///< Starting LBA
};

/**
 * Enumeration of NVMe flags
 *
 * TODO: document
 * @enum xnvme_spec_flag
 */
enum xnvme_spec_flag {
	// Limited Retry (LR)
	XNVME_SPEC_FLAG_LIMITED_RETRY		= 0x1 << 15,

	// Force Unit Access (FUA)
	XNVME_SPEC_FLAG_FORCE_UNIT_ACCESS	= 0x1 << 14,

	// Protection Information Check (PRCK)
	XNVME_SPEC_FLAG_PRINFO_PRCHK_REF	= 0x1 << 10,
	XNVME_SPEC_FLAG_PRINFO_PRCHK_APP	= 0x1 << 11,
	XNVME_SPEC_FLAG_PRINFO_PRCHK_GUARD	= 0x1 << 12,

	// Protection Information Action (PRACT)
	XNVME_SPEC_FLAG_PRINFO_PRACT		= 0x1 << 13,
};

/**
 * @enum xnvme_nvme_sgl_descriptor_type
 */
enum xnvme_nvme_sgl_descriptor_type {
	XNVME_SPEC_SGL_DESCR_TYPE_DATA_BLOCK		= 0x0, ///< XNVME_SPEC_SGL_DESCR_TYPE_DATA_BLOCK
	XNVME_SPEC_SGL_DESCR_TYPE_BIT_BUCKET		= 0x1, ///< XNVME_SPEC_SGL_DESCR_TYPE_BIT_BUCKET
	XNVME_SPEC_SGL_DESCR_TYPE_SEGMENT		= 0x2, ///< XNVME_SPEC_SGL_DESCR_TYPE_SEGMENT
	XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT		= 0x3, ///< XNVME_SPEC_SGL_DESCR_TYPE_LAST_SEGMENT
	XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK	= 0x4, ///< XNVME_SPEC_SGL_DESCR_TYPE_KEYED_DATA_BLOCK
	XNVME_SPEC_SGL_DESCR_TYPE_VENDOR_SPECIFIC	= 0xf, ///< XNVME_SPEC_SGL_DESCR_TYPE_VENDOR_SPECIFIC
};


/**
 * @enum xnvme_spec_sgl_descriptor_subtype
 */
enum xnvme_spec_sgl_descriptor_subtype {
	XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS	= 0x0, ///< XNVME_SPEC_SGL_DESCR_SUBTYPE_ADDRESS
	XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET	= 0x1, ///< XNVME_SPEC_SGL_DESCR_SUBTYPE_OFFSET
};

/**
 * SGL descriptor
 *
 * @struct xnvme_spec_sgl_descriptor
 */
struct xnvme_spec_sgl_descriptor {
	uint64_t addr;				///< common field

	union {
		struct {
			uint64_t rsvd    : 56;
			uint64_t subtype :  4;	///< SGL subtype
			uint64_t type    :  4;	///< SGL type
		} generic;

		struct {
			uint64_t len     : 32;	///< Length of entry
			uint64_t rsvd    : 24;
			uint64_t subtype :  4;	///< SGL subtype
			uint64_t type    :  4;	///< SGL type
		} unkeyed;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_sgl_descriptor) == 16, "Incorrect size")

/**
 * PRP or SGL for Data Transfer field
 *
 * @enum xnvme_spec_psdt
 */
enum xnvme_spec_psdt {
	XNVME_SPEC_PSDT_PRP = 0x0,
	XNVME_SPEC_PSDT_SGL_MPTR_CONTIGUOUS = 0x1,
	XNVME_SPEC_PSDT_SGL_MPTR_SGL = 0x2,
};

/**
 * NVMe Command Accessor for common use of the 64byte NVMe command
 *
 * @see Specification section 4.3, figure 2
 * @see Specification section 4.3, figure 3
 *
 * @struct xnvme_spec_cmd_common
 */
struct xnvme_spec_cmd_common {
	/* cdw 00 */
	uint16_t opcode	:  8;		///< OPC: Command Opcode
	uint16_t fuse	:  2;		///< FUSE: Fused Operation
	uint16_t rsvd	:  4;
	uint16_t psdt	:  2;
	uint16_t cid;			///< CID: Command Identifier

	/* cdw 01 */
	uint32_t nsid;			///< NSID: Namespace identifier

	uint32_t cdw02;
	uint32_t cdw03;

	/* cdw 04-05 */
	uint64_t mptr;			///< MPTR -- metadata pointer

	/* cdw 06-09: */		///< DPTR -- data pointer
	union {
		struct {
			uint64_t prp1;		///< PRP entry 1
			uint64_t prp2;		///< PRP entry 2
		} prp;

		struct xnvme_spec_sgl_descriptor sgl; ///< SGL

		/**
		 * Accessors used by the IOCTL of the Linux Kernel NVMe driver
		 */
		struct {
			uint64_t data;
			uint32_t metadata_len;
			uint32_t data_len;
		} lnx_ioctl;
	} dptr;

	/* cdw 10 */
	uint32_t ndt;	///< NDT: Number of dwords in Data Transfer

	/* cdw 11 */
	uint32_t ndm;	///< NDM: Number of dwords in Meta Transfer

	uint32_t cdw12;
	uint32_t cdw13;
	uint32_t cdw14;
	uint32_t cdw15;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_common) == 64, "Incorrect size")

/**
* NVMe Command Accessor for the Sanitize command
*
* @struct xnvme_spec_cmd_sanitize
*/
struct xnvme_spec_cmd_sanitize {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	uint32_t sanact	: 3;	///< Sanitize Action
	uint32_t ause	: 1;	///< Allow unrestr. San. Exit
	uint32_t owpass	: 4;	///< Overwrite Pass Count
	uint32_t oipbp	: 1;	///< Overwrite Invert
	uint32_t nodas	: 1;	///< NoDeallocate after Sanitize
	uint32_t rsvd	: 22;

	uint32_t ovrpat;	///< Overwrite Pattern

	uint32_t cdw12_15[4];	///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sanitize) == 64, "Incorrect size")

/**
* NVMe Command Accessor for the NVM-format command
*
* @struct xnvme_spec_cmd_format
*/
struct xnvme_spec_cmd_format {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	uint32_t lbaf		: 4;	///< The format to use
	uint32_t mset		: 1;	///< Meta-data settings
	uint32_t pi		: 3;	///< Protection Information
	uint32_t pil		: 1;	///< Protection Information Loc.
	uint32_t ses		: 3;	///< Secure Erase Settings
	uint32_t zf		: 2;	///< TBD: Zone Format
	uint32_t rsvd		: 18;

	uint32_t cdw11_15[5];		///< Command dword 11 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_format) == 64, "Incorrect size")

/**
* NVMe Command Accessor for the get-features command
*
* @struct xnvme_spec_cmd_gfeat
*/
struct xnvme_spec_cmd_gfeat {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	uint32_t fid		: 8;	///< Feature Identifier
	uint32_t sel		: 3;	///< Select
	uint32_t rsvd10		: 21;

	uint32_t cdw11_15[5];		///< Command dword 11 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_gfeat) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for the set-features command
 *
 * @struct xnvme_spec_cmd_sfeat
 */
struct xnvme_spec_cmd_sfeat {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	uint32_t fid		: 8;	///< Feature Identifier
	uint32_t rsvd10		: 23;
	uint32_t save		: 1;	///< Save

	struct xnvme_spec_feat feat;	///< Feature

	uint32_t cdw12_15[4];		///< Command dword 12 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_sfeat) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for the identify command
 *
 * @struct xnvme_spec_cmd_idfy
 */
struct xnvme_spec_cmd_idfy {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	uint32_t cns		: 8;	///< Controller or Namespace Structure
	uint32_t rsvd1		: 8;
	uint32_t cntid		: 16;	///< Controller Identifier

	uint32_t nvmsetid	: 16;	///< NVM Set Identifier
	uint32_t rsvd2		: 8;
	uint32_t csi		: 8;	///< Command Set Identifier

	uint32_t cdw12_13[2];		///< Command dword 12 to 13

	uint32_t uuid		: 7;	///< UUID index
	uint32_t rsvd3		: 25;

	uint32_t cdw15;			///< Command dword 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_idfy) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for the get-log-page command
 *
 * @struct xnvme_spec_cmd_log
 */
struct xnvme_spec_cmd_log {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	uint32_t lid	: 8;	///< Log Page Identifier
	uint32_t lsp	: 4;		///< Log Specific Field
	uint32_t rsvd10	: 3;
	uint32_t rae	: 1;	///< Retain Async. Event
	uint32_t numdl	: 16;	///< Nr. of DWORDS lower-bits

	uint32_t numdu	: 16;	///< Nr. of DWORDS upper-bits
	uint32_t rsvd11	: 16;

	uint32_t lpol;		///< Log-page offset lower 32bits
	uint32_t lpou;		///< Log-page offset upper 32bits

	uint32_t cdw14_15[2];		///< Command dword 14 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_log) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for the NVM Command Set
 *
 * @struct xnvme_spec_cmd_nvm
 */
struct xnvme_spec_cmd_nvm {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	uint64_t slba;		///< SLBA: Start Logical Block Address

	uint32_t nlb	: 16;	///< NLB: Number of logical blocks
	uint32_t rsvd	:  4;
	uint32_t dtype	:  4;	///< DT: Directive Type
	uint32_t rsvd2	:  2;
	uint32_t prinfo	:  4;	///< PI: Protection Information Field
	uint32_t fua	:  1;	///< FUA: Force unit access
	uint32_t lr	:  1;	///< LR: Limited retry

	uint32_t cdw13_15[3];	///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd_nvm) == 64, "Incorrect size")

struct xnvme_spec_nvm_write_zeroes {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba;		///< Start LBA

	/* cdw 12 */
	uint32_t nlb	: 16;	///< Number of logical blocks, zero-based
	uint32_t rsvd1	: 8;
	uint32_t deac	: 1;	///< Dealocate
	uint32_t prinfo	: 4;	///< Protection Info.
	uint32_t fua	: 1;	///< Force Unit Access
	uint32_t lr	: 1;	///< Limited Retry

	/* cdw 13 */
	uint32_t cdw_13;	///< Command dword 13

	/* cdw 14 */
	uint32_t ilbrt	: 32;	///< Initial Logical Block Reference Tag

	/* cdw 15 */
	uint32_t lbat	: 16;	///< Logical Block Application Tag
	uint32_t lbatm	: 16;	///< logical Block Application Tag Mask
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_write_zeroes) == 64, "Incorrect size")

/**
 * Command-set specific status codes related to Logical Block Namespaces
 *
 * @see Specification Section 6.TBD.1, figure 355
 */
enum xnvme_spec_nvm_cmd_cpl_sc {
	/// Copy Command Specific Status Values
	XNVME_SPEC_NVM_CMD_CPL_SC_WRITE_TO_RONLY	= 0x82,	///< XNVME_SPEC_NVM_CMD_CPL_SC_WRITE_TO_RONLY
};

/**
 * @struct xnvme_spec_nvm_scopy_fmt_zero
 */
struct xnvme_spec_nvm_scopy_fmt_zero {
	uint8_t rsvd0[8];

	uint64_t slba;		///< Start LBA

	uint32_t nlb	: 16;	///< Number of logical blocks, zero-based

	uint32_t rsvd20	: 16;

	uint32_t eilbrt;	///< Expected Initial Logical Block Ref. Tag
	uint32_t elbatm;	///< Expected Logical Block App. Tag Mask
	uint32_t elbat;		///< Expected Logical Block App. Tag
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_scopy_fmt_zero) == 32, "Incorrect size")

/**
 * Structure conversion for support Kernel format
 *
 * @enum xnvme_nvm_scopy_fmt
 */
enum xnvme_nvm_scopy_fmt {
	XNVME_NVM_SCOPY_FMT_ZERO	= 0x1,		///< user provides entries formatted as ::XNVME_NVM_SCOPY_FMT_ZERO
	XNVME_NVM_SCOPY_FMT_SRCLEN	= 0x1 << 8,	///< user provides entries formatted as ::XNVME_NVM_SCOPY_FMT_SRCLEN
};

#define XNVME_SPEC_NVM_SCOPY_NENTRY_MAX 128

/**
 * @see Specification Section 6.TBD.1
 */
struct xnvme_spec_nvm_scopy_source_range {
	struct xnvme_spec_nvm_scopy_fmt_zero entry[XNVME_SPEC_NVM_SCOPY_NENTRY_MAX];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_scopy_source_range) == 4096, "Incorrect size")


/**
 * NVMe Command Accessor for command with opcode XNVME_SPEC_NVM_CMD_OPC_SCOPY
 *
 * @struct xnvme_spec_nvm_cmd_scopy
 */
struct xnvme_spec_nvm_cmd_scopy {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t sdlba;			///< Start Destination LBA

	/* cdw 12 */
	uint32_t nr		: 8;	///< Number of Ranges
	uint32_t df		: 4;	///< Descriptor Format
	uint32_t prinfor	: 4;	///< Protection Info. Field Read
	uint32_t rsvd1		: 4;
	uint32_t dtype		: 4;	///< Directive Type
	uint32_t rsvd2		: 2;
	uint32_t prinfow	: 4;	///< Protection Info. Field Write
	uint32_t fua		: 1;	///< Force Unit Access
	uint32_t lr		: 1;	///< Limited Retry

	/* cdw 13 */
	uint32_t rsvd3		: 16;
	uint32_t dspec		: 16;	///< Directive Specific

	/* cdw 14 */
	uint32_t ilbrt;			///< Initial Logical Block Ref. Tag

	/* cdw 15 */
	uint32_t lbat		: 16;	///< Logical Block App. Tag
	uint32_t lbatm		: 16;	///< Logical Block App. Tag Mask

};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_cmd_scopy) == 64, "Incorrect size")

/**
 * Kernel format structure for scopy
 *
 * @note Intruder alert!T his is not defined in the NVMe specification. Yet, it is provided here as though it was.
 * It is intensionally done so to provide a format within the framework provided by NVMe.
 *
 * @struct xnvme_spec_nvm_cmd_scopy_fmt_srclen
 */
struct xnvme_spec_nvm_cmd_scopy_fmt_srclen {
	uint64_t  start;
	uint64_t  len;
};

/**
 * NVMe Command Accessors for the NVM Command Set
 *
 * @struct xnvme_spec_nvm_cmd
 */
struct xnvme_spec_nvm_cmd {
	union {
		struct xnvme_spec_nvm_cmd_scopy scopy;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_cmd) == 64, "Incorrect size")

struct xnvme_spec_nvm_idfy_ctrlr {
	uint8_t byte0_519[520];

	/** optional nvm command support */
	union {
		struct {
			uint16_t compare		: 1;
			uint16_t write_unc		: 1;
			uint16_t dsm			: 1;
			uint16_t write_zeroes		: 1;
			uint16_t set_features_save	: 1;
			uint16_t reservations		: 1;
			uint16_t timestamp		: 1;
			uint16_t verify			: 1;
			uint16_t copy			: 1;
			uint16_t reserved		: 7;
		};
		uint16_t val;
	} oncs;

	uint8_t byte522_533[12];

	union {
		struct {
			uint16_t copy_fmt0	: 1;
			uint16_t rsvd		: 15;
		};
		uint16_t val;
	} ocfs;	///< Optional Copy Format Supported

	uint8_t byte536_4095[3559];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ctrlr) == 4096, "Incorrect size")


/**
 * @todo Document this
 */
struct xnvme_spec_nvm_idfy_ns {
	uint8_t byte0_73[74];

	uint16_t mssrl;	///< Maximum Single Source Range Length
	uint32_t mcl;	///< Maximum Copy Length
	uint8_t msrc;	///< Maximum Source Range Count

	uint8_t byte81_4095[4014];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy_ns) == 4096, "Incorrect size")

/**
 * Representation of the NVMe Identify Namespace command completion result
 *
 * @struct xnvme_spec_znd_idfy
 */
struct xnvme_spec_nvm_idfy {
	union {
		struct xnvme_spec_idfy base;
		struct xnvme_spec_nvm_idfy_ctrlr ctrlr;
		struct xnvme_spec_nvm_idfy_ns ns;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_nvm_idfy) == 4096, "Incorrect size")

/**
 * Log identifiers for Zoned Namespaces
 *
 * TODO: merge this with the set of log-identifiers
 *
 * @enum xnvme_spec_znd_log_lid
 */
enum xnvme_spec_znd_log_lid {
	XNVME_SPEC_LOG_ZND_CHANGES = 0xBF,	///< ZND_CMD_LOG_CHANGES
};

/**
 * Zoned Command Set opcodes
 *
 * @see Specification Section 6, figure ZONEDOPCODES
 *
 * @struct xnvme_spec_znd_opc
 */
enum xnvme_spec_znd_opc {
	XNVME_SPEC_ZND_OPC_MGMT_SEND	= 0x79,	///< XNVME_SPEC_ZND_OPC_MGMT_SEND
	XNVME_SPEC_ZND_OPC_MGMT_RECV	= 0x7A,	///< XNVME_SPEC_ZND_OPC_MGMT_RECV
	XNVME_SPEC_ZND_OPC_APPEND	= 0x7D,	///< XNVME_SPEC_ZND_OPC_APPEND
};

/**
 * NVMe Command Accessor for command with opcode ZND_CMD_OPC_MGMT_SEND
 *
 * @struct xnvme_spec_znd_cmd_mgmt_send
 */
struct xnvme_spec_znd_cmd_mgmt_send {
	uint32_t cdw00_09[10];		///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba;			///< Start LBA

	uint32_t cdw12;

	/* cdw 13 */
	uint32_t zsa		: 8;	///< Zone Send Action
	uint32_t zsasf		: 1;	///< Select All
	uint32_t rsvd		: 23;

	uint32_t cdw14_15[2];		///< Command dword 14 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_send) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for a command with opcode XNVME_SPEC_ZND_OPC_MGMT_RECV
 *
 * @see enum xnvme_spec_znd_cmd_mgmt_recv
 * @struct xnvme_spec_znd_cmd_mgmt_recv
 */
struct xnvme_spec_znd_cmd_mgmt_recv {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	/* cdw 10-11 */
	uint64_t slba;		///< Start LBA

	/* cdw 12 */
	uint32_t ndwords;	///< Number of dwords in data-payload

	/* cdw 13 */
	uint32_t zra		: 8;	///< Zone Receive Action
	uint32_t zrasf		: 8;	///< Zone Receive Action Specific Field
	uint32_t partial	: 1;	///< Partial

	uint32_t rsvd		: 15;

	/* cdw 14-15 */
	uint64_t addrs_dst;		///< destination addresses
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_mgmt_recv) == 64, "Incorrect size")

/**
 * NVMe Command Accessor for a command with opcode ZND_CMD_OPC_APPEND
 *
 * @struct xnvme_spec_znd_cmd_append
 */
struct xnvme_spec_znd_cmd_append {
	uint32_t cdw00_09[10];	///< Command dword 0 to 9

	uint64_t zslba;		///< SLBA: Start Logical Block Address

	uint32_t nlb	: 16;	///< NLB: Number of logical blocks
	uint32_t rsvd	:  4;
	uint32_t dtype	:  4;	///< DT: Directive Type
	uint32_t prinfo	:  4;	///< PI: Protection Information Field
	uint32_t rsvd2	:  2;
	uint32_t fua	:  1;	///< FUA: Force unit access
	uint32_t lr	:  1;	///< LR: Limited retry

	uint32_t cdw13_15[3];	///< Command dword 13 to 15
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd_append) == 64, "Incorrect size")

/**
 * NVMe Command Accessors for the Zoned Command Set
 *
 * @struct xnvme_spec_znd_cmd
 */
struct xnvme_spec_znd_cmd {
	union {
		struct xnvme_spec_znd_cmd_mgmt_send mgmt_send;
		struct xnvme_spec_znd_cmd_mgmt_recv mgmt_recv;
		struct xnvme_spec_znd_cmd_append append;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_cmd) == 64, "Incorrect size")

/**
 * NVMe Command Accessors
 *
 * @struct xnvme_spec_cmd
 */
struct xnvme_spec_cmd {
	union {
		struct xnvme_spec_cmd_common common;
		struct xnvme_spec_cmd_sanitize sanitize;
		struct xnvme_spec_cmd_format format;
		struct xnvme_spec_cmd_log log;
		struct xnvme_spec_cmd_gfeat gfeat;
		struct xnvme_spec_cmd_sfeat sfeat;
		struct xnvme_spec_cmd_idfy idfy;
		struct xnvme_spec_cmd_nvm nvm;
		struct xnvme_spec_nvm_cmd_scopy scopy;
		struct xnvme_spec_nvm_write_zeroes write_zeroes;
		struct xnvme_spec_znd_cmd znd;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_cmd) == 64, "Incorrect size")

/**
 * Command-set specific status codes related to Zoned Namespaces
 *
 * @see Specification Section 4.1.1.1.1, figures GENERICSTATUSCODES and
 * STATUSCODES
 *
 * @enum xnvme_spec_znd_status_code
 */
enum xnvme_spec_znd_status_code {
	// Namespace Management
	XNVME_SPEC_ZND_SC_INVALID_FORMAT	= 0x7F,	///< ZND_SC_INVALID_FORMAT

	/// Zoned Command Set
	XNVME_SPEC_ZND_SC_BOUNDARY_ERROR	= 0xB8,	///< ZND_SC_BOUNDARY_ERROR
	XNVME_SPEC_ZND_SC_IS_FULL		= 0xB9,	///< ZND_SC_IS_FULL
	XNVME_SPEC_ZND_SC_IS_READONLY		= 0xBA,	///< ZND_SC_IS_READONLY
	XNVME_SPEC_ZND_SC_IS_OFFLINE		= 0xBB,	///< ZND_SC_IS_OFFLINE
	XNVME_SPEC_ZND_SC_INVALID_WRITE		= 0xBC,	///< ZND_SC_INVALID_WRITE
	XNVME_SPEC_ZND_SC_TOO_MANY_ACTIVE	= 0xBD,	///< ZND_SC_TOO_MANY_ACTIVE
	XNVME_SPEC_ZND_SC_TOO_MANY_OPEN		= 0xBE,	///< ZND_SC_TOO_MANY_OPEN
	XNVME_SPEC_ZND_SC_INVALID_TRANS		= 0xBF,	///< ZND_SC_INVALID_TRANS
};

/**
 * This is not defined in any spec... it would just seem sane if it was...
 *
 * @enum xnvme_spec_znd_mgmt_send_action_sf
 */
enum xnvme_spec_znd_mgmt_send_action_sf {
	XNVME_SPEC_ZND_MGMT_SEND_ASF_SALL	= 0x1,	///< XNVME_SPEC_ZND_MGMT_SEND_ASF_SALL
};

/**
 * Actions for the Zone Management Send command
 *
 * @see Specification Section 6.1, figure TBDZMDW13
 *
 * @enum xnvme_spec_znd_cmd_mgmt_send_action
 */
enum xnvme_spec_znd_cmd_mgmt_send_action {
	XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE	= 0x1,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_CLOSE
	XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH	= 0x2,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_FINISH
	XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN	= 0x3,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_OPEN
	XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET	= 0x4,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_RESET
	XNVME_SPEC_ZND_CMD_MGMT_SEND_OFFLINE	= 0x5,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_OFFLINE
	XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR	= 0x10,	///< XNVME_SPEC_ZND_CMD_MGMT_SEND_DESCRIPTOR
};

/**
 * Zone Receive Action (::xnvme_spec_znd_cmd_mgmt_recv_action) specific field values
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 *
 * @enum xnvme_spec_znd_cmd_mgmt_recv_action_sf
 */
enum xnvme_spec_znd_cmd_mgmt_recv_action_sf {
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL	= 0x0,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_ALL
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY	= 0x1,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EMPTY
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN	= 0x2,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_IOPEN
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN	= 0x3,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_EOPEN
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED	= 0x4,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_CLOSED
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL	= 0x5,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_FULL
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY	= 0x6,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_RONLY
	XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE	= 0x7,	///< XNVME_SPEC_ZND_CMD_MGMT_RECV_SF_OFFLINE
};

/**
 * Zoned Receive Action
 *
 * @see Specification Section 6.2, figure TBDZMRDW13
 * @enum xnvme_spec_znd_cmd_mgmt_recv_action
 */
enum xnvme_spec_znd_cmd_mgmt_recv_action {
	///< XNvME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT
	XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT		= 0x0,
	///< XNvME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED
	XNVME_SPEC_ZND_CMD_MGMT_RECV_ACTION_REPORT_EXTENDED	= 0x1,
};

/**
 * Zone Type
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRD
 * @enum xnvme_spec_znd_type
 */
enum xnvme_spec_znd_type {
	XNVME_SPEC_ZND_TYPE_SEQWR = 0x2,	///< XNVME_SPEC_ZND_TYPE_SEQWR
};

/**
 * Zone State as reported by Zone Management Receive
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 * @enum xnvme_spec_znd_state
 */
enum xnvme_spec_znd_state {
	XNVME_SPEC_ZND_STATE_EMPTY		= 0x1,	///< XNVME_SPEC_ZND_STATE_EMPTY
	XNVME_SPEC_ZND_STATE_IOPEN		= 0x2,	///< XNVME_SPEC_ZND_STATE_IOPEN
	XNVME_SPEC_ZND_STATE_EOPEN		= 0x3,	///< XNVME_SPEC_ZND_STATE_EOPEN
	XNVME_SPEC_ZND_STATE_CLOSED	= 0x4,	///< XNVME_SPEC_ZND_STATE_CLOSED
	XNVME_SPEC_ZND_STATE_RONLY		= 0xD,	///< XNVME_SPEC_ZND_STATE_RONLY
	XNVME_SPEC_ZND_STATE_FULL		= 0xE,	///< XNVME_SPEC_ZND_STATE_FULL
	XNVME_SPEC_ZND_STATE_OFFLINE	= 0xF,	///< XNVME_SPEC_ZND_STATE_OFFLINE
};

/**
 * Identify controller accessor only for Zoned specific fields
 *
 * @see Specification, section 3.1.2
 * @struct xnvme_spec_znd_idfy_ctrlr
 */
struct xnvme_spec_znd_idfy_ctrlr {
	uint8_t zasl;		///< Zone Append Size Limit

	uint8_t rsvd8[4095];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ctrlr) == 4096, "Incorrect size")

/**
 * LBA Format Extension
 *
 * @see Specification Section 5.1.2, figure LBAFE
 *
 * @struct xnvme_spec_znd_idfy_lbafe
 */
struct xnvme_spec_znd_idfy_lbafe {
	uint64_t zsze;		///< Zone Size in number of logical blocks
	uint8_t zdes;		///< Zone Descriptor Extensions Size

	uint8_t rsvd[7];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_lbafe) == 16, "Incorrect size")

/**
 * Zoned Command Set specific identify namespace data structure
 *
 * @see TP4053, section 3
 * @struct xnvme_spec_znd_idfy_ns
 */
struct xnvme_spec_znd_idfy_ns {
	struct {
		uint16_t vzcap	: 1;	///< Variable Zone Capacity
		uint16_t zae	: 1;	///< Zone Active Excursions

		uint16_t rsvd	: 14;
	} zoc; ///< Zone Operation Characteristics

	struct {
		uint16_t razb	: 1;	///< Read Across zone boundaries

		uint16_t rsvd	: 15;
	} ozcs; ///< Optional Zoned Command Support

	uint32_t mar;		///< Maximum Active Resources
	uint32_t mor;		///< Maximum Open Resources

	uint32_t rrl;		///< Reset Recommended Limit
	uint32_t frl;		///< Finish Recommended Limit

	uint8_t rsvd20_2815[2796];

	struct xnvme_spec_znd_idfy_lbafe lbafe[16];

	uint8_t rsvd3072_3839[768];

	uint8_t vs[256];	///< Vendor Specific
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy_ns) == 4096, "Incorrect size")

/**
 * Representation of the NVMe Identify Namespace command completion result
 *
 * @todo merge this into the libxnvme_spec.h providing a one-stop shop for idfy-representation
 * @struct xnvme_spec_znd_idfy
 */
struct xnvme_spec_znd_idfy {
	union {
		struct xnvme_spec_idfy base;
		struct xnvme_spec_znd_idfy_ctrlr zctrlr;
		struct xnvme_spec_znd_idfy_ns zns;
	};
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_idfy) == 4096, "Incorrect size")

#define ZND_CHANGES_LEN 511

/**
 * Zone Identifier List as returned by get-log-page
 *
 * @see Specification Section 5.3.1.1, figure TBDZONELIST
 *
 * @struct xnvme_spec_znd_log_changes
 */
struct xnvme_spec_znd_log_changes {
	uint16_t nidents;			///< Number of Zone Identifiers
	uint8_t rsvd2[6];
	uint64_t idents[ZND_CHANGES_LEN];	///< Zone Identifiers
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_log_changes) == 4096, "Incorrect size")

/**
 * Zone Descriptor as reported by Zone Management Receive command
 *
 * @see Specification Section 6.2.2.3, figure TBDZMRZD
 *
 * @struct xnvme_spec_znd_descr
 */
struct xnvme_spec_znd_descr {
	uint8_t zt		: 4;	///< Zone Type
	uint8_t rsvd0		: 4;

	uint8_t rsvd1		: 4;
	uint8_t zs		: 4;	///< Zone State

	/**
	 * Zone Attributes
	 */
	union {
		struct {
			uint8_t zfc: 1;		///< Zone Finished by controller
			uint8_t zfr: 1;		///< Zone Finish Recommended
			uint8_t rzr: 1;		///< Reset Zone Recommended

			uint8_t rsvd3 : 4;

			uint8_t zdev: 1;	///< Zone Descriptor Valid
		};
		uint8_t val;
	} za;

	uint8_t rsvd7[5];

	uint64_t zcap;			///< Zone Capacity (in number of LBAs)
	uint64_t zslba;			///< Zone Start LBA
	uint64_t wp;			///< Write Pointer
	uint8_t rsvd63[32];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_descr) == 64, "Incorrect size")

/**
 * Report Zones Data Structure header
 *
 * The first 16 bytes of the two Report Zoned data structures
 *
 * @see Specification Section 6.2.2.1, figure TBDZMRRZ
 * @see Specification Section 6.2.2.2, figure TBDZMRERZ
 * @struct xnvme_spec_znd_report_hdr
 */
struct xnvme_spec_znd_report_hdr {
	/**
	 * Number of zones in namespace when receiving with partial=0
	 * Number of zones in the data-buffer, when receiving with partial=1
	 */
	uint64_t nzones;
	uint8_t rsvd[56];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_spec_znd_report_hdr) == 64, "Incorrect size")

/**
 * Prints the given :;xnvme_spec_log_health_entry to the given output stream
 *
 * @param stream output stream used for printing
 * @param log pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_fpr(FILE *stream,
			  const struct xnvme_spec_log_health_entry *log,
			  int opts);

/**
 * Prints the given :;xnvme_spec_log_health_entry to stdout
 *
 * @param log
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_health_pr(const struct xnvme_spec_log_health_entry *log,
			 int opts);

/**
 * Prints the given :;xnvme_spec_log_erri_entry to stdout
 *
 * @param stream output stream used for printing
 * @param log
 * @param limit
 * @param opts
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_fpr(FILE *stream,
			const struct xnvme_spec_log_erri_entry *log, int limit,
			int opts);

/**
 * Prints the given :;xnvme_spec_log_erri_entry to stdout
 *
 * @param log
 * @param limit
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_log_erri_pr(const struct xnvme_spec_log_erri_entry *log, int limit,
		       int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ns to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_fpr(FILE *stream, const struct xnvme_spec_idfy_ns *idfy,
		       int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ns to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ns_pr(const struct xnvme_spec_idfy_ns *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ctrlr to the given output stream
 *
 * @param stream output stream used for printing
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrl_fpr(FILE *stream, const struct xnvme_spec_idfy_ctrlr *idfy,
			 int opts);

/**
 * Prints the given :;xnvme_spec_idfy_ctrlr to stdout
 *
 * @param idfy pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_idfy_ctrl_pr(const struct xnvme_spec_idfy_ctrlr *idfy, int opts);

int
xnvme_spec_idfy_cs_fpr(FILE *stream, const struct xnvme_spec_idfy_cs *idfy,
		       int opts);

int
xnvme_spec_idfy_cs_pr(const struct xnvme_spec_idfy_cs *idfy, int opts);

/**
 * Prints the given :;xnvme_spec_feat to the given output stream
 *
 * @param stream output stream used for printing
 * @param fid feature identifier
 * @param feat feature values
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_fpr(FILE *stream, uint8_t fid, struct xnvme_spec_feat feat,
		    int opts);

/**
 * Prints the given :;xnvme_spec_feat to the given output stream
 *
 * @param fid feature identifier
 * @param feat feature values
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_feat_pr(uint8_t fid, struct xnvme_spec_feat feat, int opts);

/**
 * Prints the given :;xnvme_spec_cmd to the given output stream
 *
 * @param stream output stream used for printing
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_cmd_fpr(FILE *stream, struct xnvme_spec_cmd *cmd, int opts);

/**
 * Prints the given :;xnvme_spec_cmd to stdout
 *
 * @param cmd pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_cmd_pr(struct xnvme_spec_cmd *cmd, int opts);

int
xnvme_spec_nvm_scopy_fmt_zero_fpr(FILE *stream,
				  const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
				  int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_fmt_zero to stdout
 *
 * @param entry pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_fmt_zero_pr(const struct xnvme_spec_nvm_scopy_fmt_zero *entry,
				 int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_source_range to the given output stream
 *
 * @param stream output stream used for printing
 * @param srange pointer to structure to print
 * @param nr zero-based number of entries, at most ::XNVME_SPEC_NVM_SCOPY_NENTRY_MAX -1
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_fpr(FILE *stream,
				      const struct xnvme_spec_nvm_scopy_source_range *srange,
				      uint8_t nr, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_scopy_source_range to stdout
 *
 * @param srange pointer to structure to print
 * @param nr nr=0: print ::XNVME_SPEC_NVM_SCOPY_NENTRY_MAX entries from the given , nr>0:
 * print no more than nr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_scopy_source_range_pr(const struct xnvme_spec_nvm_scopy_source_range *srange,
				     uint8_t nr,
				     int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Logical Block Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ctrlr to stdout
 *
 * @param idfy pointer to ::xnvme_spec_nvm_idfy_ctrlr
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ctrlr_pr(struct xnvme_spec_nvm_idfy_ctrlr *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ns to the given output stream
 *
 * Only fields specific to Logical Block Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param idfy pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ns_fpr(FILE *stream, struct xnvme_spec_nvm_idfy_ns *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_nvm_idfy_ns to stdout
 *
 * @param idfy pointer to ::xnvme_spec_nvm_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_nvm_idfy_ns_pr(struct xnvme_spec_nvm_idfy_ns *idfy, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ctrlr_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ctrlr to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param zctrlr pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ctrlr_pr(struct xnvme_spec_znd_idfy_ctrlr *zctrlr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_lbafe to the given output stream
 *
 * @param stream output stream used for printing
 * @param zonef pointer to structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_lbafe_fpr(FILE *stream, struct xnvme_spec_znd_idfy_lbafe *zonef, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ns to the given output stream
 *
 * Only fields specific to Zoned Namespaces are printed by this function
 *
 * @param stream output stream used for printing
 * @param zns pointer to the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ns_fpr(FILE *stream, struct xnvme_spec_znd_idfy_ns *zns, int opts);

/**
 * Prints the given ::xnvme_spec_znd_idfy_ns to stdout
 *
 * @param zns pointer to ::xnvme_spec_znd_idfy_ns
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned
 */
int
xnvme_spec_znd_idfy_ns_pr(struct xnvme_spec_znd_idfy_ns *zns, int opts);

/**
 * Prints the given ::xnvme_spec_znd_log_changes to the given output stream
 *
 * @param stream output stream used for printing
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_fpr(FILE *stream, const struct xnvme_spec_znd_log_changes *changes,
			       int opts);

/**
 * Pretty-printer of ::xnvme_spec_znd_log_changes
 *
 * @param changes pointer to subject to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_log_changes_pr(const struct xnvme_spec_znd_log_changes *changes, int opts);

/**
 * Prints the given ::xnvme_spec_znd_descr to the given stream
 *
 * @param stream output stream used for printing
 * @param descr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_fpr(FILE *stream, const struct xnvme_spec_znd_descr *descr, int opts);

/**
 * Prints the given ::xnvme_spec_znd_descr to stdout
 *
 * @param descr the structure to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_descr_pr(const struct xnvme_spec_znd_descr *descr, int opts);

/**
 * Print the given ::xnvme_spec_znd_report_hdr to the given output stream
 *
 * @param stream output stream used for printing
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_fpr(FILE *stream, const struct xnvme_spec_znd_report_hdr *hdr, int opts);

/**
 * Print the given ::xnvme_spec_znd_report_hdr to the stdout
 *
 * @param hdr the struct to print
 * @param opts printer options, see ::xnvme_pr
 *
 * @return On success, the number of characters printed is returned.
 */
int
xnvme_spec_znd_report_hdr_pr(const struct xnvme_spec_znd_report_hdr *hdr, int opts);

int
xnvme_spec_znd_descr_fpr_yaml(FILE *stream, const struct xnvme_spec_znd_descr *descr, int indent,
			      const char *sep);

#endif /* __LIBXNVME_SPEC_H */

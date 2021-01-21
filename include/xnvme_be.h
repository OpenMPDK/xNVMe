// Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
// SPDX-License-Identifier: Apache-2.0
#ifndef __INTERNAL_XNVME_BE_H
#define __INTERNAL_XNVME_BE_H
#include <paths.h>
#include <libxnvme_pp.h>
#include <xnvme_be_registry.h>
#include <stdbool.h>

#define XNVME_FREEBSD_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_FREEBSD_NS_SCAN _PATH_DEV "nvme%1uns%1u%[^\n]"

#define XNVME_FREEBSD_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_FREEBSD_NS_FMT _PATH_DEV "nvme%1uns%1u"

#define XNVME_LINUX_CTRLR_SCAN _PATH_DEV "nvme%1u%[^\n]"
#define XNVME_LINUX_NS_SCAN _PATH_DEV "nvme%1un%1u%[^\n]"

#define XNVME_LINUX_CTRLR_FMT _PATH_DEV "nvme%1u"
#define XNVME_LINUX_NS_FMT _PATH_DEV "nvme%1un%1u"

#define XNVME_BE_QUEUE_STATE_NBYTES 256

#define XNVME_BE_ASYNC_NBYTES 64
#define XNVME_BE_SYNC_NBYTES 40
#define XNVME_BE_DEV_NBYTES 24
#define XNVME_BE_MEM_NBYTES 32
#define XNVME_BE_ATTR_NBYTES 24
#define XNVME_BE_STATE_NBYTES 128
#define XNVME_BE_NBYTES \
	( XNVME_BE_ASYNC_NBYTES + XNVME_BE_SYNC_NBYTES + XNVME_BE_DEV_NBYTES + XNVME_BE_MEM_NBYTES + XNVME_BE_ATTR_NBYTES + XNVME_BE_STATE_NBYTES )

struct xnvme_be_async {
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	int (*poke)(struct xnvme_queue *, uint32_t);

	int (*wait)(struct xnvme_queue *);

	int (*init)(struct xnvme_queue *, int opts);

	int (*term)(struct xnvme_queue *);

	int (*supported)(struct xnvme_dev *, uint32_t);

	const char *id;

	uint64_t enabled;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_async) == XNVME_BE_ASYNC_NBYTES, "Incorrect size")

struct xnvme_be_sync {
	/**
	 * Pass a NVMe I/O Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	/**
	 * Pass a NVMe Admin Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_admin)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	int (*supported)(struct xnvme_dev *, uint32_t);

	const char *id;
	uint64_t enabled;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_sync) == XNVME_BE_SYNC_NBYTES, "Incorrect size")

struct xnvme_be_dev {
	/**
	 * Enumerate devices on/at the given 'sys_uri' when NULL local devices
	 */
	int (*enumerate)(struct xnvme_enumeration *, const char *, int);

	/**
	 * Construct a device from the given identifier
	 */
	int (*dev_from_ident)(const struct xnvme_ident *, struct xnvme_dev **);

	/**
	 * Close the given device
	 */
	void (*dev_close)(struct xnvme_dev *);
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_dev) == XNVME_BE_DEV_NBYTES, "Incorrect size")

struct xnvme_be_mem {
	/**
	 * Allocate a buffer usable for NVMe commands
	 */
	void *(*buf_alloc)(const struct xnvme_dev *, size_t, uint64_t *);

	/**
	 * Resolve the physical address of the given buffer
	 */
	int (*buf_vtophys)(const struct xnvme_dev *, void *, uint64_t *);

	/**
	 * Re-allocate a buffer usable for NVMe commands
	 */
	void *(*buf_realloc)(const struct xnvme_dev *, void *, size_t,
			     uint64_t *);

	/**
	 * Free a buffer usable for NVMe commands
	 */
	void (*buf_free)(const struct xnvme_dev *, void *);
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_mem) == XNVME_BE_MEM_NBYTES, "Incorrect size")

/**
 * Backend function-interface
 */

/**
 * Backend interface consisting of functions, attributes and instance state
 */
struct xnvme_be {
	struct xnvme_be_async async;		///< Asynchronous commands
	struct xnvme_be_sync sync;		///< Synchronous commands
	struct xnvme_be_dev dev;		///< Device functions
	struct xnvme_be_attr attr;		///< Attributes
	struct xnvme_be_mem mem;		///< Memory management
	uint8_t state[XNVME_BE_STATE_NBYTES];	///< Instance state
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be) == XNVME_BE_NBYTES, "Incorrect size")

/**
 * Auxilary helpers
 */

/**
 * Resolve the name of the given backend identifier
 */
const char *
xnvme_be_id2name(int bid);

/**
 * Resolve the internal identifier of the given backend name.
 *
 * @returns Returns numerical identifier on success. On error, e.g. the given
 * name does not exists in the list of backend implementations, -1 is returned.
 */
int
xnvme_be_name2id(const char *bname);

/**
 * Produce a device with a backend attached
 */
int
xnvme_be_factory(const char *uri, struct xnvme_dev **dev);

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent, const char *sep, int head);

int
xnvme_be_fpr(FILE *stream, const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_pr(const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev);

int
uri_parse_scheme(const char *uri, char *scheme);

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head);

int
xnvme_enumeration_alloc(struct xnvme_enumeration **list, uint32_t capacity);

void
xnvme_enumeration_free(struct xnvme_enumeration *list);

int
xnvme_enumeration_append(struct xnvme_enumeration *list, struct xnvme_ident *entry);

bool
has_scheme(const char *needle, const char *haystack[], int len);

int
path_to_ll(const char *path, uint64_t *val);

bool
xnvme_ident_opt_to_val(const struct xnvme_ident *ident, const char *opt, uint32_t *val);

bool
xnvme_ident_opt_to_char_val(const struct xnvme_ident *ident, const char *opt, const char *val);

bool
check_cmask_validity(const char *cmask, int nproc);
#endif /* __INTERNAL_XNVME_BE_H */

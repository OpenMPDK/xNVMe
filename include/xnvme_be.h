// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_H
#define __INTERNAL_XNVME_BE_H
#ifndef WIN32
#include <paths.h>
#endif
#include <libxnvme.h>
#include <xnvme_be_registry.h>

#define XNVME_BE_QUEUE_STATE_NBYTES 256

#define XNVME_BE_ASYNC_NBYTES  56
#define XNVME_BE_SYNC_NBYTES   24
#define XNVME_BE_ADMIN_NBYTES  24
#define XNVME_BE_DEV_NBYTES    32
#define XNVME_BE_MEM_NBYTES    56
#define XNVME_BE_ATTR_NBYTES   24
#define XNVME_BE_STATE_NBYTES  128
#define XNVME_BE_MIXINS_NBYTES 16
#define XNVME_BE_NBYTES                                                         \
	(XNVME_BE_ASYNC_NBYTES + XNVME_BE_SYNC_NBYTES + XNVME_BE_ADMIN_NBYTES + \
	 XNVME_BE_DEV_NBYTES + XNVME_BE_MEM_NBYTES + XNVME_BE_ATTR_NBYTES +     \
	 XNVME_BE_STATE_NBYTES + XNVME_BE_MIXINS_NBYTES)

XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_attr) == XNVME_BE_ATTR_NBYTES, "Incorrect size");

struct xnvme_be_async {
	// Submit an async io command to be processed on the backend's io path
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	// Submit a vectored async io command to be processed on the backend's io path
	int (*cmd_iov)(struct xnvme_cmd_ctx *, struct iovec *, size_t, size_t, struct iovec *,
		       size_t, size_t);

	// Non-blocking reaping of up to `max` io completions
	int (*poke)(struct xnvme_queue *, uint32_t);

	// Blocking reaping of all outstanding io completions
	int (*wait)(struct xnvme_queue *);

	// Do initialization of the underlying backend's io path
	int (*init)(struct xnvme_queue *, int);

	// Close resources allocated for the underlying backend's io path
	int (*term)(struct xnvme_queue *);

	// Check if the backend is supported in the current environment
	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_async) == XNVME_BE_ASYNC_NBYTES, "Incorrect size")

struct xnvme_be_sync {
	/**
	 * Pass a NVMe I/O Command Through to the device with minimal driver
	 * intervention
	 */
	int (*cmd_io)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	/**
	 * Pass a vectored NVMe I/O Command Through to the device with minimal
	 * driver intervention
	 */
	int (*cmd_iov)(struct xnvme_cmd_ctx *, struct iovec *, size_t, size_t, struct iovec *,
		       size_t, size_t);

	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_sync) == XNVME_BE_SYNC_NBYTES, "Incorrect size")

struct xnvme_be_admin {
	/**
	 * Pass a NVMe Admin Command Through to the device with minimal driver intervention
	 */
	int (*cmd_admin)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	/**
	 * Pass pseudo admin-commands
	 *
	 * That is representation of various operations in NVMe commands, that look
	 * like but are not actual NVMe commands.
	 */
	int (*cmd_pseudo)(struct xnvme_cmd_ctx *, void *, size_t, void *, size_t);

	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_admin) == XNVME_BE_ADMIN_NBYTES, "Incorrect size")

struct xnvme_be_dev {
	/**
	 * enumerate devices on/at the given 'sys_uri' when NULL local devices
	 */
	int (*enumerate)(const char *, struct xnvme_opts *, xnvme_enumerate_cb, void *);

	/**
	 * Construct a device from the given identifier
	 */
	int (*dev_open)(struct xnvme_dev *);

	/**
	 * Close the given device
	 */
	void (*dev_close)(struct xnvme_dev *);

	const char *id;
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
	void *(*buf_realloc)(const struct xnvme_dev *, void *, size_t, uint64_t *);

	/**
	 * Free a buffer usable for NVMe commands
	 */
	void (*buf_free)(const struct xnvme_dev *, void *);

	/**
	 * Map a buffer usable for NVMe commands
	 */
	int (*mem_map)(const struct xnvme_dev *, void *, size_t, uint64_t *);

	/**
	 * Unmap a buffer usable for NVMe commands
	 */
	int (*mem_unmap)(const struct xnvme_dev *, void *);
	const char *id;
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_mem) == XNVME_BE_MEM_NBYTES, "Incorrect size")

/**
 * Backend function-interface
 */
enum xnvme_be_mixin_type {
	XNVME_BE_MEM   = 0x1 << 0,
	XNVME_BE_ADMIN = 0x1 << 1,
	XNVME_BE_SYNC  = 0x1 << 2,
	XNVME_BE_ASYNC = 0x1 << 3,
	XNVME_BE_DEV   = 0x1 << 4,
	XNVME_BE_ATTR  = 0x1 << 5,
	XNVME_BE_END   = 0x1 << 6
};
#define XNVME_BE_CONFIGURED \
	(XNVME_BE_ADMIN | XNVME_BE_DEV | XNVME_BE_MEM | XNVME_BE_SYNC | XNVME_BE_ASYNC)

static inline const char *
xnvme_be_mixin_key(enum xnvme_be_mixin_type mtype)
{
	switch (mtype) {
	case XNVME_BE_ASYNC:
		return "async";
	case XNVME_BE_SYNC:
		return "sync";
	case XNVME_BE_ADMIN:
		return "admin";
	case XNVME_BE_DEV:
		return "dev";
	case XNVME_BE_ATTR:
		return "attr";
	case XNVME_BE_MEM:
		return "mem";
	case XNVME_BE_END:
		return "end";
	}

	return "enosys";
}

struct xnvme_be_mixin {
	enum xnvme_be_mixin_type mtype;
	const char *name;
	const char *descr;
	union {
		struct xnvme_be_async *async;
		struct xnvme_be_sync *sync;
		struct xnvme_be_admin *admin;
		struct xnvme_be_mem *mem;
		struct xnvme_be_dev *dev;
		void *obj;
	};
	int (*check_support)(struct xnvme_dev *, uint32_t);
};

#define XNVME_BE_MIXIN_NAME_LEN 32

/**
 * Backend interface consisting of functions, attributes and instance state
 */
struct xnvme_be {
	struct xnvme_be_async async;          ///< Asynchronous I/O command interface
	struct xnvme_be_sync sync;            ///< Synchronous I/O command interface
	struct xnvme_be_admin admin;          ///< Administrative command interface
	struct xnvme_be_dev dev;              ///< Device functions
	struct xnvme_be_attr attr;            ///< Backend Attributes
	struct xnvme_be_mem mem;              ///< Memory Management interface
	uint8_t state[XNVME_BE_STATE_NBYTES]; ///< Backend instance state

	struct xnvme_be_mixin *objs;
	uint64_t nobjs;
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
 * Instantiate a backend instance for the given device
 */
int
xnvme_be_factory(struct xnvme_dev *dev, struct xnvme_opts *opts);

int
xnvme_be_yaml(FILE *stream, const struct xnvme_be *be, int indent, const char *sep, int head);

int
xnvme_be_fpr(FILE *stream, const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_pr(const struct xnvme_be *be, enum xnvme_pr opts);

int
xnvme_be_dev_derive_geometry(struct xnvme_dev *dev);

int
xnvme_be_dev_idfy(struct xnvme_dev *dev);

int
xnvme_ident_yaml(FILE *stream, const struct xnvme_ident *ident, int indent, const char *sep,
		 int head);

static inline int
xnvme_be_supported(struct xnvme_dev *XNVME_UNUSED(dev), uint32_t XNVME_UNUSED(opts))
{
	return 1;
}

#endif /* __INTERNAL_XNVME_BE_H */

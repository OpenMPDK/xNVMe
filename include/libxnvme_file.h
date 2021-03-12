/*
 * The User space library for Non-Volatile Memory NVMe Namespaces based on xNVMe, the
 * Cross-platform libraries and tools for NVMe devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @headerfile libxnvme_nvm.h
 */
#ifndef __LIBXNVME_FILE_H
#define __LIBXNVME_FILE_H

#ifdef __cplusplus
extern "C" {
#endif
#include <sys/stat.h>
#include <libxnvme.h>

/**
 * Flags supported when opening files
 *
 * @enum XNVME_FILE_OFLG
 */
enum XNVME_FILE_OFLG {
	XNVME_FILE_OFLG_RDONLY = 0x1 << 0,
	XNVME_FILE_OFLG_WRONLY = 0x1 << 1,
	XNVME_FILE_OFLG_RDWR   = 0x1 << 2,
	XNVME_FILE_OFLG_CREATE = 0x1 << 3,
	XNVME_FILE_OFLG_DIRECT_ON = 0x1 << 4,
	XNVME_FILE_OFLG_DIRECT_OFF = 0x1 << 5,
};

/**
 * Open the file identified by pathname for I/O operation
 *
 * @param pathname Path to the file
 * @param flags Mode of operation for the given file, one of #XNVME_FILE_OFLG
 *
 * @return On success, an initialized struct xnvme_dev is returned. On error,
 * NULL is returned.
 */
struct xnvme_dev *
xnvme_file_open(const char *pathname, int flags);

/**
 * Close the file encapsulated by the given ::xnvme_dev handle
 *
 * @param fh File-handle as obtained by with ::xnvme_file_open
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_close(struct xnvme_dev *fh);

/**
 * Perform a stateless read to the file or device encapsulated by 'ctx'
 *
 * Retrieve a synchronous context using ::xnvme_cmd_ctx_from_dev and an asynchronous context using
 * ::xnvme_cmd_ctx_from_queue.
 *
 * @note Unlike, the pread() syscall, then this function does not return the number of bytes
 * written. To retrieve that information, then inspect the completion-result: 'ctx->cpl.result'.
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param buf Pointer to data-payload
 * @param count The number of bytes to read
 * @param offset The offset, in bytes, to start reading from
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_pread(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset);

/**
 * Perform a stateless write to the file or device encapsulated by 'ctx'
 *
 * Retrieve a synchronous context using ::xnvme_cmd_ctx_from_dev and an asynchronous context using
 * ::xnvme_cmd_ctx_from_queue
 *
 * @note Unlike, the pwrite() syscall, then this function does not return the number of bytes read.
 * To retrieve that information, then inspect the completion-result: 'ctx->cpl.result'.
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param buf Pointer to data-payload
 * @param count The number of bytes to write
 * @param offset The offset, in bytes, to start writing to
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_pwrite(struct xnvme_cmd_ctx *ctx, void *buf, size_t count, off_t offset);

/**
 * Force a sync of the file or device encapsulated by 'fh', forcing a flush of
 * all OS buffers related to the file to disk
 *
 * Retrieve a synchronous context using ::xnvme_cmd_ctx_from_dev and an asynchronous context using
 * ::xnvme_cmd_ctx_from_queue
 *
 * @param fh File-handle as obtained by with ::xnvme_file_open
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_sync(struct xnvme_dev *fh);

/**
 * Fills the given 'statbuf' with information about a file
 *
 * @param fh File-handle as obtained by with ::xnvme_file_open
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_file_stat(struct xnvme_dev *fh);

/**
 * Returns a synchronous command-context for the given file-handle
 *
 * For asynchronous I/O, see the ::xnvme_queue interface and use xnvme_cmd_ctx_from_queue
 */
struct xnvme_cmd_ctx
xnvme_file_get_cmd_ctx(struct xnvme_dev *fh);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_FILE */

/**
 * Cross-platform I/O library for NVMe based devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme_be.h
 */
#ifndef __LIBXNVME_BUF_H
#define __LIBXNVME_BUF_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_phys_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_phys_alloc(const struct xnvme_dev *dev, size_t nbytes, uint64_t *phys);

/**
 * Free the given buffer of physical memory allocated with xnvme_buf_phys_alloc()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_phys_alloc()
 */
void
xnvme_buf_phys_free(const struct xnvme_dev *dev, void *buf);

/**
 * Re-allocate a buffer of physical memory, aligned for IO with the given device
 *
 * @note
 * nbytes must be greater than zero and a multiple of minimal granularity
 * @note
 * De-allocate the buffer using xnvme_buf_phys_free()
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf The buffer to reallocate
 * @param nbytes The size of the allocated buffer in bytes
 * @param phys A pointer to the variable to hold the physical address of the allocated buffer. If
 * NULL, the physical address is not returned.
 *
 * @return On success, a pointer to the allocated memory is returned. On error, NULL is returned
 * and `errno` set to indicate the error.
 */
void *
xnvme_buf_phys_realloc(const struct xnvme_dev *dev, void *buf, size_t nbytes, uint64_t *phys);

/**
 * Retrieve the physical address of the given buffer
 *
 * The intended use for this function is to provide the physical-address of a buffer-allocation
 * allocated with xnvme_buf_phys_alloc() or xnvme_buf_alloc(), where the 'phys' argument was either
 * not provided.
 *
 * @param dev Device handle obtained with xnvme_dev_open() / xnvme_dev_openf()
 * @param buf Pointer to a buffer allocated with xnvme_buf_alloc()
 * @param phys A pointer to the variable to hold the physical address of the given buffer.
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_buf_vtophys(const struct xnvme_dev *dev, void *buf, uint64_t *phys);

/**
 * Allocate a buffer of virtual memory of the given `alignment` and `nbytes`
 *
 * @note
 * You must use xnvme_buf_virt_free() to de-allocate the buffer
 *
 * @param alignment The alignment in bytes
 * @param nbytes The size of the buffer in bytes
 *
 * @return On success, a pointer to the allocated memory is return. On error, NULL is returned and
 * `errno` set to indicate the error
 */
void *
xnvme_buf_virt_alloc(size_t alignment, size_t nbytes);

/**
 * Free the given virtual memory buffer
 *
 * @param buf Pointer to a buffer allocated with xnvme_buf_virt_alloc()
 */
void
xnvme_buf_virt_free(void *buf);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_BUF_H */

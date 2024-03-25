/**
 * SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * @headerfile libxnvme_nvm.h
 */

#ifndef __LIBXNVME_NVM_H
#define __LIBXNVME_NVM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme.h>

/**
 * Submit, and optionally wait for completion of, a NVMe Read
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start reading from
 * @param nlb The number of LBAs to read. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to data-payload
 * @param mbuf Pointer to meta-payload
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_read(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb, void *dbuf,
	       void *mbuf);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 * @param mbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_write(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		const void *dbuf, const void *mbuf);

/**
 * Submit a write uncorrected command
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The Starting Destination LBA
 * @param nlb Number Of Logical Blocks
 */
int
xnvme_nvm_write_uncorrectable(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba,
			      uint16_t nlb);

/**
 * Submit a write zeroes command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number Of Logical Blocks
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and param ctx.cpl filled with lower-level status codes
 */
int
xnvme_nvm_write_zeroes(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb);

/**
 * Submit, and optionally wait for completion of, a NVMe Write
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param opcode Opcode for the NVMe command
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the write at
 * @param nlb Number of LBAs to be written. NOTE: nlb is a zero-based value
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
void
xnvme_prep_nvm(struct xnvme_cmd_ctx *ctx, uint8_t opcode, uint32_t nsid, uint64_t slba,
	       uint16_t nlb);

/**
 * Submit, and optionally wait for completion of a NVMe Simple-Copy-Command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param sdlba The Starting Destination LBA to start copying to
 * @param ranges Pointer to ranges-buffer, see ::xnvme_spec_nvm_scopy_fmt_zero
 * @param nr Number of ranges in the given ranges-buffer, zero-based value
 * @param copy_fmt For of ranges
 *
 * @return On success, 0 is returned. On error, -1 is returned, `errno` set to
 * indicate and `ctx.cpl` filled with lower-level status codes
 */
int
xnvme_nvm_scopy(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t sdlba,
		struct xnvme_spec_nvm_scopy_fmt_zero *ranges, uint8_t nr,
		enum xnvme_nvm_scopy_fmt copy_fmt);

/**
 * Deallocate or hint at read/write usage of a range.
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param dsm_range A buffer of ranges, each with context attributes, slba and nlb
 * @param nr Number of ranges in the given ranges-buffer, zero-based value
 * @param ad The NVM subsystem may deallocate all provided ranges
 * @param idw The dataset should be optimized for write access as an integral unit
 * @param idr The dataset should be optimized for read access as an integral unit
 *
 * @return On success, 0 is returned. On error, -1 is returned.
 */
int
xnvme_nvm_dsm(struct xnvme_cmd_ctx *ctx, uint32_t nsid, struct xnvme_spec_dsm_range *dsm_range,
	      uint8_t nr, bool ad, bool idw, bool idr);

/**
 * Submit, and wait for completion of a I/O management receive command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param mo Management operation
 * @param mos Management operation specific field
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of the data-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_mgmt_recv(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes);

/**
 * Submit, and wait for completion of a I/O management send command
 *
 * @see xnvme_cmd_opts
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param mo Management operation
 * @param mos Management operation specific field
 * @param dbuf pointer to data-payload
 * @param dbuf_nbytes size of the data-payload in bytes
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_mgmt_send(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint8_t mo, uint16_t mos, void *dbuf,
		    uint32_t dbuf_nbytes);

/**
 * Submit, and optionally wait for completion of, a NVMe Compare
 *
 * @param ctx Pointer to command context (::xnvme_cmd_ctx)
 * @param nsid Namespace Identifier
 * @param slba The LBA to start the compare at
 * @param nlb Number of LBAs to be compared. NOTE: nlb is a zero-based value
 * @param dbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 * @param mbuf Pointer to buffer; Payload as indicated by 'ctx->opts'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_nvm_compare(struct xnvme_cmd_ctx *ctx, uint32_t nsid, uint64_t slba, uint16_t nlb,
		  void *dbuf, void *mbuf);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_NVM */

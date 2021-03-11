/**
 * Cross-platform I/O library for NVMe based devices
 *
 * Copyright (C) Simon A. F. Lund <simon.lund@samsung.com>
 * Copyright (C) Klaus B. A. Jensen <k.jensen@samsung.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file libxnvme_ident.h
 */
#ifndef __LIBXNVME_IDENT
#define __LIBXNVME_IDENT

#ifdef __cplusplus
extern "C" {
#endif

#include <libxnvme_util.h>

#define XNVME_IDENT_URI_LEN 384

#define XNVME_IDENT_SCHM_LEN 5
#define XNVME_IDENT_TRGT_LEN 155
#define XNVME_IDENT_OPTS_LEN 160
#define XNVME_IDENT_OPTS_SEP '?'
#define XNVME_IDENT_OPT_MAX 20

#define XNVME_IDENT_URI_LEN_MIN (XNVME_IDENT_SCHM_LEN + 1)

/**
 * Representation of device identifiers once decoded from text-representation
 *
 * @struct xnvme_ident
 */
struct xnvme_ident {
	char uri[XNVME_IDENT_URI_LEN];

	char schm[XNVME_IDENT_SCHM_LEN];
	char trgt[XNVME_IDENT_TRGT_LEN];
	char opts[XNVME_IDENT_OPTS_LEN];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_ident) == 704, "Incorrect size")

/**
 * Parse the given 'uri' into ::xnvme_ident
 *
 * @param uri
 * @param ident Pointer to ident to fill with values parsed from 'uri'
 *
 * @return On success, 0 is returned. On error, negative `errno` is returned.
 */
int
xnvme_ident_from_uri(const char *uri, struct xnvme_ident *ident);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXNVME_IDENT */

// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#ifndef __INTERNAL_XNVME_BE_MACOS_DRIVERKIT_H
#define __INTERNAL_XNVME_BE_MACOS_DRIVERKIT_H
#include <IOKit/IOKitLib.h>

#include "MacVFNShared.h"

/**
 * Internal representation of XNVME_BE_MACOS_DRIVERKIT state
 *
 * NOTE: When changing this struct, ensure compatibility with 'struct xnvme_be_cbi_state'
 */
struct xnvme_be_macos_driverkit_state {
	io_service_t device_service;
	io_connect_t device_connection;
	uint64_t qidmap;

	uint64_t qid_sync;

	uint8_t _rsvd[104];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_be_macos_driverkit_state) == XNVME_BE_STATE_NBYTES,
		    "Incorrect size")

int
_xnvme_be_driverkit_create_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qd,
				   int flags);
int
_xnvme_be_driverkit_delete_ioqpair(struct xnvme_be_macos_driverkit_state *state, int qid);

#ifdef __cplusplus
extern std::map<uint64_t, std::tuple<uint64_t, uint64_t>> address_mapping;

static inline std::tuple<uint64_t, uint64_t>
find_address_mapping(uint64_t vaddr)
{
	std::map<uint64_t, std::tuple<uint64_t, uint64_t>>::iterator lookup =
		address_mapping.upper_bound(vaddr);
	// NOTE: If the answer is the last element, you'll find out in the comparison below. If
	// not, it will fail regardless. std::map::upper_bound has no distinction from "no element
	// after X" and "no match in list"

	lookup = std::prev(lookup);

	uint64_t base_vaddr = lookup->first;
	uint64_t token;
	uint64_t base_size;
	std::tie(token, base_size) = address_mapping[base_vaddr];
	if (!((base_vaddr <= vaddr) && (base_vaddr + base_size >= vaddr))) {
		XNVME_DEBUG("find_address_mapping: abort() %llx, %llx, %llx, %llx", vaddr,
			    base_vaddr, token, base_size);
		abort();
	}
	uint64_t offset = vaddr - base_vaddr;

	return std::tuple<uint64_t, uint64_t>{token, offset};
}
#endif

extern struct xnvme_be_admin g_xnvme_be_macos_driverkit_admin;
extern struct xnvme_be_sync g_xnvme_be_macos_driverkit_sync;
extern struct xnvme_be_dev g_xnvme_be_macos_driverkit_dev;
extern struct xnvme_be_async g_xnvme_be_macos_driverkit_async;
extern struct xnvme_be_mem g_xnvme_be_macos_driverkit_mem;

#endif /* __INTERNAL_XNVME_BE_MACOS_DRIVERKIT */

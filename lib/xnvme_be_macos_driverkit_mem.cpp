// SPDX-FileCopyrightText: Samsung Electronics Co., Ltd
//
// SPDX-License-Identifier: BSD-3-Clause

#include <map>
extern "C" {

#include <libxnvme.h>
#include <xnvme_be.h>
#include <xnvme_be_nosys.h>
#ifdef XNVME_BE_MACOS_ENABLED

#include <mach/mach_error.h>
#include <CoreFoundation/CoreFoundation.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>

#include <xnvme_dev.h>
#include <xnvme_be_macos_driverkit.h>
#include <errno.h>
#include <sys/mman.h>

void *
xnvme_be_macos_driverkit_buf_alloc(const struct xnvme_dev *dev, size_t nbytes,
				   uint64_t *XNVME_UNUSED(phys))
{
	uint64_t token;
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)dev->be.state;
	long sz = sysconf(_SC_PAGESIZE);

	if (sz == -1) {
		XNVME_DEBUG("FAILED: sysconf(), errno: %d", errno);
		return NULL;
	}
	XNVME_DEBUG("Page size: %llx", sz);

	// nbytes has to be a multiple of alignment. Therefore, we round up to the nearest
	// multiple.
	XNVME_DEBUG("Unaligned size: %llx", nbytes);
	nbytes = (1 + ((nbytes - 1) / sz)) * (sz);
	XNVME_DEBUG("Aligned size: %llx", nbytes);

	void* ptr = xnvme_buf_virt_alloc(sz, nbytes);
	size_t output_cnt = sizeof(token);
	*(uint64_t*) ptr = nbytes; // Put actual size inside allocated memory. Driver on the other end doesn't correctly get this.
	mach_vm_address_t address = (mach_vm_address_t) ptr;
	kern_return_t ret = IOConnectCallStructMethod(state->device_connection, NVME_ALLOC_BUFFER, ptr, nbytes, &token, &output_cnt);
	assert(ret == kIOReturnSuccess);

	address_mapping[address] = std::tuple<uint64_t, uint64_t>{token, (uint64_t)nbytes};
	return (void*) address;
}

void
xnvme_be_macos_driverkit_buf_free(const struct xnvme_dev *dev, void *buf)
{
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)dev->be.state;
	uint64_t token;
	uint64_t _buf_base_offset;
	std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) buf);

	size_t output_cnt = 0;
	kern_return_t ret = IOConnectCallStructMethod(state->device_connection, NVME_DEALLOC_BUFFER, &token, sizeof(token), NULL, &output_cnt);
	address_mapping.erase((mach_vm_address_t) buf);
}

#endif

struct xnvme_be_mem g_xnvme_be_macos_driverkit_mem = {
	.id = "driverkit",
#ifdef XNVME_BE_MACOS_ENABLED
	.buf_alloc = xnvme_be_macos_driverkit_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_macos_driverkit_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#else
	.buf_alloc = xnvme_be_nosys_buf_alloc,
	.buf_realloc = xnvme_be_nosys_buf_realloc,
	.buf_free = xnvme_be_nosys_buf_free,
	.buf_vtophys = xnvme_be_nosys_buf_vtophys,
	.mem_map = xnvme_be_nosys_mem_map,
	.mem_unmap = xnvme_be_nosys_mem_unmap,
#endif
};
}
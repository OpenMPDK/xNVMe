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
#include <IOKit/IODataQueueShared.h>

#include <xnvme_dev.h>
#include <xnvme_queue.h>
#include <xnvme_be_macos_driverkit.h>
#include <DriverKit/IOLib.h>
#include <DriverKit/IOBufferMemoryDescriptor.h>

struct xnvme_queue_macos_driverkit {
	struct xnvme_queue_base base;
	RingQueue* sq_queue;
	RingQueue* cq_queue;

	uint id;

	uint8_t _rsvd[208];
};
XNVME_STATIC_ASSERT(sizeof(struct xnvme_queue_macos_driverkit) == XNVME_BE_QUEUE_STATE_NBYTES,
		    "Incorrect size")

int
xnvme_be_macos_driverkit_queue_init(struct xnvme_queue *_queue, int XNVME_UNUSED(opts))
{
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)_queue;
	struct xnvme_be_macos_driverkit_state *state = (xnvme_be_macos_driverkit_state *)_queue->base.dev->be.state;

	int qpid;
	qpid = _xnvme_be_driverkit_create_ioqpair(state, q->base.capacity, 0);
	if (qpid < 0) {
		XNVME_DEBUG("FAILED: nvme_create_ioqpair(); err: %d", qpid);
		return qpid;
	}
	q->id = qpid;

	mach_vm_size_t size = 0;
	mach_vm_address_t addr;
    kern_return_t res;
	res = IOConnectMapMemory64(
		state->device_connection,
		1 << 16 | qpid,
		mach_task_self(),
		&addr,
		&size,
		kIOMapAnywhere
	);
	XNVME_DEBUG("IOConnectMapMemory64 returned: %llx, %s", res, mach_error_string(res));
	assert(res == kIOReturnSuccess);
	q->sq_queue = (RingQueue*)addr;
	assert(q->sq_queue->buffer_size == size);

	size = 0;
	res = IOConnectMapMemory64(
		state->device_connection,
		qpid,
		mach_task_self(),
		&addr,
		&size,
		kIOMapAnywhere
	);
	XNVME_DEBUG("IOConnectMapMemory64 returned: %llx, %s", res, mach_error_string(res));
	assert(res == kIOReturnSuccess);
	q->cq_queue = (RingQueue*)addr;
	assert(q->cq_queue->buffer_size == size);

	return 0;
}

int
xnvme_be_macos_driverkit_queue_term(struct xnvme_queue *_queue)
{
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)_queue;
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)q->base.dev->be.state;
	_xnvme_be_driverkit_delete_ioqpair(state, q->id);

	q->cq_queue = NULL;
	q->sq_queue = NULL;
	return 0;
}

int
xnvme_be_macos_driverkit_queue_poke(struct xnvme_queue *queue, uint32_t max)
{
	int queue_ret;
	struct xnvme_queue_macos_driverkit *q = (struct xnvme_queue_macos_driverkit *)queue;
	struct xnvme_be_macos_driverkit_state *state = (xnvme_be_macos_driverkit_state *)queue->base.dev->be.state;
	uint64_t qid[2] = {q->id, q->id}; // Submission queue, completion queue

	kern_return_t ret = IOConnectCallScalarMethod(state->device_connection, NVME_POKE, qid, 2, NULL, 0);
	if (ret != kIOReturnSuccess) {
		return -1;
	}

    NvmeSubmitCmd nvme_cmd;
	int reaped = 0;
	while ((queue_ret = queue_dequeue(q->cq_queue, &nvme_cmd)) == 0){
		struct xnvme_cmd_ctx* ctx = (struct xnvme_cmd_ctx*)nvme_cmd.backend_opaque;
		memcpy(&ctx->cpl, nvme_cmd.cpl, sizeof(struct xnvme_spec_cpl));
		ctx->async.cb(ctx, ctx->async.cb_arg);

		reaped += 1;
		queue->base.outstanding -= 1;
	}

	if (queue_ret == QUEUE_ERROR) {
		return -EIO;
	}

	return reaped;
}

int
xnvme_be_macos_driverkit_async_cmd_io(struct xnvme_cmd_ctx *ctx, void *dbuf, size_t dbuf_nbytes,
				      void *mbuf, size_t mbuf_nbytes)
{
	struct xnvme_queue_macos_driverkit *q =
		(struct xnvme_queue_macos_driverkit *)ctx->async.queue;
	struct xnvme_be_macos_driverkit_state *state = (struct xnvme_be_macos_driverkit_state *)ctx->dev->be.state;

	NvmeSubmitCmd cmd = {0};
	int queue_ret;
	uint64_t token;
	uint64_t _buf_base_offset;
	cmd.queue_id = state->qid_sync;

	switch (ctx->cmd.common.opcode) {
	case XNVME_SPEC_NVM_OPC_READ:
		break;

	case XNVME_SPEC_FS_OPC_READ:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_READ;
		break;

	case XNVME_SPEC_NVM_OPC_WRITE:
		break;

	case XNVME_SPEC_FS_OPC_WRITE:
		ctx->cmd.nvm.slba = ctx->cmd.nvm.slba >> ctx->dev->geo.ssw;
		ctx->cmd.common.opcode = XNVME_SPEC_NVM_OPC_WRITE;
		break;
	}

	if (dbuf){
		std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) dbuf);
		cmd.dbuf_nbytes = dbuf_nbytes;
		cmd.dbuf_offset = _buf_base_offset;
		cmd.dbuf_token = token;
	}
	if (mbuf){
		std::tie(token, _buf_base_offset) = find_address_mapping((uint64_t) mbuf);
		cmd.mbuf_nbytes = mbuf_nbytes;
		cmd.mbuf_offset = _buf_base_offset;
		cmd.mbuf_token = token;
	}
	memcpy(cmd.cmd, &ctx->cmd, sizeof(struct xnvme_spec_cmd));
	cmd.backend_opaque = (uint64_t)ctx;

	queue_ret = queue_enqueue(q->sq_queue, &cmd);
	if (queue_ret == QUEUE_FULL){
		return -EBUSY;
	}
	else if (queue_ret){
		return -EIO;
	}

	q->base.outstanding += 1;
	return 0;
}

#endif

struct xnvme_be_async g_xnvme_be_macos_driverkit_async = {
	.id = "driverkit",
#ifdef XNVME_BE_MACOS_ENABLED
	.cmd_io = xnvme_be_macos_driverkit_async_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_macos_driverkit_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_macos_driverkit_queue_init,
	.term = xnvme_be_macos_driverkit_queue_term,
#else
	.cmd_io = xnvme_be_nosys_queue_cmd_io,
	.cmd_iov = xnvme_be_nosys_queue_cmd_iov,
	.poke = xnvme_be_nosys_queue_poke,
	.wait = xnvme_be_nosys_queue_wait,
	.init = xnvme_be_nosys_queue_init,
	.term = xnvme_be_nosys_queue_term,
#endif
};
}
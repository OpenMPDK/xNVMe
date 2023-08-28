//
//  MacVFNShared.h
//  MacVFN
//
//  Created by Mads Ynddal on 24/11/2023.
//

#ifndef MacVFNShared_h
#define MacVFNShared_h

typedef enum {
	NVME_INIT              = 0,
	NVME_ONESHOT           = 1,
	NVME_CREATE_QUEUE_PAIR = 2,
	NVME_DELETE_QUEUE_PAIR = 3,
	NVME_ALLOC_BUFFER      = 4,
	NVME_DEALLOC_BUFFER    = 5,
	NVME_POKE              = 6,
	NUMBER_OF_EXTERNAL_METHODS // Has to be last
} ExternalMethodType;

typedef struct {
	struct {
		uint8_t cmd[64];         // NVMe command
		uint8_t cpl[16];         // NVMe completion
		uint64_t backend_opaque; // Reserved for backend (xNVMe dev, async, etc.)
	};
	uint64_t queue_id;

	uint64_t dbuf_nbytes;
	uint64_t dbuf_offset;
	uint64_t dbuf_token;

	uint64_t mbuf_nbytes;
	uint64_t mbuf_offset;
	uint64_t mbuf_token;
} NvmeSubmitCmd;

typedef struct {
	uint64_t id;
	uint64_t depth;
	int64_t vector;
	uint64_t flags;
} NvmeQueue;

typedef struct {
	uint64_t depth;
	volatile _Atomic uint64_t tail;
	volatile _Atomic uint64_t head;
	uint64_t buffer_size;
	uint8_t buffer[0];
} RingQueue;

static inline void
queue_get_entry(RingQueue *queue, uint64_t index, NvmeSubmitCmd *cmd)
{
	memcpy((void *)cmd, (uint8_t *)queue->buffer + sizeof(*cmd) * index, sizeof(*cmd));
}

static inline void
queue_put_entry(RingQueue *queue, uint64_t index, NvmeSubmitCmd *cmd)
{
	memcpy((uint8_t *)queue->buffer + sizeof(*cmd) * index, (void *)cmd, sizeof(*cmd));
}

#define QUEUE_FULL  -1
#define QUEUE_EMPTY -2
#define QUEUE_ERROR -3

static int
queue_enqueue(RingQueue *queue, NvmeSubmitCmd *cmd)
{
	uint64_t tail = __c11_atomic_load(&queue->tail, __ATOMIC_ACQUIRE);
	uint64_t head = __c11_atomic_load(&queue->head, __ATOMIC_RELAXED);

	if ((tail + 1) % queue->depth == head) {
		// Queue full
		return QUEUE_FULL;
	}

	queue_put_entry(queue, tail, cmd);

	uint64_t new_tail = (tail + 1) % queue->depth;
	if (tail != __c11_atomic_exchange(&queue->tail, new_tail, __ATOMIC_RELEASE)) {
		// No way of recovering. Somebody else modified queue at the same time.
		return QUEUE_ERROR;
	}
	return 0;
}

static int
queue_dequeue(RingQueue *queue, NvmeSubmitCmd *cmd)
{
	uint64_t head = __c11_atomic_load(&queue->head, __ATOMIC_ACQUIRE);
	uint64_t tail = __c11_atomic_load(&queue->tail, __ATOMIC_RELAXED);

	if (head == tail) {
		// Queue empty
		return QUEUE_EMPTY;
	}

	queue_get_entry(queue, head, cmd);

	uint64_t new_head = (head + 1) % queue->depth;
	if (head != __c11_atomic_exchange(&queue->head, new_head, __ATOMIC_RELEASE)) {
		// No way of recovering. Somebody else modified queue at the same time.
		return QUEUE_ERROR;
	}
	return 0;
}

static bool
queue_full(RingQueue *queue)
{
	uint64_t head = __c11_atomic_load(&queue->head, __ATOMIC_RELAXED);
	uint64_t tail = __c11_atomic_load(&queue->tail, __ATOMIC_RELAXED);
	return (tail + 1) % queue->depth == head;
}

static int
queue_peek(RingQueue *queue)
{
	uint64_t head = __c11_atomic_load(&queue->head, __ATOMIC_RELAXED);
	uint64_t tail = __c11_atomic_load(&queue->tail, __ATOMIC_RELAXED);
	return ((int64_t)head - (int64_t)tail + (int64_t)queue->depth) % (int64_t)queue->depth;
}

static int
queue_dequeue_ready(RingQueue *queue, uint64_t *head, uint64_t *tail)
{
	*head = __c11_atomic_load(&queue->head, __ATOMIC_RELAXED);
	*tail = __c11_atomic_load(&queue->tail, __ATOMIC_RELAXED);
	return ((int64_t)*tail - (int64_t)*head + (int64_t)queue->depth) % (int64_t)queue->depth;
}

static int
queue_enqueue_ready(RingQueue *queue, uint64_t *head, uint64_t *tail)
{
	*head = __c11_atomic_load(&queue->head, __ATOMIC_RELAXED);
	*tail = __c11_atomic_load(&queue->tail, __ATOMIC_RELAXED);
	return ((int64_t)*head - (int64_t)*tail + (int64_t)queue->depth) % (int64_t)queue->depth;
}

#endif /* MacVFNShared_h */

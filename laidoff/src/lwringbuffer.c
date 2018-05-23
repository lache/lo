#include "lwringbuffer.h"
#include <string.h>

int ringbuffer_init(LWRINGBUFFER* rb, void* buf0, int stride, int capacity) {
	if (rb == 0) {
		return -1;
	}
	if (buf0 == 0) {
		return -2;
	}
	if (stride <= 0) {
		return -3;
	}
	if (stride % sizeof(int) != 0) {
		return -4;
	}
	if (capacity <= 0) {
		return -5;
	}
	memset(rb, 0, sizeof(LWRINGBUFFER));
	rb->buf0 = buf0;
	rb->stride = stride;
	rb->capacity = capacity;
	ZMUTEX_INIT(rb->mutex);
	return 0;
}

int ringbuffer_queue(LWRINGBUFFER* rb, const void* p) {
	if (rb == 0) {
		return -1;
	}
	if (p == 0) {
		return -2;
	}
	ZMUTEX_LOCK(rb->mutex);
	memcpy((char*)rb->buf0 + rb->tail * rb->stride, p, rb->stride);
	if (rb->full) {
		rb->head = (rb->head + 1) % rb->capacity;
	}
	rb->tail = (rb->tail + 1) % rb->capacity;
	if (rb->full == 0 && rb->head == rb->tail) {
		rb->full = 1;
	}
	ZMUTEX_UNLOCK(rb->mutex);
	return 0;
}

int ringbuffer_peek(LWRINGBUFFER* rb, void* pout) {
	if (rb == 0) {
		return -1;
	}
	if (pout == 0) {
		return -2;
	}
	ZMUTEX_LOCK(rb->mutex);
	if (rb->full == 0 && rb->head == rb->tail) {
		ZMUTEX_UNLOCK(rb->mutex);
		return -3;
	}
	memcpy(pout, (char*)rb->buf0 + rb->head * rb->stride, rb->stride);
	ZMUTEX_UNLOCK(rb->mutex);
	return 0;
}

int ringbuffer_dequeue(LWRINGBUFFER* rb, void* pout) {
	if (rb == 0) {
		return -1;
	}
	if (pout == 0) {
		return -2;
	}
	ZMUTEX_LOCK(rb->mutex);
	if (rb->full == 0 && rb->head == rb->tail) {
		ZMUTEX_UNLOCK(rb->mutex);
		return -3;
	}
	void* p = (char*)rb->buf0 + rb->head * rb->stride;
	rb->head = (rb->head + 1) % rb->capacity;
	rb->full = 0;
	memcpy(pout, p, rb->stride);
	ZMUTEX_UNLOCK(rb->mutex);
	return 0;
}

int ringbuffer_size(LWRINGBUFFER* rb) {
	if (rb == 0) {
		return -1;
	}
	ZMUTEX_LOCK(rb->mutex);
	int ret = 0;
	if (rb->full) {
		ret = rb->capacity;
	}
	else if (rb->head <= rb->tail) {
		ret = rb->tail - rb->head;
	}
	else {
		ret = rb->capacity - (rb->head - rb->tail);
	}
	ZMUTEX_UNLOCK(rb->mutex);
	return ret;
}

int ringbuffer_capacity(LWRINGBUFFER* rb) {
	if (rb == 0) {
		return -1;
	}
	ZMUTEX_LOCK(rb->mutex);
	int ret = rb->capacity;
	ZMUTEX_UNLOCK(rb->mutex);
	return ret;
}

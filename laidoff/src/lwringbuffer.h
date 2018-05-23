#pragma once

#include "platform_detection.h"
//  Mutex macros
#if defined LW_PLATFORM_WIN32
#include <windows.h>
typedef CRITICAL_SECTION zsys_mutex_t;
#   define ZMUTEX_INIT(m)    InitializeCriticalSection (&m);
#   define ZMUTEX_LOCK(m)    EnterCriticalSection (&m);
#   define ZMUTEX_UNLOCK(m)  LeaveCriticalSection (&m);
#   define ZMUTEX_DESTROY(m) DeleteCriticalSection (&m);
#else
#include <pthread.h>
typedef pthread_mutex_t zsys_mutex_t;
#   define ZMUTEX_INIT(m)    pthread_mutex_init (&m, NULL);
#   define ZMUTEX_LOCK(m)    pthread_mutex_lock (&m);
#   define ZMUTEX_UNLOCK(m)  pthread_mutex_unlock (&m);
#   define ZMUTEX_DESTROY(m) pthread_mutex_destroy (&m);
#endif

typedef struct _LWRINGBUFFER {
	void* buf0;
	int stride;
	int capacity;
	int head;
	int tail;
	int full;
	zsys_mutex_t mutex;
} LWRINGBUFFER;

int ringbuffer_init(LWRINGBUFFER* rb, void* buf0, int stride, int capacity);
int ringbuffer_queue(LWRINGBUFFER* rb, const void* p);
int ringbuffer_dequeue(LWRINGBUFFER* rb, void* pout);
int ringbuffer_peek(LWRINGBUFFER* rb, void* pout);
int ringbuffer_size(LWRINGBUFFER* rb);
int ringbuffer_capacity(LWRINGBUFFER* rb);

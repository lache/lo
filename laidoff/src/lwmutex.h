#pragma once

#include "platform_detection.h"

//  Mutex macros
#if !LW_PLATFORM_WIN32
typedef pthread_mutex_t LWMUTEX;
#   define LWMUTEX_INIT(m)    pthread_mutex_init (&m, NULL);
#   define LWMUTEX_LOCK(m)    pthread_mutex_lock (&m);
#   define LWMUTEX_UNLOCK(m)  pthread_mutex_unlock (&m);
#   define LWMUTEX_DESTROY(m) pthread_mutex_destroy (&m);
#else
typedef CRITICAL_SECTION LWMUTEX;
#   define LWMUTEX_INIT(m)    InitializeCriticalSection (&m);
#   define LWMUTEX_LOCK(m)    EnterCriticalSection (&m);
#   define LWMUTEX_UNLOCK(m)  LeaveCriticalSection (&m);
#   define LWMUTEX_DESTROY(m) DeleteCriticalSection (&m);
#endif

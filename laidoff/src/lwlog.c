#include "lwlog.h"
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <tinycthread.h>
static mtx_t lw_log_mutex;
#endif

void lwlog_init() {
#if LW_PLATFORM_WIN32
	mtx_init(&lw_log_mutex, mtx_plain);
#endif
}

void lwlog_destroy() {
#if LW_PLATFORM_WIN32
	mtx_destroy(&lw_log_mutex);
#endif
}

void lwlog_lock() {
#if LW_PLATFORM_WIN32
	mtx_lock(&lw_log_mutex);
#endif
}

void lwlog_unlock() {
#if LW_PLATFORM_WIN32
	mtx_unlock(&lw_log_mutex);
#endif
}

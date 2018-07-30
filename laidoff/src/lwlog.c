#include "lwlog.h"
#include <tinycthread.h>

static mtx_t lw_log_mutex;

void lwlog_init() {
	mtx_init(&lw_log_mutex, mtx_plain);
}

void lwlog_destroy() {
	mtx_destroy(&lw_log_mutex);
}

void lwlog_lock() {
	mtx_lock(&lw_log_mutex);
}

void lwlog_unlock() {
	mtx_unlock(&lw_log_mutex);
}

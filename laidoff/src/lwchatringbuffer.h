#pragma once

#include <time.h>

typedef struct _LWCHATLINE {
    time_t rawtime;
    char strtime[64];
    char line[512];
} LWCHATLINE;

typedef struct _LWCHATRINGBUFFER {
    LWCHATLINE lines[8]; // array length should be power of 2 (wrapping index by bitwise operator)
    int top;
    int count;
    int scroll_to_bottom;
} LWCHATRINGBUFFER;

#ifdef __cplusplus
extern "C" {;
#endif
void lwchatringbuffer_add(LWCHATRINGBUFFER* crb, const char* line);
int lwchatringbuffer_count(const LWCHATRINGBUFFER* crb);
const LWCHATLINE* lwchatringbuffer_get(const LWCHATRINGBUFFER* crb, int index);
void lwchatringbuffer_clear(LWCHATRINGBUFFER* crb);
int lwchatringbuffer_flush_scroll_to_bottom(LWCHATRINGBUFFER* crb);
#ifdef __cplusplus
}
#endif

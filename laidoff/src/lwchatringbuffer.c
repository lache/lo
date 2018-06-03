#include "lwchatringbuffer.h"
#include "lwmacro.h"
#include <string.h>

void lwchatringbuffer_add(LWCHATRINGBUFFER* crb, const char* line) {
    strncpy(crb->lines[crb->top].line, line, ARRAY_SIZE(crb->lines[crb->top].line) - 1);
    crb->lines[crb->top].line[sizeof(crb->lines[crb->top].line) - 1] = 0;
    time(&crb->lines[crb->top].rawtime);
    struct tm* tm_info = localtime(&crb->lines[crb->top].rawtime);
    strftime(crb->lines[crb->top].strtime, ARRAY_SIZE(crb->lines[crb->top].strtime), "%Y-%m-%d %H:%M:%S", tm_info);
    crb->top = (crb->top + 1) & (ARRAY_SIZE(crb->lines) - 1);
    crb->count = LWMIN(crb->count + 1, ARRAY_SIZE(crb->lines));
    crb->scroll_to_bottom = 1;
}

int lwchatringbuffer_count(const LWCHATRINGBUFFER* crb) {
    return crb->count;
}

const LWCHATLINE* lwchatringbuffer_get(const LWCHATRINGBUFFER* crb, int index) {
    if (index < 0 || index >= crb->count) {
        return 0;
    }
    int wrapped_index = (crb->top - crb->count + index) & (ARRAY_SIZE(crb->lines) - 1);
    return &crb->lines[wrapped_index];
}

void lwchatringbuffer_clear(LWCHATRINGBUFFER* crb) {
    crb->count = 0;
}

int lwchatringbuffer_flush_scroll_to_bottom(LWCHATRINGBUFFER* crb) {
    if (crb->scroll_to_bottom) {
        crb->scroll_to_bottom = 0;
        return 1;
    }
    return 0;
}
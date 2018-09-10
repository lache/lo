#include "lwcharptrstream.h"
#include "lwmacro.h"
#include <string.h>
#include <stdlib.h>

void lwcharptrstream_init(LWCHARPTRSTREAM* cps, char* data) {
    memset(cps, 0, sizeof(LWCHARPTRSTREAM));
    if (cps == 0 || data == 0) {
        return;
    }
    cps->data = data;
    cps->length = strlen(data);
    cps->current = 0;
}

void lwcharptrstream_deinit(LWCHARPTRSTREAM* cps) {
    if (cps && cps->data) {
        free(cps->data);
    }
    memset(cps, 0, sizeof(LWCHARPTRSTREAM));
}

int lwcharptrstream_fgets(LWCHARPTRSTREAM* cps, char* buffer, int max_count) {
    if (cps->current >= cps->length) {
        // EOF
        return 0;
    }
    char* p = strchr(cps->data + cps->current, '\n');
    if (p == 0) {
        // copy all remains
        strncpy(buffer, cps->data + cps->current, max_count - 1);
        buffer[max_count - 1] = 0;
        cps->current = cps->length;
        return 0;
    } else {
        int copy_length = LWMIN(p - (cps->data + cps->current), max_count - 1);
        strncpy(buffer, cps->data + cps->current, copy_length);
        buffer[copy_length] = 0;
        cps->current += copy_length;
        cps->current++; // skip '\n' itself
        return 1;
    }
}

void lwcharptrstream_reset_cursor(LWCHARPTRSTREAM* cps) {
    cps->current = 0;
}

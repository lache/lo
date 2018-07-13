#include <stdlib.h>
#include <string.h>
#include "lwchat.h"

#define LWCHAT_MAX_LINES (32)
#define LWCHAT_MAX_LINE_LENGTH (256)

typedef struct _LWCHAT {
    char lines[LWCHAT_MAX_LINES][LWCHAT_MAX_LINE_LENGTH];
    int line_count;
} LWCHAT;

LWCHAT* lwchat_new() {
    LWCHAT* chat = (LWCHAT*)calloc(1, sizeof(LWCHAT));
    return chat;
}

void lwchat_add(LWCHAT* chat, const char* name, const char* line) {
    // shift
    for (int i = 1; i < LWCHAT_MAX_LINES - 1; i++) {
        memcpy(chat->lines[i - 1], chat->lines[i], LWCHAT_MAX_LINE_LENGTH);
    }
    // add new line at last
    chat->lines[LWCHAT_MAX_LINES - 1][0] = 0;
    strncat(chat->lines[LWCHAT_MAX_LINES - 1], name, LWCHAT_MAX_LINE_LENGTH - 1);
    strncat(chat->lines[LWCHAT_MAX_LINES - 1], " : ", LWCHAT_MAX_LINE_LENGTH - 1);
    strncat(chat->lines[LWCHAT_MAX_LINES - 1], line, LWCHAT_MAX_LINE_LENGTH - 1);
    chat->lines[LWCHAT_MAX_LINES - 1][LWCHAT_MAX_LINE_LENGTH - 1] = 0;
}

const char* lwchat_line(const LWCHAT* chat, int index) {
    if (index < 0 || index >= LWCHAT_MAX_LINES) {
        return 0;
    }
    return chat->lines[index];
}

int lwchat_max_lines() {
    return LWCHAT_MAX_LINES;
}

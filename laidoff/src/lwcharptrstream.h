#pragma once

typedef struct _LWCHARPTRSTREAM {
    char* data;
    int length;
    int current;
} LWCHARPTRSTREAM;

void lwcharptrstream_init(LWCHARPTRSTREAM* cps, char* data);
void lwcharptrstream_deinit(LWCHARPTRSTREAM* cps);
int lwcharptrstream_fgets(LWCHARPTRSTREAM* cps, char* buffer, int max_count);
void lwcharptrstream_reset_cursor(LWCHARPTRSTREAM* cps);
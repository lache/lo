#pragma once

typedef struct _LWCHAT LWCHAT;

LWCHAT* lwchat_new();
void lwchat_add(LWCHAT* chat, const char* name, const char* line);
const char* lwchat_line(const LWCHAT* chat, int index);
int lwchat_max_lines();

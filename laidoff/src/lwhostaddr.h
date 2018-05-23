#pragma once

typedef struct _LWHOSTADDR {
    char host[128];
    char port_str[16];
    unsigned long host_resolved;
    int port;
} LWHOSTADDR;

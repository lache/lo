#pragma once

typedef struct _LWCONTEXT LWCONTEXT;

void init_net(LWCONTEXT* pLwc);
void net_rtt_test(LWCONTEXT* pLwc);
void deinit_net(LWCONTEXT* pLwc);

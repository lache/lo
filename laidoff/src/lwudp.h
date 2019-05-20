#pragma once
#include "platform_detection.h"
#if LW_PLATFORM_WIN32
#include <WinSock2.h>
#else
#include <fcntl.h>
#include <sys/socket.h>
#if !LW_PLATFORM_OSX && !LW_PLATFORM_IOS && !LW_PLATFORM_LINUX
#include <linux/in.h>
#include <endian.h>
#endif
#include <stdlib.h>
#include <czmq_prelude.h>
#endif
#include "puckgamepacket.h"
#include "lwringbuffer.h"
#include "numcomp_puck_game.h"

#define LW_UDP_BUFLEN (1024*8)
#define LW_STATE_RING_BUFFER_CAPACITY (16)

typedef struct _LWUDP {
#if LW_PLATFORM_WIN32
	WSADATA wsa;
#endif
	struct sockaddr_in si_other;
	SOCKET s;
	int slen;
	char buf[LW_UDP_BUFLEN];
	char message[LW_UDP_BUFLEN];
	fd_set readfds;
	struct timeval tv;
	int recv_len;
	int ready;
    int reinit_next_update;
	// State ring buffer
	LWPSTATE state_buffer[LW_STATE_RING_BUFFER_CAPACITY];
	LWRINGBUFFER state_ring_buffer;
	double puck_state_sync_server_timepoint;
	double puck_state_sync_client_timepoint;
	int state_count;
	double state_start_timepoint;
    // State2 ring buffer
    LWPSTATE state2_buffer[LW_STATE_RING_BUFFER_CAPACITY];
    LWRINGBUFFER state2_ring_buffer;
    double puck_state2_sync_server_timepoint;
    double puck_state2_sync_client_timepoint;
    int state2_count;
    double state2_start_timepoint;
    LWNUMCOMPPUCKGAME numcomp;
    float last_updated;
    int ping_seq;
} LWUDP;

typedef struct _LWCONTEXT LWCONTEXT;

LWUDP* new_udp(void);
void udp_update_addr_host(LWUDP* udp, const char* host, unsigned short port, const char* port_str);
void udp_update_addr(LWUDP* udp, unsigned long ip, unsigned short port);
void destroy_udp(LWUDP** udp);
void udp_send(LWUDP* udp, const char* data, int size);
void udp_update(LWCONTEXT* pLwc, LWUDP* udp);
const char* lw_udp_addr(const LWCONTEXT* pLwc);
unsigned long lw_udp_addr_resolved(const LWCONTEXT* pLwc);
int lw_udp_port(const LWCONTEXT* pLwc);


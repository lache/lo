#include "lwudp.h"
#include "lwlog.h"
#include "puckgamepacket.h"
#include "lwcontext.h"
#include "puckgameupdate.h"
#include "lwtimepoint.h"
#include "puckgame.h"
#include "platform_detection.h" 
#if LW_PLATFORM_WIN32
#include <Ws2tcpip.h>
#endif
#include "remtex.h"
#include "lwcontext.h"

static int make_socket_nonblocking(SOCKET sock) {
#if defined(WIN32) || defined(_WIN32) || defined(IMN_PIM)
    unsigned long arg = 1;
    return ioctlsocket(sock, FIONBIO, &arg) == 0;
#elif defined(VXWORKS)
    int arg = 1;
    return ioctl(sock, FIONBIO, (int)&arg) == 0;
#else
    int curFlags = fcntl(sock, F_GETFL, 0);
    return fcntl(sock, F_SETFL, curFlags | O_NONBLOCK) >= 0;
#endif
}

LWUDP* new_udp() {
    LWUDP* udp = (LWUDP*)malloc(sizeof(LWUDP));
    memset(udp, 0, sizeof(LWUDP));
    udp->slen = sizeof(udp->si_other);
#if LW_PLATFORM_WIN32
    LOGI("Initialising WinSock...");
    if (WSAStartup(MAKEWORD(2, 2), &udp->wsa) != 0) {
        LOGE("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
    LOGI("WinSock Initialised.");
#endif

    //create socket
    if ((udp->s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
        LOGE("socket() failed with error code : %d", WSAGetLastError());
#else
        LOGE("socket() failed...");
#endif
        exit(EXIT_FAILURE);
    }
#if LW_PLATFORM_IOS
    int set = 1;
    setsockopt(udp->s, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(int));
#endif
    //setup address structure
    memset((char *)&udp->si_other, 0, sizeof(udp->si_other));
    udp->si_other.sin_family = AF_INET;
    //struct hostent* he = gethostbyname(LW_UDP_SERVER);
    //struct in_addr** addr_list = (struct in_addr **) he->h_addr_list;
    //udp_update_addr(udp, addr_list[0]->s_addr, LW_UDP_PORT);
    udp->tv.tv_sec = 0;
    udp->tv.tv_usec = 0;
    make_socket_nonblocking(udp->s);
    udp->ready = 1;
    ringbuffer_init(&udp->state_ring_buffer, udp->state_buffer, sizeof(LWPSTATE), LW_STATE_RING_BUFFER_CAPACITY);
    ringbuffer_init(&udp->state2_ring_buffer, udp->state2_buffer, sizeof(LWPSTATE2), LW_STATE_RING_BUFFER_CAPACITY);
    numcomp_puck_game_init(&udp->numcomp);
    return udp;
}

static unsigned long udp_hostname_to_ip(const char* hostname, const char* port_str) {
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET; // use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo(hostname, port_str, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 0;
    }

    int ip = 0;
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
        h = (struct sockaddr_in *) p->ai_addr;
        ip = h->sin_addr.s_addr;
        break;
    }

    freeaddrinfo(servinfo); // all done with this structure
    return ip;
}

void udp_update_addr_host(LWUDP* udp, const char* host, unsigned short port, const char* port_str) {
    udp_update_addr(udp, udp_hostname_to_ip(host, port_str), port);
}

void udp_update_addr(LWUDP* udp, unsigned long ip, unsigned short port) {
    udp->si_other.sin_addr.s_addr = ip;
    udp->si_other.sin_port = htons(port);
}

void destroy_udp(LWUDP** udp) {
    closesocket((*udp)->s);
    free(*udp);
    *udp = 0;
}

void udp_send(LWUDP* udp, const char* data, int size) {
    if (!udp) {
        return;
    }
    //send the message
    if (sendto(udp->s, data, size, 0, (struct sockaddr *) &udp->si_other, udp->slen) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
        LOGE("sendto() failed with error code : %d", WSAGetLastError());
#else
        //LOGE("sendto() failed with error...");
#endif
        //exit(EXIT_FAILURE);
    }
}

void queue_state(LWUDP* udp, const LWPSTATE* p) {
    ringbuffer_queue(&udp->state_ring_buffer, p);
}

void queue_state2(LWUDP* udp, const LWPSTATE2* p) {
    ringbuffer_queue(&udp->state2_ring_buffer, p);
}

void udp_update(LWCONTEXT* pLwc, LWUDP* udp) {
    if (udp->ready == 0) {
        return;
    }
    FD_ZERO(&udp->readfds);
    FD_SET(udp->s, &udp->readfds);
    int rv = 0;
    while ((rv = select((int)(udp->s + 1), &udp->readfds, NULL, NULL, &udp->tv)) == 1) {
        if ((udp->recv_len = recvfrom(udp->s, udp->buf, LW_UDP_BUFLEN, 0, (struct sockaddr*)&udp->si_other, (socklen_t*)&udp->slen)) == SOCKET_ERROR) {
#if LW_PLATFORM_WIN32
            int wsa_error_code = WSAGetLastError();
            if (wsa_error_code == WSAECONNRESET) {
                // UDP server not ready?
                // Go back to single play mode
                //udp->master = 1;
                return;
            } else {
                LOGEP("recvfrom() failed with error code : %d", wsa_error_code);
                exit(EXIT_FAILURE);
            }
#else
            // Socket recovery needed
            LOGEP("UDP socket error! Socket recovery needed...");
            udp->ready = 0;
            return;
#endif
        }
        const int packet_type = *(int*)udp->buf;
        switch (packet_type) {
        case LPGP_LWPSTATE:
        {
            if (udp->recv_len != sizeof(LWPSTATE)) {
                LOGE("LWPSTATE: Size error %d (%zu expected)", udp->recv_len, sizeof(LWPSTATE));
            }
            //int tick_diff = p->update_tick - pLwc->puck_game_state.update_tick;
            /*if (tick_diff > 0)*/
            {
                /*if (tick_diff != 1) {
                 LOGI("Packet jitter");
                 }*/
                 //memcpy(&pLwc->puck_game_state, p, sizeof(LWPSTATE));
                double last_received = lwtimepoint_now_seconds();
                double state_packet_interval = last_received - pLwc->puck_game_state_last_received;
                pLwc->puck_game_state_last_received = last_received;
                pLwc->puck_game_state_last_received_interval = state_packet_interval * 1000;
                // IGNORE LWPSTATE packet
                //queue_state(pLwc->udp, p);
                int rb_size = ringbuffer_size(&pLwc->udp->state_ring_buffer);
                if (pLwc->udp->state_count == 0) {
                    pLwc->udp->state_start_timepoint = lwtimepoint_now_seconds();
                }
                pLwc->udp->state_count++;
                double elapsed_from_start = lwtimepoint_now_seconds() - pLwc->udp->state_start_timepoint;
                LOGIx("State packet interval: %.3f ms (rb size=%d) (%.2f pps)",
                      pLwc->puck_game_state_last_received_interval,
                      rb_size,
                      (float)pLwc->udp->state_count / elapsed_from_start);
            }
            break;
        }
        default:
        {
            const unsigned char packet_type_short = *(unsigned char*)udp->buf;
            switch (packet_type_short) {
            case LPGP_LWPSTATE2:
            {
                LWPSTATE2* p = (LWPSTATE2*)udp->buf;
                if (udp->recv_len != sizeof(LWPSTATE2)) {
                    LOGE("LWPSTATE2: Size error %d (%zu expected)", udp->recv_len, sizeof(LWPSTATE));
                }
                double last_received = lwtimepoint_now_seconds();
                double state_packet_interval = last_received - pLwc->puck_game_state2_last_received;
                pLwc->puck_game_state2_last_received = last_received;
                pLwc->puck_game_state2_last_received_interval = state_packet_interval * 1000;
                queue_state2(pLwc->udp, p);
                if (pLwc->udp->state2_count == 0) {
                    pLwc->udp->state2_start_timepoint = lwtimepoint_now_seconds();
                }
                pLwc->udp->state2_count++;
                break;
            }
            default:
            {
                LOGE("Unknown datagram (UDP packet) received.");
                break;
            }
            }
            break;
        }
        }
    }
}

const char* lw_udp_addr(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.host;
}

unsigned long lw_udp_addr_resolved(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.host_resolved;
}

int lw_udp_port(const LWCONTEXT* pLwc) {
    return pLwc->udp_host_addr.port;
}

#include "platform_detection.h"
#include <stdio.h>      /* for printf(), fprintf() */
#if LW_PLATFORM_WIN32
#include <winsock.h>    /* for socket(),... */
typedef int socklen_t;
#else
#include <fcntl.h>
#include <sys/socket.h>
#if !LW_PLATFORM_OSX
#include <linux/in.h>
#include <endian.h>
#endif
#include <stdlib.h>
#include <czmq_prelude.h>
#endif
#include <stdlib.h>     /* for exit() */
#include "net.h"
#include "lwlog.h"

#define ECHOMAX 255     /* Longest string to echo */

#ifndef UINT_PTR
#define UINT_PTR unsigned int *
#endif

SOCKET sock;                        /* Socket descriptor */
struct sockaddr_in echoServAddr; /* Echo server address */
struct sockaddr_in fromAddr;     /* Source address of echo */
unsigned short echoServPort;     /* Echo server port */
int fromSize;           /* In-out of address size for recvfrom() */
char *servIP;                    /* IP address of server */
char *echoString;                /* String to send to echo server */
char echoBuffer[ECHOMAX];        /* Buffer for echo string */
int echoStringLen;               /* Length of string to echo */
int respStringLen;               /* Length of response string */
#if LW_PLATFORM_WIN32
WSADATA wsa_data;                 /* Structure for WinSock setup communication */
#endif

void DieWithError(char *errorMessage) {
	LOGE("NET ERROR: %s", errorMessage);
}

void init_net(LWCONTEXT* pLwc) {
	servIP = "222.110.4.119";
	echoString = "desktop!!";
	echoServPort = 10001;
	echoStringLen = (int)strlen(echoString);
#if LW_PLATFORM_WIN32
	/* Load Winsock 2.0 DLL */
	if (WSAStartup(MAKEWORD(2, 0), &wsa_data) != 0) {
		fprintf(stderr, "WSAStartup() failed");
		exit(1);
	}
#endif
	/* Create a best-effort datagram socket using UDP */
	sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == 0) {
		DieWithError("socket() failed");
	}
		
	/* Construct the server address structure */
	memset(&echoServAddr, 0, sizeof(echoServAddr));    /* Zero out structure */
	echoServAddr.sin_family = AF_INET;                 /* Internet address family */
	echoServAddr.sin_addr.s_addr = inet_addr(servIP);  /* Server IP address */
	echoServAddr.sin_port = htons(echoServPort);     /* Server port */
}

void deinit_net(LWCONTEXT* pLwc) {
	closesocket(sock);
#if LW_PLATFORM_WIN32
	WSACleanup();  /* Cleanup Winsock */
#endif
}

void net_rtt_test(LWCONTEXT* pLwc) {
	if (sendto(sock, echoString, echoStringLen, 0, (struct sockaddr *)&echoServAddr, sizeof(echoServAddr)) != echoStringLen)
		DieWithError("sendto() sent a different number of bytes than expected");

	/* Recv a response */

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
		LOGE("NET Error: setsockopt");
	}

	fromSize = sizeof(fromAddr);
	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, (socklen_t*)&fromSize)) < 0) {
		DieWithError("recvfrom() failed");
	} else {
		if (echoServAddr.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
			LOGE("Error: received a packet from unknown source.");
		} else {
			echoBuffer[respStringLen] = '\0';
			LOGI("Received: %s", echoBuffer);    /* Print the echoed arg */
		}
	}
}

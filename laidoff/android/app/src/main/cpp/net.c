#include <stdio.h> //printf
#include <string.h> //memset
#include <stdlib.h> //exit(0);
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include "lwlog.h"
#include "net.h"

#define SERVER "222.110.4.119"
#define BUFLEN 512  //Max length of buffer
#define PORT 10001   //The port on which to send data
#define ECHOMAX 255     /* Longest string to echo */

struct sockaddr_in si_other;
struct sockaddr_in fromAddr;     /* Source address of echo */
int sock, i, slen=sizeof(si_other);
char buf[BUFLEN];
const char* message = "android ^__^";
int respStringLen;               /* Length of response string */
char echoBuffer[ECHOMAX];        /* Buffer for echo string */

void DieWithError(char *s)
{
	perror(s);
	exit(1);
}

void init_net(struct _LWCONTEXT* pLwc) {
	if ( (sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
		DieWithError("socket");
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(PORT);

	if (inet_aton(SERVER , &si_other.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}
}

void deinit_net(struct _LWCONTEXT* pLwc) {
	close(sock);
}

void net_rtt_test(struct _LWCONTEXT* pLwc) {
	if (sendto(sock, message, strlen(message), 0, (struct sockaddr *)&si_other, sizeof(si_other)) != strlen(message)) {
		DieWithError("sendto() sent a different number of bytes than expected");
	}


	/* Recv a response */

	struct timeval tv;
	tv.tv_sec = 5;
	tv.tv_usec = 0;
	if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv)) < 0) {
		LOGE("NET Error: setsockopt");
	}
	int fromSize;           /* In-out of address size for recvfrom() */
	fromSize = sizeof(fromAddr);
	if ((respStringLen = recvfrom(sock, echoBuffer, ECHOMAX, 0, (struct sockaddr *) &fromAddr, &fromSize)) < 0) {
		DieWithError("recvfrom() failed");
	} else {
		if (si_other.sin_addr.s_addr != fromAddr.sin_addr.s_addr) {
			LOGE("Error: received a packet from unknown source.");
		} else {
			echoBuffer[respStringLen] = '\0';
			LOGI("Received: %s", echoBuffer);    /* Print the echoed arg */
		}
	}
}

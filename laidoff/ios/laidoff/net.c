/*
 Simple udp client
 */
#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include "lwlog.h"
#include "net.h"
#include "lwtimepoint.h"

#define SERVER "222.110.4.119"
#define BUFLEN 512  //Max length of buffer
#define PORT 10001   //The port on which to send data


struct sockaddr_in si_other;
int s, i, slen=sizeof(si_other);
char buf[BUFLEN];
const char* message = "iOS!!! ^_______^";

void die(char *s)
{
    perror(s);
    exit(1);
}

void init_net(struct _LWCONTEXT* pLwc) {
    
    
    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
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
    close(s);
}

void net_rtt_test(struct _LWCONTEXT* pLwc) {
    LWTIMEPOINT tp;
    lwtimepoint_now(&tp);
    //send the message
    if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        die("sendto()");
    }
    
    //receive a reply and print it
    //clear the buffer by filling null, it might have previously received data
    memset(buf,'\0', BUFLEN);
    //try to receive some data, this is a blocking call
    if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
    {
        die("recvfrom()");
    }
    LWTIMEPOINT tp2;
    lwtimepoint_now(&tp2);
    float rtt = (float)lwtimepoint_diff(&tp2, &tp);
    char buf2[BUFLEN];
    sprintf(buf2,"NET: %s (rtt: %.3f ms)", buf, rtt * 1e3);
    LOGI("%s", buf2);
    
    if (sendto(s, buf2, strlen(buf2) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        die("sendto() - 2");
    }
    
    //close(s);
}

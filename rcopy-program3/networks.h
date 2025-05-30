
// 	Writen - HMS April 2017
//  Supports TCP and UDP - both client and server


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "cpe464.h"
#include "gethostbyname.h"

#define DEBUG_FLAG 1 // ~!*

#define LISTEN_BACKLOG 10
#define MAX_FNAME_LEN 101   // including null terminator

typedef struct connection
{
    int32_t socketNum;			// socket number
    struct sockaddr_in6 remote;	// address of the connection
    uint32_t addrLen;		// length of the address
} Connection;

// struct for all rcopy inputs file SRC and DST, window size, buffer size, error rate, host name, port number
typedef struct rcopyInputs
{
    char srcFile[MAX_FNAME_LEN];
    char dstFile[MAX_FNAME_LEN];
    int windowSize;
    int bufferSize;
    float errorRate;
    char hostName[MAX_FNAME_LEN];
    int portNumber;
} RcopyInputs;

int safeGetUdpSocket();

// // for the TCP server side
// int tcpServerSetup(int serverPort);
// int tcpAccept(int mainServerSocket, int debugFlag);

// // for the TCP client side
// int tcpClientSetup(char * serverName, char * serverPort, int debugFlag);

// For UDP Server and Client
int udpServerSetup(int serverPort);
int udpClientSetup(char * hostName, int serverPort, Connection * connection);
// int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort);
int selectCall(int32_t sockNum, int32_t sec, int32_t usec);
int safeSendTo(uint8_t * buff, int len, Connection * to);
int safeRecvFrom(int recvSockNum, uint8_t * buff, int len, Connection * from);

#endif

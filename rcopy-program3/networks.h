
// 	Writen - HMS April 2017
//  Supports TCP and UDP - both client and server


#ifndef __NETWORKS_H__
#define __NETWORKS_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "gethostbyname.h"

#define LISTEN_BACKLOG 10

typedef struct connection
{
    int32_t socketNum;			// socket number
    struct sockaddr_in6 remote;	// address of the connection
    uint32_t addrLen;		// length of the address
} Connection;

int safeGetUdpSocket();

// // for the TCP server side
// int tcpServerSetup(int serverPort);
// int tcpAccept(int mainServerSocket, int debugFlag);

// // for the TCP client side
// int tcpClientSetup(char * serverName, char * serverPort, int debugFlag);

// For UDP Server and Client
int udpServerSetup(int serverPort);
*int udpClientSetup(char * hostName, int serverPort, Connection * connection);
// int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort);
int selectCall(int32_t sockNum, int32_t sec, int32_t usec);
*int safeSendto(uint8_t * packet, uint32_t len, Connection * connection);
*int safeRecvfrom(uint8_t * buffer, uint32_t len, Connection * connection);

#endif

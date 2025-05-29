#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"
#include "srej.h"
#include <bits/waitflags.h>

#define MAX_PACK_LEN 1500
#define MAX_PAYLOAD 1400

int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);

int main(int argc, char *argv[])
{
    int serverSock = 0;
    int portNumber = 0;
    float errorRate = 0;

    checkArgs(argc, argv, &errorRate, &portNumber);

    serverSock = udpServerSetup(portNumber);

    sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

    uint8_t buff[MAX_PACK_LEN] = {0};
    Connection *client = (Connection *)calloc(1, sizeof(Connection));
    client->addrLen = sizeof(client->remote);
    uint8_t flag = 0;
    uint32_t seqNum = 0;
    int32_t recvLen = 0;

    // block waiting for new client
    recvLen = recvBuff(buff, MAX_PACK_LEN, serverSock, client, &flag, &seqNum);

    if (client->addrLen > sizeof(client->remote))
    {
        fprintf(stderr, "Error: client address length exceeds expected size.\n");
        close(serverSock);
        free(client);
        exit(EXIT_FAILURE);
    }

    int32_t buffSize = 0;
    uint8_t response[1];
    char fname[MAX_FNAME_LEN] = {0};

    // extract header size on packet for sending data and filename
    memcpy(&buffSize, buff, sizeof(uint32_t));
    buffSize = ntohl(buffSize);

    if (DEBUG_FLAG)
    {
        printf("\n{DEBUG} Received packet size: %d\n{DEBUG}Received buffSize: %d\n\n", recvLen, buffSize);
    }

    int fnameLen = recvLen - sizeof(uint32_t);
    if (fnameLen >= MAX_FNAME_LEN)
        fnameLen = MAX_FNAME_LEN - 1;
    memcpy(fname, &buff[sizeof(uint32_t)], fnameLen);
    fname[fnameLen] = '\0';

    client->socketNum = safeGetUdpSocket();

    // Print client remote address info
    char addrStr[INET6_ADDRSTRLEN] = {0};
    void *addrPtr = NULL;
    uint16_t port = 0;

    if (client->remote.sin6_family == AF_INET6)
    {
        addrPtr = &client->remote.sin6_addr;
        port = ntohs(client->remote.sin6_port);
    }
    else
    {
        addrPtr = &((struct sockaddr_in *)&client->remote)->sin_addr;
        port = ntohs(((struct sockaddr_in *)&client->remote)->sin_port);
    }

    inet_ntop(AF_INET6, addrPtr, addrStr, sizeof(addrStr));
    printf("\n{DEBUG} Client connected from [%s]:%d\n\n", addrStr, port);

    // Reply to client using the address/port filled by recvBuff
    sendBuff(response, 0, client, FNAME_BAD, 0, buff);

    free(client);
    close(serverSock);

    return 0;
}

// usage: server <error rate> [port number]
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber)
{
	// Checks args and returns port number
	int status = 0;
	*portNumber = 0;
	*errorRate = 0;

	if (argc < 2 || argc > 3)
	{
		fprintf(stderr, "Usage %s <error rate> [optional port number]\n", argv[0]);
		exit(-1);
	}

	*errorRate = atof(argv[1]);
	if (*errorRate < 0 || *errorRate >= 1)
	{
		fprintf(stderr, "Error rate must be between 0 and less than 1\n");
		exit(-1);
	}
	if (argc == 3)
	{
		*portNumber = atoi(argv[2]);
	}

	return status;
}


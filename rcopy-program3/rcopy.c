// Client side - UDP Code				    
// By Hugh Smith	4/1/2017	
// Modified by Lukas Shipley on 5/23/2025

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

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"

#define MAX_PACK_LEN 1500
#define MAX_BUFF_SIZE 1400

typedef enum State STATE;

enum State
{
	START, FILE_OK, FILENAME, RECV_DATA, DONE
};

void transferFile(char * argv[]);
STATE start_state(char ** argv, Connection * server, uint32_t * clientSeqNum);
STATE filename(char * fname, int32_t buf_size, Connection * server);
STATE recv_data(int32_t outFile, Connection * server, uint32_t * clientSeqNum);
STATE file_ok(int * outFileFd, char * outFileName);
void checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);

int main (int argc, char *argv[])
{
	int socketNum = 0;
	int inputStatus = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0;
	float errorRate = 0;

	checkArgs(argc, argv, &errorRate, &portNumber);

	socketNum = setupUdpClientToServer(&server, argv[2], portNumber);

	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	
	talkToServer(socketNum, &server);
	
	close(socketNum);

	return 0;
}

void transferFile(char * argv[])
{
    Connection * server = (Connection *) calloc(1, sizeof(Connection));
    STATE state = START;
    int outFileFd = 0;
    uint32_t clientSeqNum = 0;

    while (state != DONE)
    {
        switch (state)
        {
            case START:
                state = start_state(argv, server, &clientSeqNum);
                break;
            case FILENAME:
                state = filename(argv[1], atoi(argv[4]), server);
                break;
            case RECV_DATA:
                state = recv_data(outFileFd, server, &clientSeqNum);
                break;
            case FILE_OK:
                state = file_ok(&outFileFd, argv[2]);
                break;
            case DONE:
                break;
            default:
                fprintf(stderr, "ERROR - In default state (transferFile)\n");
                exit(-1);
        }
    }
}

STATE start_state(char ** argv, Connection * server, uint32_t * clientSeqNum)
{
    uint8_t packet[MAX_PACK_LEN];
    uint8_t buffer[MAX_PACK_LEN];
    int fileNameLen = strlen(argv[1]);
    STATE retVal = FILENAME;
    uint32_t bufferLen = 0;

    // if previous server connection, close before reconnect
    if (server->socketNum > 0)
    {
        close(server->socketNum);
    }

    if (setupUdpClientToServer(&server->remote, argv[2], atoi(argv[3])) < 0)
    {
        // could not connect to server
        retVal = DONE;
    }
    else
    {
        // build PDU (filename packet flag = 9)
        bufferLen = htonl(atoi(argv[4]));
        memcpy(buffer, &bufferLen, MAX_BUFF_SIZE);
        memcpy(&buffer[MAX_BUFF_SIZE], argv[1], fileNameLen);
        printIPv6Info(&server->remote);
        // send buffer to server
        sendtoErr(server->socketNum, buffer, MAX_BUFF_SIZE + fileNameLen, 0,
                   (struct sockaddr *)&server->remote, sizeof(server->remote));
// CHECK THIS ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
        (* clientSeqNum)++;

    }

    return retVal;
}

void checkArgs(int argc, char *argv[], float *errorRate, int *portNumber)
{
	*portNumber = 0;
	*errorRate = 0;

    /* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s <filepath src> <filepath dest> <window size> <buffer size> <error rate> <host name> <port number> \n", argv[0]);
		exit(1);
	}

	*portNumber = atoi(argv[7]);
	*errorRate = atof(argv[5]);
	if (*errorRate < 0 || *errorRate >= 1)
	{
		fprintf(stderr, "Error rate must be between 0 and less than 1\n");
		exit(-1);
	}
}


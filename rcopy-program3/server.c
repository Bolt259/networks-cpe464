/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/
// Modified by Lukas Shipley on 5/23/2025

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"

#define MAXBUF 80

void processClient(int socketNum);
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);

int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;				
	int inputStatus = 0;
	int portNumber = 0;
	float errorRate = 0;

	inputStatus = checkArgs(argc, argv, &errorRate, &portNumber);
	if (inputStatus == -1)
	{
		fprintf(stderr, "Error in command line arguments\n");
		exit(-1);
	}

	socketNum = udpServerSetup(portNumber);

	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

	processClient(socketNum);

	close(socketNum);
	
	return 0;
}

void processClient(int socketNum)
{
	char pduBuffer[MAX_UDP];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	while (1)
	{
		int recvLen = safeRecvfrom(socketNum, pduBuffer, sizeof(pduBuffer), 0, (struct sockaddr *)&client, &clientAddrLen);
		if (recvLen <= 0)
		{
			fprintf(stderr, "Error receiving data\n");
			exit(-1);
		}

		printf("Server recieved PDU from client:\n");
		printIPInfo(&client);
		printPDU((uint8_t *)pduBuffer, recvLen);

		// just for fun send back to client number of bytes received
		safeSendto(socketNum, pduBuffer, recvLen, 0, (struct sockaddr *)&client, clientAddrLen);
	}
}

// usage: server [error rate] [port number]
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber)
{
	// Checks args and returns port number
	int status = 0;
	*portNumber = 0;
	*errorRate = 0;
	if (argc < 1 || argc > 3)
	{
		fprintf(stderr, "Usage %s [optional error rate] [optional port number]\n", argv[0]);
		exit(-1);
	}

	if (argc == 3)
	{
		*errorRate = atof(argv[1]);
		if (*errorRate < 0 || *errorRate >= 1)
		{
			fprintf(stderr, "Error rate must be between 0 and less than 1\n");
			exit(-1);
		}
		*portNumber = atoi(argv[2]);
	}
	else if (argc == 2)
	{
		*portNumber = atoi(argv[1]);
	}

	return status;
}



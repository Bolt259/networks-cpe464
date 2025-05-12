/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

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

#define MAXBUF 80

void processClient(int socketNum);
int checkArgs(int argc, char *argv[], int *errorRate, int *portNumber);

int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;				
	int inputStatus = 0;
	int portNumber = 0;
	int errorRate = 0;

	inputStatus = checkArgs(argc, argv, &errorRate, &portNumber);
	if (inputStatus == -1)
	{
		fprintf(stderr, "Error in command line arguments\n");
		exit(-1);
	}

	socketNum = udpServerSetup(portNumber);

	processClient(socketNum);

	close(socketNum);
	
	return 0;
}

void processClient(int socketNum)
{
	int dataLen = 0; 
	char buffer[MAXBUF + 1];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);
	
		printf("Received message from client with ");
		printIPInfo(&client);
		printf(" Len: %d \'%s\'\n", dataLen, buffer);

		// just for fun send back to client number of bytes received
		sprintf(buffer, "bytes: %d", dataLen);
		safeSendto(socketNum, buffer, strlen(buffer)+1, 0, (struct sockaddr *) & client, clientAddrLen);

	}
}

// usage: server [error rate] [port number]
int checkArgs(int argc, char *argv[], int *errorRate, int *portNumber)
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



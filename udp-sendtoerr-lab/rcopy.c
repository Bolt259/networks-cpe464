// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

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

#define MAXBUF 80

void talkToServer(int socketNum, struct sockaddr_in6 * server);
int readFromStdin(char * buffer);
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);

int main (int argc, char *argv[])
 {
	int socketNum = 0;
	int inputStatus = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int portNumber = 0;
	float errorRate = 0;

	inputStatus = checkArgs(argc, argv, &errorRate, &portNumber);
	if (inputStatus == -1)
	{
		fprintf(stderr, "Error in command line arguments\n");
		exit(-1);
	}

	socketNum = setupUdpClientToServer(&server, argv[2], portNumber);

	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	
	talkToServer(socketNum, &server);
	
	close(socketNum);

	return 0;
}

void talkToServer(int socketNum, struct sockaddr_in6 * server)
{
	int serverAddrLen = sizeof(struct sockaddr_in6);
	char input[MAXBUF+1];
	uint8_t pduBuffer[MAX_UDP];
	uint8_t recvBuffer[MAX_UDP];
	uint32_t seqNum = 0;
	
	while(1)
	{
		int inputLen = readFromStdin(input);
		if (inputLen > MAXBUF)
		{
			fprintf(stderr, "Error: input too long\n");
			exit(-1);
		}
		if (input[0] == '.') break;

		// build PDU
		int pduLen = createPDU(pduBuffer, seqNum, 0xAB, (uint8_t *)input, inputLen);

		// printf("Debug PDU Buffer (raw bytes): [");	// DXXG ----------------------^-
		// for (int i = 0; i < pduLen; i++) {
		// 	printf("%02X ", pduBuffer[i]);
		// }
		// printf("]\n");
		// uint16_t calculatedChksum = in_cksum((unsigned short *)pduBuffer, pduLen);
		// if (calculatedChksum != 0)
		// {
		// 	fprintf(stderr, "createPDU - error in PDU creation: expected chksum %u\n", calculatedChksum);
		// 	return;
		// }

		if (pduLen < 0)
		{
			fprintf(stderr, "Error creating PDU\n");
			exit(-1);
		}

		printf("Client sending PDU to server:\n");
		printPDU(pduBuffer, pduLen);

		// send PDU
		safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, serverAddrLen);

		// recv echoed PDU
		int recvLen = safeRecvfrom(socketNum, recvBuffer, sizeof(recvBuffer), 0, (struct sockaddr *) server, &serverAddrLen);
		if (recvLen < 0)
		{
			fprintf(stderr, "Error receiving PDU\n");
			exit(-1);
		}
		printf("Client received PDU from server:\n");
		printPDU(recvBuffer, recvLen);

		seqNum++;
	}
}

int readFromStdin(char * buffer)
{
	char aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber)
{
	int status = 0;
	*portNumber = 0;
	*errorRate = 0;

    /* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s error-rate host-name port-number \n", argv[0]);
		exit(1);
	}

	*portNumber = atoi(argv[3]);
	*errorRate = atof(argv[1]);
	if (*errorRate < 0 || *errorRate >= 1)
	{
		fprintf(stderr, "Error rate must be between 0 and less than 1\n");
		exit(-1);
	}

	return status;
}






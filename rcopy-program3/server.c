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
#include <signal.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"
#include "srej.h"
#include <bits/waitflags.h>

#define MAX_PACK_LEN 1500
#define MAX_BUFF_SIZE 1400

typedef enum State STATE;

enum State
{
	START, FILENAME, SEND_DATA, WAIT_ON_ACK, TIMEOUT_ON_ACK, WAIT_ON_EOF_ACK,
	TIMEOUT_ON_EOF_ACK, DONE
};

void serverTransfer(int serverSock);
void processClient(int32_t serverSock, uint8_t *buff, int32_t recvLen,
	Connection *client);
STATE filename(Connection *client, uint8_t *buff, int32_t recvLen,
	int32_t *dataFile, int32_t *buffSize);
STATE sendData(Connection *client, uint8_t *packet, int32_t *packetLen,
	int32_t dataFile, int32_t buffSize, uint32_t *seqNum);
STATE waitOnAck(Connection *client);
STATE timeoutOnAck(Connection *client, uint8_t *packet, int32_t packetLen);
STATE waitOnEofAck(Connection *client);
STATE timeoutOnEofAck(Connection *client, uint8_t *packet, int32_t packetLen);
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);
void reapZombies(int sig);

int main ( int argc, char *argv[]  )
{ 
	int serverSock = 0;				
	int portNumber = 0;
	float errorRate = 0;

	checkArgs(argc, argv, &errorRate, &portNumber);

	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
	
	serverSock = udpServerSetup(portNumber);
	
	serverTransfer(serverSock);

	close(serverSock);
	
	return 0;
}

void serverTransfer(int serverSock)
{
	// This function is the main loop for the server, it waits for clients
	// to connect and processes their requests in a forked child process.
	pid_t pid = 0;
	uint8_t buff[MAX_PACK_LEN];
	Connection * client = (Connection *) calloc(1, sizeof(Connection));
	uint8_t flag = 0;
	uint32_t seqNum = 0;
	int32_t recvLen = 0;

	// struct sockaddr_in6 client;		
	// int clientAddrLen = sizeof(client);	
	
	// clean up forked children
	signal(SIGCHLD, reapZombies);

	while (1)
	{
		// block waiting for new client
		recvLen = recvBuff(buff, MAX_PACK_LEN, serverSock, client, &flag, &seqNum);
		// recvLen = safeRecvfrom(socketNum, buff, sizeof(buff), 0, (struct sockaddr *)&client, &clientAddrLen);
		if (recvLen != CRC_ERROR)
		{
			if ((pid = fork()) < 0)
			{
				perror("fork failed");
				exit(-1);
			}
			if (pid == 0)
			{
				// child process
				printf("Child fork() - child pid: %d\n", getpid());
				processClient(serverSock, buff, recvLen, client);
				exit(0);
			}
		}
	}
}

void processClient(int32_t serverSock, uint8_t *buff, int32_t recvLen, Connection *client)
{
	// Process each new clients request
	STATE state = START;
	int32_t dataFile = 0;
	int32_t packetLen = 0;
	uint8_t packet[MAX_PACK_LEN];
	int32_t buffSize = 0;
	uint32_t seqNum = START_SEQ_NUM;

	while (state != DONE)
	{
		switch (state)
		{
			case START:
				state = FILENAME;
				break;
			case FILENAME:
				state = filename(client, buff, recvLen, &dataFile, &buffSize);
				break;
			case SEND_DATA:
				state = sendData(client, packet, &packetLen, dataFile, buffSize, &seqNum);
				break;
			case WAIT_ON_ACK:
				state = waitOnAck(client);
				break;
			case TIMEOUT_ON_ACK:
				state = timeoutOnAck(client, packet, packetLen);
				break;
			case WAIT_ON_EOF_ACK:
				state = waitOnEofAck(client);
				break;
			case TIMEOUT_ON_EOF_ACK:
				state = timeoutOnEofAck(client, packet, packetLen);
				break;
			case DONE:
				break;
			default:
				printf("ERROR - In default state (processClient)\n");
				state = DONE;
				break;
		}
	}
}

STATE filename(Connection *client, uint8_t *buff, int32_t recvLen,
	int32_t *dataFile, int32_t *buffSize)
{
	uint8_t response[1];
	char fname[MAX_PACK_LEN];	// <~!*> check the length here
	STATE retVal = DONE;

	// extract buffer size used for sending data and filename
	memcpy(buffSize, buff, BUFF_SIZE);
	*buffSize = ntohl(*buffSize);
	memcpy(fname, &buff[sizeof(*buffSize)], recvLen - BUFF_SIZE);

	// create client socket for each particular client
	client->socketNum = safeGetUdpSocket();

	if (((*dataFile) = open(fname, O_RDONLY)) < 0)
	{
		sendBuff(response, 0, client, FNAME_BAD, 0, buff);
		retVal = DONE;
	}
	else
	{
		sendBuff(response, 0, client, FNAME_OK, 0, buff);
		retVal = SEND_DATA;
	}
	return retVal;
}

STATE sendData(Connection *client, uint8_t *packet, int32_t *packetLen,
	int32_t dataFile, int32_t buffSize, uint32_t *seqNum)
{
	uint8_t dataBuff[MAX_BUFF_SIZE];
	int32_t lenRead = 0;
	STATE retVal = DONE;

	lenRead = read(dataFile, dataBuff, buffSize);

	switch (lenRead)
	{
		case -1:
			perror("sendData, read error");
			retVal = DONE;
			break;
		case 0:
			// end of file, send EOF packet
			(*packetLen) = sendBuff(dataBuff, 1, client, END_OF_FILE, *seqNum, packet);
			retVal = WAIT_ON_EOF_ACK;
			break;
		default:
			// send data packet
			(*packetLen) = sendBuff(dataBuff, lenRead, client, DATA, *seqNum, packet);
			retVal = WAIT_ON_ACK;
			break;
	}
	return retVal;
}

STATE waitOnAck(Connection *client)
{
	// Wait for ACK from client
	STATE retVal = DONE;
	uint32_t crcCheck = 0;
	uint8_t buff[MAX_PACK_LEN];
	int32_t len = MAX_PACK_LEN;
	uint8_t flag = 0;
	uint32_t seqNum = 0;
	static int retryCnt = 0;

	if ((retVal = processSelect(client, &retryCnt, TIMEOUT_ON_EOF_ACK, DONE, DONE)) == DONE)
	{
		crcCheck = recvBuff(buff, len, client->socketNum, client, &flag, &seqNum);
		// if crc error ignore packet
		if (crcCheck == CRC_ERROR)
		{
			retVal = WAIT_ON_EOF_ACK;
		}
		else if (flag != EOF_ACK)
		{
			printf("In wait_on_eof_ack but its not an EOF_ACK flag (this should never happen) is: %d\n", flag);
			retVal = DONE;
		}
		else
		{
			printf("File transfer completed successfully.\n");
			retVal = DONE;
		}
	}
	return retVal;
}

STATE waitOnAck(Connection *client)
{
	STATE retVal = DONE;
	uint32_t crcCheck = 0;
	uint8_t buff[MAX_PACK_LEN];
	int32_t len = MAX_PACK_LEN;
	uint8_t flag = 0;
	uint32_t seqNum = 0;
	static int retryCnt = 0;

	if ((retVal = processSelect(client, &retryCnt, TIMEOUT_ON_ACK, SEND_DATA, DONE)) == SEND_DATA)
	{
		crcCheck = recvBuff(buff, len, client->socketNum, client, &flag, &seqNum);
		// if crc error ignore packet
		if (crcCheck == CRC_ERROR)
		{
			retVal = WAIT_ON_ACK;
		}
		else if (flag != ACK)
		{
			printf("In wait_on_ack but its not an ACK flag (this should never happen) is: %d\n", flag);
			retVal = DONE;
		}
	}
	return retVal;
}

STATE waitOnEofAck(Connection *client)
{
	STATE retVal = DONE;
	uint32_t crcCheck = 0;
	uint8_t buff[MAX_PACK_LEN];
	int32_t len = MAX_PACK_LEN;
	uint8_t flag = 0;
	uint32_t seqNum = 0;
	static int retryCnt = 0;

	if ((retVal = processSelect(client, &retryCnt, TIMEOUT_ON_EOF_ACK, DONE, DONE)) == DONE)
	{
		crcCheck = recvBuff(buff, len, client->socketNum, client, &flag, &seqNum);
		// if crc error ignore packet
		if (crcCheck == CRC_ERROR)
		{
			retVal = WAIT_ON_EOF_ACK;
		}
		else if (flag != EOF_ACK)
		{
			printf("In wait_on_eof_ack but its not an EOF_ACK flag (this should never happen) is: %d\n", flag);
			retVal = DONE;
		}
		else
		{
			printf("File transfer completed successfully.\n");
			retVal = DONE;
		}
	}
	return retVal;
}

STATE timeoutOnAck(Connection *client, uint8_t *packet, int32_t packetLen)
{
	safeSendTo(packet, packetLen, client);

	return WAIT_ON_ACK;
}

STATE timeoutOnEofAck(Connection *client, uint8_t *packet, int32_t packetLen)
{
	safeSendTo(packet, packetLen, client);

	return WAIT_ON_EOF_ACK;
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

// SIGCHLD handler to reap zombies
void reapZombies(int sig)
{
	int status = 0;
	while (waitpid(-1, &status, WNOHANG) > 0) {}
}

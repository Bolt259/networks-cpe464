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
#include <sys/wait.h>
#include <bits/waitflags.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"
#include "srej.h"
#include "window.h"

#define MAX_PACK_LEN 1500
#define MAX_PAYLOAD 1400

typedef enum State STATE;

enum State
{
	START,
	FILENAME,
	SEND_DATA,
	POLL_ACK_SREJ,
	WAIT_ON_ACK_SREJ,
	TIMEOUT_ON_ACK,
	WAIT_ON_EOF_ACK,
	TIMEOUT_ON_EOF_ACK,
	DONE
};

void serverTransfer(int serverSock);
void processClient(int32_t serverSock, uint8_t *buff, int32_t recvLen, Connection *client);
STATE filename(Connection *client, uint8_t *buff, int32_t recvLen, int32_t *dataFile, int32_t *buffSize);
STATE sendData(Connection *client, uint8_t *packet, int32_t *packetLen, int32_t dataFile, int32_t buffSize, uint32_t *seqNum);
STATE pollAckSrej(Connection *client, uint8_t *packet, int32_t *packetLen, uint32_t *seqNum);
STATE waitOnAckSrej(Connection *client, uint32_t *seqNum);
STATE timeoutOnAck(Connection *client, uint8_t *packet, int32_t packetLen, uint32_t *seqNum);
STATE waitOnEofAck(Connection *client);
STATE timeoutOnEofAck(Connection *client, uint8_t *packet, int32_t packetLen, uint32_t *seqNum);
int checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);
void reapZombies(int sig);

int main(int argc, char *argv[])
{
	int serverSock = 0;
	int portNumber = 0;
	float errorRate = 0;

	checkArgs(argc, argv, &errorRate, &portNumber);

	serverSock = udpServerSetup(portNumber);

	sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON); // TODO: turn RSEED_ON for turn in

	serverTransfer(serverSock);

	close(serverSock);

	return 0;
}

void serverTransfer(int serverSock)
{
	// This function is the main loop for the server, it waits for clients
	// to connect and processes their requests in a forked child process.
	pid_t pid = 0;
	uint8_t buff[MAX_PACK_LEN] = {0};
	Connection *client = (Connection *)calloc(1, sizeof(Connection));
	client->addrLen = sizeof(client->remote); // this line took me probably 7 hours to add and fix AHHHHHHH
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
				//~!*
				printf("Press Enter to continue into child process...\n");
				getchar(); // wait for user input to debug

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
	uint8_t packet[MAX_PACK_LEN] = {0};
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
		case POLL_ACK_SREJ:
			state = pollAckSrej(client, packet, &packetLen, &seqNum);
			break;
		case WAIT_ON_ACK_SREJ:
			state = waitOnAckSrej(client, &seqNum);
			break;
		case TIMEOUT_ON_ACK:
			state = timeoutOnAck(client, packet, packetLen, &seqNum);
			break;
		case WAIT_ON_EOF_ACK:
			state = waitOnEofAck(client);
			break;
		case TIMEOUT_ON_EOF_ACK:
			state = timeoutOnEofAck(client, packet, packetLen, &seqNum);
			break;
		case DONE:
			if (dataFile > 0)
			{
				close(dataFile);
			}
			freeWindow();
			if (client->socketNum > 0)
			{
				close(client->socketNum);
			}
			free(client);	// free each child's Connection struct
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
	char fname[MAX_FNAME_LEN] = {0};
	STATE retVal = DONE;
	uint32_t winSize = 0;

	if (DEBUG_FLAG && ((recvLen - BUFF_SIZE) > 100))
	{
		fprintf(stderr, "FNAME_ERROR: filename is greater than 100 characters, this should never happen!\n");
		return DONE;
	}

	if (client->addrLen > sizeof(client->remote))
	{
		fprintf(stderr, "Error: client addrlen from recvfrom call exceeds expected size.\n");
		return DONE;
		// ~!* maybe free(client) here?
	}

	// extract window size and number of packets in flight
	memcpy(&winSize, buff, BUFF_SIZE);
	winSize = ntohl(winSize);
	memcpy(buffSize, &buff[BUFF_SIZE], BUFF_SIZE);
	*buffSize = ntohl(*buffSize);

	if (DEBUG_FLAG)
	{
		printf(
			"\n{DEBUG} Received packet size: %d\
			\n{DEBUG} Received buffSize: %d\n\
			\n{DEBUG} Received window size: %d\n\
			{DEBUG} Zero client sockNum for server to set: %d\n\
			{DEBUG} Client port: %d\n",
			recvLen, *buffSize, winSize, client->socketNum, client->remote.sin6_port);
	}

	if (recvLen < (2 * BUFF_SIZE) || recvLen > MAX_PACK_LEN)
	{
		fprintf(stderr, "FNAME_ERROR: recvLen is less than 8 bytes or greater than %d bytes, this should never happen!\n", MAX_PACK_LEN);
		return DONE;
	}
	memcpy(fname, &buff[2 * BUFF_SIZE], recvLen - BUFF_SIZE); // <~!*> (recvLen - 2 * BUFF_SIZE) should never be larger than MAX_FNAME_LEN
	fname[recvLen - WIN_BUFF_LEN] = '\0';

	//~!* - create client socket for each particular client within child server
	client->socketNum = safeGetUdpSocket();

	if (DEBUG_FLAG)
	{
		printf("{DEBUG} New client sockNum for server to use: %d\n", client->socketNum);
	}

	//~!*
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
		// fallback for IPv4-mapped IPv6 addresses
		addrPtr = &((struct sockaddr_in *)&client->remote)->sin_addr;
		port = ntohs(((struct sockaddr_in *)&client->remote)->sin_port);
	}

	inet_ntop(AF_INET6, addrPtr, addrStr, sizeof(addrStr));
	printf("\n{DEBUG} Client connected from [%s]:%d\n\n", addrStr, port);
	//~!*

	// if fname found send fname ok flag, else send fname bad flag
	if (((*dataFile) = open(fname, O_RDONLY)) < 0)
	{
		sendBuff(response, 0, client, FNAME_BAD, 0, buff);
		retVal = DONE;
	}
	else
	{
		sendBuff(response, 0, client, FNAME_OK, 0, buff);
		initWindow(winSize, *buffSize);
		retVal = SEND_DATA;
	}
	return retVal;
}

STATE sendData(Connection *client, uint8_t *packet, int32_t *packetLen,
               int32_t dataFile, int32_t buffSize, uint32_t *seqNum)
{
    uint8_t dataBuff[MAX_PAYLOAD] = {0};
    int32_t lenRead = read(dataFile, dataBuff, buffSize);
	if (lenRead == 0)
	{
		// EOF reached
		*packetLen = sendBuff(dataBuff, 1, client, END_OF_FILE, *seqNum, packet);
		(*seqNum)++;
		return WAIT_ON_EOF_ACK;
	}
	else if (lenRead < 0)
	{
		perror("sendData, read on file error");
		return DONE;
	}

	addPane(dataBuff, lenRead, *seqNum);
	*packetLen = sendBuff(dataBuff, lenRead, client, DATA, *seqNum, packet);
	(*seqNum)++;

	return POLL_ACK_SREJ;
}

STATE pollAckSrej(Connection *client, uint8_t *packet, int32_t *packetLen, uint32_t *seqNum)
{
	// non blocking poll just for server to check if rcopy has sent anything
	// and adjust seqNum as needed
	if (selectCall(client->socketNum, 0, 0) == 1)
	{
		uint8_t flag = 0;
		uint32_t ackSeqNum = 0;
		int32_t recvLen = recvBuff(packet, MAX_PACK_LEN, client->socketNum, client, &flag, &ackSeqNum);
		if (recvLen > 0)
		{
			if (flag == ACK_RR)
			{
				markPaneAck(ackSeqNum);
				slideWindow(ackSeqNum + 1);
				*seqNum = ackSeqNum + 1;	// update server state global to next packet
			}
			else if (flag == SREJ)
			{
				*seqNum = ackSeqNum;	// tell sendData to resend this seqNum
			}
		}
	}
	if (windowOpen())
	{
		return SEND_DATA;
	}
	else
	{
		return WAIT_ON_ACK_SREJ;
	}
}

STATE waitOnEofAck(Connection *client)
{
	// wait for ACK_RR from client
	STATE retVal = DONE;
	uint32_t crcCheck = 0;
	uint8_t buff[MAX_PACK_LEN] = {0};
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
		else if (flag == ACK_RR)
		{
			markPaneAck(seqNum);
			slideWindow(seqNum + 1);
			retVal = SEND_DATA;	// do this just in case rcopy is ACKing past packets
		}
		else if (flag == SREJ)
		{
			int32_t sentPacketLen = resendPane(client, SREJ_DATA, seqNum, buff);
			if (sentPacketLen < 0)
			{
				fprintf(stderr, "Error: Failed to resend pane on SREJ within waitOnEofAck.\n");
				return DONE;
			}
			retVal = WAIT_ON_EOF_ACK;
		}
		else if (flag == EOF_ACK)
		{
			printf("File transfer completed successfully.\n");
			retVal = DONE;
		}
	}
	return retVal;
}

STATE waitOnAckSrej(Connection *client, uint32_t *seqNum)
{
	STATE retVal = DONE;
	uint32_t crcCheck = 0;
	uint8_t buff[MAX_PACK_LEN] = {0};
	int32_t len = MAX_PACK_LEN;
	uint8_t flag = 0;
	uint32_t ackSeqNum = 0;
	static int retryCnt = 0;

	if ((retVal = processSelect(client, &retryCnt, TIMEOUT_ON_ACK, SEND_DATA, DONE)) == SEND_DATA)
	{
		crcCheck = recvBuff(buff, len, client->socketNum, client, &flag, &ackSeqNum);
		// if crc error ignore packet
		if (crcCheck == CRC_ERROR)
		{
			retVal = WAIT_ON_ACK_SREJ;
		}
		else if (flag == ACK_RR)
		{
			markPaneAck(ackSeqNum);
			slideWindow(ackSeqNum + 1);
			*seqNum = ackSeqNum + 1; // update server state global to next packet
		}
		else if (flag == SREJ)
		{
			*seqNum = ackSeqNum;
		}
		else if (flag != ACK_RR && flag != SREJ)
		{
			printf("In wait_on_ack but its not an ACK_RR flag (this should never happen) is: %d\n", flag);
			retVal = DONE;
		}
	}
	return retVal;
}

STATE timeoutOnAck(Connection *client, uint8_t *packet, int32_t packetLen, uint32_t *seqNum)
{
	int32_t sentPacketLen = resendPane(client, TIMEOUT_DATA, *seqNum, packet);
	if (sentPacketLen < 0)
	{
		fprintf(stderr, "Error: Failed to resend pane on timeout.\n");
		return DONE;
	}
	
	return WAIT_ON_ACK_SREJ;
}

STATE timeoutOnEofAck(Connection *client, uint8_t *packet, int32_t packetLen, uint32_t *seqNum)
{
	int32_t sentPacketLen = resendPane(client, TIMEOUT_DATA, *seqNum, packet);
	if (sentPacketLen < 0)
	{
		fprintf(stderr, "Error: Failed to resend pane on timeout.\n");
		return DONE;
	}

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

// SIGCHLD handler to reap zombies
void reapZombies(int sig)
{
	int status = 0;
	while (waitpid(-1, &status, WNOHANG) > 0);
}

/******************************************************************************
 * myClient.c
 *
 * Writen by Prof. Smith, updated Jan 2023
 * Use at your own risk.
 *
 *****************************************************************************/
/*
Modified by Lukas Shipley
*/

#include "shared.h"
#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"
#include "handleTable.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

// void sendToServer(int socketNum);
int readFromStdin(uint8_t *buffer);
void checkArgs(int argc, char *argv[]);

void clientControl(int serverSocket);
void processStdin(int serverSocket);
void processMsgFromServer(int serverSocket);
void regHandle(int serverSocket, char *handle);

int main(int argc, char *argv[])
{
	int socketNum = 0; // socket descriptor

	checkArgs(argc, argv);

	// set up the TCP Client socket
	socketNum = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	// register the handle with the server
	regHandle(socketNum, argv[1]);
	
	clientControl(socketNum);
	
	close(socketNum);
	
	return 0;
}

void regHandle(int serverSocket, char *handle)
{
	// check size of handle
	if (handle == NULL || strlen(handle) > MAX_HANDLE_LENGTH)
	{
		fprintf(stderr, "Error: Client handle length exceeds maximum allowed length or handle is somehow empty\n");
		exit(-1);
	}
	uint8_t buffer[MAXBUF];
	int handleLen = strlen(handle);
	
	buffer[0] = 1;						   // flag for registering handle
	buffer[1] = handleLen;				   // length of handle
	memcpy(&buffer[2], handle, handleLen); // copy handle into buffer
	
	// send the message to the server
	int sent = sendPDU(serverSocket, buffer, handleLen + 2);
	if (sent < 0)
	{
		perror("sendPDU call");
		exit(-1);
	}
	printf("Socket %d: Sent registration message: %.*s\n", serverSocket, handleLen, &buffer[2]);
}

void clientControl(int serverSocket)
{
	int readySocket = 0;
	int userInputFlag = 1;

	setupPollSet();
	addToPollSet(serverSocket);
	addToPollSet(STDIN_FILENO);

	while (1)
	{
		// wait for input from stdin or the server
		if (userInputFlag)
		{
			// printf("Enter data: ");
			// fflush(stdout); // Ensure the message is displayed immediately
			userInputFlag = 0;
		}

		readySocket = pollCall(POLL_WAIT_FOREVER);

		if (readySocket < 0)
		{
			perror("poll call");
			exit(-1);
		}

		// check which socket is ready
		if (readySocket == STDIN_FILENO)
		{
			processStdin(serverSocket);
		}
		else if (readySocket == serverSocket)
		{
			processMsgFromServer(serverSocket);
			userInputFlag = 1;
		}
		else
		{
			printf("Error: ready socket %d is not valid\n", readySocket);
			exit(-1);
		}
	}
}

// void processStdin(int serverSocket)
// {
// 	// this function is called when stdin is ready for read
// 	sendToServer(serverSocket);
// }

void processMsgFromServer(int serverSocket)
{
	// this function is called when the server socket is ready for read from client
	uint8_t buffer[MAXBUF];
	int messageLen = recvPDU(serverSocket, buffer, MAXBUF);

	// length check
	if (messageLen == 0)
	{
		printf("Server has terminated\n");
		exit(0);
	}
	else if (messageLen < 0)
	{
		perror("recvPDU call");
		exit(-1);
	}

	// identify the type of message based on the flag
	u_int8_t flag = buffer[0];

	switch (flag)
	{
	case 2:
	{
		printf("Server: Handle accepted\n");
		break;
	}
	case 3:
	{
		printf("Server: Handle already exists or handle table is full\n");
		break;
	}
	default:
	{
		printf("Invalid flag from server\n");
		exit(-1);
		break;
	}
	}

	// printf("Socket %d: Byte recv: %d message: %s\n", serverSocket, messageLen, buffer);
}

void processStdin(int socketNum)
{
	uint8_t buffer[MAXBUF]; // data buffer
	int sendLen = 0;		// amount of data to send
	int sent = 0;			// actual amount of data sent/* get the data and send it   */

	// get the data from stdin
	sendLen = readFromStdin(buffer);
	printf("read: %s string len: %d (including null)\n", buffer, sendLen);

	// send to server
	sent = sendPDU(socketNum, buffer, sendLen);
	if (sent < 0)
	{
		perror("sendPDU call");
		exit(-1);
	}

	printf("Socket:%d: Sent, Length: %d msg: %s\n", socketNum, sent, buffer);
}

int readFromStdin(uint8_t *buffer)
{
	char aChar = 0;
	int inputLen = 0;

	// Important you don't input more characters than you have space
	buffer[0] = '\0';
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

void checkArgs(int argc, char *argv[])
{
	/* check command line arguments  */
	if (argc != 4)
	{
		printf("usage: %s handle host-name port-number \n", argv[0]);
		exit(1);
	}
}

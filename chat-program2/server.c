/******************************************************************************
 * myServer.c
 *
 * Writen by Prof. Smith, updated Jan 2023
 * Use at your own risk.
 *
 *****************************************************************************/
/*
Modified by Lukas Shipley
*/

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
#include <stdint.h>
#include <errno.h>

#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

// void recvFromClient(int clientSocket);
int checkArgs(int argc, char *argv[]);

void serverControl(int serverSocket);
void addNewSocket(int serverSocket);
void processClient(int clientSocket);
static void cleanupClient(int clientSocket, const char *msg, const char *syscall);

int main(int argc, char *argv[])
{
	int mainServerSocket = 0; // socket descriptor for the server socket
	int portNumber = 0;

	portNumber = checkArgs(argc, argv);

	// create the server socket
	mainServerSocket = tcpServerSetup(portNumber);

	serverControl(mainServerSocket);

	close(mainServerSocket);

	return 0;
}

void serverControl(int serverSocket)
{
	// this function is called when the server socket is ready for read
	// setup polling and add mainServerSocket to the poll set
	setupPollSet();
	addToPollSet(serverSocket);

	while (1)
	{
		int readySocket = pollCall(POLL_WAIT_FOREVER);

		if (readySocket == serverSocket)
		{
			addNewSocket(serverSocket); // handle new client
		}
		else if (readySocket > 0)
		{
			processClient(readySocket); // handle existing client
		}
		else
		{
			perror("poll call");
			exit(-1);
		}
	}
}

void addNewSocket(int serverSocket)
{
	int clientSocket = tcpAccept(serverSocket, DEBUG_FLAG);
	if (clientSocket < 0)
	{
		perror("addNewSocket: tcpAccept failed");
		return; // exit the function if accept fails
	}

	addToPollSet(clientSocket); // add the new client socket to the poll set
								// printf("New client added to poll set: %d\n", clientSocket);
}

static void cleanupClient(int clientSocket, const char *msg, const char *syscall)
{
	removeFromPollSet(clientSocket);
	close(clientSocket);
	fprintf(stderr, "Socket %d: %s\n", clientSocket, msg);
	perror(syscall);
}

// void processClient(int clientSocket)
// {
// 	// this function is called when the client socket is ready for read
// 	recvFromClient(clientSocket);
// }

void processClient(int clientSocket)
{
	u_int8_t dataBuffer[MAXBUF];
	int messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);

	if (messageLen > 0)
	{
		printf("Message received on socket %d, length: %d, Data: %s\n", clientSocket, messageLen, dataBuffer);

		messageLen = sendPDU(clientSocket, dataBuffer, messageLen);
		if (messageLen < 0)
		{
			cleanupClient(clientSocket, "Error sending message", "send call");
			return;
		}
		printf("Message sent on socket %d: %d bytes, text: %s\n", clientSocket, messageLen, dataBuffer);
	}
	else
	{
		const char *reason = (messageLen == 0)
								 ? "Connection closed by client"
								 : "Error receiving message";
		cleanupClient(clientSocket, reason, "recv call");
	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;

	if (argc > 2)
	{
		fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
		exit(-1);
	}

	if (argc == 2)
	{
		portNumber = atoi(argv[1]);
	}

	return portNumber;
}

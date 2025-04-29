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

#include "shared.h"
#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"
#include "handleTable.h"

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
	removeHandle(clientSocket);
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
	u_int8_t buffer[MAXBUF];
	int packetLen = recvPDU(clientSocket, buffer, MAXBUF);

	// length check
	if (packetLen <= 0)
	{
		const char *reason = (packetLen == 0)
								 ? "Connection closed by client"
								 : "Error receiving message";
		cleanupClient(clientSocket, reason, "recv call");
		return;
	}

	// identify the type of message based on the flag
	u_int8_t flag = buffer[0];
	u_int8_t handle_len = buffer[1];

	if (handle_len > MAX_HANDLE_LENGTH)
	{
		// invalid handle length
		printf("Invalid handle length from socket %d\n", clientSocket);
		cleanupClient(clientSocket, "Invalid handle length", "recv call");
		return;
	}
	char handle[MAX_HANDLE_LENGTH + 1];	// +1 for null terminator
	memcpy(handle, &buffer[2], handle_len);
	handle[handle_len] = '\0'; // null terminate just in case

	switch (flag)
	{
		case 1:
		{
			// clients initial packet to the server registering their handle
			printf("Registering handle: %s on socket %d\n", handle, clientSocket);

			// try to add handle
			if (addHandle(handle, clientSocket) == 0)
			{
				// success: send back flag 2
				u_int8_t replyFlag[1];
				replyFlag[0] = 2;
				if (sendPDU(clientSocket, replyFlag, 1) < 0)
				{
					cleanupClient(clientSocket, "Error sending handle accepted (flag=2)", "sendPDU");
					return;
				}
			}
			else
			{
				// dupe or table full: send back flag 3:
				u_int8_t replyFlag[1];
				replyFlag[0] = 3;
				sendPDU(clientSocket, replyFlag, 1);
				cleanupClient(clientSocket, "Handle already exists or handle table is full (flag=3)", "sendPDU");
			}
			break;
		}
		case 4:
		{
			// client sending broadcast message to server
			break;
		}
		case 5:
		{
			// client sending normal message %M
			int receiverSocket = lookupHandle(handle);
			// if (receiverSocket < 0)
			// {
			// 	cleanupClient(clientSocket, "Receiver handle not found", "lookupHandle");
			// 	return;
			// }

			// send error flag 7 back to client if receiver handle not found
			if (receiverSocket < 0)
			{
				u_int8_t packet[MAX_PACKET_SIZE];
				int packetSize = buildErrPacket(packet, handle);
				if (packetSize < 0)
				{
					fprintf(stderr, "Error building error packet\n");
					return;
				}
				if (sendPDU(clientSocket, packet, packetSize) < 0)
				{
					cleanupClient(clientSocket, "Error sending error packet to client", "sendPDU");
					return;
				}
				return;
			}

			// forward message to receiver
			if (sendPDU(receiverSocket, buffer, packetLen) < 0)
			{
				cleanupClient(clientSocket, "Error sending message to receiver", "sendPDU");
				return;
			}
			break;
		}
		default:
		{
			printf("Invalid flag from socket %d\n", clientSocket);
			cleanupClient(clientSocket, "Invalid flag", "recv call");
			return;
		}
	}


	// PREVIOUS CODE ARTIFACT
	// printf("Message received on socket %d, length: %d, Data: %s\n", clientSocket, packetLen, buffer);

	// packetLen = sendPDU(clientSocket, buffer, packetLen);
	// if (packetLen < 0)
	// {
	// 	cleanupClient(clientSocket, "Error sending message", "send call");
	// 	return;
	// }
	// printf("Message sent on socket %d: %d bytes, text: %s\n", clientSocket, packetLen, buffer);
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

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
int buildHandleListReq(u_int8_t packet[MAX_HANDLE_LENGTH + 2], char *handle);

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
	char srcHandle[MAX_HANDLE_LENGTH + 1];	// +1 for null terminator
	memcpy(srcHandle, &buffer[2], handle_len);
	srcHandle[handle_len] = '\0'; // null terminate just in case
	
	// DEBUGGINHG ONLY
	printf("FLAG: %d\n", flag);
	printf("HANDLE LENGTH: %d\n", handle_len);
	printf("SRC_HANDLE: %s\n", srcHandle);
	// DEBUGGING ONLY

	switch ((int)flag)
	{
		case 1:
		{
			// clients initial packet to the server registering their handle
			printf("Registering handle: %s on socket %d\n", srcHandle, clientSocket);

			// try to add handle
			if (addHandle(srcHandle, clientSocket) == 0)
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
		case 4:	// broadcast
		{
			char **handleList = NULL;
			int totalClients = getHandles(&handleList);
			if (totalClients < 0)
			{
				fprintf(stderr, "Error getting handles from handle table\n");
				return;
			}

			for (int i = 0; i < totalClients; i++)
			{
				int receiverSocket = lookupHandle(handleList[i]);
				if (receiverSocket < 0)
				{
					fprintf(stderr, "Error looking up handle %s\n", handleList[i]);
					continue;
				}

				if (receiverSocket == clientSocket)
				{
					continue; // don't send to broadcast sender
				}

				if (sendPDU(receiverSocket, buffer, packetLen) < 0)
				{
					cleanupClient(clientSocket, "Error sending broadcast message", "sendPDU");
					continue;
				}
				printf("Broadcasting message to %s\n", handleList[i]);	// DEBUGGING ONLY
			}

			freeHandleList(handleList);
			break;
		}
		case 5:	// message
		{
			int destHandleLen = getHandleFromBuffer(buffer, srcHandle, 1);
			if (destHandleLen > MAX_HANDLE_LENGTH)
			{
				// invalid handle length
				fprintf(stderr, "Invalid destination handle length from socket %d in message (processClient)\n", clientSocket);
				return;
			}
			int receiverSocket = lookupHandle(srcHandle);
			// if (receiverSocket < 0)
			// {
			// 	cleanupClient(clientSocket, "Receiver handle not found", "lookupHandle");
			// 	return;
			// }

			// send error flag 7 back to client if receiver handle not found
			if (receiverSocket < 0)
			{
				u_int8_t packet[MAX_PACKET_SIZE];
				int packetSize = buildErrPacket(packet, srcHandle);
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

			// DEBUGGING ONLY
			printPacket(buffer, packetLen, 1);
			// DEBUGGING ONLY

			// forward message to receiver
			if (sendPDU(receiverSocket, buffer, packetLen) < 0)
			{
				cleanupClient(clientSocket, "Error sending message to receiver", "sendPDU");
				return;
			}
			break;
		}
		case 6:	// multicast
		{
			uint8_t numDestHandlesIdx = 2 + handle_len;
			uint8_t numDestHandles = buffer[numDestHandlesIdx];
			if (numDestHandles > MAX_DEST_HANDLES)
			{
				fprintf(stderr, "Invalid number of destination handles from socket %d\n", clientSocket);
				return;
			}

			int idx = numDestHandlesIdx + 1; // skip to the first destination handle

			// forward message to each destination handle
			for (int i = 0; i < numDestHandles; i++)
			{
				uint8_t destHandleLen = buffer[idx];
				char destHandle[MAX_HANDLE_LENGTH + 1];

				if (destHandleLen > MAX_HANDLE_LENGTH)
				{
					fprintf(stderr, "Invalid destination handle length from socket %d in multicast (processClient)\n", clientSocket);
					return;
				}

				memcpy(destHandle, &buffer[idx + 1], destHandleLen);
				destHandle[destHandleLen] = '\0'; // null terminate just in case
				idx += destHandleLen + 1; // skip to the next destination handle

				int receiverSocket = lookupHandle(destHandle);
				if (receiverSocket < 0)
				{
					uint8_t errPacket[MAX_PACKET_SIZE];
					int errPacketLen = buildErrPacket(errPacket, srcHandle);
					if (errPacketLen >= 0)
					{
						if (sendPDU(clientSocket, errPacket, errPacketLen) < 0)
						{
							cleanupClient(clientSocket, "Failed to send error packet", "sendPDU");
						}
					}
					continue; // skip to the next destination handle
				}

				if (sendPDU(receiverSocket, buffer, packetLen) < 0)
				{
					cleanupClient(clientSocket, "Failed to forward multicast message", "sendPDU");
				}
			}
			break;
		}
		case 10: // list
		{
			char **handleList = NULL;
			int totalClients = getHandles(&handleList);
			if (totalClients < 0)
			{
				fprintf(stderr, "Error getting handles from handle table\n");
				return;
			}

			// FLAG 11: send number of clients (4 bytes in network order)
			uint8_t ccountPacket[5];
			ccountPacket[0] = 11;
			uint32_t numClients = htonl(totalClients);
			printf("[DEBUG] Number of clients: %d\n", totalClients);	// DEBUGGING ONLY
			memcpy(&ccountPacket[1], &numClients, sizeof(numClients));

			if (sendPDU(clientSocket, ccountPacket, 5) < 0)
			{
				cleanupClient(clientSocket, "Failed to send client count", "sendPDU");
				freeHandleList(handleList);
				return;
			}

			// FLAG 12: send each handle in list
			for (int i = 0; i < totalClients; i++)
			{
				uint8_t handlePacket[MAX_HANDLE_LENGTH + 2];	// +2 for flag and length
				int handlePacketLen = buildHandleListReq(handlePacket, handleList[i]);

				printPacket(handlePacket, handlePacketLen, 0);	// DEBUGGING ONLY

				if (sendPDU(clientSocket, handlePacket, handlePacketLen) < 0)
				{
					cleanupClient(clientSocket, "Failed to send handle (flag 12)", "sendPDU");
					freeHandleList(handleList);
					return;
				}
			}

			// FLAG 13: finish
			uint8_t donePacket[1] = {13};
			if (sendPDU(clientSocket, donePacket, 1) < 0)
			{
				cleanupClient(clientSocket, "Failed to send done packet (flag 13)", "sendPDU");
			}

			freeHandleList(handleList);
			break;
		}
		default:
		{
			printf("Invalid flag from socket %d\n", clientSocket);
			cleanupClient(clientSocket, "Invalid flag", "recv call");
			return;
		}
	}
}

int buildHandleListReq(u_int8_t packet[MAX_HANDLE_LENGTH + 2], char *handle)
{
	// check handle
	if (handle == NULL || strlen(handle) == 0)
	{
		fprintf(stderr, "Error: Handle is empty (buildHandleListReq)\n");
		return -1;
	}

	int idx = 0;
	packet[idx++] = 12;
	uint8_t handleLen = strlen(handle);
	packet[1] = handleLen;
	memcpy(&packet[2], handle, handleLen);
	idx += handleLen;
	return idx;
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

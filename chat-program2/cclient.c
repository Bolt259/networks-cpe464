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

#include <ctype.h> // for tolower()

#include "shared.h"
#include "networks.h"
#include "safeUtil.h"
#include "pollLib.h"
#include "handleTable.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

char clientHandle[MAX_HANDLE_LENGTH + 1]; // global variable for handle

int expectingHandleList = 0; // flag for waiting for rest of handles
int remainingHandles = 0; // number of handles left to be received`

// void sendToServer(int serverSocket);
void checkArgs(int argc, char *argv[]);
int parseUserCmd
(
	char *input,
	uint8_t *flag,
	char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1],
	int *numDestHandles,
	char messageText[MAX_MESSAGE_LENGTH]
);
int readFromStdin(uint8_t *buffer);
void regHandle(int serverSocket, char *handle);
uint8_t getFlag(char cmdType);
int getMsgIdx(uint8_t *buffer, int broadcast);

void clientControl(int serverSocket);
void processStdin(int serverSocket);
void processMsgFromServer(int serverSocket);

int main(int argc, char *argv[])
{
	int serverSocket = 0; // socket descriptor

	checkArgs(argc, argv);

	// set up the TCP Client socket
	serverSocket = tcpClientSetup(argv[2], argv[3], DEBUG_FLAG);

	// register the handle with the server
	regHandle(serverSocket, argv[1]);
	
	clientControl(serverSocket);
	
	close(serverSocket);
	
	return 0;
}

// parseUserCmd parses the user input and extracts the command type, destination handles, and message text
int parseUserCmd
(
	char *input,
	uint8_t *flag,
	char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1],
	int *numDestHandles,
	char messageText[MAX_MESSAGE_LENGTH]
)
{
	char *token = NULL;
	char cmdType;

	if (input == NULL || input[0] != '%')
	{
		fprintf(stderr, "Error: Invalid command, missing %% prefix\n");
		return -1;
	}
	token = strtok(input, " \n");	// tokenize input
	if (token == NULL)
	{
		fprintf(stderr, "Error: Tokenizing command\n");
		return -1;
	}

	cmdType = tolower(token[1]); // command is %M, %C, %B, %L, case-insensitive

	printf("\n[debug]Command type: %c\n", cmdType); ////////////////////////////////////////////DEBUG

	*flag = getFlag(cmdType);
	if (*flag == (uint8_t)-1)
	{
		fprintf(stderr, "Error: Invalid command type: '%c' (in parseUserCmd)\n", cmdType);
		return -1;
	}
	
	// parse the rest according to what is at the int flag pointer
	switch (*flag)
	{
		case 4: // %B [message]
			*numDestHandles = 0;
			token = strtok(NULL, "\n");
			if (token == NULL || strlen(token) > MAX_MESSAGE_LENGTH - 1)
			{
				fprintf(stderr, "Error: No message provided or message exceeds maximum allowed length\n");
				return -1;
			}
			strcpy(messageText, token ? token : "");
			break;

		case 5: // %M destHandle [message]
			token = strtok(NULL, " \n"); // advance to next token
			if (token == NULL)
			{
				fprintf(stderr, "Error: Missing destination handle\n");
				return -1;
			}
			strcpy(destHandles[0], token);
			*numDestHandles = 1;

			token = strtok(NULL, "\n");
			if (token == NULL || strlen(token) > MAX_MESSAGE_LENGTH - 1)
			{
				fprintf(stderr, "Error: No message provided or message exceeds maximum allowed length\n");
				return -1;
			}
			strcpy(messageText, token ? token : "");
			break;
			
		case 6: // %C destHandle1 destHandle2 ... [message]
			token = strtok(NULL, " \n");
			if (token == NULL)
			{
				fprintf(stderr, "Error: Missing number of destination handles\n");
				return -1;
			}
			*numDestHandles = atoi(token);
			if (*numDestHandles < 2 || *numDestHandles > MAX_DEST_HANDLES)
			{
				fprintf(stderr, "Error: Number of destination handles must be between 2 and %d\n", MAX_DEST_HANDLES);
				return -1;
			}
			for (int i = 0; i < *numDestHandles; i++)
			{
				token = strtok(NULL, " \n");
				if (token == NULL)
				{
					fprintf(stderr, "Error: Missing destination handle #%d\n", i + 1);
					return -1;
				}
				strcpy(destHandles[i], token);
			}

			token = strtok(NULL, "\n");
			if (token == NULL || strlen(token) > MAX_MESSAGE_LENGTH - 1)
			{
				fprintf(stderr, "Error: No message provided or message exceeds maximum allowed length\n");
				return -1;
			}
			strcpy(messageText, token ? token : "");
			break;

		case 10: // %L
			*numDestHandles = 0;
			strcpy(messageText, "");
			break;

		default:
			fprintf(stderr, "Error: Entered invalid command type (in parseUserCmd)\n");
			return -1;
	}

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

	// set handle in global variable
	strcpy(clientHandle, handle);
	
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
	printf("Socket %d: Clientname: %.*s\n", serverSocket, handleLen, &buffer[2]);
}

void clientControl(int serverSocket)
{
	int readySocket = 0;

	setupPollSet();
	addToPollSet(serverSocket);
	addToPollSet(STDIN_FILENO);

	while (1)
	{
		if (!expectingHandleList)
		{
			printf("$: ");	// ba$h prompt
			fflush(stdout);
		}

		// wait for input from stdin or the server
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
		case 4:
		case 5:
		case 6: // message or broadcast or multicast packet
		{
			char srcHandle[MAX_HANDLE_LENGTH + 1];
			getHandleFromBuffer(buffer, srcHandle, 0);

			// int destHandleLen = getHandleFromBuffer(buffer, srcHandle, 1);

			int msgStartIdx;

			if (flag == 4) msgStartIdx = getMsgIdx(buffer, 1);	// broadcast packet
			else msgStartIdx = getMsgIdx(buffer, 0);

			char *msgText = (char *)&buffer[msgStartIdx];	// >>* CAUTION: message text in packets must be null terminated
			printf("\n%s: %s\n", srcHandle, msgText);
			break;
		}
		case 7: // error handle not found packet
		{
			char badHandle[MAX_HANDLE_LENGTH + 1];
			getHandleFromBuffer(buffer, badHandle, 1);

			printf("\nClient with handle %s does not exist\n", badHandle);
			break;
		}
		case 11: // number of clients packet
		{
			expectingHandleList = 1;
			uint32_t netCnt = 0;
			memcpy(&netCnt, &buffer[1], sizeof(netCnt));
			uint32_t clientCnt = ntohl(netCnt);
			printf("\nNumber of clients: %d\n", clientCnt);
			remainingHandles = (int)clientCnt;
			printf("[DEBUG] Remaining handles init: %d\n", remainingHandles); // DEBUG
			break;
		}
		case 12: // handle list packet
		{
			if (!expectingHandleList)
			{
				fprintf(stderr, "Unexpected handle list packet (flag 12)\n");
				break;
			}

			uint8_t handleLen = buffer[1];
			if (handleLen > MAX_HANDLE_LENGTH)
			{
				fprintf(stderr, "Error: Invalid handle length (processMsgFromServer)\n");
				exit(-1);
			}
			printf("[DEBUG] handle length: %d\n", handleLen); // DEBUG
			printf("[DEBUG] Client handle: %.*s\n", handleLen, &buffer[2]); // DEBUG

			char clientHandle[MAX_HANDLE_LENGTH + 1] = {0};
			memcpy(clientHandle, &buffer[2], handleLen);
			clientHandle[handleLen] = '\0';
			printf("  %s\n", clientHandle);
			printf("[DEBUG] Remaining handles before decrement: %d\n", remainingHandles); // DEBUG
			remainingHandles = (remainingHandles > 0) ? remainingHandles - 1 : 0;
			printf("[DEBUG] Remaining handles after decrement: %d\n", remainingHandles);	
			
			break;
		}
		case 13: // finish
		{
			if (!expectingHandleList)
			{
				fprintf(stderr, "Unexpected finish packet (flag 13)\n");
				break;
			}
			if (remainingHandles != 0)
			{
				fprintf(stderr, "Error: Expected %d handles, but received none\n", remainingHandles);
				exit(-1);
			}
			printf("End of handle list\n"); // DEBUG
			expectingHandleList = 0;
			remainingHandles = 0;
			break;
		}
		default:
		{
			printf("Invalid flag from server %d\n", flag);
			exit(-1);
			break;
		}
	}
}

void processStdin(int serverSocket)
{
	uint8_t userCmd[MAXBUF]; 		// data buffer
	uint8_t packet[MAX_PACKET_SIZE];
	int packetLen = 0;
	// int cmdLen = 0;					// amount of data to send

	// get the data from stdin
	// cmdLen = readFromStdin(userCmd);
	readFromStdin(userCmd);
	// printf("[debugging] read: |%s| string len: %d (including null)\n", userCmd, cmdLen);

	// parse the user cmd
	uint8_t flag = 0;
	char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1];
	int numDestHandles = 0;
	char messageText[MAX_MESSAGE_LENGTH + 1];

	if (parseUserCmd((char *)userCmd, &flag, destHandles, &numDestHandles, messageText) < 0)
	{
		fprintf(stderr, "Please enter a valid command (%%M, %%B, %%C, %%L)\n");
		return;
	}

	// DEBUGGING ONLY
	printf("[debug] Flag value: %d\n", flag);
	printf("[debug] Number of Destination Handles: %d\n", numDestHandles);
	for (int i = 0; i < numDestHandles; i++)
	{
		printf("[debug] Destination Handle %d: %s\n", i + 1, destHandles[i]);
	}
	printf("[debug] Message Text: %s\n", messageText);
	// DEBUGGING ONLY
	
	// decision tree for what packet to build and send to server based on flag
	switch ((int)flag)
	{
		case 5:
		{
			// build message packet
			packetLen = buildMsgPacket(packet, flag, clientHandle, numDestHandles, destHandles, messageText);
			
			// DEBUGGING ONLY
			printPacket(packet, packetLen, 1);
			// DEBUGGING ONLY

			if (sendPDU(serverSocket, packet, packetLen) < 0)
			{
				perror("sendPDU call");
				exit(-1);
			}
			break;
		}
		case 6:
		{
			// build multicast packet
			packetLen = buildMsgPacket(packet, flag, clientHandle, numDestHandles, destHandles, messageText);
			if (sendPDU(serverSocket, packet, packetLen) < 0)
			{
				perror("sendPDU call");
				exit(-1);
			}
			break;
		}
		case 4:
		{
			// build broadcast packet
			packetLen = buildMsgPacket(packet, flag, clientHandle, 0, 0, messageText);
			if (sendPDU(serverSocket, packet, packetLen) < 0)
			{
				perror("sendPDU call");
				exit(-1);
			}
			break;
		}
		case 10:
		{
			// build handle list request packet
			packet[0] = flag;
			if (sendPDU(serverSocket, packet, 1) < 0)
			{
				perror("sendPDU call");
				exit(-1);
			}
			break;
		}
		default:
		{
			fprintf(stderr, "Error: Invalid command type (processStdin)\n");
			return;
		}
	}
	return;
}

// returns the index of the message text in the buffer --> ONLY FOR MESSAGE PACKETS!!
int getMsgIdx(uint8_t *buffer, int broadcast)
{
	int idx = 0;
	uint8_t srcHandleLen = buffer[1];
	idx += 2 + srcHandleLen;	// skip over flag and src handle

	if (broadcast) return idx;	// broadcast packet no dest handles

	uint8_t numDestHandles = buffer[idx];
	idx += 1;

	// skip over destination handles
	for (int i = 0; i < numDestHandles; i++)
	{
		uint8_t destHandleLen = buffer[idx];
		idx += 1 + destHandleLen;
	}
	return idx;
}

// returns flag based on command type
uint8_t getFlag(char cmdType)
{
	if (cmdType == 'm') return 5;
    if (cmdType == 'c') return 6;
    if (cmdType == 'b') return 4;
    if (cmdType == 'l') return 10;
    fprintf(stderr, "Error: Invalid command type (getFlag)\n");
    return -1;
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

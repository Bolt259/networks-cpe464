// Lukas Shipley
// The shared attributes and functions of the client and server

#include "shared.h"

void printPacket(uint8_t *packet, int packetLen, int regular)
{
    int idx = 0;

    // // Extract total length (2 bytes)
    // uint16_t totalLength = (packet[idx] << 8) | packet[idx + 1];
    // idx += 2;

    // Extract flag (1 byte)
    uint8_t flag = packet[idx];
    idx += 1;

    // Extract sender handle length (1 byte)
    uint8_t senderHandleLen = packet[idx];
    idx += 1;

    // Extract sender handle name (senderHandleLen bytes)
    char senderHandle[senderHandleLen + 1];
    memcpy(senderHandle, &packet[idx], senderHandleLen);
    senderHandle[senderHandleLen] = '\0'; // Null-terminate the string
    idx += senderHandleLen;

    // Extract number of destination handles (1 byte)
    uint8_t numDestHandles = packet[idx];
    idx += 1;

    // Extract destination handle length (1 byte)
    uint8_t destHandleLen = packet[idx];
    idx += 1;

    // Extract destination handle name (destHandleLen bytes)
    char destHandle[destHandleLen + 1];
    memcpy(destHandle, &packet[idx], destHandleLen);
    destHandle[destHandleLen] = '\0'; // Null-terminate the string
    idx += destHandleLen;

    // Extract text message (null-terminated string)
    char *messageText = (char *)&packet[idx];

    // Print the packet contents
    printf("Packet Contents:\n");
    printf("----------------\n");
    // printf("Total Length: %d bytes\n", totalLength);
    printf("Flag: %d\n", flag);
    printf("Sender Handle Length: %d\n", senderHandleLen);
    printf("Sender Handle: %s\n", senderHandle);
    if (regular)
    {
        printf("Number of Destination Handles: %d\n", numDestHandles);
        printf("Destination Handle Length: %d\n", destHandleLen);
        printf("Destination Handle: %s\n", destHandle);
        printf("Message Text: %s\n", messageText);
    }
}

// returns the length of the handle copied into handle from buffer[2] the location of src handle in msg packets
uint8_t getHandleFromBuffer(uint8_t *buffer, char handle[MAX_HANDLE_LENGTH + 1], int whichHandle)
{
	uint8_t srcHandleLen = buffer[1];

	if (!whichHandle)
	{
		// src handle
		if (srcHandleLen > MAX_HANDLE_LENGTH)
		{
			fprintf(stderr, "Error: Invalid handle length (getHandleFromBuffer)\n");
			exit(-1);
		}
		memcpy(handle, &buffer[2], srcHandleLen);
		handle[srcHandleLen] = '\0'; // null terminate just in case
		return srcHandleLen;
	}
	else if (whichHandle)
	{
		// dest handle
		int lenDestHandleIdx = 3 + srcHandleLen;
		uint8_t destHandleLen = buffer[lenDestHandleIdx];
		if (destHandleLen > MAX_HANDLE_LENGTH)
		{
			fprintf(stderr, "Error: Invalid handle length (getHandleFromBuffer)\n");
			exit(-1);
		}
		int startDestHandleIdx = lenDestHandleIdx + 1;
		memcpy(handle, &buffer[startDestHandleIdx], destHandleLen);
		handle[destHandleLen] = '\0'; // null terminate just in case
		return destHandleLen;
	}
	return -1;
}

// make it so that all parameters that are unused for a given flag don't affect the packet
// consolidate the error checking and consider breaking this up with helpers
int buildMsgPacket(u_int8_t *packet, u_int8_t flag, char *srcHandle, int numDestHandles, char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1], char *messageText)
{
    // check if handle is valid
    if (srcHandle == NULL || strlen(srcHandle) == 0)
    {
        fprintf(stderr, "Error: Sender handle is empty\n");
        return -1;
    }
    uint8_t srcHandleLen = strlen(srcHandle);

    if (srcHandleLen >= MAX_HANDLE_LENGTH)
    {
        fprintf(stderr, "Error: Sender handle length exceeds maximum allowed length\n");
        return -1;
    }
    if ((flag == 5 && numDestHandles != 1) || (flag == 6 && (numDestHandles < 2 || numDestHandles > MAX_DEST_HANDLES)))
    {
        fprintf(stderr, "Error: Invalid number of destination handles for flag %d\n", flag);
        return -1;
    }
    if ((flag == 5 || flag == 6) && destHandles == NULL)
    {
        fprintf(stderr, "Error: Destination handles are missing for flag %d\n", flag);
        return -1;
    }
    int messageLen = strlen(messageText) + 1; // +1 for null terminator
    if (messageLen >= MAX_MESSAGE_LENGTH)
    {
        fprintf(stderr, "Error: Message length exceeds maximum allowed length\n");
        return -1;
    }

    int idx = 0;

    packet[idx++] = flag;   // use idx as 0 then update it for the next part of packet w ++

    // add sender handle - don't include null terminator
    packet[idx++] = srcHandleLen;
    memcpy(&packet[idx], srcHandle, srcHandleLen);
    idx += srcHandleLen;

    if (flag == 5 || flag == 6) // for %M and %C, must add num of destinations
    {
        packet[idx++] = numDestHandles;
        for (int i = 0; i < numDestHandles; i++)
        {
            uint8_t destHandleLen = strlen(destHandles[i]);
            if (destHandleLen >= MAX_HANDLE_LENGTH)
            {
                fprintf(stderr, "Error: Destination handle length exceeds maximum allowed length\n");
                return -1;
            }
            packet[idx++] = destHandleLen;
            memcpy(&packet[idx], destHandles[i], destHandleLen);
            idx += destHandleLen;
        }
    }

    // add text message including null
    memcpy(&packet[idx], messageText, messageLen);
    idx += messageLen;

    return idx; // returning total length to pass into sendPDU
}

int buildErrPacket(u_int8_t *packet, char *missingHandle)
{
    // check if handle is valid
    if (missingHandle == NULL || strlen(missingHandle) == 0)
    {
        fprintf(stderr, "Error: Missing handle is empty\n");
        return -1;
    }

    int idx = 0;

    packet[idx++] = 7; // flag for error packet

    // add missing handle - don't include null terminator
    uint8_t missingHandleLen = strlen(missingHandle);
    if (missingHandleLen >= MAX_HANDLE_LENGTH)
    {
        fprintf(stderr, "Error: Missing handle length exceeds maximum allowed length\n");
        return -1;
    }
    packet[idx++] = missingHandleLen;
    memcpy(&packet[idx], missingHandle, missingHandleLen);
    idx += missingHandleLen;

    return idx; // returning total length to pass into sendPDU
}

// int buildBroadcastPacket(u_int8_t *packet, u_int8_t flag, char *srcHandle, int *numDestHandles, char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1], char *messageText)
// {}

// int buildHandleListReq(u_int8_t *packet)
// {
//     packet[0] = 10; // flag for handle list request
//     return 1; // returning total length to pass into sendPDU
// }

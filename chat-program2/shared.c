// Lukas Shipley
// The shared attributes and functions of the client and server

#include "shared.h"

// make it so that all parameters that are unused for a given flag don't affect the packet
// consolidate the error checking and consider breaking this up with helpers
int buildMsgPacket(u_int8_t *packet, u_int8_t flag, char *srcHandle, char **destHandles, int numDestHandles, char *messageText)
{
    // check if handle is valid
    if (srcHandle == NULL || strlen(srcHandle) == 0)
    {
        fprintf(stderr, "Error: Sender handle is empty\n");
        return -1;
    }

    int idx = 0;

    packet[idx++] = flag;   // use idx as 0 then update it for the next part of packet w ++

    // add sender handle - don't include null terminator
    uint8_t srcHandleLen = strlen(srcHandle);
    if (srcHandleLen >= MAX_HANDLE_LENGTH)
    {
        fprintf(stderr, "Error: Sender handle length exceeds maximum allowed length\n");
        return -1;
    }
    packet[idx++] = srcHandleLen;
    memcpy(&packet[idx], srcHandle, srcHandleLen);
    idx += srcHandleLen;

    // for %M and %C, must add num of destinations
    if (flag == 5 && numDestHandles != 1)
    {
        fprintf(stderr, "Error: For %%M flag, only one destination handle is allowed\n");
        return -1;
    }

    if (flag == 5 || flag == 6)
    {
        // check if numDestHandles is valid
        if (numDestHandles <= 0)
        {
            fprintf(stderr, "Error: Number of destination handles must be greater than 0 for %%M and %%C flags\n");
            return -1;
        }
        else if (numDestHandles > MAX_DEST_HANDLES)
        {
            fprintf(stderr, "Error: Number of destination handles exceeds maximum allowed number\n");
            return -1;
        }

        packet[idx++] = numDestHandles;
        for (int i = 0; i < numDestHandles; i++)
        {
            int destHandleLen = strlen(destHandles[i]);
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

    // add text message
    int messageLen = strlen(messageText) + 1; // +1 for null terminator
    if (messageLen >= MAX_MESSAGE_LENGTH)
    {
        fprintf(stderr, "Error: Message length exceeds maximum allowed length\n");
        return -1;
    }
    packet[idx++] = messageLen;
    memcpy(&packet[idx], messageText, messageLen);
    idx += messageLen;

    return idx; // returning total length to pass into sendPDU
}

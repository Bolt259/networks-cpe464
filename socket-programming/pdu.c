// Lukas Shipley
// Adding support for PDU (Protocol Data Unit) in the socket programming library
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "networks.h"
#include "safeUtil.h"

/*
sends [2-byte length in network byte order] + [dataBuffer of length lengthOfData]
returns the number of bytes sent (excluding the 2-byte length header)
*/
int sendPDU(int clientSocket, uint8_t * dataBuffer, int lengthOfData)
{
    // total PDU = 2 byte length + payload
    uint16_t length = lengthOfData + 2;
    uint16_t netLength = htons(length);

    // allocate buffer for PDU
    uint8_t *pduBuff = (uint8_t *)malloc(length);
    if (pduBuff == NULL)
    {
        perror("sendPDU: malloc error");
        return -1;
    }
    
    // copy header and payload into buffer
    memcpy(pduBuff, &netLength, 2);
    memcpy(pduBuff + 2, dataBuffer, lengthOfData);
    
    // send the PDU
    int bytesSent = safeSend(clientSocket, pduBuff, length, 0);
    if (bytesSent != length)
    {
        fprintf(stderr, "sendPDU: sent %d bytes, expected %d\n", bytesSent, length);
        free(pduBuff);
        return -1;
    }

    free(pduBuff);
    return (bytesSent - 2);
}


/*
receives [2-byte length in network byte order] + [dataBuffer of length lengthOfData]
returns the number of bytes received (excluding the 2-byte length header)
returns 0 if the connection is closed
returns -1 if there is an error
*/
int recvPDU(int clientSocket, uint8_t * dataBuffer, int bufferSize)
{
    uint16_t netLength;

    // receive the first 2 bytes for length
    int bytesReceived = safeRecv(clientSocket, (uint8_t *)&netLength, 2, MSG_WAITALL);

    if (bytesReceived == 0)
    {
        // connection closed
        return 0;
    }
    else if (bytesReceived != 2)
    {
        fprintf(stderr, "recvPDU: failed to receive length header, received %d bytes, expected 2\n", bytesReceived);
        return -1;
    }

    // convert to host byte order
    uint16_t length = ntohs(netLength);

    // printf("Header info: netLength = %u, length = %u\n", (unsigned)netLength, (unsigned)length);

    // payload length is length - 2
    int payloadLength = length - 2;

    if (length < 2 || payloadLength > bufferSize)
    {
        fprintf(stderr, "recvPDU: invalid payload length (payloadLength = %d, bufferSize = %d)\n", payloadLength, bufferSize);
        return -1;
    }

    // receive the payload
    bytesReceived = safeRecv(clientSocket, dataBuffer, payloadLength, MSG_WAITALL);
    if (bytesReceived == 0)
    {
        // printf("recvPDU: connection closed by peer\n");
        return 0; // connection closed
    }
    else if (bytesReceived != payloadLength)
    {
        fprintf(stderr, "recvPDU: failed to receive payload, received %d bytes, expected %d\n", bytesReceived, payloadLength);
        return -1;
    }

    return bytesReceived;
}

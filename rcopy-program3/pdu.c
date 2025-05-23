// Lukas Shipley
// pdu.c

// PDU implementation for UDP

#include "pdu.h"

int createPDU(uint8_t * pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t * payload, int payloadLen)
{
    if (payloadLen > MAX_PAYLOAD_LEN)
    {
        fprintf(stderr, "Payload length exceeds maximum size of 1400 bytes\n");
        return -1;
    }

    int pduLength = 0;
    uint32_t netSeqNum = htonl(sequenceNumber);
    uint16_t netChksum = 0;
    uint8_t *currentPos = pduBuffer;

    /* memcpy data into the PDU based on format 
    [4 bytes seq num in network byte order]
    [2 bytes checksum]
    [1 byte flag]
    [payload (<=1400 bytes)]
    */
    memcpy(currentPos, &netSeqNum, sizeof(netSeqNum));
    currentPos += sizeof(netSeqNum); // increment position
    pduLength += sizeof(netSeqNum); // increment size

    memcpy(currentPos, &netChksum, sizeof(netChksum));
    currentPos += sizeof(netChksum);
    pduLength += sizeof(netChksum);
    
    memcpy(currentPos, &flag, sizeof(flag));
    currentPos += sizeof(flag);
    pduLength += sizeof(flag);
    
    memcpy(currentPos, payload, payloadLen);
    currentPos += payloadLen;
    pduLength += payloadLen;
    
    // printf("Debug: Checksum before network byte order conversion: %u (0x%04X)\n", (uint16_t)in_cksum((unsigned short *)pduBuffer, pduLength), (uint16_t)in_cksum((unsigned short *)pduBuffer, pduLength));
    // printf("Debug: Checksum after network byte order conversion: %u (0x%04X)\n", htons((uint16_t)in_cksum((unsigned short *)pduBuffer, pduLength)), htons((uint16_t)in_cksum((unsigned short *)pduBuffer, pduLength)));

    netChksum = (uint16_t)in_cksum((unsigned short *)pduBuffer, pduLength);
    memcpy(pduBuffer + sizeof(netSeqNum), &netChksum, sizeof(netChksum));

    return pduLength;
}

// verifies checksum and prints the sequence number, flag, payload and payload length
// it prints error message if pdu is corrupted
void printPDU(uint8_t * pduBuffer, int pduLen)
{
    uint32_t seqNum = 0;
    uint16_t chksum = 0;
    uint8_t flag = 0;
    uint8_t *currentPos = pduBuffer;
    uint8_t payload[MAX_PAYLOAD_LEN + 1]; // +1 for null-termination
    int payloadLen = 0;

    memcpy(&seqNum, currentPos, sizeof(seqNum));
    seqNum = ntohl(seqNum);
    currentPos += sizeof(seqNum);

    memcpy(&chksum, currentPos, sizeof(chksum));
    chksum = ntohs(chksum);
    currentPos += sizeof(chksum);

    uint16_t calculatedChksum = in_cksum((unsigned short *)pduBuffer, pduLen);
    if (calculatedChksum != 0)
    {
        fprintf(stderr, "PDU is corrupted. Checksum mismatch: expected %u, got %u\n", calculatedChksum, chksum);
        return;
    }

    memcpy(&flag, currentPos, sizeof(flag));
    currentPos += sizeof(flag);

    payloadLen = pduLen - (sizeof(seqNum) + sizeof(chksum) + sizeof(flag));
    if (payloadLen > MAX_PAYLOAD_LEN)
    {
        fprintf(stderr, "Payload length exceeds maximum size of 1400 bytes\n");
        return;
    }
    memcpy(payload, currentPos, payloadLen);
    payload[payloadLen] = '\0'; // null-terminate the payload for printing

    printf("PDU:\n");
    printf("  Sequence Number: %u\n", seqNum);
    printf("  Checksum: %u\n", chksum);
    printf("  Flag: %u\n", flag);
    printf("  Payload: %s\n", payload);
    printf("  Payload Length: %d\n", payloadLen);
}

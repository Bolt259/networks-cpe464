// Crude circular queue buffering library for rcopy Project 3 Networks 464 class
// written by Lukas Shipley

#include "buffer.h"

// globs
PacketBuffer *pb = NULL;

// func defs start

// setup packet buffer
void initPacketBuffer(uint32_t winSize)
{
    if (pb != NULL || winSize > MAX_PACKS)
    {
        fprintf(stderr, "Error: Invalid packet buffer or window size exceeds maximum.\n");
        return;
    }

    pb = (PacketBuffer *)calloc(1, sizeof(PacketBuffer));
    if (pb == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for packet buffer.\n");
        return;
    }

    pb->buffer = (Packet *)calloc(winSize, sizeof(Packet));
    if (pb->buffer == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for packet buffer array.\n");
        free(pb);
        pb = NULL;
        return;
    }

    pb->nextSeqNum = 0;
    pb->winSize = winSize;

    if (DEBUG_FLAG)
    {
        printf("Packet buffer initialized with window size: %u\n", winSize);
    }
}

// self explanatory
void freePacketBuffer()
{
    if (pb == NULL)
    {
        fprintf(stderr, "Error: Tried to free a NULL packet buffer.\n");
        return;
    }

    if (pb->buffer)
    {
        for (uint32_t i = 0; i < pb->winSize; i++)
        {
            Packet *packet = &pb->buffer[i];
            if (packet->packetData)
            {
                free(packet->packetData);
                packet->packetData = NULL;
            }
        }
        free(pb->buffer);
        pb->buffer = NULL;
    }
    free(pb);
    pb = NULL;

    if (DEBUG_FLAG)
    {
        printf("Packet buffer freed successfully.\n");
    }
}

// add a packet to the buffer
int addPacket(uint8_t *packet, int packetLen, uint32_t seqNum)
{
    if (pb == NULL || packet == NULL || packetLen <= 0 || seqNum < pb->nextSeqNum ||
        seqNum >= pb->nextSeqNum + pb->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for addPacket.\n");
        }
        return -1; // invalid parameters
    }

    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (pkt->packetData != NULL)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Packet at index %u is already occupied.\n", idx);
        }
        return -1; // packet already occupied
    }

    pkt->packetData = (uint8_t *)calloc(packetLen, sizeof(uint8_t));
    if (pkt->packetData == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for packet data.\n");
        return -1;
    }

    memcpy(pkt->packetData, packet, packetLen);
    pkt->packetLen = packetLen;
    pkt->recv = 0; // not received yet

    pb->storedPackets++;

    if (DEBUG_FLAG)
    {
        printf("Packet added successfully at index %u with sequence number %u.\n", idx, seqNum);
    }

    return 0; // success
}

// get a packet from the buffer
int getPacket(uint8_t *packet, int *packetLen, uint32_t seqNum)
{
    if (pb == NULL || packet == NULL || packetLen == NULL || seqNum < pb->nextSeqNum ||
        seqNum >= pb->nextSeqNum + pb->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for getPacket.\n");
        }
        return -1; // invalid parameters
    }

    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (pkt->packetData == NULL)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Packet at index %u is not occupied.\n", idx);
        }
        return -1; // packet not occupied
    }

    memcpy(packet, pkt->packetData, pkt->packetLen);
    *packetLen = pkt->packetLen;

    if (DEBUG_FLAG)
    {
        printf("Packet retrieved successfully from index %u with sequence number %u.\n", idx, seqNum);
    }

    return 0; // success
}

// mark a packet as received
int markPacketReceived(uint32_t seqNum)
{
    if (pb == NULL || seqNum < pb->nextSeqNum || seqNum >= pb->nextSeqNum + pb->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for markPacketReceived.\n");
        }
        return -1; // invalid parameters
    }

    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (pkt->packetData == NULL)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Packet at index %u is not occupied.\n", idx);
        }
        return -1; // packet not occupied
    }

    pkt->recv = 1; // mark as received

    if (DEBUG_FLAG)
    {
        printf("Packet at index %u with sequence number %u marked as received.\n", idx, seqNum);
    }

    return 0; // success
}

// check if a packet is received
int isPacketReceived(uint32_t seqNum)
{
    if (pb == NULL || seqNum < pb->nextSeqNum || seqNum >= pb->nextSeqNum + pb->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for isPacketReceived.\n");
        }
        return -1; // invalid parameters
    }

    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (pkt->packetData == NULL)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Packet at index %u is not occupied.\n", idx);
        }
        return 0; // packet not occupied
    }

    return pkt->recv; // return received status
}

// check if the buffer is full
int bufferFull()
{
    if (pb == NULL)
    {
        if (DEBUG_FLAG)
            fprintf(stderr, "Error: Packet buffer is NULL.\n");
        return 1;
    }

    if (pb->storedPackets < pb->winSize)
        return 0;

    return 1;
}
// func defs end

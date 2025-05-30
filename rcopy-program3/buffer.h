// written by Lukas Shipley

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#define DEBUG_FLAG 1         // ~!*
#define MAX_PACKS 1073741824 // 2^30 bytes = 1 GiB - winSize must be less than this number

typedef struct
{
    uint8_t *packetData;
    int packetLen;
    int recv;    // 1 = received, 0 = not received
    int written; // 1 = written, 0 = not written to file
} Packet;

typedef struct
{
    uint32_t winSize; // number of packets in flight - size of the associated window and therefore buffer
    Packet *buffer;
    uint32_t nextSeqNum; // next sequence number to write
    int storedPackets;   // number of packets currently stored in the buffer

} PacketBuffer;

void initPacketBuffer(uint32_t winSize);
void freePacketBuffer();

int addPacket(uint8_t *packet, int packetLen, uint32_t seqNum);
int getPacket(uint8_t *packet, int *packetLen, uint32_t seqNum);

int markPacketReceived(uint32_t seqNum);
int isPacketReceived(uint32_t seqNum);
int bufferFull(); // 1 = full, 0 = not full

#endif

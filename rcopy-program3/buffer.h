// written by Lukas Shipley

#ifndef __BUFFER_H__
#define __BUFFER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define DEBUG_FLAG 1         // ~!*
#define MAX_PACKS 1073741824 // 2^30 bytes = 1 GiB - winSize must be less than this number

typedef struct
{
    uint8_t *packetData;
    int packetLen;
    int written; // 1 = written and used as invalid to flush, 0 = not written to file and valid to addPacket
} Packet;

typedef struct
{
    uint32_t winSize; // number of packets in flight - size of the associated window and therefore buffer
    Packet *buffer;
    uint32_t nextSeqNum; // next sequence number to write
    int storedPackets;   // number of packets currently stored in the buffer
    int outFileFd;      // file descriptor for the output file to be written to
} PacketBuffer;

void initPacketBuffer(uint32_t winSize, int32_t buffSize, int outFileFd);
void freePacketBuffer();

int addPacket(uint8_t *packet, int packetLen, uint32_t seqNum);
int getPacket(uint8_t *packet, int *packetLen, uint32_t seqNum);    // X
int flushBuffer(); // write packets to the output file and slide

int isWritten(uint32_t seqNum); // returns 1 if packet is written, 0 if not
int getNextSeqNum();
int getStoredPackets();
int bufferOpen(); // 1 = open, 0 = full
int needFlush(); // 1 = has packets, 0 = no packets to write

#endif

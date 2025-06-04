// written by Lukas Shipley

#ifndef __WINDOW_H__
#define __WINDOW_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

#include "networks.h"
#include "srej.h"

#define DEBUG_FLAG 1 // ~!*
#define MAX_PANES 1073741824 // 2^30 bytes = 1 GiB - winSize must be less than this number

typedef struct
{
    int packetLen;
    uint8_t *packet;
    uint32_t seqNum;
    int ack;        // 1 = ACK, 0 = NAK
    // int occupied;   // 1 = occupied, 0 = empty
} Pane; // like a pane of glass in the sliding window - panes hold relevant packet data for storage

typedef struct
{
    uint32_t winSize;   // max packets in flight - winSize panes wide
    Pane *paneBuff; // array of Pane structs
    uint32_t lower; // lowest unACKed sequence number
    uint32_t curr;  // current sequence number to send
} Window;

void initWindow(uint32_t winSize, int buffSize);
void freeWindow();

int addPane(uint8_t *packet, int packetLen, uint32_t seqNum);
int markPaneAck(uint32_t ackedSeqNum);
int checkPaneAck(uint32_t seqNum);
void slideWindow(uint32_t newLow);

int32_t resendPane(Connection *client, uint8_t flag, uint32_t seqNum, uint8_t *packet);
uint32_t getLowerBound();   // lowest unACKed sequence number
uint32_t getCurrSeqNum();

int windowOpen(); // 1 = open, 0 = closed

#endif

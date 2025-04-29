// Lukas Shipley
// call this message.h in the future

#ifndef __SHARED_H__
#define __SHARED_H__

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdint.h>
#include <errno.h>

#include "handleTable.h"

#define MAX_PACKET_SIZE 1024
#define MAX_MESSAGE_LENGTH 200  // including null
#define MAX_DEST_HANDLES 9

void printPacket(uint8_t *packet, int packetLen);

int buildMsgPacket(u_int8_t *packet, u_int8_t flag, char *srcHandle, int numDestHandles, char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1], char *messageText);
int buildErrPacket(u_int8_t *packet, char *missingHandle);
// int buildBroadcastPacket(u_int8_t *packet, u_int8_t flag, char *srcHandle, int *numDestHandles, char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1], char *messageText);
// int buildHandleListReq(u_int8_t *packet);

int parseMsgPacket(u_int8_t *buffer, int bufferLen, char* srcHandle, int *numDestHandles, char destHandles[MAX_DEST_HANDLES][MAX_HANDLE_LENGTH + 1], char *messageText);
uint8_t getHandleFromBuffer(uint8_t *buffer, char handle[MAX_HANDLE_LENGTH + 1], int whichHandle);

#endif // __SHARED_H__

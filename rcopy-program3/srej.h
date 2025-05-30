// SREJ Written by Hugh Smith
// modified by Lukas Shipley on 5/23/2025

#ifndef __SREJ_H__
#define __SREJ_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "safeUtil.h"
#include "networks.h"
#include "cpe464.h"
#include "checksum.h"

#define MAX_PACK_LEN 1500
#define BUFF_SIZE 4
#define START_SEQ_NUM 1
#define MAX_TRIES 10
#define LONG_TIME 10
#define SHORT_TIME 1

#pragma pack(1)

typedef struct header
{
    uint32_t seqNum;
    uint16_t chksum;
    uint8_t flag;
} Header;

enum FLAG
{
    ACK_RR = 5,
    SREJ = 6,
    FNAME = 8,
    FNAME_OK = 9,
    END_OF_FILE = 10,
    DATA = 16,
    SREJ_DATA = 17,
    TIMEOUT_DATA = 18,
    FNAME_BAD = 32,
    EOF_ACK = 33,
    CRC_ERROR = -1
};

int32_t sendBuff(uint8_t *buff, uint32_t len, Connection *connection,
                 uint8_t flag, uint32_t seqNum, uint8_t *packet);
int createHeader(uint32_t len, uint8_t flag, uint32_t seqNum, uint8_t *packet);
int32_t recvBuff(uint8_t *buff, int32_t len, int32_t recvSockNum,
                 Connection *connection, uint8_t *flag, uint32_t *seqNum);
int retrieveHeader(uint8_t *dataBuff, int recvLen, uint8_t *flag, uint32_t *seqNum);
int processSelect(Connection *client, int *retryCnt, int selectTimeoutState, int dataReadyState, int doneState);

#endif

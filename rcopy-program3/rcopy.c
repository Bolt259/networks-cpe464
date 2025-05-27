// Client side - UDP Code
// By Hugh Smith	4/1/2017
// Modified by Lukas Shipley on 5/23/2025

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

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pdu.h"
#include "cpe464.h"
#include "pollLib.h"
#include "srej.h"

#define MAX_PACK_LEN 1500
#define MAX_PAYLOAD 1400
#define MAX_FNAME_LEN 100

typedef enum State STATE;

enum State
{
    START,
    FILENAME,
    FILE_OK,
    RECV_DATA,
    DONE
};

void transferFile(char *argv[]);
STATE start_state(char **argv, Connection *server, uint32_t *clientSeqNum);
STATE filename(char *fname, int32_t buf_size, Connection *server);
STATE recvData(int32_t outFile, Connection *server, uint32_t *clientSeqNum);
STATE file_ok(int *outFileFd, char *outFileName);
void checkArgs(int argc, char *argv[], float *errorRate, int *portNumber);

int main(int argc, char *argv[])
{
    // int socketNum = 0;
    // struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
    int portNumber = 0;
    float errorRate = 0;

    checkArgs(argc, argv, &errorRate, &portNumber);

    // socketNum = setupUdpClientToServer(&server, argv[2], portNumber);

    sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    transferFile(argv);

    // close(socketNum);

    return 0;
}

void transferFile(char *argv[])
{
    Connection *server = (Connection *)calloc(1, sizeof(Connection));
    STATE state = START;
    int outFileFd = 0;
    uint32_t clientSeqNum = 0;

    while (state != DONE)
    {
        switch (state)
        {
        case START:
            state = start_state(argv, server, &clientSeqNum);
            break;
        case FILENAME:
            state = filename(argv[1], atoi(argv[4]), server);
            break;
        case FILE_OK:
            state = file_ok(&outFileFd, argv[2]);
            break;
        case RECV_DATA:
            state = recvData(outFileFd, server, &clientSeqNum);
            break;
        case DONE:
            break;
        default:
            fprintf(stderr, "ERROR - In default state (transferFile)\n");
            exit(-1);
        }
    }
}

STATE start_state(char **argv, Connection *server, uint32_t *clientSeqNum)
{
    uint8_t packet[MAX_PACK_LEN] = {0};
    uint8_t buffer[MAX_PACK_LEN] = {0};
    int fileNameLen = strlen(argv[1]);
    STATE retVal = FILENAME;
    uint32_t bufferLen = 0;

    // check if fileNameLen is too long
    if (fileNameLen > MAX_FNAME_LEN)
    {
        fprintf(stderr, "Filename too long, must be less than %d characters\n", MAX_FNAME_LEN);
        return DONE;
    }

    // if previous server connection, close before reconnect
    if (server->socketNum > 0)
    {
        close(server->socketNum);
    }

    if (udpClientSetup(argv[6], atoi(argv[7]), server) < 0)
    {
        // could not connect to server
        retVal = DONE;
    }
    else
    {
        // build PDU (filename packet flag = 9) -> [bufferLen (4 bytes)][fileName (MAX_FNAME_LEN bytes)]
        bufferLen = htonl(atoi(argv[4]));
        memcpy(buffer, &bufferLen, BUFF_SIZE);
        memcpy(&buffer[BUFF_SIZE], argv[1], fileNameLen);
        printIPInfo(&server->remote);
        // send packet to server with filename
        sendBuff(buffer, fileNameLen + MAX_PAYLOAD, server, FNAME, *clientSeqNum, packet);
        (*clientSeqNum)++;
    }

    return retVal;
}

STATE filename(char *fname, int32_t buf_size, Connection *server)
{
    // get server response
    // returns START if no reply, DONE if bad filename, FILE_OK otherwise
    int retVal = START;
    uint8_t packet[MAX_PACK_LEN];
    uint8_t flag = 0;
    uint32_t seqNum = 0;
    int32_t recvCheck = 0;
    static int retryCnt = 0;

    if ((retVal = processSelect(server, &retryCnt, START, FILE_OK, DONE)) == FILE_OK)
    {
        recvCheck = recvBuff(packet, MAX_PACK_LEN, server->socketNum, server, &flag, &seqNum);

        // check for bit flips
        if (recvCheck == CRC_ERROR)
        {
            retVal = START;
        }
        else if (flag == FNAME_BAD)
        {
            printf("File %s is not found\n", fname);
            retVal = DONE;
        }
        else if (flag == DATA)
        {
            // file yes/no packet lost - instead its a data packet
            retVal = FILE_OK;
        }
    }
    return retVal;
}

STATE file_ok(int *outFileFd, char *outFileName)
{
    STATE retVal = DONE;

    if ((*outFileFd = open(outFileName, O_CREAT | O_TRUNC | O_WRONLY, 0600)) < 0)
    {
        perror("File open error: ");
        retVal = DONE;
    }
    else
    { // file opened and ready to receive data
        retVal = RECV_DATA;
    }
    return retVal;
}

STATE recvData(int32_t outFile, Connection *server, uint32_t *clientSeqNum)
{
    uint32_t seqNum = 0;
    uint32_t ackSeqNum = 0;
    uint8_t flag = 0;
    int32_t dataLen = 0;
    uint8_t dataBuff[MAX_PACK_LEN];
    uint8_t packet[MAX_PACK_LEN];
    static int32_t expectedSeqNum = START_SEQ_NUM;

    if (selectCall(server->socketNum, LONG_TIME, 0) == 0)
    {
        printf("Timeout after 10 seconds, server must be gone.\n");
        return DONE;
    }

    dataLen = recvBuff(dataBuff, MAX_PACK_LEN, server->socketNum, server, &flag, &seqNum);

    // do state RECV_DATA again if there is a  crc error (don't send ack, don't write data)
    if (dataLen == CRC_ERROR)
    {
        return RECV_DATA;
    }
    if (flag == END_OF_FILE)
    {
        // send ACK
        sendBuff(packet, 1, server, EOF_ACK, *clientSeqNum, packet);
        (*clientSeqNum)++;
        printf("File done\n");
        return DONE;
    }
    else
    {
        // send ACK
        ackSeqNum = htonl(seqNum);
        sendBuff((uint8_t *)&ackSeqNum, sizeof(ackSeqNum), server, ACK, *clientSeqNum, packet);
        (*clientSeqNum)++;
    }

    if (seqNum == expectedSeqNum)
    {
        // write data to file
        expectedSeqNum++;
        write(outFile, &dataBuff, dataLen);
        // write(outFile, &dataBuff[sizeof(Header)], dataLen)    <~!*> CHECK THE OUT FILE FOR HEADER WRITTEN ACCIDENTALLY
    }
    return RECV_DATA;
}

void checkArgs(int argc, char *argv[], float *errorRate, int *portNumber)
{
    *portNumber = 0;
    *errorRate = 0;

    /* check command line arguments  */
    if (argc != 8)
    {
        printf("usage: %s <filepath src> <filepath dest> <window size> <buffer size> <error rate> <host name> <port number> \n", argv[0]);
        exit(1);
    }
    if (strlen(argv[1]) > 1000)
    {
        printf("FROM filename too long needs to be less than 1000 and is %ld\n", strlen(argv[1]));
        exit(-1);
    }
    if (strlen(argv[2]) > 1000)
    {
        printf("TO filename too long needs to be less than 1000 and is %ld\n", strlen(argv[2]));
        exit(-1);
    }
    if (atoi(argv[3]) < 1 || atoi(argv[3]) > 229)
    {
        fprintf(stderr, "Window size must be between 1 and 229 inclusive and is %d\n", atoi(argv[3]));
        exit(-1);
    }
    if (atoi(argv[4]) < 400 || atoi(argv[4]) > 1400)
    {
        fprintf(stderr, "Buffer size must be between 400 and 1400 and is %d\n", atoi(argv[4]));
        exit(-1);
    }
    if (atoi(argv[5]) < 0 || atoi(argv[5]) >= 1)
    {
        fprintf(stderr, "Error rate must be between 0 and less than 1 and is %f\n", atof(argv[5]));
        exit(-1);
    }

    *portNumber = atoi(argv[7]);
    *errorRate = atof(argv[5]);
}

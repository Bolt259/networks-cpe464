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

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        fprintf(stderr, "Usage: %s <errorRate> <hostname> <portNumber>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    float errorRate = atof(argv[1]);
    char *hostname = argv[2];
    int portNumber = atoi(argv[3]);

    sendtoErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_ON);

    Connection *server = (Connection *)calloc(1, sizeof(Connection));
    uint32_t clientSeqNum = 0;

    uint8_t packet[MAX_PACK_LEN] = {0};
    uint8_t buffer[MAX_PACK_LEN] = {0};

    // For testing, use a fixed filename and bufferLen
    const char *testFilename = "testfile.txt";
    int fileNameLen = strlen(testFilename);
    uint32_t bufferLen = htonl(1024);

    // Setup server address in Connection struct
    if (udpClientSetup(hostname, portNumber, server) < 0)
    {
        fprintf(stderr, "Failed to setup UDP client\n");
        exit(-1);
    }

    // build PDU (filename packet flag = 9) -> [bufferLen (4 bytes)][fileName]
    memcpy(buffer, &bufferLen, sizeof(uint32_t));
    memcpy(&buffer[sizeof(uint32_t)], testFilename, fileNameLen);

    // send packet to server with filename
    sendBuff(buffer, sizeof(uint32_t) + fileNameLen, server, FNAME, clientSeqNum, packet);
    clientSeqNum++;

    // Now receive the server's reply
    uint8_t recvBuffData[MAX_PACK_LEN] = {0};
    uint8_t flag = 0;
    uint32_t seqNum = 0;
    int32_t recvLen = 0;

    recvLen = recvBuff(recvBuffData, MAX_PACK_LEN, server->socketNum, server, &flag, &seqNum);

    printf("Received reply from server: recvLen=%d, flag=%d, seqNum=%u\n", recvLen, flag, seqNum);

    return 0;
}

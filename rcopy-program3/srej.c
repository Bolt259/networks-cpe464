// SREJ Written by Hugh Smith
// modified by Lukas Shipley on 5/23/2025

#include "srej.h"

int32_t send_buff(uint8_t * buff, uint32_t len, Connection * connection,
                  uint8_t flag, uint32_t seqNum, uint8_t * packet)
{
    int32_t sentLen = 0;
    int32_t sendingLen = 0;

    // set up packet (seq#, crc, flag, data)
    if (len > 0)
    {
        memcpy(&packet[sizeof(Header)], buff, len);
    }
    sendingLen = createHeader(len, flag, seqNum, packet);

    sentLen = safeSendToErr(packet, sendingLen, connection);
    return sentLen;
}

int createHeader(uint32_t len, uint8_t flag, uint32_t seqNum, uint8_t * packet)
{
    // creates the regular header (puts in packet) including seqNum, chksum, and flag

    Header *hdr = (Header *)packet;
    uint16_t chksum = 0;

    seqNum = htonl(seqNum);
    memcpy(&(hdr->seqNum), &seqNum, sizeof(seqNum));

    hdr->flag = flag;

    memset(&(hdr->chksum), 0, sizeof(chksum));
    chksum = (uint16_t)in_cksum((unsigned short *)packet, sizeof(Header) + len);
    memcpy(&(hdr->chksum), &chksum, sizeof(chksum));

    return sizeof(Header) + len;
}

int32_t recv_buff(uint8_t * buff, int32_t len, int32_t recvSockNum,
                  Connection * connection, uint8_t * flag, uint32_t * seqNum)
{
    uint8_t dataBuff[MAX_PACK_LEN];
    int32_t recvLen = 0;
    int32_t dataLen = 0;

    recvLen = safeRecvFromErr(recvSockNum, dataBuff, len, connection);

    dataLen = retrieveHeader(dataBuff, recvLen, flag, seqNum);

    // dataLen could be -1 if crc error or 0 if no data
    if (dataLen > 0)
        memcpy(buff, &dataBuff[sizeof(Header)], dataLen);

    return dataLen;
}

int retrieveHeader(uint8_t * dataBuff, int recvLen, uint8_t * flag, uint32_t * seqNum)
{
    Header * hdr = (Header *)dataBuff;
    int retVal = 0;

    if (in_chksum((unsigned short *)dataBuff, recvLen) != 0)
    {
        retVal = CRC_ERROR; // checksum error
    }
    else
    {
        *flag = hdr->flag;
        memcpy(seqNum, &(hdr->seqNum), sizeof(hdr->seqNum));
        *seqNum = ntohl(*seqNum);

        retVal = recvLen - sizeof(Header);
    }

    return retVal;
}

int processSelect(Connection * client, int * retryCnt, int selectTimeoutState, int dataReadyState, int doneState)
{
    // returns:
    // doneState if calling this function exceeds MAX_TRIES
    // selectTimeoutState if select times out without receiving anything
    // dataReadyState if select() returns saying data is ready to be read

    int retVal = dataReadyState;

    (*retryCnt)++;
    if (*retryCnt > MAX_TRIES)
    {
        printf("No response from other side for %d seconds, termination connection\n",
        MAX_TRIES);
        retVal = doneState;
    }
    else
    {
        if (selectCall(client->socketNum, SHORT_TIME, 0) == 1)
        {
            *retryCnt = 0; // reset retry count
            retVal = dataReadyState;
        }
        else
        {
            // data not yet ready
            retVal = selectTimeoutState;
        }
    }
    return retVal;
}

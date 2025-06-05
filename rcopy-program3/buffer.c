// Crude circular queue buffering library for rcopy Project 3 Networks 464 class

#include "buffer.h"

// globs
PacketBuffer *pb = NULL;

// func defs start

// setup packet buffer
void initPacketBuffer(uint32_t winSize, int32_t buffSize, int outFileFd)
{
    if (pb != NULL || winSize > MAX_PACKS || winSize == 0 || outFileFd < 0)
    {
        fprintf(stderr, "Error: Invalid packet buffer or window size or output file descriptor.\n");
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

    // initialize each packet in the buffer
    for (int i = 0; i < winSize; i++)
    {
        pb->buffer[i].packetData = (uint8_t *)calloc(buffSize, sizeof(uint8_t));
        if (pb->buffer[i].packetData == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory for packet data at index %d.\n", i);
            // free all previously allocated packets
            for (int j = 0; j < i; j++)
            {
                free(pb->buffer[j].packetData);
                pb->buffer[j].packetData = NULL;
            }
            free(pb->buffer);
            free(pb);
            pb = NULL;
            return;
        }
        pb->buffer[i].packetLen = 0;
        pb->buffer[i].written = 1;  // mark packets as written initially meaning open for adding
    }

    // initialize packet buffer metadata
    pb->winSize = winSize;
    pb->nextSeqNum = 1;
    pb->storedPackets = 0;
    pb->outFileFd = outFileFd;
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
                packet->packetLen = 0;
                packet->written = 0;
            }
        }
        free(pb->buffer);
        pb->buffer = NULL;
        pb->winSize = 0;
        pb->nextSeqNum = 0;
        pb->storedPackets = 0;
        pb->outFileFd = -1;
    }
    free(pb);
    pb = NULL;
 }

// add a packet to the buffer
int addPacket(uint8_t *packet, int packetLen, uint32_t seqNum)
{
    if (pb == NULL || packet == NULL || packetLen <= 0 || seqNum < 1)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for addPacket.\n");
        }
        return -1;
    }

    // first valid packet arrival
    if (pb->storedPackets == 0) pb->nextSeqNum = seqNum;

    if (seqNum < pb->nextSeqNum || seqNum >= pb->nextSeqNum + pb->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Sequence number %u is out of bounds for the current buffer window with bounds [%u, %u).\n", seqNum, pb->nextSeqNum, pb->nextSeqNum + pb->winSize);
        }
        return -1;
    }

    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (!pkt->written)
    {
        // fprintf(stderr, "Error: Packet at index %u is already occupied and hasn't been written, sequence number %u. Please flushBuffer before adding another packet.\n", idx, seqNum);
        return 0;   // packet already in buffer and not written, do nothing
    }

    // packetData should be already allocated
    if (!pkt->packetData)
    {
        fprintf(stderr, "Error: addPacket called before initPacketBuffer.\n");
        return -1;
    }

    // check for overwrite
    if (pkt->written == 0 && pkt->packetLen > 0) return 0;

    memcpy(pkt->packetData, packet, packetLen);
    pkt->packetLen = packetLen;
    pkt->written = 0;

    // if (pb->storedPackets == 0) pb->nextSeqNum = seqNum;    // only set nextSeqNum if this is the first packet buffered or buffer has been flushed
    pb->storedPackets++;

    return 0; // success
}

// get a packet from the buffer - DEPRACTED DO NOT USE
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

// returns number of bytes written on success or -1 on error
int flushBuffer()
{
    if (pb == NULL)
    {
        fprintf(stderr, "Error: Packet buffer is NULL.\n");
        return -1;
    }
    int totBytesWritten = 0;
    int bytesWritten = 0;

    // write all packets that have been received and not written to the output file up to nextSeqNum
    for (int i = 0; i < pb->winSize; i++)
    {
        uint32_t idx = pb->nextSeqNum % pb->winSize;
        Packet *pkt = &pb->buffer[idx];

        if (pkt->packetData && !pkt->written)
        {
            bytesWritten = write(pb->outFileFd, pkt->packetData, pkt->packetLen);
            if (bytesWritten < 0)
            {
                fprintf(stderr, "Error flushing buffer packet at idx %u to output file\n", idx);
                return -1;
            }

            // //~!*> fsync the file to ensure data is written to disk
            // if (fsync(pb->outFileFd) < 0)
            // {
            //     fprintf(stderr, "Error syncing output file after writing packet at idx %u\n", idx);
            //     return -1;
            // }

            totBytesWritten += bytesWritten;
            memset(pkt->packetData, 0, pkt->packetLen);
            pkt->packetLen = 0;
            pkt->written = 1;
            pb->storedPackets--;
            pb->nextSeqNum++;
        }
        else
        {
            // stop at first written or open packet
            break;
        }
    }

    // pb->nextSeqNum = 0; // reset nextSeqNum to something it should never be
    pb->storedPackets = 0; // reset stored packets after flushing
    return totBytesWritten;
}

// returns 1 if the packet is written, 0 if not, -1 on error
int isWritten(uint32_t seqNum)
{
    uint32_t idx = seqNum % pb->winSize;
    Packet *pkt = &pb->buffer[idx];

    if (pkt->written) return 1;

    return 0;
}

// return the next sequence number to write
int getNextSeqNum()
{
    if (pb == NULL)
    {
        fprintf(stderr, "Error: Packet buffer is NULL.\n");
        return -1; // error
    }
    return pb->nextSeqNum;
}

// returns the number of packets currently stored in the buffer
int getStoredPackets()
{
    if (pb == NULL)
    {
        fprintf(stderr, "Error: Packet buffer is NULL.\n");
        return -1; // error
    }
    return pb->storedPackets;
}

// returns 1 if the buffer is not full, 0 if it is full
int bufferOpen()
{
    if (pb == NULL)
    {
        if (DEBUG_FLAG)
            fprintf(stderr, "Error: Packet buffer hasn't beeen initialized.\n");
        return 1;
    }

    if (pb->storedPackets < pb->winSize)
        return 1;

    return 0;
}

// returns 1 if there are packets to write, 0 if not
int needFlush()
{
    if (pb == NULL)
    {
        if (DEBUG_FLAG)
            fprintf(stderr, "Error: Packet buffer hasn't been initialized.\n");
        return 0;
    }

    if (pb->storedPackets > 0)
        return 1;

    return 0;
}

// func defs end

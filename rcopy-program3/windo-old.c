// Crude circular queue windowing library for rcopy Project 3 Networks 464 class

#include "window.h"

// globs
Window *win = NULL;

// func defs start

// setup window
void initWindow(uint32_t winSize, int buffSize)
{
    if (win != NULL || winSize > MAX_PANES)
    {
        fprintf(stderr, "Error: Size exceeds maximum window size (2^30 or 1 GiB) or window already initialized which should not happen.\n");
        return;
    }

    win = (Window *)calloc(1, sizeof(Window));
    if (win == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for window struct.\n");
        return;
    }

    win->winSize = winSize;

    win->paneBuff = (Pane *)calloc(winSize, sizeof(Pane));
    if (win->paneBuff == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for pane buffer.\n");
        free(win);
        win = NULL;
        return;
    }

    // initialize each pane in the buffer
    for (int i = 0; i < winSize; i++)
    {
        win->paneBuff[i].packet = (uint8_t *)calloc(buffSize, sizeof(uint8_t));
        if (win->paneBuff[i].packet == NULL)
        {
            fprintf(stderr, "Error: Failed to allocate memory for packet in pane at index %d.\n", i);
            // free all previously allocated packets
            for (uint32_t j = 0; j < i; j++)
            {
                free(win->paneBuff[j].packet);
                win->paneBuff[j].packet = NULL;
            }
            free(win->paneBuff);
            free(win);
            win = NULL;
            return;
        }
        win->paneBuff[i].packetLen = 0;
        win->paneBuff[i].seqNum = 0;
        win->paneBuff[i].ack = 1;   // set ack to 1 so that addPane() can overwrite it
        // win->paneBuff[i].occupied = 0;
    }

    win->lower = 1; // the first expected seqNum should be 1
    win->curr = 1;
}

// self explanatory
void freeWindow()
{
    if (win == NULL)
    {
        fprintf(stderr, "Error: Tried to free a NULL window.\n");
        return;
    }
    if (win->paneBuff)
    {
        for (uint32_t i = 0; i < win->winSize; i++)
        {
            Pane *pane = &win->paneBuff[i];
            if (pane->packet)
            {
                free(pane->packet);
                pane->packet = NULL;
                pane->packetLen = 0;
                pane->seqNum = 0;
                pane->ack = 0;
                // pane->occupied = 0;
            }
        }
        free(win->paneBuff);
        win->paneBuff = NULL;
        win->winSize = 0;
        win->lower = 0;
        win->curr = 0;
    }
    free(win);
    win = NULL;
}

// insert a pane into the window
int addPane(uint8_t *packet, int packetLen, uint32_t seqNum)
{
    if (win == NULL || !(windowOpen()) || packet == NULL || packetLen <= 0 ||
        seqNum < win->lower || seqNum >= win->lower + win->winSize)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Invalid parameters for addPane:\n");
            fprintf(stderr, " - Window: %p\n", (void *)win);
            fprintf(stderr, " - Window Size to see if full: %u\n", win ? win->winSize : 0);
            fprintf(stderr, " - Packet: %p\n", (void *)packet);
            fprintf(stderr, " - Packet Length: %d\n", packetLen);
            fprintf(stderr, " - Sequence Number: %u\n", seqNum);
        }
        else
        {
            fprintf(stderr, "Error: Invalid parameters for addPane or window is full.\n");
        }
        return -1;
    }
    if (seqNum != win->curr)
    {
        fprintf(stderr, "Can only add panes in order, current sequence number is %u, but trying to add %u.\n", win->curr, seqNum);
        return -1;
    }

    uint32_t idx = seqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];
    if (!pane->ack)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Pane at index %u is not ACKed, sequence number %u. Please resendPane, wait for ACK, then call slideWindow before adding yet another Pane, for the love of Smith.\n", idx, seqNum);
        }
        return -1; // pane not ACKed
    }

    // increment after adding
    win->curr++;

    // pane->packet should already be allocated
    if (!pane->packet)
    {
        fprintf(stderr, "Error: addPacket called before initPacketBuffer.\n");
        return -1;
    }
    memcpy(pane->packet, packet, packetLen);
    pane->packetLen = packetLen;
    pane->seqNum = seqNum;
    pane->ack = 0;

    if (DEBUG_FLAG)
    {
        printf("Pane added successfully at index %u with sequence number %u.\n", idx, seqNum);
    }
    return 0;
}

// mark a pane as acked
int markPaneAck(uint32_t ackedSeqNum)
{
    if (win == NULL || ackedSeqNum < win->lower || ackedSeqNum >= win->lower + win->winSize)
    {
        fprintf(stderr, "Error: Invalid window or sequence number for marking ACK.\n");
        return -1;
    }

    uint32_t idx = ackedSeqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];   // temp pane for reference
    if (pane->seqNum == ackedSeqNum)
    {
        pane->ack = 1;
        if (DEBUG_FLAG)
        {
            printf("Pane at index %u with sequence number %u marked as ACKed.\n", idx, ackedSeqNum);
        }
        return 0;
    }
    fprintf(stderr, "Error: Pane at index %u with sequence number %u not found or not occupied.\n", idx, ackedSeqNum);
    return -1;
}

// returns 1 if the pane is ACKed, 0 if not, -1 on error
int checkPaneAck(uint32_t seqNum)
{
    if (win == NULL || seqNum < win->lower || seqNum >= win->lower + win->winSize)
    {
        fprintf(stderr, "Error: Invalid window or sequence number for checking ACK.\n");
        return -1;
    }

    uint32_t idx = seqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];
    if (pane->seqNum == seqNum)
    {
        return pane->ack;
    }
    return -1;
}

// slide window up to a new lower bound
void slideWindow(uint32_t newLow)
{
    if (win == NULL || newLow < win->lower || newLow > win->curr)
    {
        fprintf(stderr, "Error: Invalid window or new base.\n");
        return;
    }
    if (DEBUG_FLAG)
    {
        printf("Sliding window from lower %u to new lower %u.\n", win->lower, newLow);
    }

    // clear lower panes
    for (uint32_t seqNum = win->lower; seqNum < newLow; seqNum++)
    {
        uint32_t idx = seqNum % win->winSize;
        Pane *pane = &win->paneBuff[idx];
        if (pane->ack)
        {
            // don't free packet memory just set to 0 so that it c an be overwritten later
            memset(pane->packet, 0, pane->packetLen);
            pane->packetLen = 0;
            pane->seqNum = 0;
            // leave ack set so that addPane() can overwrite it
        }
    }
    win->lower = newLow;
}

// resends the lowest unACKed pane in the window
int32_t resendPane(Connection *client, uint8_t flag, uint32_t seqNum, uint8_t *packet)
{
    if (win == NULL || seqNum < win->lower || seqNum > win->curr)
    {
        fprintf(stderr, "Error: Invalid window or sequence number for resending panes.\n");
        return -1;
    }

    int32_t packetLen = -1;
    uint32_t idx = seqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];
    if (pane->ack == 0 && pane->seqNum == seqNum)
    {
        packetLen = sendBuff(pane->packet, pane->packetLen, client, flag, seqNum, packet);
        if (DEBUG_FLAG)
        {
            printf("Resending pane at index %u with sequence number %u.\n", idx, seqNum);
        }
        return packetLen;
    }

    fprintf(stderr, "Error: Pane at index %u with sequence number %u not found or already ACKed.\n", idx, seqNum);
    return -1;
}

// get the base sequence number
uint32_t getLowerBound()
{
    if (win == NULL)
    {
        fprintf(stderr, "Error: Window is NULL.\n");
        return 0;
    }
    return win->lower;
}

// get the current sequence number
uint32_t getCurrSeqNum()
{
    if (win == NULL)
    {
        fprintf(stderr, "Error: Window is NULL.\n");
        return 0;
    }
    return win->curr;
}

// returns 1 if window is open, 0 if closed
int windowOpen()
{
    if (win == NULL)
    {
        fprintf(stderr, "Error: Window is NULL.\n");
        return -1;
    }
    // uint32_t idx = win->curr % win->winSize;
    // Pane *pane = &win->paneBuff[idx];
    // if (pane->ack)
    // {
    //     return 1; // window is open
    // }
    // return 0; // window is full
    return ((win->curr - win->lower) < win->winSize) && (win->paneBuff[win->curr % win->winSize].ack);  // span is less than winSize and the current pane is free
}
// func defs end

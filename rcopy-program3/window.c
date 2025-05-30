// Crude circular queue windowing library for rcopy Project 3 Networks 464 class
// written by Lukas Shipley

#include "window.h"

// globs
Window *win = NULL;

// func defs start

// setup window
void initWindow(uint32_t winSize)
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

    win->lower = 0;
    win->curr = 0;
    if (DEBUG_FLAG)
    {
        printf("Window initialized with size: %u\n", winSize);
    }
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
            if (pane->occupied && pane->packet)
            {
                free(pane->packet);
                pane->packet = NULL;
                pane->packetLen = 0;
                pane->seqNum = 0;
                pane->ack = 0;
                pane->occupied = 0;
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
    if (DEBUG_FLAG)
    {
        printf("Window freed successfully.\n");
    }
}

// insert a pane into the window upon 
int addPane(uint8_t *packet, int packetLen, uint32_t seqNum)
{
    if (win == NULL || windowFull() || packet == NULL || packetLen <= 0 ||
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

    uint32_t idx = seqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];
    if (pane->occupied)
    {
        if (DEBUG_FLAG)
        {
            fprintf(stderr, "Error: Pane at index %u is already occupied.\n", idx);
        }
        return -1; // pane already occupied
    }

    pane->packet = (uint8_t *)calloc(packetLen, sizeof(uint8_t));
    if (pane->packet == NULL)
    {
        fprintf(stderr, "Error: Failed to allocate memory for packet in pane.\n");
        return -1;
    }

    memcpy(pane->packet, packet, packetLen);
    pane->packetLen = packetLen;
    pane->seqNum = seqNum;
    pane->ack = 0;
    pane->occupied = 1;

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
    if (pane->occupied && pane->seqNum == ackedSeqNum)
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

// slide window up to a new lower bound
void slideWindow(uint32_t newLow)
{
    if (win == NULL || newLow < win->lower || newLow >= win->curr)
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
        if (pane->occupied)
        {
            free(pane->packet);
            pane->packet = NULL;
            pane->packetLen = 0;
            pane->seqNum = 0;
            pane->ack = 0;
            pane->occupied = 0;
        }
    }
    win->lower = newLow;
}

// resend panes starting from seqNum
Pane *resendPanes(uint32_t seqNum)
{
    if (win == NULL || seqNum < win->lower || seqNum >= win->curr)
    {
        fprintf(stderr, "Error: Invalid window or sequence number for resending panes.\n");
        return NULL;
    }

    uint32_t idx = seqNum % win->winSize;
    Pane *pane = &win->paneBuff[idx];
    if (pane->occupied && pane->ack == 0 && pane->seqNum == seqNum)
    {
        if (DEBUG_FLAG)
        {
            printf("Resending pane at index %u with sequence number %u.\n", idx, seqNum);
        }
        return pane;
    }

    fprintf(stderr, "Error: Pane at index %u with sequence number %u not found or not occupied.\n", idx, seqNum);
    return NULL;
}

// check if window is full
int windowFull()
{
    if (win == NULL)
    {
        fprintf(stderr, "Error: Window is NULL.\n");
        return 1;
    }
    for (uint32_t i = 0; i < win->winSize; i++)
    {
        Pane *pane = &win->paneBuff[i];
        if (!pane->occupied)
        {
            return 0;
        }
    }
    if (DEBUG_FLAG)
    {
        printf("Window is full.\n");
    }
    return 1;
}
// func defs end

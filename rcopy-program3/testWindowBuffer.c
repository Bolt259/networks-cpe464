// test_window_buffer.c
// Standalone test cases for buffer.c and window.c

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "window.h"
#include "buffer.h"

#define TEST_WIN_SIZE 5
#define TEST_DATA_SIZE 100

void test_window()
{
    printf("\n--- Testing Window Library ---\n");
    printf("\ntest:window:init\n");

    initWindow(TEST_WIN_SIZE);

    uint8_t data[TEST_DATA_SIZE];
    memset(data, 'A', TEST_DATA_SIZE);

    printf("\ntest:window:addPane\n");
    // Add panes
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        int result = addPane(data, TEST_DATA_SIZE, i);
        printf("addPane(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:window:addPane:full\n");
    // Should fail: window full
    int result = addPane(data, TEST_DATA_SIZE, TEST_WIN_SIZE);
    printf("addPane(seqNum=%u) = %d (expect fail)\n", TEST_WIN_SIZE, result);
    
    printf("\ntest:window:resendPanes\n");
    // Resend panes
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        Pane *pane = resendPanes(i);
        if (pane)
        {
            printf("Resending pane at seqNum=%u, packetLen=%d\n", pane->seqNum, pane->packetLen);
        }
        else
        {
            printf("No pane to resend for seqNum=%u\n", i);
        }
    }

    printf("\ntest:window:markPaneAck\n");
    // Mark ACKs
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        result = markPaneAck(i);
        printf("markPaneAck(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:window:checkPaneAck\n");
    // Check ACKs
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        result = checkPaneAck(i);
        printf("checkPaneAck(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:window:slideWindow\n");
    // Slide window
    slideWindow(4);

    printf("\ntest:window:addPane:afterSlide\n");
    // Add again
    for (uint32_t i = TEST_WIN_SIZE; i < TEST_WIN_SIZE * 2; i++)
    {
        result = addPane(data, TEST_DATA_SIZE, i);
        printf("addPane(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:window:free\n");
    freeWindow();
}

void test_buffer()
{
    printf("\n--- Testing Buffer Library ---\n");
    printf("\ntest:buffer:init\n");

    initPacketBuffer(TEST_WIN_SIZE);

    uint8_t data[TEST_DATA_SIZE];
    memset(data, 'B', TEST_DATA_SIZE);

    printf("\ntest:buffer:addPacket\n");
    // Add packets
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        int result = addPacket(data, TEST_DATA_SIZE, i);
        printf("addPacket(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:buffer:addPacket:full\n");
    // Should fail: buffer full
    int result = addPacket(data, TEST_DATA_SIZE, TEST_WIN_SIZE);
    printf("addPacket(seqNum=%u) = %d (expect fail)\n", TEST_WIN_SIZE, result);

    printf("\ntest:buffer:markPacketReceived\n");
    // Mark packets as received
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        result = markPacketReceived(i);
        printf("markPacketReceived(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:buffer:isPacketReceived\n");
    // Check isPacketReceived
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        result = isPacketReceived(i);
        printf("isPacketReceived(seqNum=%u) = %d\n", i, result);
    }

    printf("\ntest:buffer:getPacket\n");
    // Get packets
    uint8_t recvData[TEST_DATA_SIZE];
    int packetLen = 0;
    for (uint32_t i = 0; i < TEST_WIN_SIZE; i++)
    {
        result = getPacket(recvData, &packetLen, i);
        printf("getPacket(seqNum=%u) = %d, len=%d\n", i, result, packetLen);
    }

    printf("\ntest:buffer:free\n");
    freePacketBuffer();
}

int main()
{
    test_window();
    test_buffer();
    printf("\nAll tests completed.\n");
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "handleTable.h"

void testAddHandle();
void testLookupHandle();
void testRemoveHandle();
void testGetHandles();
void testHandleTableCleanup();

int main()
{
    printf("Running handleTable tests...\n");

    testAddHandle();
    testLookupHandle();
    testRemoveHandle();
    testGetHandles();
    testHandleTableCleanup();

    printf("All tests passed!\n");
    return 0;
}

void testAddHandle()
{
    printf("Testing addHandle...\n");

    handleTableCleanup(); // Ensure the table is empty before starting

    int result = addHandle("user1", 1);
    if (result != 0)
    {
        fprintf(stderr, "Failed to add handle 'user1'\n");
        exit(-1);
    }

    result = addHandle("user2", 2);
    if (result != 0)
    {
        fprintf(stderr, "Failed to add handle 'user2'\n");
        exit(-1);
    }

    // Test adding a duplicate handle
    result = addHandle("user1", 3);
    if (result == 0)
    {
        fprintf(stderr, "Duplicate handle 'user1' was added incorrectly\n");
        exit(-1);
    }

    printf("addHandle passed!\n");
}

void testLookupHandle()
{
    printf("Testing lookupHandle...\n");

    handleTableCleanup(); // Ensure the table is empty before starting

    addHandle("user1", 1);
    addHandle("user2", 2);

    int socket = lookupHandle("user1");
    if (socket != 1)
    {
        fprintf(stderr, "lookupHandle failed for 'user1'\n");
        exit(-1);
    }

    socket = lookupHandle("user2");
    if (socket != 2)
    {
        fprintf(stderr, "lookupHandle failed for 'user2'\n");
        exit(-1);
    }

    // Test looking up a non-existent handle
    socket = lookupHandle("user3");
    if (socket != -1)
    {
        fprintf(stderr, "lookupHandle incorrectly found 'user3'\n");
        exit(-1);
    }

    printf("lookupHandle passed!\n");
}

void testRemoveHandle()
{
    printf("Testing removeHandle...\n");

    handleTableCleanup(); // Ensure the table is empty before starting

    addHandle("user1", 1);
    addHandle("user2", 2);

    int result = removeHandle(1);
    if (result != 0)
    {
        fprintf(stderr, "Failed to remove handle with socket 1\n");
        exit(-1);
    }

    // Ensure the handle is removed
    int socket = lookupHandle("user1");
    if (socket != -1)
    {
        fprintf(stderr, "Handle 'user1' was not removed properly\n");
        exit(-1);
    }

    // Test removing a non-existent handle
    result = removeHandle(3);
    if (result != -1)
    {
        fprintf(stderr, "removeHandle incorrectly removed a non-existent handle\n");
        exit(-1);
    }

    printf("removeHandle passed!\n");
}

void testGetHandles()
{
    printf("Testing getHandles...\n");

    handleTableCleanup(); // Ensure the table is empty before starting

    addHandle("user1", 1);
    addHandle("user2", 2);

    char **handleList = NULL;
    int count = getHandles(&handleList);

    if (count != 2)
    {
        fprintf(stderr, "getHandles returned incorrect count: %d\n", count);
        exit(-1);
    }

    // Ensure the handles are correct
    if (strcmp(handleList[0], "user1") != 0 && strcmp(handleList[1], "user1") != 0)
    {
        fprintf(stderr, "getHandles did not return 'user1'\n");
        exit(-1);
    }

    if (strcmp(handleList[0], "user2") != 0 && strcmp(handleList[1], "user2") != 0)
    {
        fprintf(stderr, "getHandles did not return 'user2'\n");
        exit(-1);
    }

    // Free the handle list
    for (int i = 0; i < count; i++)
    {
        free(handleList[i]);
    }
    free(handleList);

    printf("getHandles passed!\n");
}

void testHandleTableCleanup()
{
    printf("Testing handleTableCleanup...\n");

    addHandle("user1", 1);
    addHandle("user2", 2);

    handleTableCleanup();

    // Ensure the table is empty
    if (lookupHandle("user1") != -1 || lookupHandle("user2") != -1)
    {
        fprintf(stderr, "handleTableCleanup did not clear the table properly\n");
        exit(-1);
    }

    printf("handleTableCleanup passed!\n");
}
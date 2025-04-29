// Outlines a handleTable data structure for managing resources in a chat program
// Lukas Shipley

#ifndef __HANDLE_TABLE_H__
#define __HANDLE_TABLE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HANDLES 100
#define MAX_HANDLE_LENGTH 100
#define MAX_BUFFER_SIZE 1024

typedef struct HandleNode
{
    char handle[MAX_HANDLE_LENGTH];
    int socket;
    struct HandleNode *next;
} HandleNode;

// typedef struct HandleTable
// {
//     HandleNode **table;
//     int handleCnt, capacity;
// } HandleTable;

// void handleTableInit(void);
int addHandle(char* handle, int socket);
int lookupHandle(char* handle);
int removeHandle(int socket);
int getHandles(char ***handleList);
void freeHandleList(char **handleList);
void handleTableCleanup(void);

#endif // __HANDLE_TABLE_H__

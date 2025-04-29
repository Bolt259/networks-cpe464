// implementation of handleTable data structure and functions

#include "handleTable.h"
#include "safeUtil.h"

HandleNode *head = NULL;
int handleCount = 0;
int capacity = 0;

// adds handle to the handle table
int addHandle(char *handle, int socket)
{
    if (handleCount >= MAX_HANDLES)
    {
        fprintf(stderr, "Error: Handle table is full\n");
        return -1;
    }

    // check if handle already exists
    HandleNode *curr = head;
    while (curr != NULL)
    {
        if (strcmp(curr->handle, handle) == 0)
        {
            return -1;  // handle already exists
        }
        curr = curr->next;
    }

    // create new handle node
    HandleNode *newNode = (HandleNode *)malloc(sizeof(HandleNode));
    if (newNode == NULL)
    {
        perror("malloc");
        exit(-1);
    }

    strcpy(newNode->handle, handle);
    newNode->socket = socket;
    newNode->next = head;
    head = newNode;
    handleCount++;
    
    return 0;
}

// returns the socket associated with the handle
int lookupHandle(char *handle)
{
    HandleNode *curr = head;
    while (curr != NULL)
    {
        if (strcmp(curr->handle, handle) == 0)
        {
            return curr->socket;
        }
        curr = curr->next;
    }
    return -1;  // handle not found
}

// removes the handle associated with the socket
int removeHandle(int socket)
{
    HandleNode *curr = head;
    HandleNode *prev = NULL;

    while (curr != NULL)
    {
        if (curr->socket == socket)
        {
            if (prev == NULL)
            {
                head = curr->next;
            }
            else
            {
                prev->next = curr->next;
            }
            free(curr);
            handleCount--;
            return 0;
        }
        prev = curr;
        curr = curr->next;
    }
    return -1;  // handle not found
}

// returns the number of handles copied into handleList
int getHandles(char ***handleList)
{
    HandleNode *curr = head;
    
    if (handleCount == 0)
    {
        *handleList = NULL;
        return 0;  // no handles to return
    }

    // allocate array
    *handleList = (char **)sCalloc(handleCount, sizeof(char *));
    if (*handleList == NULL)
    {
        perror("calloc");
        exit(-1);
    }

    // fill array
    // curr = head; curr is already set to head
    for (int i = 0; i < handleCount; i++)
    {
        (*handleList)[i] = strdup(curr->handle);
        if ((*handleList)[i] == NULL)
        {
            perror("strdup");
            exit(-1);
        }
        curr = curr->next;
    }
    return handleCount;  // return the number of handles copied
}

// frees all memory associated with the handle table
void handleTableCleanup(void)
{
    HandleNode *curr = head;
    while (curr != NULL)
    {
        HandleNode *temp = curr;
        curr = curr->next;
        free(temp);
    }
    head = NULL;
    handleCount = 0;
}

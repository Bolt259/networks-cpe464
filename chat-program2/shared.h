// Lukas Shipley
// call this message.h in the future

#ifndef __SHARED_H__
#define __SHARED_H__

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
#include <stdint.h>
#include <errno.h>

#include "handleTable.h"

#define MAX_MESSAGE_LENGTH 200  // including null
#define MAX_DEST_HANDLES 9

int buildMsgPacket(u_int8_t *packet, u_int8_t flag, char *senderHandle, char **destHandles, int numDestHandles, char *messageText);

#endif // __SHARED_H__

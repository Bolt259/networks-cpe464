// Lukas Shipley
// pdu.h

// PDU implementation for UDP

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "cpe464.h"

#define MAX_UDP 1500
#define MAX_PAYLOAD_LEN 1400
#define PDU_HEADER_LEN 6 // 4 bytes for sequence number, 2 bytes for checksum, 1 byte for flag

int createPDU(uint8_t * pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t * payload, int payloadLen);
void printPDU(uint8_t * pduBuffer, int pduLen);
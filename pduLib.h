#ifndef PDULIB_H
#define PDULIB_H

#include <stdint.h>

int createPDU(uint8_t *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen);
void printPDU(uint8_t *aPDU, int pduLength);
uint16_t calculateChecksum(uint8_t *pduBuffer, int pduLen);

#endif

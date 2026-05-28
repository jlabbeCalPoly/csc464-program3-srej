#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>

void setupCircularBuffer(int windowSize, int bufferSize);
void addToCircularBuffer(uint32_t sequenceNumber, int size, uint8_t *dataBuffer);
int getEntry(uint32_t sequenceNumber, uint8_t *dataBuffer);
int getValid(uint32_t sequenceNumber);
void setValid(uint32_t sequenceNumber, int value);

#endif

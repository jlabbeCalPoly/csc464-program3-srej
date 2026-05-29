#ifndef SERVER_SLIDING_WINDOW_H
#define SERVER_SLIDING_WINDOW_H

#include <stdint.h>

void setupSlidingWindow(int windowSize, int bufferSize);
void addToSlidingWindow(uint32_t sequenceNumber, int size, uint8_t *dataBuffer);
int getExpected();
void setExpected(int value);
int getHighest();
void setHighest(int value);
int getSequenceNumberSlidingWindow(uint32_t sequenceNumber);
int getValidSlidingWindow(uint32_t sequenceNumber);
void setValidSlidingWindow(uint32_t sequenceNumber, int value);
int getSlidingWindowEntry(uint32_t sequenceNumber, uint8_t *dataBuffer);

#endif

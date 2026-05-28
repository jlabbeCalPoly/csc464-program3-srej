#ifndef RCOPY_SLIDING_WINDOW_H
#define RCOPY_SLIDING_WINDOW_H

void setupSlidingWindow(int windowSize, int bufferSize);
void addToSlidingWindow(uint32_t sequenceNumber, int size, uint8_t *dataBuffer);
int incrementCurrent();
void incrementBounds(uint32_t rr, int windowSize);
int isWindowOpen();
int isServerDoneReceiving();
int getSlidingWindowEntry(uint32_t sequenceNumber, uint8_t *dataBuffer);
int getRR();

#endif

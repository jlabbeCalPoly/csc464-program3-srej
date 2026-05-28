#include <stdint.h>

#include "circularBuffer.h"

int highest = -1;
int expected = 0;

// Setup the sliding window for rcopy, adjust the upper bound and setup the circular buffer
void setupSlidingWindow(int windowSize, int bufferSize) {
    setupCircularBuffer(windowSize, bufferSize);
}

void addToSlidingWindow(uint32_t sequenceNumber, int size, uint8_t *dataBuffer) {
    addToCircularBuffer(sequenceNumber, size, dataBuffer);
}

int getExpected() {
    return expected;
}

void setExpected(int value) {
    expected = value;
}

int getHighest() {
    return highest;
}

void setHighest(int value) {
    highest = value;
}

int getValidSlidingWindow(uint32_t sequenceNumber) {
    return getValid(sequenceNumber);
}

// Set the value of the valid field  in the circular buffer(either 0 for not valid, 1 for valid)
void setValidSlidingWindow(uint32_t sequenceNumber, int value) {
    setValid(sequenceNumber, value);
}

// Return the size of the buffer and store the data from the circular buffer into the provided buffer
int getSlidingWindowEntry(uint32_t sequenceNumber, uint8_t *dataBuffer) {
    return getEntry(sequenceNumber, dataBuffer);
}

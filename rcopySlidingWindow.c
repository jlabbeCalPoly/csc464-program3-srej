
#include "circularBuffer.h"

// Keep track of the bounds of the sliding window and the latest data packet sent, as well as the latest RR packet received
int lower = 0;
int upper = 0;
int current = -1;
int rr = 0;

// Setup the sliding window for rcopy, adjust the upper bound and setup the circular buffer
void setupSlidingWindow(int windowSize, int bufferSize) {
    upper = windowSize;
    setupCircularBuffer(windowSize, bufferSize);
}

void addToSlidingWindow(uint32_t sequenceNumber, int size, uint8_t *dataBuffer) {
    addToCircularBuffer(sequenceNumber, size, dataBuffer);
}

// Increment current and return the value (representing the sequence number)
int incrementCurrent() {
    current++;
    return current;
}

// Increment the bounds of the sliding window based on the provided rr
void incrementBounds(uint32_t rrReceived, int windowSize) {
    rr = rrReceived;
    lower = rrReceived;
    upper = rrReceived + windowSize;
} 

// Returns 0 if the window is closed (ie. upper and the sequence number for the packet to be sent are the same)
int isWindowOpen() {
    return upper - (current + 1);
}

// Determines if the server has all of the data packets
int isServerDoneReceiving() {
    return rr > current;
}

// Return the size of the buffer and store the data from the circular buffer into the provided buffer
int getSlidingWindowEntry(uint32_t sequenceNumber, uint8_t *dataBuffer) {
    return getEntry(sequenceNumber, dataBuffer);
}

// Represents the lowest possible packet in the buffer
int getRR() {
    return rr;
}
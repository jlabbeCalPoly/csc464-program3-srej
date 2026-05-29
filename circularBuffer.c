// Manage a circular buffer
#include <stdio.h>

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int cachedWindowSize = 0;
struct bufferEntry {
    int valid; // Used by the server
    int sequenceNumber;
    int size;
    uint8_t buffer[];
};
struct bufferEntry **buffer;

/**
 * Malloc space for the circular buffer
 * 
 * @param windowSize The maximum amount of packets that can be sent at once
 * @param bufferSize The maximum amount of bytes that can be in a data packet
 */
void setupCircularBuffer(int windowSize, int bufferSize) {
    cachedWindowSize = windowSize;
    buffer = malloc(windowSize * sizeof(struct bufferEntry *));

    for (int i = 0; i < windowSize; i++) {
        buffer[i] = malloc(sizeof(struct bufferEntry) + bufferSize + 7);
        buffer[i]->size = 0;
        buffer[i]->sequenceNumber = -1;
        buffer[i]->valid = 0;
    }
}

// Calculate the index using a provided sequence number and window size
int getIndex(uint32_t sequenceNumber) {
    return sequenceNumber % cachedWindowSize;
}

// Add an entry to the circular buffer based on the provided sequence number and details
void addToCircularBuffer(uint32_t sequenceNumber, int size, uint8_t *dataBuffer) {
    uint32_t index = getIndex(sequenceNumber);

    buffer[index]->sequenceNumber = sequenceNumber;
    buffer[index]->size = size;
    memcpy(buffer[index]->buffer, dataBuffer, size);
}

int getEntry(uint32_t sequenceNumber, uint8_t *dataBuffer) {
    uint32_t index = getIndex(sequenceNumber);

    memcpy(dataBuffer, buffer[index]->buffer, buffer[index]->size);
    return buffer[index]->size;
}

// Retrieve the sequence number at the corresponding index inside the circular buffer
int getSequenceNumber(uint32_t sequenceNumber) {
    uint32_t index = getIndex(sequenceNumber);
    return buffer[index]->sequenceNumber;
}

int getValid(uint32_t sequenceNumber) {
    uint32_t index = getIndex(sequenceNumber);
    return buffer[index]->valid;
}

// Set the value of the valid field (either 0 for not valid, 1 for valid)
void setValid(uint32_t sequenceNumber, int value) {
    uint32_t index = getIndex(sequenceNumber);
    buffer[index]->valid = value;
}

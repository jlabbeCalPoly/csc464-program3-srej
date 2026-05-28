// Manage a circular buffer
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int cachedWindowSize = 0;
int cachedBufferSize = 0;
struct bufferEntry {
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
    cachedBufferSize = bufferSize;
    buffer = malloc(windowSize * sizeof(struct bufferEntry));

    for (int i = 0; i < windowSize; i++) {
        buffer[i] = malloc(sizeof(struct bufferEntry) + bufferSize + 7);
        buffer[i]->size = 0;
    }
}

// Add an entry to the circular buffer based on the provided sequence number and details
void addToCircularBuffer(uint32_t sequenceNumber, int size, uint8_t *dataBuffer) {
    uint32_t index = sequenceNumber % cachedWindowSize;
    buffer[index]->size = size;
    memcpy(buffer[index]->buffer, dataBuffer, size);
}

int getEntry(uint32_t sequenceNumber, uint8_t *dataBuffer) {
    uint32_t index = sequenceNumber % cachedWindowSize;

    memcpy(dataBuffer, buffer[index]->buffer, buffer[index]->size);
    return buffer[index]->size;
}
 
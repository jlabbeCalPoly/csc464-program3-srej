#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "cpe464.h"

// Handles copying data into the portion of the pduBuffer that's designated as the header
void copyHeader(uint8_t *pduBuffer, uint32_t sequenceNumber, uint8_t flag) {
    // Convert the sequence number into network order, then memcpy it into the pduBuffer
    uint32_t sequenceNumberNet = htonl(sequenceNumber);
    memcpy(pduBuffer, &sequenceNumberNet, 4);

    // Temporarily copy two 0 bytes into the checksum field, which will be calculated later
    uint16_t temporaryChecksum = 0;
    memcpy(pduBuffer + 4, &temporaryChecksum, 2);

    // Copy the flag into the pduBuffer
    memcpy(pduBuffer + 6, &flag, 1);
}

// Simply copies the contents of the payload into the pduBuffer
void copyPayload(uint8_t *pduBuffer, int headerLen, uint8_t *payload, int payloadLen) {
    memcpy(pduBuffer + headerLen, payload, payloadLen);
}

// Calculates the checksum, then copies the complement value into the pduBuffer
void calculateAndCopyChecksum(uint8_t *pduBuffer, int pduLen) {
    // Store the complement of the checksum value returned by in_cksum, then memcpy it into the pduBuffer
    uint16_t checksum = in_cksum((unsigned short *)pduBuffer, pduLen);
    memcpy(pduBuffer + 4, &checksum, 2);
}

// Calculates the checksum value (used to check if the recieved pdu is corrupted or not)
uint16_t calculateChecksum(uint8_t *pduBuffer, int pduLen) {
    return in_cksum((unsigned short *)pduBuffer, pduLen);
}
 
/**
 * Creates a pdu in the required format for lab and program #3, returning the length of the pdu
 */
int createPDU(uint8_t *pduBuffer, uint32_t sequenceNumber, uint8_t flag, uint8_t *payload, int payloadLen) {
    int headerLen = 7;
    int pduLen = headerLen + payloadLen;

    copyHeader(pduBuffer, sequenceNumber, flag);
    copyPayload(pduBuffer, headerLen, payload, payloadLen);
    calculateAndCopyChecksum(pduBuffer, pduLen);

    return pduLen;
}

// print a "field-value"-style message for a string
void printStringFormat(char *field, uint8_t *value) {
    printf("\t%s: %s\n", field, value);
}

// print a "field-value"-style message for an integer
void printIntFormat(char *field, int value) {
    printf("\t%s: %d\n", field, value);
}

// Extract the sequence number, print it
void printSequenceNumber(uint8_t *pduStart) {
    // Convert the sequence number back into host order, then print it
    uint32_t sequenceNumberNet;
    memcpy(&sequenceNumberNet, pduStart, 4);
    uint32_t sequenceNumberHost = ntohl(sequenceNumberNet);

    printIntFormat("Sequence Number", sequenceNumberHost);
}

// Extract the flag, print it
void printFlag(uint8_t *pduStart) {
    uint8_t flag;
    memcpy(&flag, pduStart, 1);

    printIntFormat("Flag", flag);
}

// Extract the payload from the pdu provided its start address and size
void printPayload(uint8_t *pduStart, uint8_t size) {
    uint8_t buffer[size + 1];
    memcpy(buffer, pduStart, size);
    buffer[size] = 0;

    printStringFormat("Payload", buffer);
}

// Print the payload length
void printPayloadLength(int payloadLen) {
    printIntFormat("Payload Length", payloadLen);
}

/**
 * Prints the contents of the pdu
 */
void printPDU(uint8_t * aPDU, int pduLength) {
     // first, validate the checksum
    uint16_t checksum;
    if ((checksum = in_cksum((unsigned short *)aPDU, pduLength)) != 0) {
        printf("PDU is corrupted, checksum value is: %d\n", checksum);
        return;
    }

    // Print out the message highlighting where the contents of the pdu will begin to print
    printf("Checksum passed, contents of the PDU:\n");

    // Next, calculate the size of the payload (7 is the size of the header)
    int payloadLen = pduLength - 7;

    // Now, print out each segment of the pdu
    printSequenceNumber(aPDU);
    printFlag(aPDU + 6);
    printPayload(aPDU + 7, payloadLen);
    printPayloadLength(payloadLen);
}
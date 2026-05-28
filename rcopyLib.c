#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>

#include "flags.h"
#include "states.h"
#include "pduLib.h"
#include "pollLib.h"
#include "safeUtil.h"
#include "rcopySlidingWindow.h"

// Constants for rcopyLib
#define MOVE_TO_DONE_STATE -1
#define STAY_IN_DATA_STATE 0

// Setup for rcopy
RCOPY_STATE onStart(int socketNum, int windowSize, int bufferSize) {
    setupPollSet();
	addToPollSet(socketNum);

    setupSlidingWindow(windowSize, bufferSize);

    return RCOPY_FILENAME_STATE;
}

// Helper function for determining the next state after receiving a valid pdu from the server in the filename state
RCOPY_STATE onFilenameGetNextState(int recvLen, uint8_t recvBuffer[], char *toFilename) {
    if (recvLen == 0) {
        return RCOPY_DONE_STATE;
    }

    uint8_t flag = getFlag(recvBuffer + 6);
    // Response packet may be the for the filename response
    if (flag == FILENAME_RESPONSE_FLAG) {
        uint8_t filenameFlag = getFlag(recvBuffer + 7);
        if (filenameFlag == FILENAME_OK_FLAG) {
            return RCOPY_DATA_STATE;
        } else {
            printf("Error on open of output file: %s\n", toFilename);
        }
    // Response packet may also be an RR in the event of the filename packet being lost
    } else if (flag == RR_FLAG) {
        return RCOPY_DATA_STATE;
    }

    return RCOPY_DONE_STATE;
}

/**
 * Stop and wait procedure for filename transfer
 * 
 * @param socketNum The main server socket
 * @param server Information on the server
 * @param serverAddrLen The size of the sockaddr_in6
 * @param toFilename The filename to be created/written to on the server
 * @param MAXBUF The maximum buffer size
 */ 
RCOPY_STATE onFilename(
    int socketNum, 
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int windowSize,
    int bufferSize,
    int MAXBUF
) {
    int pollRecv = 0;
    uint8_t counter = 0;

    // Copy the toFilename into a uint8_t payload buffer
    int filenameLen = strlen(toFilename);
    // 8 additional bytes for the windowSize and bufferSize
    int payloadLen = filenameLen + 8;
    uint8_t payload[payloadLen];
    uint32_t windowSizeNet = htonl(windowSize);
    uint32_t bufferSizeNet = htonl(bufferSize);
    memcpy(payload, &windowSizeNet, 4);
    memcpy(payload + 4, &bufferSizeNet, 4);
    memcpy(payload + 8, toFilename, filenameLen);

    // Build buffer to store the pdu contents and space to receieve the response from the server
	uint8_t pduBuffer[payloadLen + 7];
	uint8_t recvBuffer[MAXBUF + 7];
	
    int pduLen = createPDU(pduBuffer, 0, FILENAME_FLAG, payload, payloadLen);
    while (counter < 10) {
        safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, serverAddrLen);
        // Data to process from the server if pollCall doesn't return -1
        if ((pollRecv = pollCall(1000)) != -1) {
            int recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) server, &serverAddrLen);   
            // debug
		    // printPDU(recvBuffer, recvLen); 

            // Check for data corruption
            if (calculateChecksum(recvBuffer, recvLen) == 0) {
                return onFilenameGetNextState(recvLen, recvBuffer, toFilename);
            } else {
                printf("Data corrupted...\n");
            }
        }
        // Timeout occurred or the received pdu was corrupted, increment the counter and resend the pdu
        counter++;
        printf("Incrementing counter: %d\n", counter);
    };

    return RCOPY_DONE_STATE;
}

void onDataRR(uint32_t sequenceNumberHost, int windowSize) {
    incrementBounds(sequenceNumberHost, windowSize);
}

// Resend a specific packet
void onDataResend(uint32_t sequenceNumberHost, int socketNum, struct sockaddr_in6 * server, int serverAddrLen, int bufferSize) {
    uint8_t dataBuffer[bufferSize + 7];
    int size = getSlidingWindowEntry(sequenceNumberHost, dataBuffer);
    safeSendto(socketNum, dataBuffer, size, 0, (struct sockaddr *) server, serverAddrLen);
}

// Handle sending the RR or SREJ in the event poll returned
void handlePollReturn(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize,
    int MAXBUF
) {
    uint8_t recvBuffer[MAXBUF + 7];
    int recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) server, &serverAddrLen);

    if (recvLen == 0) {
        perror("Error: recvLen is 0");
        exit(-1);
    }

    // Check for data corruption
    if (calculateChecksum(recvBuffer, recvLen) != 0) {
        return;
    }

    uint8_t flag = getFlag(recvBuffer + 6);
    uint32_t sequenceNumberNet;
    memcpy(&sequenceNumberNet, recvBuffer + 7, 4);
    uint32_t sequenceNumberHost = ntohl(sequenceNumberNet);

    if (flag == RR_FLAG) {
        onDataRR(sequenceNumberHost, windowSize);
    } else {
        // SREJ, resend the specific packet
        onDataResend(sequenceNumberHost, socketNum, server, serverAddrLen, bufferSize);
    }
}

// Poll for one second and resend the lowest data packet if needed. Returns the 
int onDataPoll(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize,
    int MAXBUF
) {
    int pollRecv = 0;
    int counter = 0;

    while ((pollRecv = pollCall(1000)) == -1) {
        // Timeout and counter > 9, meaning other side most likely terminated
        if (counter > 9) {
            printf("Moving to done state...\n");
            return MOVE_TO_DONE_STATE;
        }
        // Increment the timer and resend the lowest packet
        counter++;
        printf("Incrementing counter: %d\n", counter);
        int lowestSequenceNumber = getRR();
        onDataResend(lowestSequenceNumber, socketNum, server, serverAddrLen, bufferSize);
    }

    handlePollReturn(
        socketNum, 
        server,
        serverAddrLen,
        windowSize,
        bufferSize,
        MAXBUF
    );

    return STAY_IN_DATA_STATE;
}

// Process while the window is open
int onDataOpen(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize, 
    int fileDescriptor,
    int MAXBUF
) {
    int action = STAY_IN_DATA_STATE;
    uint8_t readBuffer[bufferSize];

    while (isWindowOpen() != 0 && action == STAY_IN_DATA_STATE) {
        ssize_t readBytes = read(fileDescriptor, readBuffer, bufferSize);
        if (readBytes < 0) {
            perror("Error: read returned a negative value\n");
            exit(-1);
        } else if (readBytes == 0) {
            if (isServerDoneReceiving()) {
                uint8_t pduBuffer[7];
                int pduLen = createPDU(pduBuffer, 0, DATA_FLAG, readBuffer, 0);

                // Send the data packet to the server that signfies EOF (no data), then move to the done state
                safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, serverAddrLen);
                action = MOVE_TO_DONE_STATE;
            } else {
                action = onDataPoll(
                    socketNum, 
                    server,
                    serverAddrLen,
                    windowSize,
                    bufferSize,
                    MAXBUF
                );
            }
        } else {
            uint8_t pduBuffer[readBytes + 7];
            uint32_t sequenceNumber = incrementCurrent();
            printf("Packet sent with seq number: %d\n", sequenceNumber);

            int pduLen = createPDU(pduBuffer, sequenceNumber, DATA_FLAG, readBuffer, readBytes);

            printf("Created pdu\n");

            addToSlidingWindow(sequenceNumber, pduLen, pduBuffer);

            printf("Added to sliding window, childsocket is: %d\n", socketNum);

            safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) server, serverAddrLen);

            printf("Safe sent\n");

            int pollRecv = 0;
            // handle any RR/SREJ packets
            while ((pollRecv = pollCall(0)) != -1) {
                handlePollReturn(
                    socketNum, 
                    server,
                    serverAddrLen,
                    windowSize,
                    bufferSize,
                    MAXBUF
                );
            }
        }
    }

    return action;
}

int onDataClosed(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize,
    int MAXBUF
) {
    int action = STAY_IN_DATA_STATE;

    while (isWindowOpen() == 0 && action == STAY_IN_DATA_STATE) {
        action = onDataPoll(
            socketNum, 
            server,
            serverAddrLen,
            windowSize,
            bufferSize,
            MAXBUF
        );
    }

    return action;
}

RCOPY_STATE onData(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize, 
    int fileDescriptor,
    int MAXBUF
) {
    int onDataOpenStatus = onDataOpen(
        socketNum,
        server,
        serverAddrLen,
        windowSize,
        bufferSize,
        fileDescriptor,
        MAXBUF
    );
    int onDataClosedStatus = onDataClosed(
        socketNum,
        server,
        serverAddrLen,
        windowSize,
        bufferSize,
        MAXBUF
    );

    if (onDataOpenStatus == MOVE_TO_DONE_STATE || onDataClosedStatus == MOVE_TO_DONE_STATE) {
        return RCOPY_DONE_STATE;
    } else {
        return RCOPY_DATA_STATE;
    }
}
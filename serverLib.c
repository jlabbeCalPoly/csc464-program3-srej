#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include "flags.h"
#include "states.h"
#include "pduLib.h"
#include "pollLib.h"
#include "serverSlidingWindow.h"
#include "safeUtil.h"
#include "networks.h"
#include "cpe464.h"

// Constants for serverLib
#define MOVE_TO_DONE_STATE -1
#define STAY_IN_DATA_STATE 0

// Handles setting up the child server process appropriately, returns the new socket
int onStart(struct sockaddr_in6 client, float errorRate, uint8_t *payload) {
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
    int socket = createUdpSocket();

    // Initialize the poll set for the child server (listens on this socket)
    setupPollSet();
	addToPollSet(socket);

    // Initalize the sliding window/circular buffer for the child server (based on the provided window and buffer sizes)
    uint32_t windowSizeNet = 0;
    memcpy(&windowSizeNet, payload, 4);
    u_int32_t windowSizeHost = ntohl(windowSizeNet);

    uint32_t bufferSizeNet = 0;
    memcpy(&bufferSizeNet, payload + 4, 4);
    u_int32_t bufferSizeHost = ntohl(bufferSizeNet);
    
    setupSlidingWindow(windowSizeHost, bufferSizeHost);

    return socket;
}

int getFileDescriptor(uint8_t *payload, uint8_t payloadLen) {
    char filename[payloadLen + 1];
    memcpy(filename, payload, payloadLen);
    filename[payloadLen] = '\0';

    printf("Payload is: %s", filename);

    int fileDesriptor = open(filename, O_WRONLY | O_CREAT);
    return fileDesriptor;
}

// Validate that the file can be created and written to on the server
int getFlagFromFileDescriptor(int fileDescriptor) {
    if (fileDescriptor < 0) {
        return FILENAME_BAD_FLAG;
    } else {
        return FILENAME_OK_FLAG;
    }
}

// Simply send the packet with the filename response once (onData handles if this packet is lost), returns the following state
SERVER_STATE onFilename(
    int socketNum, 
    struct sockaddr_in6 client,
    int clientAddrLen,
    uint8_t *payload, 
    uint8_t payloadLen,
    int fileDesciptor
) {
    uint8_t filenameFlag = getFlagFromFileDescriptor(fileDesciptor);
    // 7 bytes for the header, 1 byte for the fname ok/bad flag
    uint8_t pduBuffer[8];
    int pduLen = createPDU(pduBuffer, 0, FILENAME_RESPONSE_FLAG, &filenameFlag, 1);
    safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) &client, clientAddrLen);

    // Determine which state to move the child server process into
    if (filenameFlag == FILENAME_OK_FLAG) {
        return SERVER_DATA_STATE;
    } else {
        return SERVER_DONE_STATE;
    }
}

// Poll for data, handling timeouts appropriately
int onDataPoll() {
    int counter = 0;
    while (pollCall(1000) == -1) {
        // Timeout and counter > 9, meaning other side most likely terminated
        if (counter > 9) {
            printf("Moving to done state...\n");
            return MOVE_TO_DONE_STATE;
        }
        // Increment the timer and resend the lowest packet
        counter++;
    }
    return STAY_IN_DATA_STATE;
}

// Send a response packet back to rcopy
void sendPacket(
    int socketNum, 
    struct sockaddr_in6 client,
    int clientAddrLen,
    uint8_t flag, 
    u_int32_t sequenceNumber
) {
    uint32_t sequenceNumberNet = htonl(sequenceNumber);

    uint8_t pduBuffer[11];
    int pduLen = createPDU(pduBuffer, 0, flag, &sequenceNumberNet, 1);
    safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) &client, clientAddrLen);
}

void bufferPacket(uint8_t sequenceNumberHost, int recvLen, uint8_t recvBuffer[]) {
    addToSlidingWindow(sequenceNumberHost, recvLen, recvBuffer);
    setValidSlidingWindow(sequenceNumberHost, 1);
    setHighest(sequenceNumberHost);
}

void onDataFlush(
    int socketNum,
    struct sockaddr_in6 client,
    int clientAddrLen,
    int MAXBUF,
    int bufferSize,
    int fileDescriptor
) {
    uint8_t dataBuffer[bufferSize + 7];
    int expected = getExpected();
    while (getValidSlidingWindow(expected)) {
        int size = getSlidingWindowEntry(expected, dataBuffer);
        write(fileDescriptor, dataBuffer, size);
        setValidSlidingWindow(expected, 0);

        expected++;
        setExpected(expected);
    }

    sendPacket(socketNum, client, clientAddrLen, RR_FLAG, expected);
    if (expected < getHighest()) {
        sendPacket(socketNum, client, clientAddrLen, SREJ_FLAG, expected);

        onDataBuffer(
            socketNum,
            client,
            clientAddrLen,
            MAXBUF,
            fileDescriptor
        );
    } else {
        onDataInOrder(
            socketNum,
            client,
            clientAddrLen,
            MAXBUF,
            fileDescriptor
        );
    }
}

void onDataBuffer(
    int socketNum,
    struct sockaddr_in6 client,
    int clientAddrLen,
    int MAXBUF,
    int fileDescriptor
) {
    // If the max amount of timeouts occur, move to the done state
    if (onDataPoll() == MOVE_TO_DONE_STATE) {
        return;
    }

    uint8_t recvBuffer[MAXBUF + 7];
    int recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) &client, &clientAddrLen);

    if (recvLen == 0) {
        perror("Error: recvLen is 0");
        exit(-1);
    }

    // Check for data corruption
    if (calculateChecksum(recvBuffer, recvLen) == 0) {
        uint32_t sequenceNumberNet;
        memcpy(&sequenceNumberNet, recvBuffer + 7, 4);
        uint32_t sequenceNumberHost = ntohl(sequenceNumberNet);

        int expected = getExpected();
        if (sequenceNumberHost == expected) {
            write(fileDescriptor, recvBuffer + 7, recvLen - 7);
            expected++;
            setExpected(expected);

            onDataFlush(
                socketNum,
                client,
                clientAddrLen,
                MAXBUF,
                bufferSize,
                fileDescriptor
            );
        } else {
            if (sequenceNumberHost > expected) {
                bufferPacket(sequenceNumberHost, recvLen, recvBuffer);
            }

            onDataBuffer(
                socketNum,
                client,
                clientAddrLen,
                MAXBUF,
                fileDescriptor
            );
        }
    } else {
        onDataBuffer(
            socketNum,
            client,
            clientAddrLen,
            MAXBUF,
            fileDescriptor
        );
    }
}

// Handle the state when packets are arriving as expected
void onDataInOrder(
    int socketNum,
    struct sockaddr_in6 client,
    int clientAddrLen,
    int MAXBUF,
    int fileDescriptor
) {
    // If the max amount of timeouts occur, move to the done state
    if (onDataPoll() == MOVE_TO_DONE_STATE) {
        return;
    }

    uint8_t recvBuffer[MAXBUF + 7];
    int recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) &client, &clientAddrLen);

    if (recvLen == 0) {
        perror("Error: recvLen is 0");
        exit(-1);
    }

    // Check for data corruption
    if (calculateChecksum(recvBuffer, recvLen) == 0) {
        uint8_t flag = getFlag(recvBuffer + 6);
        // filename packet was lost, need to resend
        if (flag == SETUP_FLAG) {
            uint8_t pduBuffer[8];
            int pduLen = createPDU(pduBuffer, 0, FILENAME_RESPONSE_FLAG, &FILENAME_OK_FLAG, 1);
            safeSendto(socketNum, pduBuffer, pduLen, 0, (struct sockaddr *) &client, clientAddrLen);

            // continue as if nothing was received
            onDataInOrder(
                socketNum,
                client,
                clientAddrLen,
                MAXBUF,
                fileDescriptor
            );
        } else {
            // data packet flag, determine if it contains data or represents the EOF (no data other than the header)
            if (recvLen == 7) {
                return;
            }

            uint32_t sequenceNumberNet;
            memcpy(&sequenceNumberNet, recvBuffer + 7, 4);
            uint32_t sequenceNumberHost = ntohl(sequenceNumberNet);

            int expected = getExpected();
            if (sequenceNumberHost == expected) {
                write(fileDescriptor, recvBuffer + 7, recvLen - 7);
                setHighest(expected);

                expected++;
                setExpected(expected);
                sendPacket(socketNum, client, clientAddrLen, RR_FLAG, expected);

                onDataInOrder(
                    socketNum,
                    client,
                    clientAddrLen,
                    MAXBUF,
                    fileDescriptor
                );
            } else if (sequenceNumberHost < expected) {
                // retransmit the highest rr
                sendPacket(socketNum, client, clientAddrLen, RR_FLAG, expected);

                onDataInOrder(
                    socketNum,
                    client,
                    clientAddrLen,
                    MAXBUF,
                    fileDescriptor
                );
            } else {
                // unexpected packet (greater than expected), SREJ and buffer it
                sendPacket(socketNum, client, clientAddrLen, SREJ_FLAG, expected);
                bufferPacket(sequenceNumberHost, recvLen, recvBuffer);

                onDataBuffer(
                    socketNum,
                    client,
                    clientAddrLen,
                    MAXBUF,
                    fileDescriptor
                );
            }
        }

        // printPDU(recvBuffer, recvLen);
    } else {
        // Throw away packet, continue as if nothing was received
        onDataInOrder(
            socketNum,
            client,
            clientAddrLen,
            MAXBUF,
            fileDescriptor
        );
    }

}
 
SERVER_STATE onData(
    int socketNum,
    struct sockaddr_in6 client,
    int clientAddrLen,
    int MAXBUF,
    int fileDescriptor
) {
    int pollRecv = 0;
    uint8_t recvBuffer[MAXBUF + 7];

    while ((pollRecv = pollCall(1000)) != -1) {
        int recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) &client, &clientAddrLen);

        if (recvLen == 0) {
            perror("Error: recvLen is 0");
            exit(-1);
        }

        // Check for data corruption
        if (calculateChecksum(recvBuffer, recvLen) == 0) {
            printPDU(recvBuffer, recvLen);
        }
    }

    return SERVER_DONE_STATE;
}

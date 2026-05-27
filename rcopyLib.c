#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#include "flags.h"
#include "states.h"
#include "pduLib.h"
#include "pollLib.h"
#include "safeUtil.h"

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
 * @param newSocket Buffer for the new socket number that will be returned from a response from the server
 * @param server Information on the server
 * @param serverAddrLen The size of the sockaddr_in6
 * @param toFilename The filename to be created/written to on the server
 * @param MAXBUF The maximum buffer size
 */ 
RCOPY_STATE onFilename(
    int socketNum, 
    int newSocket,
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int MAXBUF
) {
    printf("In filename\n");
    int pollRecv = 0;
    uint8_t counter = 0;

    // Copy the toFilename into a uint8_t payload buffer
    int payloadLen = strlen(toFilename);
    uint8_t payload[payloadLen];
    memcpy(payload, toFilename, payloadLen);

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
		    printPDU(recvBuffer, recvLen); 

            // Check for data corruption
            if (calculateChecksum(recvBuffer, recvLen) == 0) {
                // Update the server address with the new port
                uint16_t port = ntohs(server->sin6_port);
                memcpy(&newSocket, &port, 2);
                printf("Old port: %d  New port: %d", socketNum, port);

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
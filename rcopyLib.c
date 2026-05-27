#include <string.h>
#include <stdio.h>
#include <netinet/in.h>

#include "flags.h"
#include "states.h"
#include "pduLib.h"
#include "pollLib.h"
#include "safeUtil.h"

// Helper function for determining the next state after receiving a valid pdu from the server in the filename state
RCOPY_STATE onFilenameGetNextState(int recvLen, uint8_t recvBuffer[]) {
    if (recvLen == 0) {
        return DONE_STATE;
    }

    uint8_t flag = getFlag(recvBuffer + 6);
    if (flag == FILENAME_RESPONSE_FLAG) {
        uint8_t filenameFlag = getFlag(recvBuffer + 7);
        if (filenameFlag == FILENAME_OK_FLAG) {
            return DATA_STATE;
        }
    }

    return DONE_STATE;
}

/**
 * Stop and wait procedure for filename transfer
 * 
 * @param socketNum The main server socket
 * @param server Information on the server =
 * @param serverAddrLen The size of the sockaddr_in6
 * @param toFilename The filename to be created/written to on the server
 * @param MAXBUF The maximum buffer size 
 */ 
RCOPY_STATE onFilename(
    int socketNum, 
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int MAXBUF
) {
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
            if (calculateChecksum(recvBuffer, recvLen) != 0) {
                return onFilenameGetNextState(recvLen, recvBuffer);
            }
        }
        // Timeout occurred or the received pdu was corrupted, increment the counter and resend the pdu
        printf("Incrementing counter: %d\n", counter);
        counter++;
    };

    return DONE_STATE;
}
#include <netinet/in.h>
#include <fcntl.h>

#include "flags.h"
#include "states.h"
#include "pduLib.h"
#include "pollLib.h"
#include "safeUtil.h"
#include "networks.h"
#include "cpe464.h"

// Handles setting up the child server process appropriately, returns the new socket
int onStart(struct sockaddr_in6 client, float errorRate) {
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
    int socket = createUdpSocket();

    // Initialize the poll set for this process (listens on this socket)
    setupPollSet();
	addToPollSet(socket);

    // debug
    printf("Created a new UDP socket: %d\n", socket);

    return socket;
}

// Validate that the file can be created and written to on the server
int onFilenameValidateFilename(uint8_t payload[], uint8_t payloadLen) {
    char filename[payloadLen + 1];
    memcpy(filename, payload, payloadLen);
    filename[payloadLen] = '\0';

    if (open(filename, O_WRONLY | O_CREAT) < 0) {
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
    uint8_t payload[], 
    uint8_t payloadLen
) {
    uint8_t filenameFlag = onFilenameValidateFilename(payload, payloadLen);
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

SERVER_STATE onData() {
    return SERVER_DATA_STATE;
}

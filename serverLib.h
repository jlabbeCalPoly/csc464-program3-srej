#ifndef SERVER_LIB_H
#define SERVER_LIB_H

#include <stdint.h>
#include "states.h"

int onStart(struct sockaddr_in6 client, float errorRate, uint8_t *payload);

SERVER_STATE onFilename(
    int socketNum, 
    struct sockaddr_in6 client,
    int clientAddrLen,
    uint8_t *payload, 
    uint8_t payloadLen
);

#endif
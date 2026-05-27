#ifndef RCOPY_LIB_H
#define RCOPY_LIB_H

RCOPY_STATE onFilename(
    int socketNum, 
    int newSocket,
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int windowSize,
    int bufferSize,
    int MAXBUF
);

#endif

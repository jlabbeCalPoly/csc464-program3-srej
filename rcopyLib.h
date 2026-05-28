#ifndef RCOPY_LIB_H
#define RCOPY_LIB_H

RCOPY_STATE onStart(int socketNum, int windowSize, int bufferSize);

RCOPY_STATE onFilename(
    int socketNum, 
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int windowSize,
    int bufferSize,
    int MAXBUF
);

RCOPY_STATE onData(
    int socketNum, 
    struct sockaddr_in6 * server,
    int serverAddrLen,
    int windowSize,
    int bufferSize, 
    int fileDescriptor,
    int MAXBUF
);

#endif

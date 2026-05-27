#ifndef RCOPY_LIB_H
#define RCOPY_LIB_H

RCOPY_STATE onFilename(
    int socketNum, 
	struct sockaddr_in6 * server,
    int serverAddrLen,
	char *toFilename,
    int MAXBUF
);

#endif

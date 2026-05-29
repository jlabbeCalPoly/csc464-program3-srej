// Client side - UDP Code				    
// By Hugh Smith	4/1/2017		

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pduLib.h"
#include "states.h"
#include "pollLib.h"
#include "rcopyLib.h"
#include "cpe464.h"

#define MAXBUF 1400

void processRcopy(int socketNum, 
	struct sockaddr_in6 * server, 
	int serverAddrLen, 
	char *fromFilename, 
	char *toFilename, 
	int windowSize, 
	int bufferSize, 
	int fileDescriptor
);
void validateArgs(int argc, char * argv[]);
int validateFilenames(char *fromFilename, char *toFilename);

int main(int argc, char *argv[]) {
	int socketNum = 0;				
	struct sockaddr_in6 server;		// Supports 4 and 6 but requires IPv6 struct
	int serverAddrLen = sizeof(struct sockaddr_in6);
	
	validateArgs(argc, argv);
	char *fromFilename = argv[1];
	char *toFilename = argv[2];
	int windowSize = atoi(argv[3]);
	int bufferSize = atoi(argv[4]);
	float errorRate = atof(argv[5]);
	char *hostName = argv[6];
	int hostNumber = atoi(argv[7]);
	int fileDescriptor = validateFilenames(fromFilename, toFilename);

	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, hostName, hostNumber);
	processRcopy(socketNum, &server, serverAddrLen, fromFilename, toFilename, windowSize, bufferSize, fileDescriptor);

	close(socketNum);

	return 0;
}

void processRcopy(
	int socketNum, 
	struct sockaddr_in6 * server,
	int serverAddrLen,
	char *fromFilename,
	char *toFilename,
	int windowSize,
	int bufferSize,
	int fileDescriptor
) {
	RCOPY_STATE state = RCOPY_START_STATE;

	while (state != RCOPY_DONE_STATE) {
		switch (state) {
			case RCOPY_START_STATE:
				state = onStart(socketNum, windowSize, bufferSize);
			case RCOPY_FILENAME_STATE:
				state = onFilename(
					socketNum,
					server, 
					serverAddrLen, 
					toFilename, 
					windowSize,
					bufferSize,
					MAXBUF
				);
				break;
			case RCOPY_DATA_STATE:
				state = onData(
					socketNum,
					server,
					serverAddrLen,
					windowSize,
					bufferSize,
					fileDescriptor,
					MAXBUF
				);
				break;
			case RCOPY_DONE_STATE:
				break;
		}
	}
}

// Make sure that both filenames are valid (under 100 characters) and that you can open the fromFilename
// Additionally, return the file descriptor to read from the fromFilename on success
int validateFilenames(char *fromFilename, char *toFilename) {
	if (strlen(fromFilename) > 100 || strlen(toFilename) > 100) {
		printf("error: maximum filename length is 100 characters\n");
		exit(1);
	}

	int fileDescriptor = 0;
	if ((fileDescriptor = open(fromFilename, O_RDONLY)) < 0) {
		printf("error: could not open the from-filename\n");
		exit(1);
	}
	return fileDescriptor;
}

void validateArgs(int argc, char * argv[]) {
    /* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s from-filename to-filename window-size buffer-size error-rate remote-machine remote-port\n", argv[0]);
		exit(1);
	}
}
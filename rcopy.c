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

// typedef enum State STATE;
// enum State {
// 	START, DONE, FILENAME, SEND_DATA, WAIT_ON_ACK, TIMEOUT_ON_ACK
// };
void talkToServer(int socketNum, struct sockaddr_in6 * server);
void processRcopy(int socketNum, struct sockaddr_in6 * server, int serverAddrLen, char *fromFilename, char *toFilename, int windowSize, int bufferSize);
int readFromStdin(uint8_t * buffer);
void validateArgs(int argc, char * argv[]);
void validateFilenames(char *fromFilename, char *toFilename);

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
	validateFilenames(fromFilename, toFilename);

	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
	socketNum = setupUdpClientToServer(&server, hostName, hostNumber);
	setupPollSet();
	addToPollSet(socketNum);
	processRcopy(socketNum, &server, serverAddrLen, fromFilename, toFilename, windowSize, bufferSize);
	// talkToServer(socketNum, &server);

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
	int bufferSize
) {
	RCOPY_STATE state = RCOPY_FILENAME_STATE;
	int newSocket = 0;

	while (state != RCOPY_DONE_STATE) {
		switch (state) {
			case RCOPY_FILENAME_STATE:
				state = onFilename(
					socketNum, 
					newSocket, 
					server, 
					serverAddrLen, 
					toFilename, 
					windowSize,
					bufferSize,
					MAXBUF
				);
				break;
			case RCOPY_DATA_STATE:
				state = RCOPY_DONE_STATE;
				printf("data state");
				break;
			case RCOPY_DONE_STATE:
				break;
		}
	}
}

void talkToServer(int socketNum, struct sockaddr_in6 * server) {
	int serverAddrLen = sizeof(struct sockaddr_in6);
	// char * ipString = NULL;
	
	// A few counters/defaults for this lab
	uint32_t sequenceNumberCounter = 0;
	uint8_t defaultFlag = 1;
	int dataLen = 0; 
	uint8_t buffer[MAXBUF];
	uint8_t recvBuffer[MAXBUF + 7];
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = readFromStdin(buffer);
		// printf("Sending: %s with len: %d\n", buffer,dataLen);
		
		// Include space for the header to be added on
		uint8_t pduBuffer[7 + dataLen];
		dataLen = createPDU(pduBuffer, sequenceNumberCounter, defaultFlag, buffer, dataLen);
		sequenceNumberCounter += 1;
	
		safeSendto(socketNum, pduBuffer, dataLen, 0, (struct sockaddr *) server, serverAddrLen);
		
		dataLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) server, &serverAddrLen);
		printPDU(recvBuffer, dataLen);
		
		// print out bytes received
		// ipString = ipAddressToString(server);
		// printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs(server->sin6_port), buffer);
	}
}

int readFromStdin(uint8_t *buffer) {
	int aChar = 0;
	int inputLen = 0;        
	
	// Important you don't input more characters than you have space 
	buffer[0] = '\0';
	printf("Enter data: ");
	while (inputLen < (MAXBUF - 1) && aChar != '\n')
	{
		aChar = getchar();
		if (aChar != '\n')
		{
			buffer[inputLen] = aChar;
			inputLen++;
		}
	}
	
	// Null terminate the string
	buffer[inputLen] = '\0';
	inputLen++;
	
	return inputLen;
}

// Make sure that both filenames are valid (under 100 characters) and that you can open the fromFilename
void validateFilenames(char *fromFilename, char *toFilename) {
	if (strlen(fromFilename) > 100 || strlen(toFilename) > 100) {
		printf("error: maximum filename length is 100 characters\n");
		exit(1);
	}

	if (open(fromFilename, O_RDONLY) < 0) {
		printf("error: could not open the from-filename\n");
		exit(1);
	}
}

void validateArgs(int argc, char * argv[]) {
    /* check command line arguments  */
	if (argc != 8)
	{
		printf("usage: %s from-filename to-filename window-size buffer-size error-rate remote-machine remote-port\n", argv[0]);
		exit(1);
	}
}






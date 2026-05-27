/* Server side - UDP Code				    */
/* By Hugh Smith	4/1/2017	*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"
#include "pduLib.h"
#include "cpe464.h"

#define MAXBUF 1400 // 1400 for the max payload length, 7 bytes for the header length

void processClient(int socketNum);
int checkArgs(int argc, char *argv[]);
void handleZombies(int sig);

int main ( int argc, char *argv[]  )
{ 
	int socketNum = 0;			
	int portNumber = 0;
	float errorRate = 0;	

	// checkArgs returns the port number to use (if provided)
	portNumber = checkArgs(argc, argv);
	errorRate = atof(argv[1]);

	// debug
	// printf("Error rate: %f\n", errorRate);

	sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
		
	socketNum = udpServerSetup(portNumber);

	processClient(socketNum);

	close(socketNum);
	
	return 0;
}

// Handles the main server logic (forking children processes when appropriate)
void processServer(int socketNum) {
	pid_t pid = 0;

	int recvLen = 0;
	uint8_t recvBuffer[MAXBUF];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	

	// Set the handler for when a process dies
	signal(SIGCHLD, handleZombies);
	while (1) {
		recvLen = safeRecvfrom(socketNum, recvBuffer, MAXBUF + 7, 0, (struct sockaddr *) &client, &clientAddrLen);
		// check for packet corruption
		if (calculateChecksum(recvBuffer, recvLen) != 0) {
			if ((pid = fork()) < 0) {
				perror("Error: fork\n");
				exit(-1);
			} else {
				printf("Child fork() - child pdi: %d\n", getpid());
				processClient(socketNum, recvBuffer, recvLen, client);
			}
		}
	}
}

// Handles the child servers
void processClient(int socketNum)
{
	int dataLen = 0; 
	uint8_t buffer[MAXBUF];	  
	struct sockaddr_in6 client;		
	int clientAddrLen = sizeof(client);	
	
	buffer[0] = '\0';
	while (buffer[0] != '.')
	{
		dataLen = safeRecvfrom(socketNum, buffer, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);
	
		printf("Received message from client with ");
		printIPInfo(&client);
		printf("Len: %d Message Len: %d Message: %s\n", dataLen, dataLen - 7, buffer + 7);

		printPDU(buffer, dataLen);

		// just for fun send back to client number of bytes received
		// sprintf(buffer, "bytes: %d", dataLen);

		// safeSendto(socketNum, buffer, dataLen, 0, (struct sockaddr *) & client, clientAddrLen);
	}
}

int checkArgs(int argc, char *argv[])
{
	// Checks args and returns port number
	int portNumber = 0;
	
	if (argc < 2 || argc > 4)
	{
		fprintf(stderr, "Usage %s error-rate [optional port number]\n", argv[0]);
		exit(-1);
	}
	
	if (argc == 3)
	{
		// Port number will be the third parameter
		portNumber = atoi(argv[2]);
	}
	
	return portNumber;
}

void handleZombies(int sig) {
	int stat = 0;
	// Kill any process (-1)
	while (waitpid(-1, &stat, WNOHANG) > 0) {}
}
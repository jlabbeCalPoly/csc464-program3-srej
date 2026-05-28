# udpCode makefile
# written by Hugh Smith - Feb 2021

CC = gcc
CFLAGS = -g -Wall -std=gnu99


SRC = networks.c  gethostbyname.c safeUtil.c pollLib.c
OBJS = networks.o gethostbyname.o safeUtil.o pollLib.o

#uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_

all:  rcopy server

rcopy: rcopy.c $(OBJS) 
	$(CC) $(CFLAGS) -o rcopy rcopy.c pduLib.c rcopyLib.c rcopySlidingWindow.c circularBuffer.c flags.c $(OBJS) $(LIBS)

server: server.c $(OBJS) 
	$(CC) $(CFLAGS) -o server server.c pduLib.c serverLib.c serverSlidingWindow.c circularBuffer.c flags.c $(OBJS) $(LIBS)

%.o: %.c *.h 
	gcc -c $(CFLAGS) $< -o $@ 

cleano:
	rm -f *.o

clean:
	rm -f rcopy server *.o


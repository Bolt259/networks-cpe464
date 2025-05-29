# Makefile for CPE464 tcp and udp test code
# updated by Hugh Smith - April 2023

# all target makes UDP test code
# tcpAll target makes the TCP test code


CC= gcc
CFLAGS= -g -Wall
LIBS = 

OBJS = networks.o gethostbyname.o pollLib.o safeUtil.o pdu.o window.o buffer.o srej.o

# uncomment next two lines if your using sendtoErr() library
LIBS += libcpe464.2.21.a -lstdc++ -ldl
CFLAGS += -D__LIBCPE464_


all: udpAll testClient testServer

udpAll: rcopy server
tcpAll: myClient myServer

rcopy: rcopy.c $(OBJS) 
	$(CC) $(CFLAGS) -o rcopy rcopy.c $(OBJS) $(LIBS)

server: server.c $(OBJS) 
	$(CC) $(CFLAGS) -o server server.c  $(OBJS) $(LIBS)

testClient: testClient.c $(OBJS)
	$(CC) $(CFLAGS) -o testClient testClient.c  $(OBJS) $(LIBS)

testServer: testServer.c $(OBJS)
	$(CC) $(CFLAGS) -o testServer testServer.c $(OBJS) $(LIBS)

# .c.o:
# 	gcc -c $(CFLAGS) $< -o $@ $(LIBS)
.c.o:
	$(CC) -c $(CFLAGS) $< -o $@

cleano:
	rm -f *.o

clean:
	rm -f testServer testClient rcopy server *.o





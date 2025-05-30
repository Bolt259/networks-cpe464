
// Hugh Smith April 2017
// Network code to support TCP/UDP client and server connections

#include "networks.h"

// This function sets the server socket. The function returns the server
// socket number and prints the port number to the screen.  

int tcpServerSetup(int serverPort)
{
	// Opens a server socket, binds that socket, prints out port, call listens
	// returns the mainServerSocket
	
	int mainServerSocket = 0;
	struct sockaddr_in6 serverAddress;     
	socklen_t serverAddressLen = sizeof(serverAddress);  

	mainServerSocket= socket(AF_INET6, SOCK_STREAM, 0);
	if(mainServerSocket < 0)
	{
		perror("socket call");
		exit(1);
	}

	memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
	serverAddress.sin6_family= AF_INET6;         		
	serverAddress.sin6_addr = in6addr_any;   
	serverAddress.sin6_port= htons(serverPort);         

	// bind the name (address) to a port 
	if (bind(mainServerSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("bind call");
		exit(-1);
	}
	
	// get the port name and print it out
	if (getsockname(mainServerSocket, (struct sockaddr*)&serverAddress, &serverAddressLen) < 0)
	{
		perror("getsockname call");
		exit(-1);
	}

	if (listen(mainServerSocket, LISTEN_BACKLOG) < 0)
	{
		perror("listen call");
		exit(-1);
	}
	
	printf("Server Port Number %d \n", ntohs(serverAddress.sin6_port));
	
	return mainServerSocket;
}

// This function waits for a client to ask for services.  It returns
// the client socket number.   

int tcpAccept(int mainServerSocket, int debugFlag)
{
	struct sockaddr_in6 clientAddress;   
	int clientAddressSize = sizeof(clientAddress);
	int client_socket = 0;

	if ((client_socket = accept(mainServerSocket, (struct sockaddr*) &clientAddress, (socklen_t *) &clientAddressSize)) < 0)
	{
		perror("accept call");
		exit(-1);
	}
	  
	if (debugFlag)
	{
		printf("Client accepted.  Client IP: %s Client Port Number: %d\n",  
				getIPAddressString6(clientAddress.sin6_addr.s6_addr), ntohs(clientAddress.sin6_port));
	}
	

	return(client_socket);
}

// This funciton opens a TCP socket, and connects to the server
// returns the socket number to the server

int tcpClientSetup(char * serverName, char * serverPort, int debugFlag)
{
	// This is used by the client to connect to a server using TCP
	
	int socket_num;
	uint8_t * ipAddress = NULL;
	struct sockaddr_in6 serverAddress;      
	
	// create the socket
	if ((socket_num = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
	{
		perror("socket call");
		exit(-1);
	}

	// setup the server structure
	memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
	serverAddress.sin6_family = AF_INET6;
	serverAddress.sin6_port = htons(atoi(serverPort));
	
	// get the address of the server 
	if ((ipAddress = gethostbyname6(serverName, &serverAddress)) == NULL)
	{
		exit(-1);
	}

	if(connect(socket_num, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("connect call");
		exit(-1);
	}

	if (debugFlag)
	{
		printf("Connected to %s IP: %s Port Number: %d\n", serverName, getIPAddressString6(ipAddress), atoi(serverPort));
	}
	
	return socket_num;
}

// This funciton creates a UDP socket on the server side and binds to that socket.  
// It prints out the port number and returns the socket number.

int udpServerSetup(int serverPort)
{
	struct sockaddr_in6 serverAddress;
	int socketNum = 0;
	int serverAddrLen = 0;	
	
	// create the socket
	if ((socketNum = socket(AF_INET6,SOCK_DGRAM,0)) < 0)
	{
		perror("socket() call error");
		exit(-1);
	}
	
	// set up the socket
	memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
	serverAddress.sin6_family = AF_INET6;    		// internet (IPv6 or IPv4) family
	serverAddress.sin6_addr = in6addr_any ;  		// use any local IP address
	serverAddress.sin6_port = htons(serverPort);   // if 0 = os picks 

	// bind the name (address) to a port
	if (bind(socketNum,(struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0)
	{
		perror("bind() call error");
		exit(-1);
	}

	/* Get the port number */
	serverAddrLen = sizeof(serverAddress);
	getsockname(socketNum,(struct sockaddr *) &serverAddress,  (socklen_t *) &serverAddrLen);
	printf("Server is using port: %d\n", ntohs(serverAddress.sin6_port));

	return socketNum;
	
}

int udpClientSetup(char * hostName, int serverPort, Connection * connection)
{
	memset(&connection->remote, 0, sizeof(struct sockaddr_in6));
	connection->socketNum = 0;
	connection->addrLen = sizeof(struct sockaddr_in6);
	connection->remote.sin6_family = AF_INET6; // use IPv6 family
	connection->remote.sin6_port = htons(serverPort);

	// create the socket
	connection->socketNum = safeGetUdpSocket();

	if (gethostbyname6(hostName, &connection->remote) == NULL)
	{
		fprintf(stderr, "Error: could not resolve host name %s\n", hostName);
		return -1;
	}

	printf("Server info - ");
	printIPInfo(&connection->remote);

	return 0;
}

// This function opens a socket and fills in the serverAdress structure using the hostName and serverPort.  
// It assumes the address structure is created before calling this.
// Returns the socket number and the filled in serverAddress struct.

// int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char * hostName, int serverPort)
// {
// 	int socketNum = 0;
// 	char ipString[INET6_ADDRSTRLEN];
// 	uint8_t * ipAddress = NULL;
	
// 	// create the socket
// 	if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
// 	{
// 		perror("socket() call error");
// 		exit(-1);
// 	}
  	 	
// 	memset(serverAddress, 0, sizeof(struct sockaddr_in6));
// 	serverAddress->sin6_port = ntohs(serverPort);
// 	serverAddress->sin6_family = AF_INET6;	
	
// 	if ((ipAddress = gethostbyname6(hostName, serverAddress)) == NULL)
// 	{
// 		exit(-1);
// 	}
		
	
// 	inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
// 	printf("Server info - IP: %s Port: %d \n", ipString, serverPort);
		
// 	return socketNum;
// }

int selectCall(int32_t sockNum, int32_t sec, int32_t usec)
{
	// uses select to wait for socket to be ready
	fd_set fd_var;
	struct timeval aTimeout;
	struct timeval * timeout = NULL;

	// if either time is -1 then block
	if (sec != -1 && usec != -1)
	{
		aTimeout.tv_sec = sec;
		aTimeout.tv_usec = usec;
		timeout = &aTimeout;
	}

	FD_ZERO(&fd_var);
	FD_SET(sockNum, &fd_var);

	if (select(sockNum + 1, &fd_var, NULL, NULL, timeout) < 0)
	{
		perror("select call");
		exit(-1);
	}

	if (FD_ISSET(sockNum, &fd_var))
	{
		return 1; // socket is ready
	}
	else
	{
		return 0; // socket is not ready
	}
}

int safeGetUdpSocket()
{
	int sockNum = 0;

	if ((sockNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
	{
		perror("safeGetUdpSocket(), socket() call: ");
		exit(-1);
	}
	return sockNum;
}

// safeSendto wrapper for Connection struct
int safeSendTo(uint8_t * buff, int len, Connection * to)
{
	int returnValue = 0;
	if ((returnValue = sendtoErr(to->socketNum, buff, (size_t) len, 0, (struct sockaddr *) &(to->remote), to->addrLen)) < 0)
	{
		perror("sendtoErr: ");
		exit(-1);
	}
	
	return returnValue;
}

// safeRecv wrapper for Connection struct
int safeRecvFrom(int recvSockNum, uint8_t * buff, int len, Connection * from)
{
	int returnValue = 0;
	if ((returnValue = recvfrom(recvSockNum, buff, (size_t) len, 0,(struct sockaddr *) &(from->remote), &(from->addrLen))) < 0)
	{
		perror("recvfrom: ");
		exit(-1);
	}

	//~!*
	// Print client remote address, address length, and port
	char ipString[INET6_ADDRSTRLEN];
	inet_ntop(AF_INET6, &from->remote.sin6_addr, ipString, sizeof(ipString));
	printf("\n{DEBUG} Sender remote addr: %s, addr len: %d, port: %d\n\n",
		ipString, from->addrLen, ntohs(from->remote.sin6_port));
	//~!*
	
	return returnValue;
}

/*
 * server.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "server.h"

int openConnection(int listenerPort, const char* serverName, const char* clientName){
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(listenerPort);

	int serverSocket = 0;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		printf("%s couldn't create socket for client %s: %s\n", serverName, clientName, strerror(errno));
		return -1;
	}

	int activated = 1;
	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated)) == -1){
		printf("%s had an error in setsockopt: %s\n", serverName, strerror(errno));
		close(serverSocket);
		return -1;
	}

	if (bind(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
		printf("%s couldn't bind the port %d: %s\n", serverName, listenerPort, strerror(errno));
		close(serverSocket);
		return -1;
	}

	if(listen(serverSocket, 100) == -1){
		printf("%s couldn't start to listen in port %d: %s\n", serverName, listenerPort, strerror(errno));
		close(serverSocket);
		return -1;
	}

	printf("%s could create the socket %d to listen to %s.\n", serverName, serverSocket, clientName);
	printf("Listening...\n");

	return serverSocket;
}

int acceptClient(int serverSocket, const char* serverName, const char* clientName){

	if(serverSocket < 0){
		printf("The socket (%d) where %s is listening to %s is not a valid one\n", serverSocket, serverName, clientName);
		return -1;
	}

	struct sockaddr_in clientAddress;
	unsigned int len = sizeof(clientAddress);
	int clientSocket = accept(serverSocket, (void*) &clientAddress, &len);
	if (clientSocket == -1){
		printf("%s couldn't accept the connection from %s: %s\n", serverName, clientName, strerror(errno));
		return -1;
	}

	printf("%s could accept the connection from %s and it's set in socket: %d\n", serverName, clientName, clientSocket);

	return clientSocket;
}

int handshakeWithClient(int clientSocket, int clientHandshakeValue, const char* serverName, const char* clientName){

	if(clientSocket < 0){
		printf("The socket (%d) where %s is trying to connect to %s is not a valid one\n", clientSocket, serverName, clientName);
		return -1;
	}

	if (send(clientSocket, &clientHandshakeValue, sizeof(int), 0) < 0){
		printf("%s couldn't send a message to %s: %s\n", serverName, clientName, strerror(errno));
		close(clientSocket);
		return -1;
	}

	int response = 0;
	int result_recv = 0;
	if((result_recv = recv(clientSocket, &response, sizeof(int), 0)) <= 0){
		printf("errno: %d, recv: %d", errno, result_recv);
		printf("recv failed on %s, while trying to connect with client %s: %s\n", serverName, clientName, strerror(errno));
		close(clientSocket);
		return -1;
	}

	if(response == clientHandshakeValue + 1){
		printf("%s could handshake with %s!\n", serverName, clientName);
	}else{
		printf("%s couldn't handshake with client %s, since the response was %d != %d\n", serverName, clientName, response, clientHandshakeValue);
		close(clientSocket);
		return -1;
	}

	return 0;
}

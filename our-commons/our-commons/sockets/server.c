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

int acceptUnknownClient(int serverSocket, const char* serverName){

	if(serverSocket < 0){
		printf("The socket (%d) where %s is listening is not a valid one\n", serverSocket, serverName);
		return -1;
	}

	struct sockaddr_in clientAddress;
	unsigned int len = sizeof(clientAddress);
	int clientSocket = accept(serverSocket, (void*) &clientAddress, &len);
	if (clientSocket == -1){
		printf("%s couldn't accept a new connection: %s\n", serverName, strerror(errno));
		return -1;
	}

	printf("%s could accept a new connection and it's set in socket: %d\n", serverName, clientSocket);

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
	int resultRecv = recv(clientSocket, &response, sizeof(int), 0);
	if(resultRecv <= 0){
		printf("recv failed on %s, while trying to connect with client %s: %s\n", serverName, clientName, strerror(errno));
		close(clientSocket);
		return -1;
	}

	if(response == clientHandshakeValue){
		printf("%s could handshake with %s!\n", serverName, clientName);
	}else{
		printf("%s couldn't handshake with client %s, since the response was %d != %d\n", serverName, clientName, response, clientHandshakeValue);
		close(clientSocket);
		return -1;
	}

	return 0;
}

int welcomeClient(int listenerPort, const char* serverName, const char* clientName, int handshakeValue, int (*welcomeProcedure)(int coordinadorSocket)){
	int serverToClientSocket = 0;
	if((serverToClientSocket = openConnection(listenerPort, serverName, clientName)) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(serverToClientSocket);
		return -1;
	}

	int clientSocket = 0;

	if((clientSocket = acceptUnknownClient(serverToClientSocket, serverName)) < 0){
		close(clientSocket);
		return -1;
	}

	int planificadorHandshakeResult = handshakeWithClient(clientSocket, handshakeValue, serverName, clientName);
	if(planificadorHandshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}

	welcomeProcedure(serverToClientSocket);

	close(clientSocket);
	close(serverToClientSocket);

	return 0;
}

int recieveClientId(int clientSocket,  const char* serverName){
	int id = 0;
	int resultRecv = recv(clientSocket, &id, sizeof(int), 0);
	if(resultRecv <= 0){
		printf("%s couldn't receive client id, from client socket %d: %s\n", serverName, clientSocket, strerror(errno));
		close(clientSocket);
		return -1;
	}

	return id;
}

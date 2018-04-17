/*
 * client.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "client.h"

int handshakeWithServer(char* serverIP, int serverPort, int handshakeValue, const char* serverName) {
	struct sockaddr_in serverAddress;
	serverAddress.sin_family  = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);
	serverAddress.sin_port = htons(serverPort);

	int serverSocket = 0;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("No pudo crearse el socket");
		return 1;
	}


	if (connect(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
		perror("No se pudo conectar");
		close(serverSocket);
		return 1;
	}

	printf("I could connect to %s\n", serverName);

	int response = 0;
	if(recv(serverSocket, &response, sizeof(int), 0) <= 0){
		perror("Problema con recv");
		close(serverSocket);
		return 1;
	}

	if(response == handshakeValue){
		printf("Hanshake with %s OK\n", serverName);
	}else{
		printf("It's not the server %s, since the response was: %d\n", serverName, response);
		close(serverSocket);
		return 1;
	}

	int clientHandshakeValue = response + 1;
	if (send(serverSocket, &clientHandshakeValue, sizeof(int), 0) < 0){
		perror("Algo no anda bien con el send %d\n");
		close(serverSocket);
		return 1;
	}

	return 0;
}


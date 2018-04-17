/*
 * server.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "server.h"

int handshakeWithClient(int listenerPort, int clientHandshakeValue, const char* clientName){
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(listenerPort);


	int serverSocket;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("No pudo crearse el socket");
		return 1;
	}

	printf("El socket al que se conectaran los clientes (donde se esta haciendo listen) es: %d\n", serverSocket);

	int activated = 1;
	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated)) == -1){
		perror("Error en setsockopt");
		close(serverSocket);
		return 1;
	}

	if (bind(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
		perror("Falló el bind");
		close(serverSocket);
		return 1;
	}

	printf("Estoy escuchando\n");
	if(listen(serverSocket, 100) == -1){
		perror("Falló el listen");
		close(serverSocket);
		return 1;
	}

	struct sockaddr_in clientAddress;
	unsigned int len = sizeof(clientAddress);
	int clientSocket = accept(serverSocket, (void*) &clientAddress, &len);
	if (clientSocket == -1){
		perror("Falló el accept");
		close(serverSocket);
		return 1;
	}

	printf("Pude aceptar una conexion, y quedo establecida en el socket: %d\n", clientSocket);

	if (send(clientSocket, &clientHandshakeValue, sizeof(int), 0) < 0){
		perror("Algo no anda bien con el send %d\n");
		close(clientSocket);
		return 1;
	}

	printf("Pude enviar un mensaje al cliente\n");

	int response = 0;
	if(recv(clientSocket, &response, sizeof(int), 0) <= 0){
		perror("Problema con recv");
		close(clientSocket);
		return 1;
	}

	if(response == clientHandshakeValue + 1){
		printf("Hanshake with %s OK\n", clientName);
	}else{
		printf("It's not the client %s, since the response was: %d\n", clientName, response);
		close(clientSocket);
		return 1;
	}

	return 0;
}

void closeSocket(int socket){
	close(socket);
}

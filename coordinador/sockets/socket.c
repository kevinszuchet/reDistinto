/*
 * socket.c
 *
 *  Created on: 16 abr. 2018
 *      Author: utnso
 */

#include "socket.h"


int main(){

	//CODIGO PARA SOCKET COMO SERVIDOR DEL PLANIFICADOR, A TRAVES DEL PUERTO 8080
	/*struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(8080);


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

	int plannerHandshakeValue = 10;
	if (send(clientSocket, &plannerHandshakeValue, sizeof(int), 0) < 0){
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

	if(response == plannerHandshakeValue + 1){
		printf("Hanshake con el planificador OK\n");
	}else{
		printf("No es el planificador, ya que llego el numero: %d\n", response);
		close(clientSocket);
		return 1;
	}

	close(clientSocket);
	close(serverSocket);*/
	//CODIGO PARA SOCKET COMO SERVIDOR DEL PLANIFICADOR, A TRAVES DEL PUERTO 8080

	//CODIGO PARA SOCKET COMO SERVIDOR DEL ESI, A TRAVES DEL PUERTO 8081
		struct sockaddr_in serverAddressEsi;
		serverAddressEsi.sin_family = AF_INET;
		serverAddressEsi.sin_addr.s_addr = INADDR_ANY;
		serverAddressEsi.sin_port = htons(8081);


		int serverSocketEsi = 0;
		if ((serverSocketEsi = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			perror("No pudo crearse el socket");
			return 1;
		}

		printf("El socket al que se conectaran los clientes (donde se esta haciendo listen) es: %d\n", serverSocketEsi);

		int activated = 1;
		if(setsockopt(serverSocketEsi, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated)) == -1){
			perror("Error en setsockopt");
			close(serverSocketEsi);
			return 1;
		}

		if (bind(serverSocketEsi, (void*) &serverAddressEsi, sizeof(serverAddressEsi)) != 0) {
			perror("Falló el bind");
			close(serverSocketEsi);
			return 1;
		}

		printf("Estoy escuchando\n");
		if(listen(serverSocketEsi, 100) == -1){
			perror("Falló el listen");
			close(serverSocketEsi);
			return 1;
		}

		struct sockaddr_in clientAddressEsi;
		unsigned int len = sizeof(clientAddressEsi);
		int clientSocketEsi = accept(serverSocketEsi, (void*) &clientAddressEsi, &len);
		if (clientSocketEsi == -1){
			perror("Falló el accept");
			close(serverSocketEsi);
			return 1;
		}

		printf("Pude aceptar una conexion, y quedo establecida en el socket: %d\n", clientSocketEsi);

		int esiHandshakeValue = 20;
		if (send(clientSocketEsi, &esiHandshakeValue, sizeof(int), 0) < 0){
			perror("Algo no anda bien con el send %d\n");
			close(clientSocketEsi);
			return 1;
		}

		printf("Pude enviar un mensaje al cliente\n");

		int esiHandshakeResponse = 0;
		int resultadoRec =2;
		if((resultadoRec = recv(clientSocketEsi, &esiHandshakeResponse, sizeof(int), 0)) <= 0){
			printf("EL resultado del recv es: %d", resultadoRec);
			perror("Problema con recv del ESI");
			close(clientSocketEsi);
			return 1;
		}

		if(esiHandshakeResponse == esiHandshakeValue + 1){
			printf("Hanshake con el ESI OK\n");
		}else{
			printf("No es el ESI, ya que llego el numero: %d\n", esiHandshakeResponse);
			close(clientSocketEsi);
			return 1;
		}

		close(clientSocketEsi);
		close(serverSocketEsi);
		//CODIGO PARA SOCKET COMO SERVIDOR DEL ESI, A TRAVES DEL PUERTO 8081

	return 0;
}


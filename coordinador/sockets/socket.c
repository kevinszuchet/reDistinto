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

	//CODIGO PARA SOCKET COMO SERVIDOR DEL PLANIFICADOR, A TRAVEZ DEL PUERTO 8080
	struct sockaddr_in direccionServidor;
		direccionServidor.sin_family = AF_INET;
		direccionServidor.sin_addr.s_addr = INADDR_ANY;
		direccionServidor.sin_port = htons(8080);


		int servidor;
		if ((servidor = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			perror("No pudo crearse el socket");
			return 1;
		}

		printf("El socket al que se conectaran los clientes (donde se esta haciendo listen) es: %d\n", servidor);

		//revisar que no se estan cerrando bien los sockets porque sin estas dos lineas, tira....
		//... address already in use
		int activado = 1;

		if(setsockopt(servidor, SOL_SOCKET, SO_REUSEADDR, &activado, sizeof(activado)) == -1){
			perror("Error en setsockopt");
			close(servidor);
			return 1;
		}

		//se le asigna un puerto a la conexion
		if (bind(servidor, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
			perror("Falló el bind");
			close(servidor);
			return 1;
		}

		printf("Estoy escuchando\n");
		if(listen(servidor, 100) == -1){
			perror("Falló el listen");
			close(servidor);
			return 1;
		}

		struct sockaddr_in direccionCliente;
		unsigned int len = sizeof(direccionCliente);
		int cliente = accept(servidor, (void*) &direccionCliente, &len);
		if (cliente == -1){
			perror("Falló el accept");
			close(servidor);
			close(cliente);
			return 1;
		}

		printf("Pude aceptar una conexion, y quedo establecida en el socket: %d\n", cliente);

		if (send(cliente, "Hola NetCat!\n", 14, 0) < 0){
			printf("Cliente: %d\n", cliente);
			printf("Algo no anda bien con el send %d\n", strerror(errno));
			close(cliente);
			return 1;
		}

		printf("Pude enviar un mensaje al cliente\n");

		//ya al hacer este malloc, no se hacen los send de arriba! wtf?!
		char* buffer = malloc(5);

		int bytesRecibidos = recv(cliente, buffer, 10, 0);
		if (bytesRecibidos <= 0) {
			close(cliente);
			close(servidor);
			perror("El chabón se desconectó o bla.");
			return 1;
		}

		buffer[bytesRecibidos] = '\0';
		printf("Me llegaron %d bytes con \n%s", bytesRecibidos, buffer);

		free(buffer);
		close(cliente);
		close(servidor);

	return 0;
}

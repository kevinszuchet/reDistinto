/*
 * socket.c
 *
 *  Created on: 16 abr. 2018
 *      Author: utnso
 */

#include "socket.h"

int main(){
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

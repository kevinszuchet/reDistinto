
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>

int main(void) {
	//CODIGO PARA SOCKET COMO CLIENTE DEL COORDINADOR, A TRAVEZ DEL PUERTO 8081
	struct sockaddr_in serverAddress;
		serverAddress.sin_family  = AF_INET;
		serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
		serverAddress.sin_port = htons(8081);

		int serverSocket = 0;

		if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
			perror("No pudo crearse el socket");
			return 1;
		}


		if (connect(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
			perror("No se pudo conectar");
			return 1;
		}

		printf("Pude conectarme al coordinador\n");

		/*while (1) {
			char mensaje[1000];
			scanf("%s", mensaje);

			send(cliente, mensaje, strlen(mensaje), 0);
		}*/

		//Recibo el mensaje del coordinador
		int response = 0;
		if(recv(serverSocket, &response, sizeof(int), 0) <= 0){
			perror("Problema con recv");
			return 1;
		}
		printf("Response: %d\n", response);
		if(response == 20){
			printf("Hanshake con el coordinador OK\n");
		}else{
			printf("No es el coordinador, ya que llego el numero: %d\n", response);
			return 1;
		}

		int clientHandshakeValue = response + 1;
		if (send(serverSocket, &clientHandshakeValue, sizeof(int), 0) < 0){
			perror("Algo no anda bien con el send %d\n");
			close(serverSocket);
			return 1;
		}
	//FIN DEL CODIGO DEL SOCKET COMO CLIENTE DEL COORDINADOR

	return 0;
}

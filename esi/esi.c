
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>

int main(void) {
	//CODIGO PARA SOCKET COMO CLIENTE DEL COORDINADOR, A TRAVEZ DEL PUERTO 8081
	struct sockaddr_in direccionServidor;
	direccionServidor.sin_family  = AF_INET;
	direccionServidor.sin_addr.s_addr = inet_addr("127.0.0.1");
	direccionServidor.sin_port = htons(8081);

	int cliente = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(cliente, (void*) &direccionServidor, sizeof(direccionServidor)) != 0) {
		perror("No se pudo conectar");
		return 1;
	}

	while (1) {
		char mensaje[1000];
		scanf("%s", mensaje);

		send(cliente, mensaje, strlen(mensaje), 0);
	}
	//FIN DEL CODIGO DEL SOCKET COMO CLIENTE DEL COORDINADOR

	return 0;
}

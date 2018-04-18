/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>
#include <../our-commons/modules/names.h>

int main(void) {
	int coordinadorSocket = connectToServer("127.0.0.1", 8080, COORDINADOR, PLANIFICADOR);
	if (coordinadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithCoordinador = handshakeWithServer(coordinadorSocket, 10, COORDINADOR, PLANIFICADOR);
	if(handshakeWithCoordinador < 0){
		//que pasa si falla el handshake?
		return -1;
	}

	return 0;
}



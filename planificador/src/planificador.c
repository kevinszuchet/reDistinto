/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>
#include <../our-commons/sockets/server.h>
#include <../our-commons/modules/names.h>

int main(void) {
	/*
	 * Handshake between planificador and coordinador
	 * */
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
	/*
	 * Handshake between planificador and coordinador
	 * */

	/*
		 * Handshake between planificador and esi
	 * */
	int planificadorToEsiSocket = 0;
	if((planificadorToEsiSocket = openConnection(8082, PLANIFICADOR, ESI)) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(planificadorToEsiSocket);
		return -1;
	}

	int esiSocket = 0;

	if((esiSocket = acceptClient(planificadorToEsiSocket, PLANIFICADOR, ESI)) < 0){
		close(esiSocket);
		return -1;
	}

	int instanciaHandshakeResult = handshakeWithClient(esiSocket, 12, PLANIFICADOR, ESI);
	if(instanciaHandshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}
	/*
	 * END Handshake between planificador and esi
	 * */

	close(esiSocket);
	close(planificadorToEsiSocket);

	return 0;
}



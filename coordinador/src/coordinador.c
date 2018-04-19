/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/server.h>
#include <../our-commons/modules/names.h>

int main(){

	/*
	 * Handshake between coordinador and planificador
	 * */
	int coordinadorToPlanificadorSocket = 0;
	if((coordinadorToPlanificadorSocket = openConnection(8080, COORDINADOR, PLANIFICADOR)) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(coordinadorToPlanificadorSocket);
		return -1;
	}

	int planificadorSocket = 0;

	if((planificadorSocket = acceptClient(coordinadorToPlanificadorSocket, COORDINADOR, PLANIFICADOR)) < 0){
		close(planificadorSocket);
		return -1;
	}

	int planificadorHandshakeResult = handshakeWithClient(planificadorSocket, 10, COORDINADOR, PLANIFICADOR);
	if(planificadorHandshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}
	/*
	 * END Handshake between coordinador and planificador
	 * */

	/*
	 * Handshake between coordinador and instancia
	 * */
	int coordinadorToInstanciaSocket = 0;
	if((coordinadorToInstanciaSocket = openConnection(8081, COORDINADOR, INSTANCIA)) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(coordinadorToInstanciaSocket);
		return -1;
	}

	int instanciaSocket = 0;

	if((instanciaSocket = acceptClient(coordinadorToInstanciaSocket, COORDINADOR, INSTANCIA)) < 0){
		close(instanciaSocket);
		return -1;
	}

	int instanciaHandshakeResult = handshakeWithClient(instanciaSocket, 11, COORDINADOR, INSTANCIA);
	if(instanciaHandshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}
	/*
	 * END Handshake between coordinador and instancia
	 * */

	/*
	 * Handshake between coordinador and esi
	 * */
	int coordinadorToEsiSocket = 0;
	if((coordinadorToEsiSocket = openConnection(8083, COORDINADOR, ESI)) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(coordinadorToEsiSocket);
		return -1;
	}

	int esiSocket = 0;

	if((esiSocket = acceptClient(coordinadorToEsiSocket, COORDINADOR, ESI)) < 0){
		close(esiSocket);
		return -1;
	}

	int esiHandshakeResult = handshakeWithClient(esiSocket, 13, COORDINADOR, ESI);
	if(esiHandshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}
	/*
	 * END Handshake between coordinador and esi
	 * */

	close(esiSocket);
	close(instanciaSocket);
	close(planificadorSocket);
	close(coordinadorToPlanificadorSocket);
	close(coordinadorToInstanciaSocket);
	close(coordinadorToEsiSocket);

	return 0;
}

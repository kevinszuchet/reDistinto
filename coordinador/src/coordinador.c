/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/server.h>
#include <../our-commons/modules/names.h>

int main(){
	int coordinadorToPlanificadorSocket = 0;
	if((coordinadorToPlanificadorSocket = openConnection(8080, "coordinador", "planificador")) < 0){
		//evalauar si se va a reintentar la conexion o que... idem luego del if de abajo
		close(coordinadorToPlanificadorSocket);
		return -1;
	}

	int planificadorSocket;

	if((planificadorSocket = acceptClient(coordinadorToPlanificadorSocket, COORDINADOR, PLANIFICADOR)) < 0){
		close(planificadorSocket);
		return -1;
	}

	int handshakeResult = handshakeWithClient(planificadorSocket, 10, COORDINADOR, PLANIFICADOR);
	if(handshakeResult < 0){
		//que pasa si no se puede hacer handshake?
		return -1;
	}

	close(planificadorSocket);
	close(coordinadorToPlanificadorSocket);

	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/server.h>
#include <../our-commons/modules/names.h>
#include <commons/string.h>
#include <commons/config.h>

#define  CFG_FILE "../coordinador.cfg"

void getConfig(int* listeningPort,char** algorithm, int* cantEntry,int* entrySize,int* delay){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
}

int main(){

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	int delay;
	getConfig(&listeningPort,&algorithm,&cantEntry,&entrySize,&delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);


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

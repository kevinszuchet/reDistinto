/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
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

int welcomePlanificador(int coordinadorSocket);
int clientHandler(int clientSocket);

int main(){

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	int delay;
	/*getConfig(&listeningPort,&algorithm,&cantEntry,&entrySize,&delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);*/

	int coordinadorSocket = welcomeClient(8080, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador);

	/*if(welcomePlanificador < 0){
		//TODO: que pasa en este caso?
		return -1;
	}*/

	return 0;
}

int clientHandler(int clientSocket){
	printf("Recibi un nuevo cliente, voy a crear su hilo. ClientSocket is: %d\n", clientSocket);
	int id = 0;
	if (recieveClientId(clientSocket, id, COORDINADOR)){
		//reintentar recv?
		return -1;
	}

	printf("Recibi el id %d\n", id);

	if (id == 11){
		printf("Recibi una instancia\n");
	}else if(id == 12){
		printf("Recibi un esi\n");
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket){
	//int messageResult = welcomeClient(8081, COORDINADOR, INSTANCIA, 11, &welcomeEsi);
	printf("Recibi al planificador. Empiezo a escuchar nuevas llegadas de instancia/esi\n");

	while(1){
		printf("Por hacer el accept\n");
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

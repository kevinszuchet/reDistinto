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
	getConfig(&listeningPort,&algorithm,&cantEntry,&entrySize,&delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);

	int coordinadorSocket = welcomeClient(8080, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador);

	return 0;
}

int clientHandler(int clientSocket){
	int id = recieveClientId(clientSocket, COORDINADOR);
	if (id < 0){
		//reintentar recv?
		return -1;
	}

	if (id == 11){
		printf("Recibi una instancia\n");
	}else if(id == 12){
		printf("Recibi un esi\n");
	}else{
		printf("Recibi un desconocido!\n");
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket){
	printf("Recibi al planificador. Empiezo a escuchar nuevas llegadas de instancia/esi\n");

	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

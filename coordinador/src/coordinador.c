/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
#include <pthread.h>

int main(void) {

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	int delay;
	getConfig(&listeningPort, &algorithm, &cantEntry, &entrySize, &delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);

	int coordinadorSocket = welcomeClient(8080, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador);

	return 0;
}

void getConfig(int* listeningPort, char** algorithm, int* cantEntry, int* entrySize, int* delay){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
}

int welcomeInstancia(){
	printf("Se levanto el hilo de una instancia\n");
	return 0;
}

int welcomeEsi(){
	printf("Se levanto el hilo de un esi\n");
	return 0;
}

void* pthreadInitialize(void* clientSocket){
	int castedClientSocket = *((int*) clientSocket);
	printf("pthreadInitialize ClientSocket %d\n", castedClientSocket);
	int id = recieveClientId(castedClientSocket, COORDINADOR);
	if (id < 0){
		//reintentar recv?
	}

	if (id == 11){
		welcomeInstancia();
	}else if(id == 12){
		welcomeEsi();
	}else{
		printf("Recibi un desconocido!\n");
	}
}

int clientHandler(int clientSocket){
	pthread_t clientThread;
	printf("clientHandler ClientSocket %d\n", clientSocket);
	if(pthread_create(&clientThread, NULL, &pthreadInitialize, &clientSocket)){
		printf("Error creating thread\n");
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		printf("Couldn't detach thread\n");
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket){
	printf("Recibi al planificador. Empiezo a escuchar nuevas llegadas de instancia/esi\n");

	while(1){
		printf("Voy a bloquear mediante accept\n");
		//chequear que no esta bloqueando en el caso de que la instancia se vuelva a conectar!
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

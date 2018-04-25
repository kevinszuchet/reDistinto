/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
#include <pthread.h>
#include <unistd.h>

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

	int coordinadorSocket = welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador);

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
	printf("An instancia thread was created\n");
	return 0;
}

int welcomeEsi(){
	printf("An esi thread was created\n");
	return 0;
}

void* pthreadInitialize(void* clientSocket){
	int castedClientSocket = *((int*) clientSocket);
	int id = recieveClientId(castedClientSocket, COORDINADOR);

	if (id == 11){
		welcomeInstancia();
	}else if(id == 12){
		welcomeEsi();
	}else{
		printf("I received a strange\n");
	}
}

int clientHandler(int clientSocket){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, &pthreadInitialize, clientSocketPointer)){
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
	printf("%s recieved %s, so it'll now start listening esi/instancia connections\n", COORDINADOR, PLANIFICADOR);
	while(1){
		//chequear que no esta bloqueando en el caso de que la instancia se vuelva a conectar!
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

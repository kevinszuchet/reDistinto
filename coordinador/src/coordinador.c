/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
#include <pthread.h>

void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;

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

	//setDistributionAlgorithm(algorithm);

	int coordinadorSocket = welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador);

	instancias = list_create();

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

/*Instancia* equitativeLoad(char* keyToBeBlocked){
	return ;
}

Instancia* leastSpaceUsed(char* keyToBeBlocked){
	return ;
}

Instancia* keyExplicit(char* keyToBeBlocked){
	return ;
}

void setDistributionAlgorithm(char* algorithm){
	if(strcmp(algorithm, "EL") == 0){
		distributionAlgorithm = &equitativeLoad;
	}else if(strcmp(algorithm, "LSU") == 0){
		distributionAlgorithm = &leastSpaceUsed;
	}else if(strcmp(algorithm, "KE") == 0){
		distributionAlgorithm = &keyExplicit;
	}else{
		printf("Couldn't determine the distribution algorithm\n");
		//que pasa en este caso?
	}
}

Instancia* chooseInstancia(keyToBeBlocked){
	return *distributionAlgorithm(keyToBeBlocked);
}

int processEsi(EsiRequest esiRequest){
	keyToBeBlocked = getKeyFromRequest(esiRequest);
	choosenInstancia = chooseInstancia(keyToBeBlocked);
	if (choosenInstancia < 0){
		informPlanificador(esiRequest->id, esiRequest->operation->key);
		return -1;
	}

	sendRequest(choosenInstancia, esiRequest->socketConnection);

	response = waitForInstanciaResponse(choosenInstancia);
	if(response < 0){
		informPlanificador();
		return -1;
	}

	logResponse(responseToString(response));
	sendResponse(esi, response);

	return 0;
}

int recieveEsiToProcess(int esiSocket){
	EsiRequest* esiRequest;
	recv(esiSocket, esiRequest, sizeof(EsiRequest), 0);
	processEsi(esiRequest);
}*/

int welcomeInstancia(int instanciaSocket){
	printf("An instancia thread was created\n");

	return 0;
}

int welcomeEsi(int esiSocket){
	printf("An esi thread was created\n");

	return 0;
}

void* pthreadInitialize(void* clientSocket){
	int castedClientSocket = *((int*) clientSocket);
	int id = recieveClientId(castedClientSocket, COORDINADOR);

	if (id == 11){
		welcomeInstancia(castedClientSocket);
	}else if(id == 12){
		welcomeEsi(castedClientSocket);
	}else{
		printf("I received a strange\n");
	}

	free(clientSocket);
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
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
#include <pthread.h>

t_log* logger;

void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;

int main(void) {
	logger = log_create("coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
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

	int coordinadorSocket = welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador, logger);

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
	return instancia;
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
		//loggear el error
		//matar al coordinador
	}
}*/

Instancia* chooseInstancia(char* keyToBeBlocked){
	Instancia* instancia = NULL;
	instancia = (*distributionAlgorithm)(keyToBeBlocked);
	return instancia;
}

int waitForInstanciaResponse(Instancia* choosenInstancia){
	int response = 0;
	if (recv(choosenInstancia->socket, &response, sizeof(int), 0) <= 0){
		return -1;
	}
	return response;
}

void informPlanificador(char* key){

}

void sendResponseToEsi(int esiSocket, int response){

}

int validateAvailableKeyWithPlanificador(char* key){
	return 0;
}

//TODO estaria bueno manejarnos con otro tad de esi que conozca todos los componentes recibidos...
//esto es: los dos tamanios y los dos valores, para evitar pasar tantos parametros
int sendRequest(Instancia* choosenInstancia, char* key, char* value){
	return 0;
}

void logOperation(char* stringToLog){

}

int doSet(int esiSocket, char* stringToLog){

	//TODO estos 2 recv se pueden pasar a otra funcion para no repetir!
	int keySize = 0;
	if(recv(esiSocket, &keySize, sizeof(int), 0) <= 0){

	}

	char* key = malloc(keySize);
	if (recv(esiSocket, &key, keySize, 0) <= 0){

	}

	Instancia* choosenInstancia = chooseInstancia(key);
	if (choosenInstancia == NULL){
		informPlanificador(key);
		sendResponseToEsi(esiSocket, EXECUTION_ERROR);
		return -1;
	}

	int esiId = validateAvailableKeyWithPlanificador(key);

	if(esiId < 0){
		return -1;
	}

	int valueSize = 0;
	if(recv(esiSocket, &valueSize, sizeof(int), 0) <= 0){

	}

	char* value = malloc(valueSize);
	if (recv(esiSocket, &value, valueSize, 0) <= 0){

	}

	if (sendRequest(choosenInstancia, key, value) <= 0){
		//que pasa aca?
		return -1;
	}

	int response = waitForInstanciaResponse(choosenInstancia);
	if(response < 0){
		informPlanificador(key);
		sendResponseToEsi(esiSocket, EXECUTION_ERROR);
		return -1;
	}

	sendResponseToEsi(esiSocket, response);

	//5 es el tamanio de "ESI X" necesario para loggear, siendo X el id de esi
	stringToLog = malloc(5 + keySize + valueSize + 1);
	//hacer el string necesario
	//strcpy();

	free(key);
	free(value);

	return 0;
}

int recieveStentenceToProcess(int esiSocket){
	char operationCode;
	//primero se recibe codigo de op
	//validar recv y
	recv(esiSocket, &operationCode, sizeof(char), 0);

	char* stringToLog;
	switch (operationCode){
		case SET:
			doSet(esiSocket, stringToLog);
			break;
		case GET:

			break;
		case STORE:

			break;
		default:
			//deberiamos matar al esi?
			break;
	}

	logOperation(stringToLog);
	free(stringToLog);
	return 0;
}

int handleInstancia(int instanciaSocket){
	log_info(logger, "An instancia thread was created\n");
	while(1){
		//recv();
		//agregar a la lista de instancias
	}
	return 0;
}

int handleEsi(int esiSocket){
	log_info(logger,"An esi thread was created\n");
	while(1){
		//recieveStentenceToProcess(esiSocket);
	}
	return 0;
}

void* pthreadInitialize(void* clientSocket){
	int castedClientSocket = *((int*) clientSocket);
	int id = recieveClientId(castedClientSocket, COORDINADOR, logger);

	if (id == 11){
		handleInstancia(castedClientSocket);
	}else if(id == 12){
		handleEsi(castedClientSocket);
	}else{
		log_info(logger,"I received a strange\n");
	}

	free(clientSocket);
}

int clientHandler(int clientSocket){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, &pthreadInitialize, clientSocketPointer)){
		log_error(logger, "Error creating thread\n");
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		log_error(logger,"Couldn't detach thread\n");
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket){
	log_info(logger, "%s recieved %s, so it'll now start listening esi/instancia connections\n", COORDINADOR, PLANIFICADOR);
	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		//validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

t_log* logger;
t_log* operationsLogger;

int planificadorSocket;
void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;
int cantEntry;
int entrySize;
int delay;
int lastInstanciaChosen = 0;
int firstAlreadyPass = 0;
int greatesInstanciaId = 0;

int main(void) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	int listeningPort;
	char* algorithm;
	getConfig(&listeningPort, &algorithm);
	showConfig(listeningPort, algorithm);

	setDistributionAlgorithm(algorithm);

	instancias = list_create();

	welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador, logger);

	return 0;
}

void showConfig(int listeningPort, char* algorithm){
	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);
}

void getConfig(int* listeningPort, char** algorithm){
	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	if(strcmp(*algorithm, "EL") != 0 && strcmp(*algorithm, "LSU") != 0 && strcmp(*algorithm, "KE") != 0){
		log_error(operationsLogger, "Abortando: no se reconoce el algoritmo de distribucion");
		exit(-1);
	}
	cantEntry = config_get_int_value(config, "CANT_ENTRY");
	entrySize = config_get_int_value(config, "ENTRY_SIZE");
	delay = config_get_int_value(config, "DELAY");

	//TODO mariano esto hay que matarlo en algun momento. Aca no porque, por algun motivo, el showConfig muestra mal
	//config_destroy(config);
}

int instanciaIsAliveAndNextToActual(Instancia* instancia){
	if(instancia->isFallen){
		return 0;
	}

	if(lastInstanciaChosen == 0 && firstAlreadyPass == 0){
		firstAlreadyPass = 1;
		return 1;
	}
	return instancia->id >= lastInstanciaChosen + 1;
}

Instancia* equitativeLoad(char* keyToBeBlocked){
	Instancia* instancia = list_find(instancias, (void*) &instanciaIsAliveAndNextToActual);
	if(instancia == NULL){
		instancia = list_get(instancias, 0);
		if(instancia->isFallen){
			return NULL;
		}
	}
	lastInstanciaChosen = instancia->id;
	return instancia;
}

Instancia* leastSpaceUsed(char* keyToBeBlocked){
	//TODO
	return list_get(instancias, 0);
}

Instancia* keyExplicit(char* keyToBeBlocked){
	//TODO
	return list_get(instancias, 0);
}

void setDistributionAlgorithm(char* algorithm){
	if(strcmp(algorithm, "EL") == 0){
		distributionAlgorithm = &equitativeLoad;
	}else if(strcmp(algorithm, "LSU") == 0){
		distributionAlgorithm = &leastSpaceUsed;
	}else if(strcmp(algorithm, "KE") == 0){
		distributionAlgorithm = &keyExplicit;
	}
}

Instancia* chooseInstancia(char* keyToBeBlocked){
	if(list_size(instancias) != 0){
		return (*distributionAlgorithm)(keyToBeBlocked);
	}
	return NULL;
}

int sendResponseToEsi(EsiRequest* esiRequest, int response, char** stringToLog){
	//TODO aca tambien hay que reintentar hasta que se mande todo?
	//TODO que pasa cuando se pasa una constante por parametro? vimos que hubo drama con eso

	if(sendInt(response, esiRequest->socket) == CUSTOM_FAILURE){
		printf("Se van a pisar los strings\n");
		sprintf(*stringToLog, "ESI %d perdio conexion con el coordinador al intentar hacer %s",
				esiRequest->id, getOperationName(esiRequest->operation));
		return -1;
	}

	printf("Se envio send al esi\n");

	return 0;
}

int getActualEsiID(){
	int esiId = 0;
	int recvResult = recv(planificadorSocket, &esiId, sizeof(int), 0);
	if(recvResult <= 0){
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		exit(-1);
	}

	return esiId;
}

int getActualEsiIDDummy(){
	return 0;
}

void logOperation(char* stringToLog){
	log_info(operationsLogger, stringToLog);
}

char* getOperationName(Operation* operation){
	switch(operation->operationCode){
		case OURSET:
			return "SET";
			break;
		case OURSTORE:
			return "STORE";
			break;
		case OURGET:
			return "GET";
			break;
		default:
			return "UNKNOWN OPERATION";
			break;
	}
}

int keyIsOwnedByActualEsi(char keyStatus, EsiRequest* esiRequest, char** stringToLog){
	if(keyStatus != LOCKED){
		sprintf(*stringToLog, "ESI %d no puede hacer %s sobre la clave %s. Clave no bloqueada por el",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	return 0;
}

//TODO cuidado en estos casos que el stringToLog no se limpia y es llamado mas de una vez
int tryToExecuteOperationOnInstancia(EsiRequest* esiRequest, Instancia* chosenInstancia, char** stringToLog){
	log_info(logger, "Se usara la siguiente instancia para hacer %s", getOperationName(esiRequest->operation));
	showInstancia(chosenInstancia);

	//TODO esto hay que pasarlo al hilo de la instancia
	int response;
	//todo mariano ojo que la instancia esta caida pero esto que usa sendOperation esta pudiendo enviar...
	response = instanciaDoOperation(chosenInstancia, esiRequest->operation);
	//response = instanciaDoOperationDummy();

	//TODO mariano fijate que al hacer este sleep, si matas (rapido) al esi, se hace el send igual
	sleep(10);
	if(response < 0){
		sprintf(*stringToLog, "ESI %d no puede hacer %s sobre %s. Instancia %d se cayo. Clave inaccesible", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key, chosenInstancia->id);
		instanciaHasFallen(chosenInstancia);
		removeKeyFromFallenInstancia(esiRequest->operation->key, chosenInstancia);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	sprintf(*stringToLog, "ESI %d hizo %s sobre la clave %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

	return sendResponseToEsi(esiRequest, SUCCESS, stringToLog);
}

Instancia* lookOrRemoveKeyIfInFallenInstancia(EsiRequest* esiRequest, char** stringToLog){
	Instancia* instanciaToBeUsed = lookForKey(esiRequest->operation->key, instancias);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		sprintf(*stringToLog, "ESI %d intenta hacer %s sobre la clave %s. Clave inaccesible",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		removeKeyFromFallenInstancia(esiRequest->operation->key, instanciaToBeUsed);
		//TODO ojo que aca se esta pisando el stringToLog que se acaba de setear
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
	}
	return instanciaToBeUsed;
}

int doSet(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest, stringToLog) < 0){
		return -1;
	}

	int keyExists = 0;

	Instancia* instanciaToBeUsed = lookOrRemoveKeyIfInFallenInstancia(esiRequest, stringToLog);
	showInstancia(instanciaToBeUsed);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		return -1;
	}
	else{
		if(instanciaToBeUsed == NULL){
			instanciaToBeUsed = chooseInstancia(esiRequest->operation->key);
			if(instanciaToBeUsed == NULL){
				sprintf(*stringToLog, "ESI %d es abortado al intentar SET por no existir la clave en ninguna instancia y no haber instancias disponibles", esiRequest->id);
				sendResponseToEsi(esiRequest, ABORT, stringToLog);
				return -1;
			}
		}else{
			keyExists = 1;
		}
	}

	if(keyExists == 0){
		addKeyToInstanciaStruct(instanciaToBeUsed, esiRequest->operation->key);
		log_info(logger, "La clave %s no existe aun", esiRequest->operation->key);
	}

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed, stringToLog) < 0){
		return -1;
	}

	return 0;
}

int doStore(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest, stringToLog) < 0){
		return -1;
	}

	Instancia* instanciaToBeUsed = lookOrRemoveKeyIfInFallenInstancia(esiRequest, stringToLog);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		return -1;
	}else{
		if(instanciaToBeUsed == NULL){
			sprintf(*stringToLog, "ESI %d intenta hacer %s sobre la clave %s. Clave no identificada en instancias", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
			sendResponseToEsi(esiRequest, ABORT, stringToLog);
			return -1;
		}
	}

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed, stringToLog) < 0){
		return -1;
	}

	return 0;
}

int doGet(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyStatus == BLOCKED){
		sprintf(*stringToLog, "ESI %d intenta hacer GET sobre la clave %s. Clave bloqueada", esiRequest->id, esiRequest->operation->key);
		//todo ojo en estos casos que no se esta abortando al esi porque se supone que el murio.
		//esto es: hay que validar que el planificador se entere bien de esto para no cagarla
		return sendResponseToEsi(esiRequest, BLOCK, stringToLog);
	}

	//TODO testear cuando la instancia se caiga y el coordinador se entere por ella (y no porque un esi quiere acceder, sino se borra la clave!)
	Instancia* instanciaWithKey = lookOrRemoveKeyIfInFallenInstancia(esiRequest, stringToLog);
	if(instanciaWithKey != NULL && instanciaWithKey->isFallen){
		return -1;
	}

	if(keyStatus == LOCKED){
		sprintf(*stringToLog, "ESI %d hace GET sobre la clave %s, la cual ya tenia", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, SUCCESS, stringToLog) < 0){
			return -1;
		}
	}else{
		sprintf(*stringToLog, "ESI %d hizo GET sobre la clave %s", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, LOCK, stringToLog) < 0){
			return -1;
		}
	}
	return 0;
}

char checkKeyStatusFromPlanificador(int esiId, char* key){
	char response = 0;

	log_info(logger, "Voy a recibir el estado de la clave %s del planificador", key);
	//TODO probar esto
	if(sendString(key, planificadorSocket) == CUSTOM_FAILURE){
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		exit(-1);
	}

	log_info(logger, "Le envie la clave de interes al planificador");

	int recvResult = recv(planificadorSocket, &response, sizeof(char), 0);
	if(recvResult <= 0){
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		exit(-1);

	}
	return response;
}

char checkKeyStatusFromPlanificadorDummy(){
	return LOCKED;
}

void recieveOperationDummy(Operation* operation){
	log_info(logger, "Voy a guardar la clave en la operacion dummy");
	operation->key = "lio:messi";
	log_info(logger, "Guarde la clave en la operacion dummy");
	//operation->key = "cristiano:ronaldo";
	operation->value = "elMasCapo";
	operation->operationCode = OURSET;
}

//TODO mover a las commons junto con getOperationName
void showOperation(Operation* operation){
	printf("Operation key = %s\n", getOperationName(operation));
	printf("Key = %s\n", operation->key);
	//TODO ver como se valida esto
	//operation->value ? printf("Value = %s\n", operation->value);
}

//TODO esto tambien, mover a las commons
char* getKeyStatusName(char keyStatus){
	log_info(logger, "Voy a mostrar el estado de la clave");
	switch(keyStatus){
		case LOCK:
			return "LOCK";
			break;
		case NOTBLOCKED:
			return "NOTBLOCKED";
			break;
		case BLOCKED:
			return "BLOCKED";
			break;
		default:
			return "UNKNOWN KEY STATUS";
			break;
	}
}

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	esiId = getActualEsiID();
	//esiId = getActualEsiIDDummy();

	log_info(logger, "Llego el esi con id = %d", esiId);

	//TODO chequear esto para evitar alocar demas. por otro lado, estaria bueno evitar alocar por cada sentencia...
	char* stringToLog = calloc(200, sizeof(char));

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;

	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		//TODO testear esta partecita
		esiRequest.operation = NULL;
		sendResponseToEsi(&esiRequest, ABORT, &stringToLog);
		return -1;
	}
	/*recieveOperationDummy(esiRequest.operation);*/

	log_info(logger, "El esi %d va a hacer:", esiId);
	showOperation(esiRequest.operation);

	char keyStatus;
	keyStatus = checkKeyStatusFromPlanificador(esiId, esiRequest.operation->key);
	//keyStatus = checkKeyStatusFromPlanificadorDummy();
	log_info(logger, "El estado de la clave del esi %d es %s", esiId, getKeyStatusName(keyStatus));

	switch (esiRequest.operation->operationCode){
		case OURSET:
			if(doSet(&esiRequest, keyStatus, &stringToLog) < 0){
				operationResult = -1;
			}
			break;
		case OURSTORE:
			if(doStore(&esiRequest, keyStatus, &stringToLog)){
				operationResult = -1;
			}
			break;
		case OURGET:
			if (doGet(&esiRequest, keyStatus, &stringToLog) < 0){
				operationResult = -1;
			}
			break;
		default:
			sprintf(stringToLog, "El ESI %d envio una operacion invalida", esiId);
			sendResponseToEsi(&esiRequest, ABORT, &stringToLog);
			operationResult = -1;
			break;
	}

	if(*stringToLog) logOperation(stringToLog);

	free(esiRequest.operation);
	free(stringToLog);
	sleep(delay);
	return operationResult == 0 ? 1 : -1;
}

//TODO mover esta funcion a instanciaFunctions
int recieveInstanciaName(char** arrivedInstanciaName, int instanciaSocket){
	if(recieveString(arrivedInstanciaName, instanciaSocket) == CUSTOM_FAILURE){
		log_error(operationsLogger, "No se pudo recibir el nombre de la instancia");
		free(*arrivedInstanciaName);
		return -1;
	}else if(strlen(*arrivedInstanciaName) == 0){
		log_error(operationsLogger, "La instancia no puede no tener nombre");
		return -1;
	}
	return 0;
}

int handleInstancia(int instanciaSocket){
	//TODO hay que meter un semaforo para evitar conflictos de los diferentes hilos

	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket) < 0){
		return -1;
	}
	/*recieveInstanciaNameDummy(&arrivedInstanciaName);*/
	log_info(logger, "Llego instancia: %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize, logger) < 0){
		free(arrivedInstanciaName);
		return -1;
	}
	log_info(logger, "Se envio la configuracion a la instancia %s", arrivedInstanciaName);

	Instancia* arrivedInstanciaExists = existsInstanciaWithName(arrivedInstanciaName, instancias);
	if(arrivedInstanciaExists){
		instanciaIsBack(arrivedInstanciaExists, instanciaSocket);
		log_info(logger, "La instancia %s esta reviviendo", arrivedInstanciaName);
	}else{
		createNewInstancia(instanciaSocket, instancias, &greatesInstanciaId, arrivedInstanciaName);
		log_info(logger, "La instancia %s es nueva", arrivedInstanciaName);
	}

	showInstancias(instancias);

	//TODO semaforo binario para que la instancia se quede esperando

	//TODO aca abajo van los send/recv que se tenga que hacer con el modulo instancia

	return 0;
}

int handleEsi(int esiSocket){
	while(1){
		if (recieveStentenceToProcess(esiSocket) < 0){
			//va a matar al hilo porque sale de este while!
			break;
		}
	}
	return 0;
}

void pthreadInitialize(int* clientSocket){
	int id = recieveClientId(*clientSocket, COORDINADOR, logger);

	if (id == 11){
		handleInstancia(*clientSocket);
	}else if(id == 12){
		handleEsi(*clientSocket);
	}else{
		log_info(logger, "I received a strange");
	}

	close(*clientSocket);
	free(clientSocket);
}

int clientHandler(int clientSocket){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, (void*) &pthreadInitialize, clientSocketPointer)){
		log_error(logger, "Error creating thread");
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		log_error(logger,"Couldn't detach thread");
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket, int newPlanificadorSocket){
	planificadorSocket = newPlanificadorSocket;
	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		clientHandler(clientSocket);
	}

	return 0;
}

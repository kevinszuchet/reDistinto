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

//TODO cambiar esta por la posta
#define CLAVE_PROVISORIA_ERROR_A_PLANIFICADOR '30'
#define CLAVE_PROVISORIA_CLABE_BLOQUEADA_DE_PLANIFICADOR '31'

t_log* logger;
t_log* operationsLogger;

int planificadorSocket;
void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;
int delay;
int lastInstanciaChosen = 0;
int firstAlreadyPass = 0;
int greatesInstanciaId = 0;

int main(void) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	getConfig(&listeningPort, &algorithm, &cantEntry, &entrySize, &delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);

	setDistributionAlgorithm(algorithm);

	instancias = list_create();

	welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador, logger);

	return 0;
}

void getConfig(int* listeningPort, char** algorithm, int* cantEntry, int* entrySize, int* delay){
	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	//TODO validar algoritmo valido
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
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
	}else{
		printf("Couldn't determine the distribution algorithm\n");
		log_error(operationsLogger, "No se pudo determinar ");
		exit(-1);
	}
}

Instancia* chooseInstancia(char* keyToBeBlocked){
	//TODO este if podria estar demas
	if(list_size(instancias) != 0){
		return (*distributionAlgorithm)(keyToBeBlocked);
	}
	return NULL;
}

int sendResponseToEsi(EsiRequest* esiRequest, int response, char** stringToLog){
	//TODO aca tambien hay que reintentar hasta que se mande todo?
	//TODO que pasa cuando se pasa una constante por parametro? vimos que hubo drama con eso

	if(send(esiRequest->socket, &response, sizeof(int), 0) < 0){
		printf("Se van a pisar los strings\n");
		sprintf(*stringToLog, "ESI %d perdio conexion con el coordinador al intentar hacer %s", esiRequest->id, getOperationName(esiRequest->operation));
		return -1;
	}

	printf("Se envio send al esi\n");

	return 0;
}

int getActualEsiID(){
	int esiId = 0;
	int recvResult = recv(planificadorSocket, &esiId, sizeof(int), 0);
	if(recvResult <= 0){
		if(recvResult == 0)
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
	//ver si se puede evitar repetir este switch
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
		sprintf(*stringToLog, "ESI %d no puede hacer %s sobre la clave %s. Clave no bloqueada por el", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	return 0;
}

//TODO cuidado en estos casos que el stringToLog no se limpia y es llamado mas de una vez
int tryToExecuteOperationOnInstancia(EsiRequest* esiRequest, Instancia* chosenInstancia, char** stringToLog){
	printf("Se usara la siguiente instancia para hacer %s\n", getOperationName(esiRequest->operation));
	showInstancia(chosenInstancia);

	//TODO esto hay que pasarlo al hilo de la instancia
	int response;
	//MARIANO+MARIANO+MARIANO
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
		sprintf(*stringToLog, "ESI %d intenta hacer %s sobre la clave %s. Clave inaccesible", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
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

	//estos 2 ifs van si o si asi, si se invierten puede haber inconsistencia de claves
	if(keyExists == 0){
		addKeyToInstanciaStruct(instanciaToBeUsed, esiRequest->operation->key);
		printf("La clave no existe aun\n");
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

	//TODO mariano. (serializar y) enviar la clave al planificador (no la operacion)

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
	operation->key = "lio:messi";
	//operation->key = "cristiano:ronaldo";
	operation->value = "elMasCapo";
	operation->operationCode = OURSET;
}

void showOperation(Operation* operation){
	printf("Operation key = %s\n", getOperationName(operation));
}

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	//esiId = getActualEsiID();
	esiId = getActualEsiIDDummy();
	printf("ESI ID: %d\n", esiId);
	printf("Esi socket: %d\n", esiSocket);

	//TODO chequear esto para evitar alocar demas. por otro lado, evitar alocar por cada sentencia...
	char* stringToLog = calloc(200, sizeof(char));

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;


	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		//TODO testear esta partecita
		esiRequest.operation = NULL;
		sendResponseToEsi(&esiRequest, ABORT, &stringToLog);
		return -1;
	}
	printf("Voy a mostrar la operacion recibida\n");
	showOperation(esiRequest.operation);
	//recieveOperationDummy(esiRequest.operation);

	char keyStatus;
	//TODO descomentar
	//keyStatus = checkKeyStatusFromPlanificador(esiId, operation->key);
	keyStatus = checkKeyStatusFromPlanificadorDummy();

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

int handleInstancia(int instanciaSocket){
	log_info(logger, "An instancia thread was created\n");
	//TODO hay que meter un semaforo para evitar conflictos de los diferentes hilos, y ademas para que solo se haga...
	//... send/recv en el hilo que corresponda y no en cualquiera

	//TODO que pasa si una instancia se recupera? como la distinguimos?
	//si seguimos este camino, se va a crear una nueva y no queremos

	//TODO recibir el nombre de la instancia
	createNewInstancia(instanciaSocket, instancias, &greatesInstanciaId);
	//TODO enviarle a la instancia su configuracion

	showInstancias(instancias);

	/*int recvResult = 0;
	while(1){
		int instanciaResponse = 0;
		recvResult = recv(instanciaSocket, &instanciaResponse, sizeof(int), 0);
		if(recvResult <= 0){
			if(recvResult == 0){
				printf("Instancia on socket %d has fallen\n", instanciaSocket);

				//handlear que pasa en este caso. podriamos guardar el id de la instancia caida, sacar la instancia de la lista...
				//de instancias. Despues, cuando se reincorpore, levantarla de ahi

				close(instanciaSocket);

				//si se cae la instancia, se cae su hilo
				break;
			}
		}else{
			//TODO handlear la respuesta normal de ejecucion en una instancia
		}
	}*/
	return 0;
}

int handleEsi(int esiSocket){
	log_info(logger,"An esi thread was created\n");
	while(1){
		if (recieveStentenceToProcess(esiSocket) < 0){
			//va a matar al hilo porque sale de este while!
			break;
		}
	}
	return 0;
}

void pthreadInitialize(void* clientSocket){
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
	if(pthread_create(&clientThread, NULL, (void*) &pthreadInitialize, clientSocketPointer)){
		log_error(logger, "Error creating thread\n");
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		log_error(logger,"Couldn't detach thread\n");
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket, int newPlanificadorSocket){
	planificadorSocket = newPlanificadorSocket;
	log_info(logger, "%s recieved %s, so it'll now start listening esi/instancia connections\n", COORDINADOR, PLANIFICADOR);
	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		clientHandler(clientSocket);
	}

	return 0;
}

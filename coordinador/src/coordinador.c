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
t_list* fallenInstancias;
int delay;
int lastInstanciaChosen = 0;
int firstAlreadyPass = 0;

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
	fallenInstancias = list_create();

	welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador, logger);

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

int instanciaIsNextToActual(Instancia* instancia){
	if(lastInstanciaChosen == 0 && firstAlreadyPass == 0){
		firstAlreadyPass = 1;
		return 1;
	}
	return instancia->id >= lastInstanciaChosen + 1;
}

Instancia* equitativeLoad(char* keyToBeBlocked){
	Instancia* instancia = list_find(instancias, (void*) &instanciaIsNextToActual);
	if(instancia == NULL){
		instancia = list_get(instancias, 0);
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
		//loggear el error
		//matar al coordinador
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
		sprintf(*stringToLog, "ESI %d perdio conexion con el coordinador al intentar hacer %s", esiRequest->id, getOperationName(esiRequest->operation));
		return -1;
	}
	return 0;
}

int getActualEsiID(){
	int esiId = 0;
	int recvResult = recv(planificadorSocket, &esiId, sizeof(int), 0);
	if(recvResult <= 0){
		if(recvResult == 0)
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		//TODO decidamos: de aca en mas hacemos exit si muere la conexion con el planificador?
		//Misma decision que en recieveKeyStatusFromPlanificador
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

Instancia* lookForOrChoseInstancia(char* key, int* keyExists){
	Instancia* chosenInstancia = lookForKey(key, instancias);

	if(chosenInstancia == NULL){
		chosenInstancia = chooseInstancia(key);
	}else{
		*keyExists = 1;
	}

	return chosenInstancia;
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

//TODO cuidado en estos casos que el stringToLog no se limpia y es llamado mas de una vez
int lookForKeyAndExecuteOperation(EsiRequest* esiRequest, char** stringToLog){
	Instancia* chosenInstancia = lookForKey(esiRequest->operation->key, instancias);
	if(chosenInstancia == NULL){
		chosenInstancia = (Instancia*) fallenInstanciaThatHasKey(esiRequest->operation->key, fallenInstancias);
		if(chosenInstancia){
			//TODO si abajo se pone el removeKeyFromFallenInstancia, este de aca esta al pedo
			removeKeyFromFallenInstancia(esiRequest->operation->key, chosenInstancia);
			sprintf(*stringToLog, "ESI %d intenta hacer %s sobre la clave %s. Clave inaccesible", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		}else{
			sprintf(*stringToLog, "ESI %d intenta hacer %s sobre la clave %s. Clave no identificada", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		}
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	showInstancia(chosenInstancia);

	//TODO esto hay que pasarlo al hilo de la instancia
	int response;
	response = instanciaDoOperation(chosenInstancia, esiRequest->operation);
	//response = instanciaDoOperationDummy();

	if(response < 0){
		sprintf(*stringToLog, "ESI %d no puede hacer %s sobre %s. Instancia %d se cayo. Clave inaccesible", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key, chosenInstancia->id);
		//TODO importante aca tambien deberia llamarse a removeKeyFromFallenInstancia?
		instanciaHasFallen(chosenInstancia, instancias, fallenInstancias);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	//TODO segun otro todo de mas abajo, puede ser que aca haya que agregar la clave a la instancia elegida (y sacarla de esa lista provisoria)

	sprintf(*stringToLog, "ESI %d hizo %s sobre la clave %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

	return sendResponseToEsi(esiRequest, SUCCESS, stringToLog);
}

int doSet(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyStatus != LOCKED){
		sprintf(*stringToLog, "ESI %d no puede hacer SET sobre la clave %s. Clave no bloqueada por el", esiRequest->id, esiRequest->operation->key);
		return -1;
	}

	if(lookForKeyAndExecuteOperation(esiRequest, stringToLog) < 0){
		return -1;
	}

	return 0;
}

int doStore(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyStatus != LOCKED){
		sprintf(*stringToLog, "ESI %d no puede hacer STORE sobre la clave %s. Clave no bloqueada por el", esiRequest->id, esiRequest->operation->key);
		return -1;
	}

	if(lookForKeyAndExecuteOperation(esiRequest, stringToLog) < 0){
		return -1;
	}

	return 0;
}

//doget, set y store deben devolver 0 si salio ok y -1 si muere el hilo del esi
int doGet(EsiRequest* esiRequest, char keyStatus, char** stringToLog){
	if(keyStatus == BLOCKED){
		sprintf(*stringToLog, "ESI %d intenta hacer GET sobre la clave %s. Clave bloqueada", esiRequest->id, esiRequest->operation->key);
		//no se mata al esi en caso de clave inaccesible por si luego se recupera la clave
		return sendResponseToEsi(esiRequest, BLOCK, stringToLog);
	}

	//la clave esta tomada por el o libre, nos fijamos si la clave esta en instancia caida

	//segun respuesta de issue, esto debe quedar asi. no saco este comment aun
	Instancia* chosenInstancia = (Instancia*) fallenInstanciaThatHasKey(esiRequest->operation->key, fallenInstancias);
	if(chosenInstancia != NULL){
		removeKeyFromFallenInstancia(esiRequest->operation->key, chosenInstancia);
		sprintf(*stringToLog, "ESI %d intenta hacer GET sobre la clave %s. Clave inaccesible", esiRequest->id, esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	int keyExists = 0;
	chosenInstancia = lookForOrChoseInstancia(esiRequest->operation->key, &keyExists);

	if(chosenInstancia == NULL){
		sprintf(*stringToLog, "ESI %d es abortado por no existir la clave en ninguna instancia y no haber instancias disponibles", esiRequest->id);
		sendResponseToEsi(esiRequest, ABORT, stringToLog);
		return -1;
	}

	if(keyExists == 0){
		//segun la duda que plantee a adriano en un issue, podria ser que esto pase al SET...
		//... y que aca solo se agregue a una lista provisoria de claves bloqueadas
		addKeyToInstanciaStruct(chosenInstancia, esiRequest->operation->key);
		printf("La clave no existe aun\n");
	}

	printf("Se usara la siguiente instancia para hacer el get\n");
	showInstancia(chosenInstancia);


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
		//TODO decidamos: de aca en mas hacemos exit si muere la conexion con el planificador?
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
	operation->operationCode = OURGET;
}

void showOperation(Operation* operation){
	printf("Operation key = %s\n", operation->key);
}

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	//esiId = getActualEsiID();
	esiId = getActualEsiIDDummy();

	//TODO chequear esto para evitar alocar demas. por otro lado, evitar alocar por cada sentencia...
	char* stringToLog = malloc(200);
	//TODO al hacer este malloc, como sabemos que no queda el contenido anterior?
	//*stringToLog = '\0';

	Operation* operation = malloc(sizeof(Operation));
	//TODO descomentar, esta asi para probar
	/*if(recieveOperation(operation, esiSocket) < 1){
		//TODO en este caso se mata al esi?
		informPlanificador(operation, FALLA);
	}
	showOperation(operation);*/
	recieveOperationDummy(operation);

	char keyStatus;
	//TODO descomentar
	//keyStatus = checkKeyStatusFromPlanificador(esiId, operation->key);
	keyStatus = checkKeyStatusFromPlanificadorDummy();

	EsiRequest esiRequest;
	esiRequest.id = esiId;
	esiRequest.operation = operation;
	esiRequest.socket = esiSocket;

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
			//deberiamos matar al esi?
			//TODO se puede hacer informPlanificador con un error, pero hay que ver si esta esperando eso
			break;
	}

	logOperation(stringToLog);

	free(operation);
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
	createNewInstancia(instanciaSocket, instancias, fallenInstancias);

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

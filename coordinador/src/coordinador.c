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
Instancia* (*distributionAlgorithm)(t_list* aliveInstancias, char* key);
Instancia* (*distributionAlgorithmSimulation)(t_list* aliveInstancias, char* key);
int cantEntry;
int entrySize;
int delay;
Instancia* lastInstanciaChosen;
int firstAlreadyPass = 0;
pthread_mutex_t esisMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t instanciasListMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lastInstanciaChosenMutex = PTHREAD_MUTEX_INITIALIZER;
EsiRequest* actualEsiRequest;
sem_t* instanciaResponse;
char keyStatusFromPlanificador;
sem_t keyStatusFromPlanificadorSemaphore;
int esiIdFromPlanificador;
sem_t esiIdFromPlanificadorSemaphore;

//TODO sacar, esta para probar
int instanciaId = 0;

int main(void) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	initSerializationLogger(logger);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	int listeningPort;
	char* algorithm;
	getConfig(&listeningPort, &algorithm);
	showConfig(listeningPort, algorithm);

	setDistributionAlgorithm(algorithm);

	instancias = list_create();

	sem_init(&keyStatusFromPlanificadorSemaphore, 0, 0);
	sem_init(&esiIdFromPlanificadorSemaphore, 0, 0);

	instanciaResponse = malloc(sizeof(sem_t));
	if(sem_init(instanciaResponse, 0, 0) < 0){
		log_error(logger, "Couldn't create instanciaResponse semaphore");
		//TODO tambien revisar si esta bien este exit
		exit(-1);
	}

	int welcomePlanificadorResponse = welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, COORDINADORID, &welcomePlanificador, logger);

	if(welcomePlanificadorResponse < 0){
		log_error(logger, "Couldn't handshake with planificador, quitting...");
	}

	free(algorithm);

	sem_destroy(instanciaResponse);
	free(instanciaResponse);
	pthread_mutex_destroy(&esisMutex);
	pthread_mutex_destroy(&instanciasListMutex);
	pthread_mutex_destroy(&lastInstanciaChosenMutex);

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
	*algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	if(strcmp(*algorithm, "EL") != 0 && strcmp(*algorithm, "LSU") != 0 && strcmp(*algorithm, "KE") != 0){
		log_error(operationsLogger, "Aborting: cannot recognize distribution algorithm");
		exit(-1);
	}
	cantEntry = config_get_int_value(config, "CANT_ENTRY");
	entrySize = config_get_int_value(config, "ENTRY_SIZE");
	delay = config_get_int_value(config, "DELAY");

	config_destroy(config);
}

Instancia* chooseInstancia(char* key){
	Instancia* chosenInstancia = NULL;

	if(list_size(instancias) != 0){
		t_list* aliveInstancias = list_filter(instancias, (void*) instanciaIsAlive);
		chosenInstancia = (*distributionAlgorithm)(aliveInstancias, key);
		list_destroy(aliveInstancias);
	}
	return chosenInstancia;
}

Instancia* simulateChooseInstancia(char* key){
	Instancia* chosenInstancia = NULL;
	//TODO revisar si hace falta
	pthread_mutex_lock(&instanciasListMutex);
	if(list_size(instancias) != 0){
		t_list* aliveInstancias = list_filter(instancias, (void*) instanciaIsAlive);
		chosenInstancia = (*distributionAlgorithmSimulation)(aliveInstancias, key);
		list_destroy(aliveInstancias);
	}
	pthread_mutex_unlock(&instanciasListMutex);
	return chosenInstancia;
}

//TODO pasar el recieve al hilo de instancia
char* valueFromKeyDirect(Instancia* instancia, char* key, char* instanciaState){
	instancia->actualCommand = INSTANCIA_CHECK_KEY_STATUS;

	if(send_all(instancia->socket, &instancia->actualCommand, sizeof(char)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send command to instancia");
		*instanciaState = INSTANCIA_RESPONSE_FALLEN;
		return NULL;
	}

	if(sendString(key, instancia->socket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send key to instancia to check its value");
		*instanciaState = INSTANCIA_RESPONSE_FALLEN;
		return NULL;
	}

	char* value;
	if(recieveString(&value, instancia->socket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't receive value to respond status command");
		*instanciaState = INSTANCIA_RESPONSE_FALLEN;
		return NULL;
	}

	*instanciaState = INSTANCIA_RESPONSE_SUCCESS;

	return value;
}

//TODO mariano que pasa si se quiere enviar algo null?
//TODO mariano pasar al addToPackageGeneric
int sendPairKeyValueToPlanificador(char* instanciaThatSatisfiesStatus, char* value, char instanciaOrigin){
	//si la clave esta en instancia caida, no se simula y se devuelve NOT_SIMULATED_INSTANCIA
	char typeOfMessage = CORDINADORCONSOLERESPONSEMESSAGE;

	log_info(logger, "About to send type of message to planificador");
	if(send_all(planificadorSocket, &typeOfMessage, sizeof(typeOfMessage)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send type of message to planificador to respond status command");
		planificadorFell();
	}

	//esta habiendo un seg fault aca y supongo que es porque el valor es null. que se hace?
	if(send_all(planificadorSocket, &instanciaOrigin, sizeof(instanciaOrigin)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send instancia origin to planificador to respond status command");
		planificadorFell();
	}

	if(instanciaOrigin == STATUS_NO_INSTANCIAS_AVAILABLE){
		return -1;
	}

	if(sendString(instanciaThatSatisfiesStatus, planificadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send instancia's name to planificador to respond status command");
		planificadorFell();
	}

	if(sendString(value, planificadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send value to planificador to respond status command");
		planificadorFell();
	}

	return 0;
}

void handleInstanciaSimulationForStatus(char* key) {
	Instancia* instanciaThatWouldHaveKey = simulateChooseInstancia(key);
	if (instanciaThatWouldHaveKey) {
		log_info(logger, "Instancia %s is the response to status by simulation", instanciaThatWouldHaveKey->name);
		sendPairKeyValueToPlanificador(instanciaThatWouldHaveKey->name, NULL, STATUS_SIMULATED_INSTANCIA);
	} else {
		log_info(logger, "There are no instancias available to respond status");
		sendPairKeyValueToPlanificador(NULL, NULL, STATUS_NO_INSTANCIAS_AVAILABLE);
	}
}

//TODO + importante. fijarse si hay problemas con accesos simultaneos a campos distintos de un mismo struct

//TODO revisar casos en los que se hace exit. Se deberian liberar los recursos?
int respondStatusToPlanificador(char* key){
	pthread_mutex_lock(&instanciasListMutex);
	Instancia* instanciaThatMightHaveValue = lookForKey(key);
	pthread_mutex_unlock(&instanciasListMutex);

	char* instanciaThatSatisfiesStatus = NULL;
	char* valueThatSatisfiesStatus = NULL;

	//TODO el planificador va a tener que validar que si el valor devuelto es null, no hay valor

	if(instanciaThatMightHaveValue){
		instanciaThatSatisfiesStatus = instanciaThatMightHaveValue->name;

		if(instanciaThatMightHaveValue->isFallen){
			log_info(logger, "Instancia %s is the response to status. It's fallen", instanciaThatSatisfiesStatus);
			sendPairKeyValueToPlanificador(instanciaThatSatisfiesStatus, valueThatSatisfiesStatus, STATUS_NOT_SIMULATED_INSTANCIA);
			free(valueThatSatisfiesStatus);
			return -1;
		}

		char instanciaStatus;
		valueThatSatisfiesStatus = valueFromKeyDirect(instanciaThatMightHaveValue, key, &instanciaStatus);

		if(instanciaStatus == INSTANCIA_RESPONSE_FALLEN){
			log_info(logger, "Instancia %s is the response to status. It felt in the middle of the status request", instanciaThatSatisfiesStatus);
			sendPairKeyValueToPlanificador(instanciaThatSatisfiesStatus, valueThatSatisfiesStatus, STATUS_NOT_SIMULATED_INSTANCIA);
			free(valueThatSatisfiesStatus);
			return -1;
		}

		if(valueThatSatisfiesStatus){
			log_info(logger, "Instancia %s is the response to status. The value from %s is %s", instanciaThatSatisfiesStatus, key, valueThatSatisfiesStatus);
			sendPairKeyValueToPlanificador(instanciaThatSatisfiesStatus, valueThatSatisfiesStatus, STATUS_NOT_SIMULATED_INSTANCIA);
		}else{
			log_info(logger, "Instancia %s was going to be the response to status, but it doesen't have the key anymore", instanciaThatSatisfiesStatus);
			handleInstanciaSimulationForStatus(key);
		}
	}
	else{
		handleInstanciaSimulationForStatus(key);
	}

	free(valueThatSatisfiesStatus);
	return 0;
}

void planificadorFell(){
	log_error(logger, "Planificador disconnected from coordinador, quitting...");
	exit(-1);
}

void handleStatusRequest(){
	char* key;

	log_info(logger, "Gonna handle status...");

	if(recieveString(&key, planificadorSocket) == CUSTOM_FAILURE){
		log_error(logger, "Couldn't receive planificador key to respond status command");
		planificadorFell();
	}

	log_info(logger, "... from key %s", key);

	pthread_mutex_lock(&esisMutex);

	respondStatusToPlanificador(key);

	pthread_mutex_unlock(&esisMutex);
}

int positionInList(Instancia* instancia){
	int instanciasCounter = 0;
	int alreadyFoundInstancia = 0;

	void ifNotActualInstanciaIncrementCounter(Instancia* actualInstancia){
		if(actualInstancia != instancia && !alreadyFoundInstancia){
			instanciasCounter++;
		}else{
			alreadyFoundInstancia = 1;
		}
	}

	list_iterate(instancias, (void*) &ifNotActualInstanciaIncrementCounter);
	return instanciasCounter;
}

int instanciaIsNextToActual(Instancia* instancia){

	if(positionInList(lastInstanciaChosen) == 0 && firstAlreadyPass == 0){
		firstAlreadyPass = 1;
		return 1;
	}

	return positionInList(instancia) >= positionInList(lastInstanciaChosen) + 1;
}

Instancia* getNextInstancia(t_list* aliveInstancias){
	Instancia* instancia = list_find(aliveInstancias, (void*) &instanciaIsNextToActual);
	if(!instancia && firstAlreadyPass){

		Instancia* auxLastInstanciaChosen = lastInstanciaChosen;
		lastInstanciaChosen = list_get(instancias, 0);
		firstAlreadyPass = 0;

		instancia = list_find(aliveInstancias, (void*) &instanciaIsNextToActual);

		if(!instancia){
			lastInstanciaChosen = auxLastInstanciaChosen;
		}
	}
	return instancia;
}

Instancia* equitativeLoad(t_list* aliveInstancias, char* key){
	pthread_mutex_lock(&lastInstanciaChosenMutex);
	Instancia* chosenInstancia = getNextInstancia(aliveInstancias);
	if(chosenInstancia){
		lastInstanciaChosen = chosenInstancia;
	}
	pthread_mutex_unlock(&lastInstanciaChosenMutex);
	return chosenInstancia;
}

//TODO testear (y que no afecte al posta)
Instancia* equitativeLoadSimulation(t_list* aliveInstancias, char* key){
	pthread_mutex_lock(&lastInstanciaChosenMutex);
	Instancia* auxLastInstanciaChosen2 = lastInstanciaChosen;
	int firstAlreadyPassAux = firstAlreadyPass;
	Instancia* instancia = getNextInstancia(aliveInstancias);
	lastInstanciaChosen = auxLastInstanciaChosen2;
	pthread_mutex_unlock(&lastInstanciaChosenMutex);
	firstAlreadyPass = firstAlreadyPassAux;
	return instancia;
}

Instancia* leastSpaceUsed(t_list* aliveInstancias, char* key){
	//TODO
	return NULL;
}

Instancia* keyExplicit(t_list* aliveInstancias, char* key){
	//TODO
	return NULL;
}

Instancia* leastSpaceUsedSimulation(t_list* aliveInstancias, char* key){
	//TODO
	return NULL;
}

Instancia* keyExplicitSimulation(t_list* aliveInstancias, char* key){
	//TODO
	return NULL;
}

void setDistributionAlgorithm(char* algorithm){
	if(strcmp(algorithm, "EL") == 0){
		distributionAlgorithm = &equitativeLoad;
		distributionAlgorithmSimulation = &equitativeLoadSimulation;
	}else if(strcmp(algorithm, "LSU") == 0){
		distributionAlgorithm = &leastSpaceUsed;
		distributionAlgorithmSimulation = &leastSpaceUsedSimulation;
	}else if(strcmp(algorithm, "KE") == 0){
		distributionAlgorithm = &keyExplicit;
		distributionAlgorithmSimulation = &keyExplicitSimulation;
	}
}

int sendResponseToEsi(EsiRequest* esiRequest, char response){
	//TODO aca tambien hay que reintentar hasta que se mande todo?
	//TODO que pasa cuando se pasa una constante por parametro? vimos que hubo drama con eso

	if(send_all(esiRequest->socket, &response, sizeof(response)) == CUSTOM_FAILURE){
		log_warning(operationsLogger, "ESI %d perdio conexion con el coordinador al intentar hacer %s", esiRequest->id,
				getOperationName(esiRequest->operation));
		return -1;
	}

	printf("Se envio send al esi\n");

	return 0;
}

int getActualEsiID(){
	sem_wait(&esiIdFromPlanificadorSemaphore);
	return esiIdFromPlanificador;
}

int getActualEsiIDDummy(){
	return 1;
}

int keyIsOwnedByActualEsi(char keyStatus, EsiRequest* esiRequest){
	if(keyStatus != LOCKED){
		log_warning(operationsLogger, "Esi %d cannot do %s over key %s. Key not blocked by him",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT);
		return -1;
	}

	return 0;
}

int tryToExecuteOperationOnInstancia(EsiRequest* esiRequest, Instancia* chosenInstancia){
	log_info(logger, "The next instancia is going to be used for %s", getOperationName(esiRequest->operation));
	showInstancia(chosenInstancia);

	chosenInstancia->actualCommand = INSTANCIA_DO_OPERATION;
	actualEsiRequest = esiRequest;

	log_info(logger, "Esi's thread is gonna send the operation request to instancia");
	if(instanciaDoOperation(chosenInstancia, esiRequest->operation, logger) == CUSTOM_FAILURE){
		log_warning(logger, "Esi %d is aborted because instancia %s fell", esiRequest->id, chosenInstancia->name);
		sendResponseToEsi(actualEsiRequest, ABORT);
		return -1;
	}

	log_info(logger, "Operation sent to instancia");

	pthread_mutex_unlock(&instanciasListMutex);
	sem_wait(instanciaResponse);
	pthread_mutex_lock(&instanciasListMutex);

	log_info(logger, "Esi's thread is gonna process instancia's response");

	if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN || instanciaResponseStatus == INSTANCIA_RESPONSE_FAILED){
		if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN){
			log_warning(operationsLogger, "Esi %d cannot do %s over %s. Instancia %s fell. Inaccessible key", actualEsiRequest->id, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key, chosenInstancia->name);
		}else{
			log_warning(operationsLogger, "Esi %d cannot do %s over %s. Instancia %s couldn't do operation", actualEsiRequest->id, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key, chosenInstancia->name);
		}
		sendResponseToEsi(actualEsiRequest, ABORT);
		return -1;
	}

	log_info(operationsLogger, "Esi %d does %s over key %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

	return 0;
}

Instancia* lookForKeyAndRemoveIfInFallenInstancia(EsiRequest* esiRequest){
	Instancia* instanciaToBeUsed = lookForKey(esiRequest->operation->key);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		log_warning(operationsLogger, "Esi %d tries %s over key %s. Inaccessible key",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT);
		removeKeyFromFallenInstancia(esiRequest->operation->key, instanciaToBeUsed);
	}
	return instanciaToBeUsed;
}

int doSet(EsiRequest* esiRequest, char keyStatus){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest) < 0){
		return -1;
	}

	int keyExists = 0;

	log_info(logger, "Gonna look for instancia to set key %s:", esiRequest->operation->key);
	Instancia* instanciaToBeUsed = lookForKeyAndRemoveIfInFallenInstancia(esiRequest);
	showInstancia(instanciaToBeUsed);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		return -1;
	}
	else{
		if(instanciaToBeUsed == NULL){
			instanciaToBeUsed = chooseInstancia(esiRequest->operation->key);
			if(instanciaToBeUsed == NULL){
				log_warning(operationsLogger, "Esi %d is aborted when trying SET because the key is not in any instancia and there are no instancias available", esiRequest->id);
				sendResponseToEsi(esiRequest, ABORT);
				return -1;
			}
		}else{
			keyExists = 1;
		}
	}

	if(keyExists == 0){
		addKeyToInstanciaStruct(instanciaToBeUsed, esiRequest->operation->key);
		log_info(logger, "Key %s does not exist yet", esiRequest->operation->key);
	}

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed) < 0){
		return -1;
	}

	return sendResponseToEsi(esiRequest, SUCCESS);
}

int doStore(EsiRequest* esiRequest, char keyStatus){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest) < 0){
		return -1;
	}

	Instancia* instanciaToBeUsed = lookForKeyAndRemoveIfInFallenInstancia(esiRequest);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		return -1;
	}else{
		if(instanciaToBeUsed == NULL){
			log_warning(operationsLogger, "Esi %d tries %s over key %s. Key not identified in instancias", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
			sendResponseToEsi(esiRequest, ABORT);
			return -1;
		}
	}

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed) < 0){
		return -1;
	}

	return sendResponseToEsi(esiRequest, FREE);
}

int doGet(EsiRequest* esiRequest, char keyStatus){
	if(keyStatus == BLOCKED){
		log_info(operationsLogger, "Esi %d tries GET over key %s. Blocked key", esiRequest->id, esiRequest->operation->key);
		return sendResponseToEsi(esiRequest, BLOCK);
	}

	//TODO testear cuando la instancia se caiga y el coordinador se entere por ella (y no porque un esi quiere acceder, sino se borra la clave!)
	Instancia* instanciaWithKey = lookForKeyAndRemoveIfInFallenInstancia(esiRequest);

	if(instanciaWithKey != NULL && instanciaWithKey->isFallen){
		return -1;
	}

	if(keyStatus == LOCKED){
		log_info(operationsLogger, "Esi %d does GET over key %s, which he already had", esiRequest->id, esiRequest->operation->key);
		return sendResponseToEsi(esiRequest, SUCCESS);
	}

	log_info(operationsLogger, "Esi %d does GET over key %s", esiRequest->id, esiRequest->operation->key);
	return sendResponseToEsi(esiRequest, LOCK);
}

char checkKeyStatusFromPlanificador(int esiId, char* key){

	log_info(logger, "Gonna recieve %s's status from planificador", key);

	//TODO probar esto
	char typeOfMessage = KEYSTATUSMESSAGE;

	void* package = NULL;
	int offset = 0;
	int sizeString = strlen(key)+1;

	addToPackageGeneric(&package, &typeOfMessage, sizeof(char), &offset);
	addToPackageGeneric(&package, &sizeString, sizeof(sizeString), &offset);	
	addToPackageGeneric(&package, key, sizeString, &offset);

	if(send_all(planificadorSocket, package, offset) == CUSTOM_FAILURE){
		planificadorFell();
	}

	free(package);

	sem_wait(&keyStatusFromPlanificadorSemaphore);
	return keyStatusFromPlanificador;
}

char checkKeyStatusFromPlanificadorDummy(){
	return LOCKED;
}

void recieveOperationDummy(Operation** operation){
	*operation = malloc(sizeof(Operation));
	(*operation)->key = malloc(100);
	//strcpy((*operation)->key, "lio:messi");
	strcpy((*operation)->key, "cristiano:ronaldocristiano:ronaldocristiano:ronaldo");
	(*operation)->value = malloc(100);
	strcpy((*operation)->value, "elMasCapo");
	(*operation)->operationCode = OURSET;
}

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	//TODO reveer este log, no creo que sea correcto ponerlo aca
	log_info(logger, "Waiting for esis to arrive");

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;

	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		//TODO revisar que se estaria usando el esiRequest anterior, y esto podria fallar
		//log_info(logger, "Couldn't receive operation from esi %d. Finished or fell, so his thread dies", esiRequest.id);
		destroyOperation(esiRequest.operation);
		return -1;
	}
	//recieveOperationDummy(&esiRequest.operation);

	if(validateOperationKeySize(esiRequest.operation) < 0){
		log_warning(logger, "Esi is aborted for sending an operation whose key size is greater than 40");
		showOperation(esiRequest.operation);
		//TODO hay que rediseniar esto, porque no se conoce el id del esi aun y la funcion sendRespondeToEsi lo usa (va a romper)
		esiRequest.id = 0;
		sendResponseToEsi(&esiRequest, ABORT);
		destroyOperation(esiRequest.operation);
		return -1;
	}

	pthread_mutex_lock(&esisMutex);

	log_info(logger, "Arrived esi is going to do:");
	showOperation(esiRequest.operation);

	esiId = getActualEsiID();
	//esiId = getActualEsiIDDummy();
	esiRequest.id = esiId;
	log_info(logger, "Esi's id is %d", esiId);

	char keyStatus;
	keyStatus = checkKeyStatusFromPlanificador(esiRequest.id, esiRequest.operation->key);
	//keyStatus = checkKeyStatusFromPlanificadorDummy();

	log_info(logger, "Status from key %s, from esi %d, is %s", esiRequest.operation->key, esiRequest.id, getKeyStatusName(keyStatus));

	if(strcmp(getKeyStatusName(keyStatus), "UNKNOWN KEY STATUS") == 0){
		log_error(logger, "Couldn't receive esi key status from planificador, aborting esi %d...",  esiRequest.id);
		destroyOperation(esiRequest.operation);
		sendResponseToEsi(&esiRequest, ABORT);
		pthread_mutex_unlock(&esisMutex);
		return -1;
	}

	pthread_mutex_lock(&instanciasListMutex);
	switch (esiRequest.operation->operationCode){
		case OURGET:
			if (doGet(&esiRequest, keyStatus) < 0){
				operationResult = -1;
			}
			break;
		case OURSET:
			if(doSet(&esiRequest, keyStatus) < 0){
				operationResult = -1;
			}
			break;
		case OURSTORE:
			if(doStore(&esiRequest, keyStatus) < 0){
				operationResult = -1;
			}
			break;
		default:
			log_warning(operationsLogger, "Esi %d sent an invalid operation", esiRequest.id);
			sendResponseToEsi(&esiRequest, ABORT);
			operationResult = -1;
			break;
	}

	pthread_mutex_unlock(&instanciasListMutex);
	pthread_mutex_unlock(&esisMutex);

	destroyOperation(esiRequest.operation);
	usleep(delay * 1000);
	return operationResult;
}

Instancia* initialiceArrivedInstancia(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket, logger) < 0){
		return NULL;
	}
	log_info(logger, "Arrived instancia's name is %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize, logger) < 0){
		free(arrivedInstanciaName);
		return NULL;
	}
	log_info(logger, "Sent configuration to instancia %s", arrivedInstanciaName);

	pthread_mutex_lock(&instanciasListMutex);
	Instancia* arrivedInstancia = existsInstanciaWithName(arrivedInstanciaName);
	if(arrivedInstancia){
		instanciaIsBack(arrivedInstancia, instanciaSocket);
		log_info(logger, "Instancia %s is back", arrivedInstanciaName);
	}else{
		arrivedInstancia = createNewInstancia(instanciaSocket, arrivedInstanciaName);

		if(!arrivedInstancia){
			log_error(logger, "Couldn't initialize instancia's semaphore");
			free(arrivedInstanciaName);
			//TODO que deberia pasar aca? mientras dejo este exit. tener cuidado con los semaforos que el de abajo no se libera si se cambia exit por return
			exit(-1);
			//si se decide que hay que matar al hilo, devolver NULL!!!
		}

		if(list_size(instancias) == 1){
			pthread_mutex_lock(&lastInstanciaChosenMutex);
			lastInstanciaChosen = list_get(instancias, 0);
			pthread_mutex_unlock(&lastInstanciaChosenMutex);
		}

		log_info(logger, "Instancia %s is new", arrivedInstanciaName);
	}

	free(arrivedInstanciaName);
	pthread_mutex_unlock(&instanciasListMutex);

	return arrivedInstancia;
}

Instancia* initialiceArrivedInstanciaDummy(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket, logger) < 0){
		return NULL;
	}
	//recieveInstanciaNameDummy(&arrivedInstanciaName);
	log_info(logger, "Arrived instancia's name is %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize, logger) < 0){
		free(arrivedInstanciaName);
		return NULL;
	}
	log_info(logger, "Sent configuration to instancia %s", arrivedInstanciaName);

	pthread_mutex_lock(&instanciasListMutex);
	char* harcodedNameForTesting = malloc(strlen(arrivedInstanciaName) + 5);
	strcpy(harcodedNameForTesting, arrivedInstanciaName);
	char* instanciaNumberInString = malloc(sizeof(int));
	sprintf(instanciaNumberInString, "%d", instanciaId);
	strcat(harcodedNameForTesting, instanciaNumberInString);
	Instancia* instancia = createNewInstancia(instanciaSocket, harcodedNameForTesting);
	instanciaId++;

	if(list_size(instancias) == 1){
		pthread_mutex_lock(&lastInstanciaChosenMutex);
		lastInstanciaChosen = list_get(instancias, 0);
		pthread_mutex_unlock(&lastInstanciaChosenMutex);
	}

	free(arrivedInstanciaName);
	free(harcodedNameForTesting);
	free(instanciaNumberInString);
	pthread_mutex_unlock(&instanciasListMutex);

	return instancia;
}

void sendCompactRequest(Instancia* instanciaToBeCompacted){
	instanciaToBeCompacted->actualCommand = INSTANCIA_DO_COMPACT;
	if(send_all(instanciaToBeCompacted->socket, &instanciaToBeCompacted->actualCommand, sizeof(char)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send compact order to instancia %s, so it fell", instanciaToBeCompacted->name);
	}
}

t_list* sendCompactRequestToEveryAliveInstaciaButActual(Instancia* compactCausative){
	int instanciaIsAliveAndNotCausative(Instancia* instancia){
		return instanciaIsAlive(instancia) && instancia != compactCausative;
	}

	t_list* aliveInstanciasButCausative  = list_filter(instancias, (void*) instanciaIsAliveAndNotCausative);
	showInstancias(aliveInstanciasButCausative);
	list_iterate(aliveInstanciasButCausative , (void*) sendCompactRequest);

	return aliveInstanciasButCausative;
}

void waitInstanciaToCompact(Instancia* instancia){
	sem_wait(instancia->compactSemaphore);
	log_info(logger, "Instancia %s came back from compact", instancia->name);
}

void waitInstanciasToCompact(t_list* instanciasThatNeededToCompact){
	list_iterate(instanciasThatNeededToCompact, (void*) waitInstanciaToCompact);

	list_destroy(instanciasThatNeededToCompact);
}

/*char instanciaDoCompact(Instancia* instancia){
	char instanciaDoCompactCode = INSTANCIA_DO_COMPACT;
	log_info(logger, "About to send instancia compact order to %s", instancia->name);
	if(send_all(instancia->socket, &instanciaDoCompactCode, sizeof(char)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send compact order to instancia %s, so it fell", instancia->name);
		return INSTANCIA_RESPONSE_FALLEN;
	}
	log_info(logger, "Compact order sent to instancia %s", instancia->name);
	return waitForInstanciaResponse(instancia);
}*/

/*void handleInstanciaCompactStatus(Instancia* instancia, char compactStatus){
	if(compactStatus == INSTANCIA_RESPONSE_FALLEN){
		log_info(logger, "Instancia %s couldn't compact, it fell", instancia->name);
		instanciaHasFallen(instancia);
	}else if(compactStatus == INSTANCIA_COMPACT_SUCCESS){
		log_info(logger, "Instancia %s could compact", instancia->name);
	}else{
		log_info(logger, "Instancia %s couldn't compact", instancia->name);
	}
}*/

char instanciaDoCompactDummy(){
	return INSTANCIA_RESPONSE_FALLEN;
}

int handleInstanciaCompact(Instancia* actualInstancia, t_list* instanciasToBeCompactedButCausative){
	log_info(logger, "Instancia %s is waiting the others to compact", actualInstancia->name);
	waitInstanciasToCompact(instanciasToBeCompactedButCausative);
	log_info(logger, "All instancias came back from compact, now waiting for the causative one...");

	return 0;
}

int handleInstanciaOperation(Instancia* actualInstancia, t_list** instanciasToBeCompactedButCausative){
	log_info(logger, "%s's thread is gonna handle %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));

	char status;
	if(recv_all(actualInstancia->socket, &status, sizeof(status)) == CUSTOM_FAILURE){
		log_error(logger, "Couldn't recieve instancia response");
		return -1;
	}

	instanciaResponseStatus = status;

	log_info(logger, "Instancia's response is gonna be processed");

	if(status == INSTANCIA_RESPONSE_FALLEN){
		log_error(logger, "Instancia %s couldn't do %s because it fell. His thread dies, and key %s is deleted:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
		removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
		return -1;
	}else if(status == INSTANCIA_RESPONSE_FAILED){
		log_error(logger, "Instancia %s couldn't do %s, so the key %s is deleted:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
		removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
		showInstancia(actualInstancia);
	}else if(status == INSTANCIA_RESPONSE_SUCCESS){
		log_info(logger, "%s could do %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));
	}

	sem_post(instanciaResponse);
	return 0;
}

void instanciaExitGracefully(Instancia* instancia){
	instanciaHasFallen(instancia);

	switch(instancia->actualCommand){
		case INSTANCIA_DO_OPERATION:
			instanciaResponseStatus = INSTANCIA_RESPONSE_FALLEN;
			sem_post(instanciaResponse);
			break;

		case INSTANCIA_DO_COMPACT:

			sem_post(instancia->compactSemaphore);

			break;

		case INSTANCIA_CHECK_KEY_STATUS:

			//TODO revisar que hacer

			break;

		default:

			//TODO revisar este caso que se usa por ejemplo cuando llegaron y todavia no estan haciendo nada

			break;
	}

	showInstancias(instancias);

	pthread_mutex_unlock(&instanciasListMutex);
}

int handleInstancia(int instanciaSocket){
	t_list* instanciasToBeCompactedButCausative = NULL;

	Instancia* actualInstancia;
	//actualInstancia = initialiceArrivedInstancia(instanciaSocket);
	actualInstancia = initialiceArrivedInstanciaDummy(instanciaSocket);

	if(!actualInstancia){
		return -1;
	}

	pthread_mutex_lock(&instanciasListMutex);
	showInstancias(instancias);
	pthread_mutex_unlock(&instanciasListMutex);

	char instanciaCommandResponse;

	while(1){

		if(recv_all(actualInstancia->socket, &instanciaCommandResponse, sizeof(instanciaCommandResponse)) == CUSTOM_FAILURE){
			log_error(logger, "Couldn't recieve instancia response");
			pthread_mutex_lock(&instanciasListMutex);
			instanciaExitGracefully(actualInstancia);
			return -1;
		}

		pthread_mutex_lock(&instanciasListMutex);

		switch(instanciaCommandResponse){

			case INSTANCIA_DID_OPERATION:

				if(handleInstanciaOperation(actualInstancia, &instanciasToBeCompactedButCausative) < 0){
					instanciaExitGracefully(actualInstancia);
					return -1;
				}

				break;

			case INSTANCIA_COMPACT_REQUEST:

				log_info(logger, "Instancia %s needs to compact, so every other active instancia will do the same", actualInstancia->name);
				instanciasToBeCompactedButCausative = sendCompactRequestToEveryAliveInstaciaButActual(actualInstancia);

				handleInstanciaCompact(actualInstancia, instanciasToBeCompactedButCausative);

				break;

			case INSTANCIA_DID_COMPACT:

				//TODO agregar aca, y abajo, el exitGracefully

				break;

			case INSTANCIA_DID_CHECK_KEY_STATUS:

				break;

			default:

				log_error(logger, "Se mando a hacer un comando invalido al hilo de instancia");
				instanciaExitGracefully(actualInstancia);
				return -1;

				break;
		}

		pthread_mutex_unlock(&instanciasListMutex);
	}

	return 0;
}

int handleEsi(int esiSocket){
	while(1){
		if (recieveStentenceToProcess(esiSocket) < 0){
			//ya que la instancia cierra su socket cuando se cae
			close(esiSocket);
			//va a matar al hilo porque sale de este while!
			break;
		}
	}
	return 0;
}

void planificadorHandler(int* allocatedClientSocket){
	while(1) {
		char planificadorMessage;
		if(recv_all(planificadorSocket, &planificadorMessage, sizeof(planificadorMessage)) == CUSTOM_FAILURE) {
			planificadorFell();
		}
		switch (planificadorMessage) {
		case PLANIFICADOR_KEY_STATUS_RESPONSE:
			if(recv_all(planificadorSocket, &keyStatusFromPlanificador, sizeof(char)) == CUSTOM_FAILURE){
				planificadorFell();
			}
			sem_post(&keyStatusFromPlanificadorSemaphore);
			break;
		case PLANIFICADOR_ESI_ID_RESPONSE:
			if(recieveInt(&esiIdFromPlanificador, planificadorSocket) <= 0){
				planificadorFell();
			}
			sem_post(&esiIdFromPlanificadorSemaphore);
			break;
		case PLANIFICADOR_STATUS_REQUEST:
			handleStatusRequest();
			break;
		default:
			log_error(logger, "Invalid planificador message");
			break;
		}

	}

	free(allocatedClientSocket);

	//TODO recibir id esi, estado clave y comando status
	/*char planificadorMessage;
	while(1){
		//TODO descomentar, hasta que se adapten los mensajes necesarios en el planificador

		if(recv_all(planificadorSocket, &planificadorMessage, sizeof(planificadorMessage)) == CUSTOM_FAILURE){
			log_error(logger, "Couldn't recieve type of request from planificador");
			planificadorFell();
			break;
		}

		switch(planificadorMessage){
			case PLANIFICADOR_STATUS_REQUEST:

				handleStatusRequest();

				break;

			default:

				log_error(logger, "Couldn't understand planificador message");
				//TODO que deberia pasar aca?
re
				break;
		}
	}*/

}

void raiseThreadDependingOnId(int* clientSocket){
	char id = recieveClientId(*clientSocket, COORDINADOR, logger);

	if (id == INSTANCIAID){
		handleInstancia(*clientSocket);
	}else if(id == ESIID){
		handleEsi(*clientSocket);
	}else{
		log_info(logger, "I received a strange");
	}

	free(clientSocket);
}

int clientHandler(int clientSocket, void (*handleThreadProcedure)(int* socket)){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, (void*) handleThreadProcedure, clientSocketPointer) != 0){
		log_error(logger, "Error creating thread");
		free(clientSocketPointer);
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		log_error(logger,"Couldn't detach thread");
		free(clientSocketPointer);
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket, int newPlanificadorSocket){
	planificadorSocket = newPlanificadorSocket;

	clientHandler(planificadorSocket, planificadorHandler);

	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		clientHandler(clientSocket, raiseThreadDependingOnId);
	}

	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"

t_log* operationsLogger;

int planificadorSocket;
void setDistributionAlgorithm();
Instancia* (*distributionAlgorithm)(t_list* aliveInstancias, char* key);
Instancia* (*distributionAlgorithmSimulation)(t_list* aliveInstancias, char* key);
char* algorithm;
int delay;
int firstAlreadyPass = 0;
pthread_mutex_t esisMutex = PTHREAD_MUTEX_INITIALIZER;
char keyStatusFromPlanificador;
sem_t keyStatusFromPlanificadorSemaphore;
int esiIdFromPlanificador;
sem_t esiIdFromPlanificadorSemaphore;

int main(int argc, char* argv[]) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	initSerializationLogger(logger);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	if (argc != 2) {
		log_error(logger, "Coordinador cannot execute: you must enter a configuration file");
		return -1;
	}

	CFG_FILE = strdup(argv[1]);

	int listeningPort;
	getConfig(&listeningPort);
	showConfig(listeningPort);

	setDistributionAlgorithm();

	instancias = list_create();

	pthread_mutex_init(&instanciasListMutex, NULL);
	pthread_mutex_init(&lastInstanciaChosenMutex, NULL);
	sem_init(&instanciaResponse, 0, 0);
	sem_init(&keyStatusFromPlanificadorSemaphore, 0, 0);
	sem_init(&esiIdFromPlanificadorSemaphore, 0, 0);

	int welcomePlanificadorResponse = welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, COORDINADORID, &welcomePlanificador, logger);

	if(welcomePlanificadorResponse < 0){
		log_error(logger, "Couldn't handshake with planificador, quitting...");
		exit(-1);
	}

	return 0;
}

void showConfig(int listeningPort){
	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);
}

void getConfig(int* listeningPort){
	t_config* config;
	config = config_create(CFG_FILE);

	free(CFG_FILE);

	if (config == NULL) {
		log_error(logger, "Coordinador cannot work because of invalid configuration file");
		exit(-1);
	}

	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	if(strcmp(algorithm, "EL") != 0 && strcmp(algorithm, "LSU") != 0 && strcmp(algorithm, "KE") != 0){
		log_error(logger, "Aborting: cannot recognize distribution algorithm");
		exit(-1);
	}
	cantEntry = config_get_int_value(config, "CANT_ENTRY");
	entrySize = config_get_int_value(config, "ENTRY_SIZE");
	delay = config_get_int_value(config, "DELAY");

	config_destroy(config);
}

Instancia* applyDistributionAlgorithm(char* key, Instancia* (*theAlgorithm)(t_list* aliveInstancias, char* key)){
	Instancia* chosenInstancia = NULL;
	t_list* aliveInstancias = list_filter(instancias, (void*) instanciaIsAlive);
	if(list_size(aliveInstancias) != 0){
		chosenInstancia = (*theAlgorithm)(aliveInstancias, key);
	}
	list_destroy(aliveInstancias);
	return chosenInstancia;
}

Instancia* simulateChooseInstancia(char* key){
	return applyDistributionAlgorithm(key, distributionAlgorithmSimulation);
}

Instancia* chooseInstancia(char* key){
	return applyDistributionAlgorithm(key, distributionAlgorithm);
}

char* getValueFromKey(Instancia* instancia, char* key){
	instancia->actualCommand = INSTANCIA_CHECK_KEY_STATUS;

	if(send_all(instancia->socket, &instancia->actualCommand, sizeof(char)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send command to instancia");
		instanciaStatusFromValueRequest = INSTANCIA_RESPONSE_FALLEN;
		return NULL;
	}

	if(sendString(key, instancia->socket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send key to instancia to check its value");
		instanciaStatusFromValueRequest = INSTANCIA_RESPONSE_FALLEN;
		return NULL;
	}

	pthread_mutex_unlock(&instanciasListMutex);
	sem_wait(&valueFromKeyInstanciaSemaphore);

	return valueFromKey;
}

//TODO mariano que pasa si se quiere enviar algo null? ahora no se mandan mas strings nulos pero probarlo por las dudas.
int sendPairKeyValueToPlanificador(char* instanciaThatSatisfiesStatus, char* value, char instanciaOrigin){
	//si la clave esta en instancia caida, no se simula y se devuelve NOT_SIMULATED_INSTANCIA_BUT_FALLEN
	char typeOfMessage = CORDINADORCONSOLERESPONSEMESSAGE;

	log_info(logger, "About to send type of message to planificador");
	if(send_all(planificadorSocket, &typeOfMessage, sizeof(typeOfMessage)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send type of message to planificador to respond status command");
		pthread_mutex_unlock(&instanciasListMutex);
		planificadorFell();
	}
	log_info(logger, "Sent type of message (status response) to planificador");

	//esta habiendo un seg fault aca y supongo que es porque el valor es null. que se hace?
	//avance: no es null...
	if(send_all(planificadorSocket, &instanciaOrigin, sizeof(instanciaOrigin)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send instancia origin to planificador to respond status command");
		pthread_mutex_unlock(&instanciasListMutex);
		planificadorFell();
	}
	log_info(logger, "Sent instancia status to planificador to response status");

	if(instanciaOrigin == STATUS_NO_INSTANCIAS_AVAILABLE){
		return -1;
	}

	if(sendString(instanciaThatSatisfiesStatus, planificadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send instancia's name to planificador to respond status command");
		pthread_mutex_unlock(&instanciasListMutex);
		planificadorFell();
	}
	log_info(logger, "Sent name of instancia to response status");

	if(instanciaOrigin == STATUS_SIMULATED_INSTANCIA || instanciaOrigin == STATUS_NOT_SIMULATED_INSTANCIA_BUT_FALLEN){
		return -1;
	}

	if(sendString(value, planificadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send value to planificador to respond status command");
		pthread_mutex_unlock(&instanciasListMutex);
		planificadorFell();
	}
	log_info(logger, "Sent value %s to planificador as a status response, because an instancia was found", value);

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

void respondStatusToPlanificador(char* key){
	pthread_mutex_lock(&instanciasListMutex);
	Instancia* instanciaThatMightHaveValue = lookForKey(key);

	char* instanciaThatSatisfiesStatus = NULL;
	char* valueThatSatisfiesStatus = NULL;

	if(instanciaThatMightHaveValue){
		instanciaThatSatisfiesStatus = instanciaThatMightHaveValue->name;

		if(instanciaThatMightHaveValue->isFallen){
			log_info(logger, "Instancia %s is the response to status. It's fallen", instanciaThatSatisfiesStatus);
			sendPairKeyValueToPlanificador(instanciaThatSatisfiesStatus, valueThatSatisfiesStatus, STATUS_NOT_SIMULATED_INSTANCIA_BUT_FALLEN);
			pthread_mutex_unlock(&instanciasListMutex);
			free(valueThatSatisfiesStatus);
			return;
		}

		valueThatSatisfiesStatus = getValueFromKey(instanciaThatMightHaveValue, key);
		pthread_mutex_lock(&instanciasListMutex);

		if(instanciaStatusFromValueRequest == INSTANCIA_RESPONSE_FALLEN){
			log_info(logger, "Instancia %s is the response to status. It felt in the middle of the status request", instanciaThatSatisfiesStatus);
			sendPairKeyValueToPlanificador(instanciaThatSatisfiesStatus, valueThatSatisfiesStatus, STATUS_NOT_SIMULATED_INSTANCIA_BUT_FALLEN);
			pthread_mutex_unlock(&instanciasListMutex);
			free(valueThatSatisfiesStatus);
			return;
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

	pthread_mutex_unlock(&instanciasListMutex);
	free(valueThatSatisfiesStatus);
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
	free(key);

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
	Instancia* res = list_get(aliveInstancias, 0);

	void selector(Instancia* someInstance) {
		if (res->spaceUsed > someInstance->spaceUsed) {
			res = someInstance;
		}
	}

	list_iterate(aliveInstancias, (void*) selector);

	return res;
}

Instancia* keyExplicit(t_list* aliveInstancias, char* key){

	char toLower(char _char) {
		if (_char >= 'A' && _char <= 'Z') {
			_char += 32;
		}
		return _char;
	}

	int cantInstances = list_size(aliveInstancias);
	int cantChars = 26;
	int cantCharsByInstance = cantChars / cantInstances;

	if(cantChars % cantInstances != 0) cantCharsByInstance++;

	char initialChar = toLower(key[0]);

	int indexRes = (initialChar - 'a') / cantCharsByInstance;

	return list_get(aliveInstancias, indexRes);
}

void setDistributionAlgorithm(){
	if(strcmp(algorithm, "EL") == 0){
		distributionAlgorithm = &equitativeLoad;
		distributionAlgorithmSimulation = &equitativeLoadSimulation;
	}else if(strcmp(algorithm, "LSU") == 0){
		distributionAlgorithm = &leastSpaceUsed;
		distributionAlgorithmSimulation = &leastSpaceUsed;
	}else if(strcmp(algorithm, "KE") == 0){
		distributionAlgorithm = &keyExplicit;
		distributionAlgorithmSimulation = &keyExplicit;
	}
}

int sendResponseToEsi(EsiRequest* esiRequest, char response){
	if(send_all(esiRequest->socket, &response, sizeof(response)) == CUSTOM_FAILURE){
		log_warning(operationsLogger, "ESI %d lost conection with coordinador while trying %s", esiRequest->id,
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
	if(instanciaDoOperation(chosenInstancia, esiRequest->operation) == CUSTOM_FAILURE){
		log_warning(operationsLogger, "Esi %d is aborted because instancia %s fell", esiRequest->id, chosenInstancia->name);
		sendResponseToEsi(actualEsiRequest, ABORT);
		return -1;
	}

	log_info(logger, "Operation sent to instancia");

	pthread_mutex_unlock(&instanciasListMutex);
	sem_wait(&instanciaResponse);
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

	return 0;
}

Instancia* lookForKeyAndRemoveIfInFallenInstancia(EsiRequest* esiRequest){
	Instancia* instanciaToBeUsed = lookForKey(esiRequest->operation->key);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		log_warning(operationsLogger, "Esi %d tries %s over key %s. Inaccessible key",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT);
		removeKeyFromFallenInstancia(esiRequest->operation->key, instanciaToBeUsed);
		showInstancia(instanciaToBeUsed);
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

	log_info(operationsLogger, "Esi %d does %s over key %s with value %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key, esiRequest->operation->value);

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

	log_info(operationsLogger, "Esi %d does %s over key %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

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
	//TODO reveer este log, hay un test para correr en que se muestran dos de estos seguidos
	log_info(logger, "Waiting for esi's instruction");

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;

	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't receive operation from esi on socket %d. Finished or fell, so his thread dies", esiRequest.socket);
		destroyOperation(esiRequest.operation);
		return -1;
	}
	//recieveOperationDummy(&esiRequest.operation);

	pthread_mutex_lock(&esisMutex);

	log_info(logger, "Arrived esi is going to do:");
	showOperation(esiRequest.operation, logger);

	esiId = getActualEsiID();
	//esiId = getActualEsiIDDummy();
	esiRequest.id = esiId;
	log_info(logger, "Esi's id is %d", esiId);

	char keyStatus;
	keyStatus = checkKeyStatusFromPlanificador(esiRequest.id, esiRequest.operation->key);
	//keyStatus = checkKeyStatusFromPlanificadorDummy();

	log_info(logger, "Status from key %s, from esi %d, is %s", esiRequest.operation->key, esiRequest.id, getKeyStatusName(keyStatus));

	if(strcmp(getKeyStatusName(keyStatus), "UNKNOWN KEY STATUS") == 0){
		log_warning(operationsLogger, "Couldn't receive esi key status from planificador, aborting esi %d...",  esiRequest.id);
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

	destroyOperation(esiRequest.operation);
	usleep(delay * 1000);

	pthread_mutex_unlock(&esisMutex);

	return operationResult;
}

int handleInstancia(int instanciaSocket){
	t_list* instanciasToBeCompactedButCausative = NULL;

	Instancia* actualInstancia;
	actualInstancia = initialiceArrivedInstancia(instanciaSocket);
	//actualInstancia = initialiceArrivedInstanciaDummy(instanciaSocket);

	if(!actualInstancia){
		return -1;
	}

	pthread_mutex_lock(&instanciasListMutex);
	showInstancias(instancias);
	pthread_mutex_unlock(&instanciasListMutex);

	char instanciaCommandResponse;

	while(1){

		if(recv_all(actualInstancia->socket, &instanciaCommandResponse, sizeof(instanciaCommandResponse)) == CUSTOM_FAILURE){
			log_warning(logger, "Couldn't recieve instancia's message, it fell");
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

				sem_post(actualInstancia->compactSemaphore);

				break;

			case INSTANCIA_DID_CHECK_KEY_STATUS:

				if(recieveString(&valueFromKey, actualInstancia->socket) == CUSTOM_FAILURE){
					log_warning(logger, "Couldn't receive value from key to respond status command");
					instanciaExitGracefully(actualInstancia);
					return -1;
				}

				instanciaStatusFromValueRequest = INSTANCIA_RESPONSE_SUCCESS;
				sem_post(&valueFromKeyInstanciaSemaphore);

				break;

			default:

				log_warning(logger, "Instancia sent an invalid command. Killing his thread");
				instanciaExitGracefully(actualInstancia);
				return -1;

				break;
		}

		actualInstancia->actualCommand = 0;
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
	free(allocatedClientSocket);
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
				if(recieveInt(&esiIdFromPlanificador, planificadorSocket) == CUSTOM_FAILURE){
					planificadorFell();
				}
				sem_post(&esiIdFromPlanificadorSemaphore);
				break;
			case PLANIFICADOR_STATUS_REQUEST:
				handleStatusRequest();
				break;
			default:
				log_error(logger, "Invalid planificador message");
				exit(-1);
				break;
		}
	}
}

void raiseThreadDependingOnId(int* clientSocket){

	int clientSocketCopy = *clientSocket;

	free(clientSocket);

	char id = recieveClientId(clientSocketCopy, COORDINADOR, logger);

	if (id == INSTANCIAID){
		handleInstancia(clientSocketCopy);
	}else if(id == ESIID){
		handleEsi(clientSocketCopy);
	}else{
		log_info(logger, "I received a strange");
	}
}

int clientHandler(int clientSocket, void (*handleThreadProcedure)(int* socket)){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	pthread_create(&clientThread, NULL, (void*) handleThreadProcedure, clientSocketPointer);
	pthread_detach(clientThread);

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

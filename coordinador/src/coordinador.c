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
Instancia* (*distributionAlgorithm)(char* key);
Instancia* (*distributionAlgorithmSimulation)(char* key);
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

int main(void) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	int listeningPort;
	char* algorithm;
	getConfig(&listeningPort, &algorithm);
	showConfig(listeningPort, algorithm);

	setDistributionAlgorithm(algorithm);

	instancias = list_create();

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
		log_error(operationsLogger, "Abortando: no se reconoce el algoritmo de distribucion");
		exit(-1);
	}
	cantEntry = config_get_int_value(config, "CANT_ENTRY");
	entrySize = config_get_int_value(config, "ENTRY_SIZE");
	delay = config_get_int_value(config, "DELAY");

	config_destroy(config);
}

//TODO, estas dos deberian devolver INSTANCIA_HAS_FALLEN si se cayo
/*char valueStatus(Instancia* instancia, char* key);
char* valueFromKey(Instancia* instancia, char* key);*/

Instancia* lookForInstanciaAndGetValueStatus(char* key, char* instanciaValueResponse){
	pthread_mutex_lock(&instanciasListMutex);
	Instancia* instanciaThatMightHaveValue = lookForKey(key);
	pthread_mutex_unlock(&instanciasListMutex);

	/*if(!instanciaThatMightHaveValue){
		return NULL;
	}

	*instanciaValueResponse = valueStatus(instanciaThatMightHaveValue, key);*/
	return instanciaThatMightHaveValue;
}

int obtainPlanificadorConsoleResponse(char* key){
	/*char instanciaValueResponse;
	Instancia* instanciaThatMightHaveValue = lookForInstanciaAndGetValueStatus(key, &instanciaValueResponse);*/

	/*if(instanciaThatMightHaveValue){
		if(instanciaValueResponse == INSTANCIA_RESPONSE_HAS_VALUE){
			char* value = valueFromKey(instanciaThatMightHaveValue, key);
			//TODO devolver ese value
		}else if(instanciaValueResponse == INSTANCIA_RESPONSE_HAS_NOT_VALUE){
			//devolver que no tiene valor
		}
		//TODO devolver, en esos dos casos, la instancia
	}
	else{
		//TODO simular distribucion de la key
	}*/

	//TODO no olvidar liberar el valor una vez que se haya enviado al planificador
	return 0;
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

int instanciaIsAliveAndNextToActual(Instancia* instancia){
	if(instancia->isFallen){
		return 0;
	}

	if(positionInList(lastInstanciaChosen) == 0 && firstAlreadyPass == 0){
		firstAlreadyPass = 1;
		return 1;
	}

	return positionInList(instancia) >= positionInList(lastInstanciaChosen) + 1;
}

Instancia* getNextInstancia(){
	Instancia* instancia = list_find(instancias, (void*) &instanciaIsAliveAndNextToActual);
	if(!instancia && firstAlreadyPass){

		Instancia* auxLastInstanciaChosen = lastInstanciaChosen;
		lastInstanciaChosen = list_get(instancias, 0);
		firstAlreadyPass = 0;

		instancia = list_find(instancias, (void*) &instanciaIsAliveAndNextToActual);

		if(!instancia){
			lastInstanciaChosen = auxLastInstanciaChosen;
		}
	}
	return instancia;
}

Instancia* equitativeLoad(char* key){
	pthread_mutex_lock(&lastInstanciaChosenMutex);
	Instancia* chosenInstancia = getNextInstancia();
	if(chosenInstancia){
		lastInstanciaChosen = chosenInstancia;
	}
	pthread_mutex_unlock(&lastInstanciaChosenMutex);
	return chosenInstancia;
}

//TODO testear (y que no afecte al posta)
Instancia* equitativeLoadSimulation(char* key){
	pthread_mutex_lock(&lastInstanciaChosenMutex);
	Instancia* auxLastInstanciaChosen2 = lastInstanciaChosen;
	int firstAlreadyPassAux = firstAlreadyPass;
	Instancia* instancia = getNextInstancia();
	lastInstanciaChosen = auxLastInstanciaChosen2;
	pthread_mutex_unlock(&lastInstanciaChosenMutex);
	firstAlreadyPass = firstAlreadyPassAux;
	return instancia;
}

Instancia* leastSpaceUsed(char* key){
	//TODO
	return NULL;
}

Instancia* keyExplicit(char* key){
	//TODO
	return NULL;
}

Instancia* leastSpaceUsedSimulation(char* key){
	//TODO
	return NULL;
}

Instancia* keyExplicitSimulation(char* key){
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

Instancia* chooseInstancia(char* key){
	Instancia* chosenInstancia = NULL;

	//ojo si se quiere usar alguna funcion que use este semaforo en algun algoritmo de distribucion
	pthread_mutex_lock(&instanciasListMutex);
	if(list_size(instancias) != 0){
		chosenInstancia = (*distributionAlgorithm)(key);
	}
	pthread_mutex_unlock(&instanciasListMutex);
	return chosenInstancia;
}

Instancia* simulateChooseInstancia(char* key){
	Instancia* chosenInstancia = NULL;
		pthread_mutex_lock(&instanciasListMutex);
		if(list_size(instancias) != 0){
			chosenInstancia = (*distributionAlgorithmSimulation)(key);
		}
		pthread_mutex_unlock(&instanciasListMutex);
		return chosenInstancia;
}

int sendResponseToEsi(EsiRequest* esiRequest, char response){
	//TODO aca tambien hay que reintentar hasta que se mande todo?
	//TODO que pasa cuando se pasa una constante por parametro? vimos que hubo drama con eso

	if(send(esiRequest->socket, &response, sizeof(response), 0) < 0){
		log_warning(operationsLogger, "ESI %d perdio conexion con el coordinador al intentar hacer %s", esiRequest->id,
				getOperationName(esiRequest->operation));
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

//TODO cuidado en estos casos que el stringToLog no se limpia y es llamado mas de una vez
int tryToExecuteOperationOnInstancia(EsiRequest* esiRequest, Instancia* chosenInstancia){
	log_info(logger, "The next instancia is going to be used for %s", getOperationName(esiRequest->operation));
	showInstancia(chosenInstancia);

	actualEsiRequest = esiRequest;
	sem_post(chosenInstancia->semaphore);
	log_info(logger, "Esi's thread advised instancia's thread, waiting to continue...");
	sem_wait(instanciaResponse);
	log_info(logger, "Esi's thread is gonna process instancia's thread response");

	//TODO mariano fijate que al hacer este sleep, si matas (rapido) al esi, se hace el send igual
	//sleep(10);
	if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN){
		log_warning(operationsLogger, "Esi %d cannot do %s over %s. Instancia %s fell. Inaccessible key", actualEsiRequest->id, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key, chosenInstancia->name);
		sendResponseToEsi(actualEsiRequest, ABORT);
		return -1;
	}

	log_info(operationsLogger, "Esi %d does %s over key %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

	return 0;
}

Instancia* lookOrRemoveKeyIfInFallenInstancia(EsiRequest* esiRequest){
	pthread_mutex_lock(&instanciasListMutex);
	Instancia* instanciaToBeUsed = lookForKey(esiRequest->operation->key);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		log_warning(operationsLogger, "Esi %d tries %s over key %s. Inaccessible key",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		removeKeyFromFallenInstancia(esiRequest->operation->key, instanciaToBeUsed);
		sendResponseToEsi(esiRequest, ABORT);
	}
	pthread_mutex_unlock(&instanciasListMutex);
	return instanciaToBeUsed;
}

int doSet(EsiRequest* esiRequest, char keyStatus){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest) < 0){
		return -1;
	}

	int keyExists = 0;

	log_info(logger, "Gonna look for instancia to set key %s:", esiRequest->operation->key);
	Instancia* instanciaToBeUsed = lookOrRemoveKeyIfInFallenInstancia(esiRequest);
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

	sendResponseToEsi(esiRequest, SUCCESS);

	return 0;
}

int doStore(EsiRequest* esiRequest, char keyStatus){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest) < 0){
		return -1;
	}

	Instancia* instanciaToBeUsed = lookOrRemoveKeyIfInFallenInstancia(esiRequest);
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

	sendResponseToEsi(esiRequest, FREE);

	return 0;
}

int doGet(EsiRequest* esiRequest, char keyStatus){
	if(keyStatus == BLOCKED){
		log_info(operationsLogger, "Esi %d tries GET over key %s. Blocked key", esiRequest->id, esiRequest->operation->key);
		return sendResponseToEsi(esiRequest, BLOCK);
	}

	//TODO testear cuando la instancia se caiga y el coordinador se entere por ella (y no porque un esi quiere acceder, sino se borra la clave!)
	Instancia* instanciaWithKey = lookOrRemoveKeyIfInFallenInstancia(esiRequest);
	if(instanciaWithKey != NULL && instanciaWithKey->isFallen){
		return -1;
	}

	if(keyStatus == LOCKED){
		log_info(operationsLogger, "Esi %d does GET over key %s, which he already had", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, SUCCESS) < 0){
			return -1;
		}
	}else{
		log_info(operationsLogger, "Esi %d does GET over key %s", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, LOCK) < 0){
			return -1;
		}
	}
	return 0;
}

char checkKeyStatusFromPlanificador(int esiId, char* key){
	char response = 0;

	log_info(logger, "Gonna recieve %s's status from planificador", key);

	//TODO probar esto
	char message = KEYSTATUSMESSAGE;

	void* package = NULL;
	int offset = 0;
	int sizeString = strlen(key)+1;

	addToPackageGeneric(&package, &message, sizeof(char), &offset);
	addToPackageGeneric(&package, &sizeString, sizeof(sizeString), &offset);	
	addToPackageGeneric(&package, key, sizeString, &offset);

	if(send_all(planificadorSocket, package, offset) == CUSTOM_FAILURE){
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		exit(-1);
	}

	free(package);

	log_info(logger, "Sent key to planificador to check its status");

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

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	log_info(logger, "Waiting for esis to arrive");

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;

	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		//TODO revisar que se estaria usando el esiRequest anterior, y esto podria fallar
		//log_info(logger, "Couldn't receive operation from esi %d. Finished or fell, so his thread dies", esiRequest.id);
		destroyOperation(esiRequest.operation);
		return -1;
	}
	//recieveOperationDummy(esiRequest.operation);

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
		log_error(logger, "Couldn't receive esi key status from planificador");
		destroyOperation(esiRequest.operation);
		sendResponseToEsi(&esiRequest, ABORT);
		pthread_mutex_unlock(&esisMutex);
		return -1;
	}

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
			if(doStore(&esiRequest, keyStatus)){
				operationResult = -1;
			}
			break;
		default:
			log_warning(operationsLogger, "Esi %d sent an invalid operation", esiRequest.id);
			sendResponseToEsi(&esiRequest, ABORT);
			operationResult = -1;
			break;
	}

	pthread_mutex_unlock(&esisMutex);

	destroyOperation(esiRequest.operation);
	usleep(delay * 1000);
	return operationResult == 0 ? 1 : -1;
}

Instancia* initialiceArrivedInstancia(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket, logger) < 0){
		return NULL;
	}
	/*recieveInstanciaNameDummy(&arrivedInstanciaName);*/
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
			//TODO que deberia pasar aca? mientras dejo este exit. tener cuidado con los semaforos que el de abajo no se libera si se cambia exit por return
			exit(-1);
			//si hay que matar al hilo, devolver NULL !!!
		}

		if(list_size(instancias) == 1){
			pthread_mutex_lock(&lastInstanciaChosenMutex);
			lastInstanciaChosen = list_get(instancias, 0);
			pthread_mutex_unlock(&lastInstanciaChosenMutex);
		}

		log_info(logger, "Instancia %s is new", arrivedInstanciaName);
	}
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
	Instancia* instancia = createNewInstancia(instanciaSocket, arrivedInstanciaName);

	if(list_size(instancias) == 1){
		pthread_mutex_lock(&lastInstanciaChosenMutex);
		lastInstanciaChosen = list_get(instancias, 0);
		pthread_mutex_unlock(&lastInstanciaChosenMutex);
	}

	pthread_mutex_unlock(&instanciasListMutex);

	return instancia;
}

int handleInstancia(int instanciaSocket){
	//TODO hay que meter un semaforo para evitar conflictos de los diferentes hilos cuando puede haber varios activos (compactacion)

	Instancia* actualInstancia = initialiceArrivedInstancia(instanciaSocket);
	//Instancia* actualInstancia = initialiceArrivedInstanciaDummy(instanciaSocket);

	if(!actualInstancia){
		return -1;
	}

	//TODO no pude pasar el semaforo a coordinador.h para reutilizar en instancisaFunctions porque me tiraba error de definicion multiple
	pthread_mutex_lock(&instanciasListMutex);
	showInstancias();
	pthread_mutex_unlock(&instanciasListMutex);

	while(1){
		sem_wait(actualInstancia->semaphore);
		log_info(logger, "%s's thread is gonna handle %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));

		//TODO mariano ojo que la instancia esta caida pero esto que usa sendOperation esta pudiendo enviar...
		instanciaDoOperation(actualInstancia, actualEsiRequest->operation, logger);
		//instanciaDoOperationDummy(actualInstancia, actualEsiRequest->operation, logger);

		//TODO mariano, similar al todo de arriba. cuando la instancia se cae, no se esta mostrando este log y no muestra seg fault ni nada
		log_info(logger, "Instancia's response is gonna be processed");

		if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN){
			log_error(logger, "Instancia %s couldn't do %s. His thread dies, and key %s is deleted:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
			pthread_mutex_lock(&instanciasListMutex);
			removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
			instanciaHasFallen(actualInstancia);
			showInstancia(actualInstancia);
			pthread_mutex_unlock(&instanciasListMutex);

			sem_post(instanciaResponse);

			return -1;
		}

		log_info(logger, "%s could do %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));

		sem_post(instanciaResponse);
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

void pthreadInitialize(int* clientSocket){
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

int clientHandler(int clientSocket){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, (void*) &pthreadInitialize, clientSocketPointer)){
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
	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		clientHandler(clientSocket);
	}

	return 0;
}

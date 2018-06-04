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
t_list* instancias;
int cantEntry;
int entrySize;
int delay;
Instancia* lastInstanciaChosen;
int firstAlreadyPass = 0;

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
		exit(-1);
	}

	free(algorithm);

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
	Instancia* instanciaThatMightHaveValue = lookForKey(key, instancias);

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
	Instancia* chosenInstancia = getNextInstancia();
	if(chosenInstancia){
		lastInstanciaChosen = chosenInstancia;
	}
	return chosenInstancia;
}

//TODO testear (y que no afecte al posta)
Instancia* equitativeLoadSimulation(char* key){
	Instancia* auxLastInstanciaChosen2 = lastInstanciaChosen;
	int firstAlreadyPassAux = firstAlreadyPass;
	Instancia* instancia = getNextInstancia();
	lastInstanciaChosen = auxLastInstanciaChosen2;
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
	if(list_size(instancias) != 0){
		return (*distributionAlgorithm)(key);
	}
	return NULL;
}

Instancia* simulateChooseInstancia(char* key){
	if(list_size(instancias) != 0){
		return (*distributionAlgorithmSimulation)(key);
	}
	return NULL;
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

//TODO deprecated. sacar
void logOperation(char* stringToLog){
	log_info(operationsLogger, stringToLog);
}

int keyIsOwnedByActualEsi(char keyStatus, EsiRequest* esiRequest){
	if(keyStatus != LOCKED){
		log_warning(operationsLogger, "ESI %d no puede hacer %s sobre la clave %s. Clave no bloqueada por el",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		sendResponseToEsi(esiRequest, ABORT);
		return -1;
	}

	return 0;
}

//TODO cuidado en estos casos que el stringToLog no se limpia y es llamado mas de una vez
int tryToExecuteOperationOnInstancia(EsiRequest* esiRequest, Instancia* chosenInstancia){
	log_info(logger, "Se usara la siguiente instancia para hacer %s", getOperationName(esiRequest->operation));
	showInstancia(chosenInstancia);

	actualEsiRequest = esiRequest;
	sem_post(chosenInstancia->semaphore);
	log_info(logger, "Hilo del esi aviso al de instancia, esperando para continuar...");
	sem_wait(instanciaResponse);
	log_info(logger, "El hilo del esi va a procesar la respuesta de la instancia");

	//TODO mariano fijate que al hacer este sleep, si matas (rapido) al esi, se hace el send igual
	//sleep(10);
	if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN){
		log_warning(operationsLogger, "ESI %d no puede hacer %s sobre %s. Instancia %s se cayo. Clave inaccesible", actualEsiRequest->id, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key, chosenInstancia->name);
		sendResponseToEsi(actualEsiRequest, ABORT);
		return -1;
	}

	log_info(operationsLogger, "ESI %d hizo %s sobre la clave %s", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);

	return sendResponseToEsi(esiRequest, SUCCESS);
}

Instancia* lookOrRemoveKeyIfInFallenInstancia(EsiRequest* esiRequest){
	Instancia* instanciaToBeUsed = lookForKey(esiRequest->operation->key, instancias);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		log_warning(operationsLogger, "ESI %d intenta hacer %s sobre la clave %s. Clave inaccesible",
				esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
		removeKeyFromFallenInstancia(esiRequest->operation->key, instanciaToBeUsed);
		sendResponseToEsi(esiRequest, ABORT);
	}
	return instanciaToBeUsed;
}

int doSet(EsiRequest* esiRequest, char keyStatus){
	if(keyIsOwnedByActualEsi(keyStatus, esiRequest) < 0){
		return -1;
	}

	int keyExists = 0;

	log_info(logger, "Se va a buscar la instancia para settear la clave %s:", esiRequest->operation->key);
	Instancia* instanciaToBeUsed = lookOrRemoveKeyIfInFallenInstancia(esiRequest);
	showInstancia(instanciaToBeUsed);
	if(instanciaToBeUsed != NULL && instanciaToBeUsed->isFallen){
		return -1;
	}
	else{
		if(instanciaToBeUsed == NULL){
			instanciaToBeUsed = chooseInstancia(esiRequest->operation->key);
			if(instanciaToBeUsed == NULL){
				log_warning(operationsLogger, "ESI %d es abortado al intentar SET por no existir la clave en ninguna instancia y no haber instancias disponibles", esiRequest->id);
				sendResponseToEsi(esiRequest, ABORT);
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

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed) < 0){
		return -1;
	}

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
			log_warning(operationsLogger, "ESI %d intenta hacer %s sobre la clave %s. Clave no identificada en instancias", esiRequest->id, getOperationName(esiRequest->operation), esiRequest->operation->key);
			sendResponseToEsi(esiRequest, ABORT);
			return -1;
		}
	}

	if(tryToExecuteOperationOnInstancia(esiRequest, instanciaToBeUsed) < 0){
		return -1;
	}

	return 0;
}

int doGet(EsiRequest* esiRequest, char keyStatus){
	if(keyStatus == BLOCKED){
		log_info(operationsLogger, "ESI %d intenta hacer GET sobre la clave %s. Clave bloqueada", esiRequest->id, esiRequest->operation->key);
		//todo ojo en estos casos que no se esta abortando al esi porque se supone que el murio.
		//esto es: hay que validar que el planificador se entere bien de esto para no cagarla
		return sendResponseToEsi(esiRequest, BLOCK);
	}

	//TODO testear cuando la instancia se caiga y el coordinador se entere por ella (y no porque un esi quiere acceder, sino se borra la clave!)
	Instancia* instanciaWithKey = lookOrRemoveKeyIfInFallenInstancia(esiRequest);
	if(instanciaWithKey != NULL && instanciaWithKey->isFallen){
		return -1;
	}

	if(keyStatus == LOCKED){
		log_info(operationsLogger, "ESI %d hace GET sobre la clave %s, la cual ya tenia", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, SUCCESS) < 0){
			return -1;
		}
	}else{
		log_info(operationsLogger, "ESI %d hizo GET sobre la clave %s", esiRequest->id, esiRequest->operation->key);
		if(sendResponseToEsi(esiRequest, LOCK) < 0){
			return -1;
		}
	}
	return 0;
}

char checkKeyStatusFromPlanificador(int esiId, char* key){
	char response = 0;

	log_info(logger, "Voy a recibir el estado de la clave %s del planificador", key);

	//TODO probar esto
	char message = KEYSTATUSMESSAGE;

	void* package = NULL;
	int offset = 0;

	addToPackageGeneric(&package, &message, sizeof(char), &offset);
	addToPackageGeneric(&package, key, strlen(key)+1, &offset);

	if(send_all(planificadorSocket, package, offset) == CUSTOM_FAILURE){
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
	/*operation->key = malloc(31);
	strcpy(operation->key, "lio:messi");
	char* buffer = malloc(3);
	sprintf(buffer, "%d", lastInstanciaChosen);
	strcat(operation->key, buffer);*/
	operation->key = "lio:messi";
	//operation->key = "cristiano:ronaldo";
	operation->value = "elMasCapo";
	operation->operationCode = OURSET;
}

int recieveStentenceToProcess(int esiSocket){
	int operationResult = 0;
	int esiId = 0;
	log_info(logger, "Esperando que llegue un esi");

	EsiRequest esiRequest;
	esiRequest.socket = esiSocket;
	esiRequest.operation = malloc(sizeof(Operation));

	if(recieveOperation(&esiRequest.operation, esiSocket) == CUSTOM_FAILURE){
		log_error(logger, "Couldn't receive esi's operation");
		destroyOperation(esiRequest.operation);
		return -1;
	}
	//recieveOperationDummy(esiRequest.operation);

	log_info(logger, "El esi que llego va a hacer:");
	showOperation(esiRequest.operation);

	esiId = getActualEsiID();
	//esiId = getActualEsiIDDummy();
	esiRequest.id = esiId;
	log_info(logger, "Llego el esi con id = %d", esiId);

	char keyStatus;
	keyStatus = checkKeyStatusFromPlanificador(esiRequest.id, esiRequest.operation->key);
	//keyStatus = checkKeyStatusFromPlanificadorDummy();

	log_info(logger, "El estado de la clave %s del esi %d es %s", esiRequest.operation->key, esiRequest.id, getKeyStatusName(keyStatus));

	if(strcmp(getKeyStatusName(keyStatus), "UNKNOWN KEY STATUS") == 0){
		log_error(logger, "Couldn't recieve esi key status from planificador");
		destroyOperation(esiRequest.operation);
		sendResponseToEsi(&esiRequest, ABORT);
		return -1;
	}

	switch (esiRequest.operation->operationCode){
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
		case OURGET:
			if (doGet(&esiRequest, keyStatus) < 0){
				operationResult = -1;
			}
			break;
		default:
			log_warning(operationsLogger, "El ESI %d envio una operacion invalida", esiRequest.id);
			sendResponseToEsi(&esiRequest, ABORT);
			operationResult = -1;
			break;
	}

	destroyOperation(esiRequest.operation);
	sleep(delay);
	return operationResult == 0 ? 1 : -1;
}

Instancia* initialiceArrivedInstancia(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket, logger) < 0){
		return NULL;
	}
	/*recieveInstanciaNameDummy(&arrivedInstanciaName);*/
	log_info(logger, "Llego instancia: %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize, logger) < 0){
		free(arrivedInstanciaName);
		return NULL;
	}
	log_info(logger, "Se envio la configuracion a la instancia %s", arrivedInstanciaName);

	Instancia* arrivedInstancia = existsInstanciaWithName(arrivedInstanciaName, instancias);
	if(arrivedInstancia){
		instanciaIsBack(arrivedInstancia, instanciaSocket);
		log_info(logger, "La instancia %s esta reviviendo", arrivedInstanciaName);
	}else{
		arrivedInstancia = createNewInstancia(instanciaSocket, instancias, arrivedInstanciaName);

		if(!arrivedInstancia){
			log_error(logger, "Couldn't initalize instancia semaphre");
			//TODO que deberia pasar aca? mientras dejo este exit
			exit(-1);
			//si hay que matar al hilo, devolver NULL !!!
		}

		if(list_size(instancias) == 1){
			lastInstanciaChosen = list_get(instancias, 0);
		}

		log_info(logger, "La instancia %s es nueva", arrivedInstanciaName);
	}

	return arrivedInstancia;
}

Instancia* initialiceArrivedInstanciaDummy(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket, logger) < 0){
		return NULL;
	}
	//recieveInstanciaNameDummy(&arrivedInstanciaName);
	log_info(logger, "Llego instancia: %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize, logger) < 0){
		free(arrivedInstanciaName);
		return NULL;
	}
	log_info(logger, "Se envio la configuracion a la instancia %s", arrivedInstanciaName);

	Instancia* instancia = createNewInstancia(instanciaSocket, instancias, arrivedInstanciaName);

	if(list_size(instancias) == 1){
		lastInstanciaChosen = list_get(instancias, 0);
	}

	return instancia;
}

int handleInstancia(int instanciaSocket){
	//TODO hay que meter un semaforo para evitar conflictos de los diferentes hilos cuando puede haber varios activos (compactacion)

	Instancia* actualInstancia = initialiceArrivedInstancia(instanciaSocket);
	//Instancia* actualInstancia = initialiceArrivedInstanciaDummy(instanciaSocket);

	if(!actualInstancia){
		return -1;
	}
	showInstancias(instancias);

	while(1){
		sem_wait(actualInstancia->semaphore);
		log_info(logger, "Hilo de %s va a encargarse de hacer %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));

		//TODO mariano ojo que la instancia esta caida pero esto que usa sendOperation esta pudiendo enviar...
		instanciaDoOperation(actualInstancia, actualEsiRequest->operation, logger);
		//instanciaDoOperationDummy(actualInstancia, actualEsiRequest->operation, logger);

		//TODO mariano, similar al todo de arriba. cuando la instancia se cae, no se esta mostrando este log y no muestra seg fault ni nada
		log_info(logger, "Se va a procesar la respuesta de la instancia");

		if(instanciaResponseStatus == INSTANCIA_RESPONSE_FALLEN){
			log_error(logger, "La instancia %s no pudo hacer %s, su hilo muere, se le elimina la clave %s:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
			removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
			instanciaHasFallen(actualInstancia);
			showInstancia(actualInstancia);

			sem_post(instanciaResponse);

			return -1;
		}

		log_info(logger, "%s pudo hacer %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));

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

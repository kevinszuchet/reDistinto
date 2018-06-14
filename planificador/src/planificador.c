/*
 * planificador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"

t_log* logger;

fd_set master;

int pauseState = CONTINUE; // 1 is running, 0 is paussed

t_dictionary* blockedEsiDic;
t_list* readyEsis;
t_list* finishedEsis;
Esi* runningEsi;

t_list* allSystemTakenKeys;
t_list* allSystemEsis;

int listeningPort;
char* algorithm;
int alphaEstimation;
int initialEstimation;
char* ipCoordinador;
int portCoordinador;
char** blockedKeys;

pthread_t threadConsole;
pthread_t threadExecution;
pthread_t threadConsoleInstructions;

pthread_mutex_t mutexReadyList = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexEsiReady = PTHREAD_MUTEX_INITIALIZER;

int sentenceCounter = 0;

int actualID = 1; // ID number for ESIs, when a new one is created, this number increases by 1

int coordinadorSocket;

char* keyRecieved;
OperationResponse* esiInformation = NULL;

bool finishedExecutingInstruccion = true;

int main(void) {
	logger = log_create("../planificador.log", "tpSO", true, LOG_LEVEL_INFO);
	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	int welcomeCoordinadorResult = welcomeServer(ipCoordinador, portCoordinador, COORDINADOR, PLANIFICADOR, COORDINADORID, &welcomeNewClients, logger);
	if (welcomeCoordinadorResult < 0) {
		log_error(logger, "Couldn't handhsake with coordinador, quitting...");
		exitPlanificador();
	}

	return 0;
}

// General execute ESI functions
Esi* getNextEsi() {
	Esi* nextEsi;
	pthread_mutex_lock(&mutexReadyList);
	nextEsi = nextEsiByAlgorithm(algorithm, alphaEstimation, readyEsis);
	pthread_mutex_unlock(&mutexReadyList);

	return nextEsi;
}

bool mustDislodgeRunningEsi() {
	if (list_size(readyEsis) > 0) {
		Esi* bestPosible = simulateAlgoithm(algorithm, alphaEstimation, readyEsis);
		printEsi(runningEsi);
		if (getEstimation(bestPosible) < getEstimation(runningEsi)) {
			log_info(logger, "Must dislodge, ESI (%d) has estimation (%f). Lower than (%f) from running ESI", bestPosible->id, getEstimation(bestPosible), getEstimation(runningEsi));
			return true;
		}
	}
	return false;
}

void finishEsi(Esi* esiToFinish) {
	list_add(finishedEsis,esiToFinish);
	freeTakenKeys(esiToFinish);
	log_info(logger, "Esi (%d) succesfully finished", esiToFinish->id);
	log_info(logger, "Printing Esi (%d) final values", esiToFinish->id);
	takeRunningEsiOut();
	printEsi(esiToFinish);
}

void freeTakenKeys(Esi* esi) {
	for (int i = 0; i < list_size(esi->lockedKeys); i++) {
		char* keyToFree = (char*) list_get(esi->lockedKeys, i);
		freeKey(keyToFree, esi);
	}

	list_clean(esi->lockedKeys);
}

void addToFinishedList(Esi* finishedEsi) {
	list_add(finishedEsis, finishedEsi);
	log_info(logger, "Esi (%d) agregado a finalizados", finishedEsi->id);
}

void removeFromReady(Esi* esi) {

	pthread_mutex_lock(&mutexReadyList);
	Esi* esiFromReady;
	int idToRemove = -1;

	for (int i = 0; i < list_size(readyEsis); i++) {
		esiFromReady = list_get(readyEsis, i);
		if (esiFromReady->id == esi->id) {
			idToRemove = i;
		}
	}

	list_remove(readyEsis, idToRemove);
	pthread_mutex_unlock(&mutexReadyList);
}

void sendEsiIdToCoordinador(int id) {
	if (send(coordinadorSocket, &id, sizeof(int), 0) < 0) {
	   log_error(logger, "Coultn't send message to Coordinador about ESI id");
	   exitPlanificador();
	} else {
		log_info(logger, "Send esi ID = %d to coordinador",id);
	}
}

void dislodgeEsi(Esi* esi, bool addToReady) {
	if (addToReady) {
		addEsiToReady(esi);
	}
	takeRunningEsiOut();
}

void takeRunningEsiOut() {
	updateLastBurst(sentenceCounter, &runningEsi);
	runningEsi->lastEstimation = getEstimation(runningEsi);
	runningEsi = NULL;
	sentenceCounter = 0;
}

void moveEsiToRunning(Esi* esiToRun) {
	esiToRun->waitingTime = 0;
	runningEsi = esiToRun;
	removeFromReady(esiToRun);
}

void handleEsiInformation(OperationResponse* esiExecutionInformation, char* key) {
	sentenceCounter++;
	addWaitingTimeToAll(readyEsis);

	switch(esiExecutionInformation->coordinadorResponse) {
		case SUCCESS:
			log_info(logger, "Operation succeded, nothing to do");
			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case LOCK:
			lockKey(key, runningEsi->id);
			log_info(logger, "Key (%s) locked", key);
			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case BLOCK:
			log_info(logger, "Operation didn't succed, esi (%d) blocked in key (%s)", runningEsi->id, key);
			blockEsi(key, runningEsi->id);
			dislodgeEsi(runningEsi, false);
		break;

		case FREE:
			freeKey(key, runningEsi);
			log_info(logger, "Operation succeded, key (%s) freed", key);
			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case ABORT:
			sentenceCounter = 0;
			runningEsi = NULL;
		break;
	}

	esiInformation = NULL;
}

void handleEsiStatus(char esiStatus) {
	switch(esiStatus) {
		case FINISHED:
			finishEsi(runningEsi);
			log_info(logger, "Esi finished execution");
		break;

		case NOTFINISHED:
			log_info(logger, "Esi didn't finish execution");
		break;
	}
}

void abortEsi(Esi* esi) {

	bool isEsiById(void* element) {
		return ((Esi*) element)->id == esi->id;
	}

	freeTakenKeys(esi);
	deleteEsiFromSystemBySocket(esi->socketConection);
	list_remove_by_condition(allSystemEsis, &isEsiById);
}

void deleteEsiFromSystemBySocket(int socket) {

	bool isEsiBySocket(void* esi) {
		return ((Esi*)esi)->socketConection == socket;
	}
	t_queue* blockedEsis;
	int* actualEsi;

	switch(getEsiPlaceBySocket(socket)) {

		case INREADYLIST:
			log_info(logger, "Aborting esi (%d)", getEsiBySocket(socket)->id);
			pthread_mutex_lock(&mutexReadyList);
			list_remove_by_condition(readyEsis, &isEsiBySocket);
			pthread_mutex_unlock(&mutexReadyList);
		break;

		case INFINISHEDLIST:
			// REVIEW no hace nada?
		break;

		case INRUNNING:
			log_info(logger, "Aborting esi (%d)", getEsiBySocket(socket)->id);
			finishedExecutingInstruccion = true;
			runningEsi = NULL;
		break;

		case INBLOCKEDDIC:
			log_info(logger, "Aborting esi (%d)", getEsiBySocket(socket)->id);

			for (int i = 0; i < list_size(allSystemTakenKeys); i++) {
				blockedEsis = dictionary_get(blockedEsiDic, list_get(allSystemTakenKeys, i));

				for (int j = 0; j < queue_size(blockedEsis); j++) {
					actualEsi = (int*) queue_pop(blockedEsis);

					if (getEsiById(*actualEsi)->socketConection != socket) {
						queue_push(blockedEsis, actualEsi);
					} else {
						// REVIEW porque esta el free comentado?
						//free(actualEsi);
					}
				}
			}
		break;

		case NOWHERE:
			log_info(logger, "Aborting esi (%d)", getEsiBySocket(socket)->id);
		break;

		default:
			// REVIEW el default no hace nada?
			/*
			 * log_error(logger, "Couldn't remove ESI with socket (%d)",socket);
			 * exitPlanificador();
			*/
		break;
	}
}

Esi* getEsiBySocket(int socket) {
	bool isEsiBySocket(void* esi) {
		return ((Esi*)esi)->socketConection == socket;
	}
	t_queue* blockedEsis;
	int* actualEsi;
	Esi* targetEsi = NULL;
	t_list* filteredList = list_create();
	switch(getEsiPlaceBySocket(socket)) {

		case INREADYLIST:
			log_info(logger, "ESI was on ready list");
			return list_get(list_filter(readyEsis, &isEsiBySocket), 0);
		break;

		case INFINISHEDLIST:
			log_info(logger, "ESI was on finished list, it succesfully finished executing");
			filteredList = list_filter(finishedEsis, &isEsiBySocket);
			return list_get(filteredList, list_size(filteredList) - 1);
		break;

		case INRUNNING:
			log_info(logger, "ESI was running");
			return runningEsi;
		break;

		case INBLOCKEDDIC:
			log_info(logger, "ESI was blocked");
			for (int i = 0; i < list_size(allSystemTakenKeys); i++) {
				blockedEsis = dictionary_get(blockedEsiDic, list_get(allSystemTakenKeys, i));
				for (int j = 0; j < queue_size(blockedEsis); j++) {
					actualEsi = (int*) queue_pop(blockedEsis);
					if (getEsiById(*actualEsi)->socketConection == socket) {
						targetEsi = getEsiById(*actualEsi);
					}
					queue_push(blockedEsis, actualEsi);
				}
			}
			// REVIEW que hacemos con este free?
			//free(blockedEsis);
			return targetEsi;
		break;

		default:
			log_info(logger, "ESI was nowhere");
			return runningEsi;
		break;
	}
}

char getEsiPlaceBySocket(int socket) {
	bool isEsiBySocket(void* esi) {
		return ((Esi*)esi)->socketConection == socket;
	}

	if (list_size(list_filter(readyEsis, &isEsiBySocket)) > 0) {
		return INREADYLIST;
	}

	if (runningEsi != NULL && runningEsi->socketConection == socket) {
		return INRUNNING;
	}

	t_queue* blockedEsis;
	int* actualEsi;

	for (int i = 0; i < list_size(allSystemTakenKeys); i++) {
		blockedEsis = dictionary_get(blockedEsiDic, list_get(allSystemTakenKeys, i));
		for (int j = 0; j < queue_size(blockedEsis); j++) {
			actualEsi = (int*) queue_pop(blockedEsis);
			if (getEsiById(*actualEsi)->socketConection == socket) {
				return INBLOCKEDDIC;
			}
			queue_push(blockedEsis, actualEsi);
		}
	}

	if (list_size(list_filter(finishedEsis, &isEsiBySocket)) > 0) {
		return INFINISHEDLIST;
	}

	return NOWHERE;
}

void moveFromRunningToReady(Esi* esi) {
	addEsiToReady(runningEsi);
	runningEsi = NULL;
}

OperationResponse *recieveEsiInformation(int esiSocket) {
	OperationResponse* finishInformation = malloc(sizeof(OperationResponse));
	int resultRecv = recv(esiSocket, finishInformation, sizeof(OperationResponse), 0);
	if (resultRecv <= 0) {
		log_error(logger, "recv failed on %s, while waiting ESI message %s", ESI, strerror(errno));
		exitPlanificador();
		exit(-1);
	} else {
		return finishInformation;
	}
}

void sendKeyStatusToCoordinador(char* key) {
	bool keyCompare(void* takenKey) {
		return strcmp((char*) takenKey, key) == 0;
	}
	char keyStatus = isLockedKey(key);

	if (keyStatus == BLOCKED) {
		if (list_size(list_filter(runningEsi->lockedKeys, &keyCompare)) > 0) {
			keyStatus = LOCKED;
		}
	}

	if (send(coordinadorSocket, &keyStatus, sizeof(char), 0) < 0) {
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	} else {
		log_info(logger, "Send key status (%s) to coordinador", getKeyStatusName(keyStatus));
	}
}

void sendMessageExecuteToEsi(Esi* nextEsi) {
	int socketEsi = nextEsi->socketConection;
	int message = RUN;
    if (sendInt(message,socketEsi) == CUSTOM_FAILURE) {
	   log_error(logger, "Coultn't send message to ESI %d", nextEsi->id);
    } else {
    	log_info(logger, "Send execute message to ESI %d in socket %d", nextEsi->id, nextEsi->socketConection);
    }
}

// General use functions
void addKeyToGeneralKeys(char* key) {
	bool itemIsKey(void* item) {
		return strcmp(key, (char*) item) == 0;
	}

	if (!list_any_satisfy(allSystemTakenKeys, &itemIsKey))
		list_add(allSystemTakenKeys, key);
}

void blockEsi(char* lockedKey, int esiBlocked) {
	t_queue* esiQueue;
	int* esiBlockedCopy = malloc(sizeof(int));
	if (!dictionary_has_key(blockedEsiDic, lockedKey)) {
		log_warning(logger, "Trying to block an ESI in a key that is not already in the dictionary");
	} else {
		esiQueue = dictionary_get(blockedEsiDic, lockedKey);
		*esiBlockedCopy = esiBlocked;
		queue_push(esiQueue, esiBlockedCopy);

		log_info(logger, "Added ESI (%d) to blocked dictionary in existing key (%s)", *esiBlockedCopy, lockedKey);
	}

	if (runningEsi->id != esiBlocked) {
		removeFromReady(getEsiById(esiBlocked));
	}
}

void lockKey(char* key, int esiID) {

	addKeyToGeneralKeys(key);

	if (esiID != CONSOLE_BLOCKED) {
		addLockedKeyToEsi(&key, &runningEsi);
	}

	if (!dictionary_has_key(blockedEsiDic, key)) {
		t_queue* esiQueue = queue_create();
		dictionary_put(blockedEsiDic, key, esiQueue);
	}

	if (isLockedKey(key) == NOTBLOCKED) {
		list_add(allSystemTakenKeys, key);
	}
}

Esi* getEsiById(int id) {
	bool isId(void* element) {
		return (((Esi*) element)->id == id ? 1 : 0);
	}
	Esi* esi = list_get(list_filter(allSystemEsis, &isId), 0);
	return esi;
}

// REVIEW hace falta esta funcion?
void destroyer(void* element) {
	free(element);
}

void freeKey(char* key, Esi* esiTaker) {
	removeLockedKey(key, esiTaker);
	unlockEsi(key);
}

void unlockEsi(char* key) {
	bool keyCompare(void* takenKey) {
		return string_equals_ignore_case((char*) takenKey, key);
	}

	list_remove_by_condition(allSystemTakenKeys, &keyCompare);
	t_queue* blockedEsisQueue = dictionary_get(blockedEsiDic, key);
	int* unlockedEsi;

	if (!queue_is_empty(blockedEsisQueue)) {
		unlockedEsi = (int*) queue_pop(blockedEsisQueue);
		log_info(logger, "Unblocked ESI %d from key (%s)", *unlockedEsi, key);
		addEsiToReady(getEsiById(*unlockedEsi));
	} else {
		log_info(logger, "There are no ESIs to unlock from key (%s)", key);
	}
}

char isLockedKey(char* key) {
	bool itemIsKey(void* item) {
		return strcmp(key,(char*)item) == 0;
	}

	return (list_any_satisfy(allSystemTakenKeys, &itemIsKey) ? BLOCKED : NOTBLOCKED);
}

void addEsiToReady(Esi* esi) {
	pthread_mutex_lock(&mutexReadyList);
	list_add(readyEsis, (void*) esi);
	pthread_mutex_unlock(&mutexReadyList);
}

void exitPlanificador() {
	// REVIEW que paso aca? se puede borrar lo comentado?
	/*dictionary_destroy(blockedEsiDic);
	list_destroy(readyEsis);
	list_destroy(finishedEsis);
	list_destroy(allSystemTakenKeys);
	list_destroy(instruccionsByConsoleList);
	sem_destroy(&executionSemaphore);
	sem_destroy(&keyRecievedFromCoordinadorSemaphore);
	sem_destroy(&esiInformationRecievedSemaphore);
	sem_destroy(&readyEsisSemaphore);
	sem_destroy(&consoleInstructionSemaphore);
	log_destroy(logger);
	pthread_cancel(threadConsole);
	pthread_cancel(threadConsoleInstructions);
	pthread_cancel(threadExecution);*/
	exit(-1);
}

// Planificador setup functions
void addConfigurationLockedKeys(char** blockedKeys) {
	int i = 0;

	while(blockedKeys[i]) {
		lockKey(blockedKeys[i], CONSOLE_BLOCKED);
		addKeyToGeneralKeys(blockedKeys[i]);
		i++;
	}

	log_info(logger, "All the configuration keys where locked");
}

// New esi functions
Esi* generateEsiStruct(int esiSocket) {
	Esi* newEsi = createEsi(actualID, initialEstimation, esiSocket);
	actualID++;
	return newEsi;
}

void welcomeEsi(int clientSocket) {
	log_info(logger, "I received a new ESI");
	Esi* newEsi = generateEsiStruct(clientSocket);
	addEsiToReady(newEsi);
	list_add(allSystemEsis,newEsi);
}

int clientMessageHandler(char clientMessage, int clientSocket) {
	if (clientSocket == coordinadorSocket) {
		if (clientMessage == KEYSTATUSMESSAGE) {
			log_info(logger, "I recieved a key status message");

			if (recieveString(&keyRecieved,coordinadorSocket) == CUSTOM_FAILURE) {
				log_error(logger, "Couldn't recieve key to check from coordinador, quitting...");
				exitPlanificador();
			} else {
				log_info(logger, "Key received = %s", keyRecieved);
				sendKeyStatusToCoordinador(keyRecieved);
			}

		} else if (clientMessage == CORDINADORCONSOLERESPONSEMESSAGE) {
			log_info(logger, "I recieved a coordinador console response message");
			// TODO Para el status
		}
	} else {
		if (clientMessage == ESIID) {
			welcomeEsi(clientSocket);
		} else if (clientMessage == ESIINFORMATIONMESSAGE) {
			log_info(logger, "I recieved a esi information message");
			esiInformation = recieveEsiInformation(runningEsi->socketConection);
			log_info(logger, "Going to handle Esi execution info.CoordinadoResponse = (%s) ,esiStatus = (%s)", getCoordinadorResponseName(esiInformation->coordinadorResponse), getEsiInformationResponseName(esiInformation->esiStatus));
			handleEsiInformation(esiInformation, keyRecieved);
			log_info(logger, "Finish handling one instruction from ESI");
			executeConsoleInstruccions();
			finishedExecutingInstruccion = true;
		} else {
			log_info(logger, "I received a strange in socket %d", clientSocket);
			printf("Lo que me llego es %c", clientMessage);
			exitPlanificador();
		}
	}

	return 0;
}

int welcomeNewClients(int newCoordinadorSocket) {
	coordinadorSocket = newCoordinadorSocket;
	handleConcurrence();
	return 0;
}

int handleConcurrence() {
	initializePlanificador();

	fd_set readfds;
	int fdmax, i;
	char clientMessage = 0;
	int resultRecv = 0, serverSocket = 0, clientSocket = 0;

	// TODO revisar el hardcodeo
	serverSocket = openConnection(listeningPort, PLANIFICADOR, "UNKNOWN_CLIENT", logger);
	if (serverSocket < 0) {
		// no se pudo conectar!
		return -1;
	}

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&readfds);

	// add the listener to the master set
	FD_SET(serverSocket, &master);
	FD_SET(coordinadorSocket, &master);
	fdmax = serverSocket;

	while(1) {

		struct timeval tv;

		 tv.tv_sec = 0;
		 tv.tv_usec = 500000;

		readfds = master; // copy it

		if (select(fdmax+1, &readfds, NULL, NULL, &tv) == -1) {
			perror("select");
		}

		if (errno == EINTR) {
			continue;
		}

		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax + 1; i++) {
			if (i != fdmax+1) {
				if (FD_ISSET(i, &readfds)) { // we got one!!
					if (i == serverSocket) {

						clientSocket = acceptUnknownClient(serverSocket, PLANIFICADOR, logger);

						if (clientSocket == -1) {
							perror("accept");
						} else {
							FD_SET(clientSocket, &master); // add to master set
							if (clientSocket > fdmax) {    // keep track of the max
								fdmax = clientSocket;
							}
						}
					} else {
						clientSocket = i;
						// handle data from a client
						resultRecv = recv(clientSocket, &clientMessage, sizeof(char), 0);
						if (resultRecv <= 0) {
							if (clientSocket == coordinadorSocket) {
								log_error(logger, "Coordinador disconnected. Exit planificador");
								exitPlanificador();
							} else {
								log_warning(logger, "ESI disconnected.");
								sleep(1);
								abortEsi(getEsiBySocket(clientSocket));
							}
							close(clientSocket);
							FD_CLR(clientSocket, &master);
						} else {
							clientMessageHandler(clientMessage, clientSocket);
						}
					}
				}
			} else {

				Esi* nextEsi;
				if (pauseState == CONTINUE) {
					if (finishedExecutingInstruccion) {
						if (runningEsi == NULL) {
							if (list_size(readyEsis) > 0) {
								nextEsi = getNextEsi();
								moveEsiToRunning(nextEsi);
							}
						} else {
							if (strcmp(algorithm, "SJF-CD") == 0) {
								if (mustDislodgeRunningEsi()) {
									dislodgeEsi(runningEsi, true);
									nextEsi = getNextEsi();
									moveEsiToRunning(nextEsi);
								}
							}
						}

						if (runningEsi != NULL) {
							finishedExecutingInstruccion = false;
							log_info(logger, "Executing ESI (%d)", runningEsi->id);
							sendEsiIdToCoordinador(runningEsi->id);
							sendMessageExecuteToEsi(runningEsi);
							log_info(logger, "Waiting coordinador request");
						} else {
							executeConsoleInstruccions();
						}
					}

				} else {
					executeConsoleInstruccions();
				}
			}
		}
	}

	return 0;
}

void getConfig(int* listeningPort, char** algorithm,int* alphaEstimation, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys) {

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	*alphaEstimation = config_get_int_value(config, "ALPHA_ESTIMATION");
	*initialEstimation = config_get_int_value(config, "ESTIMATION");
	*ipCoordinador = strdup(config_get_string_value(config, "IP_COORDINADOR"));
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*blockedKeys = config_get_array_value(config, "BLOCKED_KEYS");

	config_destroy(config);
}

void initializePlanificador() {
	allSystemTakenKeys = list_create();
	blockedEsiDic = dictionary_create();
	addConfigurationLockedKeys(blockedKeys);

	allSystemEsis = list_create();
	readyEsis = list_create();
	finishedEsis = list_create();
	runningEsi = NULL;

	pthread_create(&threadConsole, NULL, (void *) openConsole, NULL);
}

/*
 * planificador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"

fd_set master;

char* keyRecieved;
OperationResponse* esiInformation = NULL;


int* actualEsi;


int main(int argc, char* argv[]) {
	logger = log_create("../planificador.log", "tpSO", true, LOG_LEVEL_INFO);
	initSerializationLogger(logger);

	if (argc != 2) {
		log_error(logger, "Planificador cannot execute: you must enter a configuration file");
		return -1;
	}

	CFG_FILE = strdup(argv[1]);
	getConfig(&listeningPort, &algorithm, &alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	printf("Listening port = %d\n", listeningPort);
	printf("Algorithm = %s\n", algorithm);
	printf("Alpha estimation = %d\n", alphaEstimation);
	printf("Estimation = %d\n", initialEstimation);
	printf("Ip coordinador= %s\n", ipCoordinador);
	printf("Port coordinador= %d\n", portCoordinador);

	sem_init(&executionSemaphore,1,1);

	int welcomeCoordinadorResult = welcomeServer(ipCoordinador, portCoordinador, COORDINADOR, PLANIFICADOR, COORDINADORID, &welcomeNewClients, logger);
	if (welcomeCoordinadorResult < 0) {
		log_error(logger, "Couldn't handhsake with coordinador, quitting...");
		exitPlanificador();
	}

	free(algorithm);
	free(ipCoordinador);

	int i = 0;
	while (blockedKeys[i])
		free(blockedKeys[i]);

	free(blockedKeys);

	exitPlanificador();

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
	pthread_mutex_lock(&mutexReadyList);
	if (list_size(readyEsis) > 0) {
		pthread_mutex_unlock(&mutexReadyList);
		Esi* bestPosible = simulateAlgoithm(algorithm, alphaEstimation, readyEsis);
		printEsi(runningEsi);
		printEsi(bestPosible);
		if (getEstimation(bestPosible) < (getEstimation(runningEsi) - runningEsi->sentenceCounter)) {
			log_info(logger, "Must dislodge, ESI (%d) has estimation (%f). Lower than (%f) from running ESI", bestPosible->id, getEstimation(bestPosible), (getEstimation(runningEsi)-runningEsi->sentenceCounter));
			return true;
		}
	}else{
		pthread_mutex_unlock(&mutexReadyList);
	}

	return false;
}

void finishEsi(Esi* esiToFinish) {
	pthread_mutex_lock(&mutexFinishedList);
	list_add(finishedEsis, esiToFinish);
	pthread_mutex_unlock(&mutexFinishedList);
	freeTakenKeys(esiToFinish);
	log_info(logger, "Esi (%d) succesfully finished", esiToFinish->id);
	log_info(logger, "Printing Esi (%d) final values", esiToFinish->id);
	printEsi(esiToFinish);
	takeRunningEsiOut(false);

}




void freeTakenKeys(Esi* esi) {

	void freeKeyGeneral(void* key){


		bool keyCompare(void* takenKey) {
			return string_equals_ignore_case((char*) takenKey, key);
		}
		log_warning(logger,"Antes de eliminar la clave %s",key); //todo borrar despues
		list_remove_by_condition(allSystemTakenKeys, &keyCompare);
		log_warning(logger,"A punto de eliminar la clave %s",key); //todo borrar despues
		freeKey(key,esi);
	}

	log_warning(logger,"Hay %d claves bloqueadas",list_size(esi->lockedKeys)); //todo borrar despues
	list_iterate(esi->lockedKeys,&freeKeyGeneral);
	//list_clean(esi->lockedKeys);
}

void addToFinishedList(Esi* finishedEsi) {
	pthread_mutex_lock(&mutexFinishedList);
	list_add(finishedEsis, finishedEsi);
	pthread_mutex_unlock(&mutexFinishedList);
	log_info(logger, "Esi (%d) added to finished list", finishedEsi->id);
}

void removeFromReady(Esi* esi) {
	bool isEsiByID(void* element) {
		return ((Esi*) element)->id == esi->id;
	}
	pthread_mutex_lock(&mutexReadyList);
	list_remove_by_condition(readyEsis, &isEsiByID);

	pthread_mutex_unlock(&mutexReadyList);
}

void sendEsiIdToCoordinador(int id){
	char op = PLANIFICADOR_ESI_ID_RESPONSE;
	send_all(coordinadorSocket, &op, sizeof(op));
	if (sendInt(id, coordinadorSocket) == CUSTOM_FAILURE){
	   log_error(logger, "Coultn't send message to Coordinador about ESI id");
	   exitPlanificador();
	} else {
		log_info(logger, "Send esi ID = %d to coordinador",id);
	}
}

void dislodgeEsi(Esi* esi, bool addToReady, bool updateEstimation) {
	if (addToReady) {
		addEsiToReady(esi);
	}
	takeRunningEsiOut(updateEstimation);
}

void takeRunningEsiOut(bool updateEstimation) {

	if(updateEstimation){
		updateLastBurst(&runningEsi);
		runningEsi->lastEstimation = getEstimation(runningEsi);
	}
	runningEsi = NULL;
}

void moveEsiToRunning(Esi* esiToRun) {
	esiToRun->waitingTime = 0;
	runningEsi = esiToRun;
	removeFromReady(esiToRun);
}

void handleEsiInformation(OperationResponse* esiExecutionInformation, char* key) {

	if(runningEsi!=NULL){
		addSentenceCounter(runningEsi);
	}
	pthread_mutex_lock(&mutexReadyList);
	addWaitingTimeToAll(readyEsis);
	pthread_mutex_unlock(&mutexReadyList);
	switch(esiExecutionInformation->coordinadorResponse) {
		case SUCCESS:

			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case LOCK:
			lockKey(key, runningEsi->id);

			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case BLOCK:

			blockEsi(key, runningEsi->id);

		break;

		case FREE:
			freeKey(key, runningEsi);

			handleEsiStatus(esiExecutionInformation->esiStatus);
		break;

		case ABORT:
			runningEsi = NULL;
		break;
	}
    free(esiExecutionInformation);
	esiInformation = NULL;
}

void handleEsiStatus(char esiStatus) {
	switch(esiStatus) {
		case FINISHED:
			finishEsi(runningEsi);
			log_info(logger, "Esi finished execution");
		break;

		case NOTFINISHED:

		break;
	}
}

void abortEsi(Esi* esi) {

	bool isEsiById(void* element) {
		return ((Esi*) element)->id == esi->id;
	}
	log_warning(logger,"Hay %d claves bloqueadas antes de entrar al freeTakenKeys", list_size(esi->lockedKeys)); //todo borrar despues

	freeTakenKeys(esi);
	deleteEsiFromSystem(esi);
	pthread_mutex_lock(&mutexFinishedList);
	t_list * filteredList = list_filter(finishedEsis, &isEsiById);
	pthread_mutex_unlock(&mutexFinishedList);
	if (list_size(filteredList) == 0) {
		list_remove_and_destroy_by_condition(allSystemEsis, &isEsiById,&destroyEsi);
	}
	list_destroy(filteredList);

}

void deleteEsiFromSystem(Esi* esiToDelete) {

	bool isEsiByID(void* esi) {
		return ((Esi*) esi)->id == esiToDelete->id;
	}

	t_queue* blockedEsis;

	pthread_mutex_lock(&mutexReadyList);
	t_list * filteredList = list_filter(readyEsis, &isEsiByID);
	pthread_mutex_unlock(&mutexReadyList);

	if (list_size(filteredList) > 0) {

		pthread_mutex_lock(&mutexReadyList);
		list_remove_and_destroy_by_condition(readyEsis, &isEsiByID, &destroyEsi);
		pthread_mutex_unlock(&mutexReadyList);
	}

	list_destroy(filteredList);

	if (runningEsi != NULL && runningEsi->id == esiToDelete->id) {
		executeConsoleInstruccions();
		setFinishedExecutingInstruccion(true);
		runningEsi = NULL;
	}

	pthread_mutex_lock(&mutexFinishedList);
	filteredList = list_filter(finishedEsis, &isEsiByID);
	pthread_mutex_unlock(&mutexFinishedList);

	if (list_size(filteredList) > 0) {
		//nothing to do, is in finished list
	}

	list_destroy(filteredList);

	for (int i = 0; i < list_size(allSystemKeys); i++) {
		char* key = list_get(allSystemKeys, i);

		blockedEsis = dictionary_get(blockedEsiDic, key);

		for (int j = 0; j < queue_size(blockedEsis); j++) {

			actualEsi = (int*) queue_pop(blockedEsis);

			if (*actualEsi == esiToDelete->id) {

				free(actualEsi);
			} else {
				queue_push(blockedEsis, actualEsi);
			}
		}
	}
}

void moveFromRunningToReady(Esi* esi) {
	addEsiToReady(runningEsi);
	runningEsi = NULL;
}

OperationResponse *recieveEsiInformation(int esiSocket) {
	OperationResponse* finishInformation = malloc(sizeof(OperationResponse));
	if(recv_all(esiSocket, finishInformation, sizeof(OperationResponse)) == CUSTOM_FAILURE) {
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
		if(runningEsi!=NULL){
			t_list * filteredList = list_filter(runningEsi->lockedKeys, &keyCompare);
			if (list_size(filteredList) > 0) {
				keyStatus = LOCKED;
			}
			list_destroy(filteredList);
		}
	}
	char op = PLANIFICADOR_KEY_STATUS_RESPONSE;
	send_all(coordinadorSocket, &op, sizeof(op));
	if (send_all(coordinadorSocket, &keyStatus, sizeof(char)) == CUSTOM_FAILURE){
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	} else {
		log_info(logger, "Send key status (%s) to coordinador", getKeyStatusName(keyStatus));
	}
}

void sendMessageExecuteToEsi(Esi* nextEsi) {
	int socketEsi = nextEsi->socketConection;
	int message = RUN;
    if (sendInt(message, socketEsi) == CUSTOM_FAILURE) {
	   log_error(logger, "Coultn't send message to ESI %d", nextEsi->id);
    } else {
    	log_info(logger, "Send execute message to ESI %d in socket %d", nextEsi->id, nextEsi->socketConection);
    }
}

// General use functions
bool addKeyToGeneralKeys(char* key) {
	bool itemIsKey(void* item) {
		return strcmp(key, (char*) item) == 0;
	}

	bool addedToList = false;
	//char* keyCopy = strdup(key);
	if (!list_any_satisfy(allSystemTakenKeys, &itemIsKey)){
		list_add(allSystemTakenKeys, strdup(key));
		addedToList = true;
	}else{
		//free(keyCopy);
	}
	if (!list_any_satisfy(allSystemKeys, &itemIsKey)){
		list_add(allSystemKeys, strdup(key));
		addedToList = true;
	}



	return addedToList;
}

void blockEsi(char* lockedKey, int esiBlocked) {
	t_queue* esiQueue;

	int* esiBlockedCopy;
	esiBlockedCopy = malloc(sizeof(int));
	*esiBlockedCopy = esiBlocked;
	if (!dictionary_has_key(blockedEsiDic, lockedKey)) {
		esiQueue = queue_create();
		queue_push(esiQueue, esiBlockedCopy);
		dictionary_put(blockedEsiDic, lockedKey, esiQueue);
		log_info(logger,"Creating a queue with an ESI in blockedEsiDic");
	} else {
		esiQueue = dictionary_get(blockedEsiDic, lockedKey);
		queue_push(esiQueue, esiBlockedCopy);
	}
	log_info(logger, "Added ESI (%d) to blocked dictionary in key (%s)", *esiBlockedCopy, lockedKey);

	if(runningEsi!=NULL && runningEsi->id == esiBlocked){
		dislodgeEsi(getEsiById(esiBlocked), false, true);
	}else{
		removeFromReady(getEsiById(esiBlocked));
	}

}

void lockKey(char* key, int esiID) {
	addKeyToGeneralKeys(key);

	if (esiID != CONSOLE_BLOCKED) {
		addLockedKeyToEsi(key, &runningEsi);
	}

	if (!dictionary_has_key(blockedEsiDic, key)) {
		t_queue* esiQueue = queue_create();
		dictionary_put(blockedEsiDic, key, esiQueue);
		log_info(logger,"Creating an empty key in blockedEsiDic");
	}
}

Esi* getEsiById(int id) {
	bool isId(void* element) {
		return ((Esi*) element)->id == id;
	}
	Esi* esi = list_find(allSystemEsis, &isId);
	return esi;
}

Esi* getEsiBySocket(int socket) {
	bool isSocket(void* element) {
		return ((Esi*) element)->socketConection == socket;
	}
	//Esi* esi = list_find(allSystemEsis, &isSocket);
	t_list* filteredList = list_filter(allSystemEsis,&isSocket);
	Esi* esi = list_get(filteredList,list_size(filteredList)-1);
	list_destroy(filteredList);
	return esi;
}

void freeKey(char* key, Esi* esiTaker) {
	// TODO revisar (estaba al reves el orden de las funciones)
	unlockEsi(key, false);
	removeLockedKey(key, esiTaker);
}

void unlockEsi(char* key,bool isConsoleInstruccion) {
	bool keyCompare(void* takenKey) {
		return string_equals_ignore_case((char*) takenKey, key);
	}

	t_queue* blockedEsisQueue = dictionary_get(blockedEsiDic, key);
	int* unlockedEsi;

	if(isConsoleInstruccion){
		if(queue_is_empty(blockedEsisQueue)){
			list_remove_by_condition(allSystemTakenKeys, &keyCompare);
			log_info(logger, "Key (%s) freed", key);
			//if(!dictionary_has_key(blockedEsiDic,key))
				//free(key);
		}
	}else{
		list_remove_by_condition(allSystemTakenKeys, &keyCompare);
		log_warning(logger, "Key (%s) freed", key);
		//if(!dictionary_has_key(blockedEsiDic,key))
						//free(key);
	}



	if (!queue_is_empty(blockedEsisQueue)) {
		unlockedEsi = (int*) queue_pop(blockedEsisQueue);
		log_info(logger, "Unblocked ESI %d from key (%s)", *unlockedEsi, key);
		addEsiToReady(getEsiById(*unlockedEsi));
	}
}

void showBlockedEsisInKey(char* key){
	 if(dictionary_has_key(blockedEsiDic,key)){
		 t_queue* blockedEsis;
		 blockedEsis = (t_queue*)dictionary_get(blockedEsiDic, key);
		 int* esiIDpointer;
		 if (queue_is_empty(blockedEsis))
		 {
			 log_info(logger,"There are no blocked ESIs in key (%s)", key);
		 }else{
			 if(queue_size(blockedEsis)==0){
				 log_info(logger,"There are no blocked ESIs in key (%s)", key);
			 }else{
				 log_info(logger,"Blocked ESIs in key (%s):", key);
				 for (int i = 0; i < queue_size(blockedEsis); i++) {
					 esiIDpointer = (int*) queue_pop(blockedEsis);
					 printEsi(getEsiById(*esiIDpointer));
					 queue_push(blockedEsis, esiIDpointer);
				 }
			 }

		 }

	 }else{
		 log_info(logger,"Key doesn't exists, there are no ESIs to show");
	 }
}

char isLockedKey(char* key) {
	bool itemIsKey(void* item) {
		return strcmp(key,(char*) item) == 0;
	}

	return (list_any_satisfy(allSystemTakenKeys, &itemIsKey) ? BLOCKED : NOTBLOCKED);
}

bool isValidEsiId(int esiID) {
	bool itemIsKey(void* element) {
		return ((Esi*) element)->id == esiID;
	}

	return list_any_satisfy(allSystemEsis, &itemIsKey);
}

void addEsiToReady(Esi* esi) {
	pthread_mutex_lock(&mutexReadyList);
	list_add(readyEsis, (void*) esi);
	pthread_mutex_unlock(&mutexReadyList);
}

// Planificador setup functions
void addConfigurationLockedKeys(char** blockedKeys) {
	int i = 0;

	while (blockedKeys[i]) {
		lockKey(blockedKeys[i], CONSOLE_BLOCKED);
		// REVIEW esto lo hace aca y dentro de la funcion lockKey
		//todo review esto addKeyToGeneralKeys(blockedKeys[i]);
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

void recieveConsoleStatusResponse(){

	//si la clave esta en instancia caida, no se simula y se devuelve NOT_SIMULATED_INSTANCIA
	char instanciaOrigin;

	if(recv_all(coordinadorSocket, &instanciaOrigin, sizeof(instanciaOrigin)) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't recieve instancia origin from coordinador to respond status command");
		exitPlanificador();
	}



	log_info(logger, "Recieved instancia status from coordinador to response status command");
	if(instanciaOrigin == STATUS_NO_INSTANCIAS_AVAILABLE){
		log_info(logger, "There are no instancias");
		return;
	}


	char* instanciaThatSatisfiesStatus = NULL;
	if(recieveString(&instanciaThatSatisfiesStatus, coordinadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't recieve instancia's name from coordinador to respond status command");
		exitPlanificador();
	}
	showBlockedEsisInKey(globalKey);

	if(instanciaOrigin == STATUS_SIMULATED_INSTANCIA){
		log_info(logger, "No instancias have the key, it would be on instancia %s", instanciaThatSatisfiesStatus);
		free(instanciaThatSatisfiesStatus);
		return;
	}
	if(instanciaOrigin == STATUS_NOT_SIMULATED_INSTANCIA_BUT_FALLEN){
		log_info(logger, "Instancia %s have the key but is fallen", instanciaThatSatisfiesStatus);
		free(instanciaThatSatisfiesStatus);
		return;
	}

	char* value = NULL;
	if(recieveString(&value, coordinadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't recieve value from coordinador to respond status command");
		free(instanciaThatSatisfiesStatus);
		exitPlanificador();
	}
	log_info(logger, "Instancia %s has the key with value %s", instanciaThatSatisfiesStatus, value);
	free(instanciaThatSatisfiesStatus);
	free(value);



}



void executeInstruccion(){

	Esi* nextEsi;
	pthread_mutex_lock(&mutexPauseState);
	if (pauseState == CONTINUE) {
		pthread_mutex_unlock(&mutexPauseState);
		pthread_mutex_lock(&mutexFinishedExecutingInstruccion);
		if (finishedExecutingInstruccion) {
			if (runningEsi == NULL) {
				pthread_mutex_lock(&mutexReadyList);
				if (list_size(readyEsis) > 0) {
					log_info(logger,"Hay (%d) ESIs para ejecutar",list_size(readyEsis));
					pthread_mutex_unlock(&mutexReadyList);
					nextEsi = getNextEsi();
					moveEsiToRunning(nextEsi);
				}else{
					pthread_mutex_unlock(&mutexReadyList);
				}

			} else {
				if (strcmp(algorithm, "SJF-CD") == 0) {
					if (mustDislodgeRunningEsi()) {
						dislodgeEsi(runningEsi, true, false);
						nextEsi = getNextEsi();
						moveEsiToRunning(nextEsi);
					}
				}
			}

			if (runningEsi != NULL) {
				finishedExecutingInstruccion = false;
				log_info(logger, "Executing ESI %d", runningEsi->id);
				log_info(logger, "Running ESI actual estimation = %f with %d sentences already executed with waitingTime = %d", runningEsi->lastEstimation, runningEsi->sentenceCounter, runningEsi->waitingTime);
				sendEsiIdToCoordinador(runningEsi->id);
				sendMessageExecuteToEsi(runningEsi);
				log_info(logger, "Waiting coordinador request");
			}
		}
		pthread_mutex_unlock(&mutexFinishedExecutingInstruccion);
	}else{
		pthread_mutex_unlock(&mutexPauseState);
	}

}

int clientMessageHandler(char clientMessage, int clientSocket) {
	if (clientSocket == coordinadorSocket) {
		if (clientMessage == KEYSTATUSMESSAGE) {
			log_info(logger, "I recieved a key status message");

			if (recieveString(&keyRecieved, coordinadorSocket) == CUSTOM_FAILURE) {
				log_error(logger, "Couldn't recieve key to check from coordinador, quitting...");
				exitPlanificador();
			} else {
				log_info(logger, "Key received = %s", keyRecieved);
				sendKeyStatusToCoordinador(keyRecieved);
			}


		} else if (clientMessage == CORDINADORCONSOLERESPONSEMESSAGE) {
			log_info(logger, "I recieved a coordinador console response message");
			recieveConsoleStatusResponse();
		}
	} else {
		if (clientMessage == ESIID) {
			welcomeEsi(clientSocket);
		} else if (clientMessage == ESIINFORMATIONMESSAGE) {
			log_info(logger, "I recieved a esi information message");
			esiInformation = recieveEsiInformation(runningEsi->socketConection);
			log_info(logger, "Going to handle Esi execution info.CoordinadoResponse = (%s) ,esiStatus = (%s)", getCoordinadorResponseName(esiInformation->coordinadorResponse), getEsiInformationResponseName(esiInformation->esiStatus));
			log_warning(logger,"Hay %d claves bloqueadas por el esi antes de ejecutar una instruccion", list_size(runningEsi->lockedKeys)); //todo borrar despues

			handleEsiInformation(esiInformation, keyRecieved);

			free(keyRecieved);
			log_info(logger, "Finish handling one instruction from ESI");
			setFinishedExecutingInstruccion(true);
		} else {
			log_info(logger, "I received a strange in socket %d", clientSocket);
			printf("Lo que me llego es %c", clientMessage);
			exitPlanificador();
		}
	}
	executeConsoleInstruccions();
	executeInstruccion();
	return 0;
}

void setFinishedExecutingInstruccion(bool value){
	if(value){
		executeConsoleInstruccions();
	}
	pthread_mutex_lock(&mutexFinishedExecutingInstruccion);
	finishedExecutingInstruccion = value;
	pthread_mutex_unlock(&mutexFinishedExecutingInstruccion);
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

	while (1) {

		readfds = master; // copy it

		if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
			perror("select failed");
			if (errno == EINTR) {
				continue;
			}
		}



		// run through the existing connections looking for data to read
		for (i = 0; i <= fdmax; i++) {
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
					sem_wait(&executionSemaphore);
					resultRecv = recv_all(clientSocket, &clientMessage, sizeof(char));

					if (resultRecv == CUSTOM_FAILURE) {
						if (clientSocket == coordinadorSocket) {
							log_error(logger, "Coordinador disconnected. Exit planificador");
							exitPlanificador();
						} else {
							log_warning(logger, "ESI disconnected.");
							printEsi(getEsiBySocket(clientSocket));
							abortEsi(getEsiBySocket(clientSocket));
							log_info(logger, "Mando a ejecutar.");
							executeInstruccion();

						}
						close(clientSocket);
						FD_CLR(clientSocket, &master);
					} else {
						clientMessageHandler(clientMessage, clientSocket);
					}
					sem_post(&executionSemaphore);
				}
			}
		}
	}
	return 0;
}

void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys) {

	t_config* config;
	config = config_create(CFG_FILE);

	free(CFG_FILE);

	if (config == NULL) {
		log_error(logger, "Planificador cannot work because of invalid configuration file");
		exit(-1);
	}

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
	allSystemKeys = list_create();
	blockedEsiDic = dictionary_create();
	addConfigurationLockedKeys(blockedKeys);

	globalKey = malloc(41);

	allSystemEsis = list_create();
	readyEsis = list_create();
	finishedEsis = list_create();
	runningEsi = NULL;

	pauseState = CONTINUE;


	actualID = 1;

	pthread_mutex_init(&mutexFinishedExecutingInstruccion,NULL);
	pthread_mutex_init(&mutexReadyList,NULL);
	pthread_mutex_init(&mutexInstruccionsByConsole,NULL);
	pthread_mutex_init(&mutexPauseState,NULL);
	pthread_mutex_init(&mutexFinishedList,NULL);

	pthread_mutex_lock(&mutexFinishedExecutingInstruccion);
	finishedExecutingInstruccion = true;
	pthread_mutex_unlock(&mutexFinishedExecutingInstruccion);
	pthread_create(&threadConsole, NULL, (void *) openConsole, NULL);
}

void idDestroyer(void* id){
	free(id);
}

void queueDestroyer(void* queue){
	queue_destroy_and_destroy_elements(queue,idDestroyer);
}

void exitPlanificador() {

	//free(esiBlockedCopy);
	free(actualEsi);
	free(globalKey);
	if (allSystemKeys)
		list_destroy_and_destroy_elements(allSystemKeys, destroyKey);

	if (allSystemEsis)
		list_destroy_and_destroy_elements(allSystemEsis, destroyEsi);

	if (readyEsis)
		list_destroy(readyEsis);

	if (finishedEsis){
		pthread_mutex_lock(&mutexFinishedList);
		list_destroy(finishedEsis);
		pthread_mutex_unlock(&mutexFinishedList);
	}

	if (allSystemTakenKeys)
		list_destroy(allSystemTakenKeys);

	if (blockedEsiDic)
		dictionary_destroy_and_destroy_elements(blockedEsiDic,queueDestroyer);

	destroyConsole();

	pthread_cancel(threadConsole);
	pthread_cancel(threadConsoleInstructions);

	log_destroy(logger);



	exit(-1);
}



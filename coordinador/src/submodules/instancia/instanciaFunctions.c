/*
 * instanciaFunctions.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "instanciaFunctions.h"

//TODO sacar, esta para probar
int instanciaIdForTesting = 0;

void instanciaIsBack(Instancia* instancia, int instanciaSocket){
	instancia->isFallen = INSTANCIA_ALIVE;
	instancia->socket = instanciaSocket;
	instancia->actualCommand = 0;
}

int sendKeysToInstancia(Instancia* arrivedInstancia){
	log_info(logger, "About to send keys to instancia");
	if(sendStingList(arrivedInstancia->storedKeys, arrivedInstancia->socket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't send key list to instancia, killing his thread");
		return -1;
	}
	log_info(logger, "Keys sent to instancia");

	return 0;
}

Instancia* existsInstanciaWithName(char* arrivedInstanciaName){
	int instanciaHasName(Instancia* instancia){
		return strcmp(instancia->name, arrivedInstanciaName) == 0;
	}

	return list_find(instancias, (void*) instanciaHasName);
}

int sendInstanciaConfiguration(int instanciaSocket, int cantEntry, int entrySize){
	InstanciaConfiguration config;
	config.entriesAmount = cantEntry;
	config.entrySize = entrySize;
	if (send_all(instanciaSocket, &config, sizeof(InstanciaConfiguration)) == CUSTOM_FAILURE) {
		log_warning(logger, "Couldn't send configuration to instancia");
		return -1;
	}
	return 0;
}

int updateSpaceUsed(Instancia* instancia) {
	int spaceUsed = 0;
	if(recieveInt(&spaceUsed, instancia->socket) == CUSTOM_FAILURE){
		return -1;
	}
	instancia->spaceUsed = spaceUsed;
	log_info(logger, "Successfully updated instancia's space used: %d", instancia->spaceUsed);
	return 0;
}

Instancia* initialiceArrivedInstancia(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket) < 0){
		return NULL;
	}
	log_info(logger, "Arrived instancia's name is %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize) < 0){
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
			//TODO aca tambien, el proceso instancia va a seguir vivo (habria que mandarle un mensaje para que muera)
			log_warning(logger, "Couldn't initialize instancia's semaphore, killing the created thread...");
			pthread_mutex_unlock(&instanciasListMutex);
			free(arrivedInstanciaName);
			return NULL;
		}

		if(list_size(instancias) == 1){
			pthread_mutex_lock(&lastInstanciaChosenMutex);
			lastInstanciaChosen = list_get(instancias, 0);
			pthread_mutex_unlock(&lastInstanciaChosenMutex);
		}

		log_info(logger, "Instancia %s is new", arrivedInstanciaName);
	}

	if(sendKeysToInstancia(arrivedInstancia) < 0){
		free(arrivedInstanciaName);
		pthread_mutex_unlock(&instanciasListMutex);
		return NULL;
	}

	if(updateSpaceUsed(arrivedInstancia) < 0){
		log_warning(logger, "Couldn't recieve instancia's space used on initialization");
		free(arrivedInstanciaName);
		pthread_mutex_unlock(&instanciasListMutex);
		return NULL;
	}

	free(arrivedInstanciaName);
	pthread_mutex_unlock(&instanciasListMutex);

	return arrivedInstancia;
}

Instancia* initialiceArrivedInstanciaDummy(int instanciaSocket){
	char* arrivedInstanciaName = NULL;
	if(recieveInstanciaName(&arrivedInstanciaName, instanciaSocket) < 0){
		return NULL;
	}
	//recieveInstanciaNameDummy(&arrivedInstanciaName);
	log_info(logger, "Arrived instancia's name is %s", arrivedInstanciaName);

	if(sendInstanciaConfiguration(instanciaSocket, cantEntry, entrySize) < 0){
		free(arrivedInstanciaName);
		return NULL;
	}
	log_info(logger, "Sent configuration to instancia %s", arrivedInstanciaName);

	pthread_mutex_lock(&instanciasListMutex);
	char* harcodedNameForTesting = malloc(strlen(arrivedInstanciaName) + 5);
	strcpy(harcodedNameForTesting, arrivedInstanciaName);
	char* instanciaNumberInString = malloc(sizeof(int));
	sprintf(instanciaNumberInString, "%d", instanciaIdForTesting);
	strcat(harcodedNameForTesting, instanciaNumberInString);
	Instancia* instancia = createNewInstancia(instanciaSocket, harcodedNameForTesting);
	instanciaIdForTesting++;

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
	pthread_mutex_unlock(&instanciasListMutex);

	list_iterate(instanciasThatNeededToCompact, (void*) waitInstanciaToCompact);

	pthread_mutex_lock(&instanciasListMutex);

	list_destroy(instanciasThatNeededToCompact);
}

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
		log_warning(logger, "Couldn't recieve instancia response");
		instanciaResponseStatus = INSTANCIA_RESPONSE_FALLEN;
		return -1;
	}

	instanciaResponseStatus = status;

	log_info(logger, "Instancia's response is gonna be processed");

	if(status == INSTANCIA_RESPONSE_FALLEN){
		log_warning(logger, "Instancia %s couldn't do %s because it fell. His thread dies, and key %s is deleted:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
		removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
		return -1;
	}else if(status == INSTANCIA_RESPONSE_FAILED){
		log_warning(logger, "Instancia %s couldn't do %s, so the key %s is deleted:", actualInstancia->name, getOperationName(actualEsiRequest->operation), actualEsiRequest->operation->key);
		removeKeyFromFallenInstancia(actualEsiRequest->operation->key, actualInstancia);
		showInstancia(actualInstancia);
	}else if(status == INSTANCIA_RESPONSE_SUCCESS){
		log_info(logger, "%s could do %s", actualInstancia->name, getOperationName(actualEsiRequest->operation));
		if(actualEsiRequest->operation->operationCode == OURSET) {
			if(updateSpaceUsed(actualInstancia) < 0){
				log_warning(logger, "Couldn't recieve instancia's space used, so it fell (but the operation was successfully done)");
				return -1;
			}
		}
	}

	sem_post(&instanciaResponse);
	return 0;
}

void instanciaExitGracefully(Instancia* instancia){

	instanciaHasFallen(instancia);

	log_info(logger, "Instancia %s actualCommand is %c", instancia->name, instancia->actualCommand);

	switch(instancia->actualCommand){
		case INSTANCIA_DO_OPERATION:
			instanciaResponseStatus = INSTANCIA_RESPONSE_FALLEN;
			sem_post(&instanciaResponse);
			break;

		case INSTANCIA_DO_COMPACT:

			sem_post(instancia->compactSemaphore);

			break;

		case INSTANCIA_CHECK_KEY_STATUS:

			instanciaStatusFromValueRequest = INSTANCIA_RESPONSE_FALLEN;
			valueFromKey = NULL;
			sem_post(&valueFromKeyInstanciaSemaphore);

			break;

		default:

			//TODO revisar este caso que se usa por ejemplo cuando llegaron y todavia no estan haciendo nada
			log_info(logger, "Instancia has fallen with no active command");

			break;
	}

	showInstancias(instancias);

	pthread_mutex_unlock(&instanciasListMutex);
}

void keyDestroyer(char* keyToBeDestroyed){
	free(keyToBeDestroyed);
}

void instanciaDestroyer(Instancia* instancia){
	list_destroy_and_destroy_elements(instancia->storedKeys, (void*) keyDestroyer);
	free(instancia->name);
	sem_destroy(instancia->compactSemaphore);
	free(instancia->compactSemaphore);
	free(instancia);
}

int instanciaIsAlive(Instancia* instancia){
	return !instancia->isFallen;
}

int recieveInstanciaName(char** arrivedInstanciaName, int instanciaSocket){
	if(recieveString(arrivedInstanciaName, instanciaSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't recieve instancia's name");
		free(*arrivedInstanciaName);
		return -1;
	}else if(strlen(*arrivedInstanciaName) == 0){
		log_warning(logger, "An Instancia must have name");
		free(*arrivedInstanciaName);
		return -1;
	}
	return 0;
}

void recieveInstanciaNameDummy(char** arrivedInstanciaName){
	*arrivedInstanciaName = "instanciaDePrueba";
}

char instanciaDoOperation(Instancia* instancia, Operation* operation){
	char message = INSTANCIA_DO_OPERATION;

	void* package = NULL;
	int offset = 0;

	int sizeOperationPackage = 0;
	void* operationPackage = generateOperationPackage(operation, &sizeOperationPackage);

	addToPackageGeneric(&package, &message, sizeof(message), &offset);
	addToPackageGeneric(&package, operationPackage, sizeOperationPackage, &offset);

	int sendResult = send_all(instancia->socket, package, offset);
	free(package);
	//TODO mariano no basta con liberar el package? sin este free me estaba tirando leaks
	free(operationPackage);
	return sendResult;
}

int isLookedKeyGeneric(char* actualKey, char* key){
	if(strcmp(actualKey, key)){
		return 0;
	}
	return 1;
}

Instancia* lookForKey(char* key){
	int isLookedKey(char* actualKey){
		return isLookedKeyGeneric(actualKey, key);
	}

	int isKeyInInstancia(Instancia* instancia){
		if(list_any_satisfy(instancia->storedKeys, (void*) isLookedKey)){
			return 1;
		}
		return 0;
	}

	return list_find(instancias, (void*) isKeyInInstancia);
}

void removeKeyFromFallenInstancia(char* key, Instancia* instancia){
	int isLookedKey(char* actualKey){
		return isLookedKeyGeneric(actualKey, key);
	}

	list_remove_and_destroy_by_condition(instancia->storedKeys, (void*) isLookedKey, (void*) keyDestroyer);
}

void addKeyToInstanciaStruct(Instancia* instancia, char* key){
	list_add(instancia->storedKeys, strdup(key));
}

//TODO donde se use esta funcion, meter tambien mutex de la lista de instancias! (esta instancia es una de esa lista!)
//ese caso podria darse cuando esten activos varios hilos de instancia (compactacion)
void instanciaHasFallen(Instancia* fallenInstancia){
	fallenInstancia->isFallen = INSTANCIA_FALLEN;
	close(fallenInstancia->socket);
}

int addSemaphoreToInstancia(Instancia* instancia){

	sem_t* compactSem = malloc(sizeof(sem_t));
	if(sem_init(compactSem, 0, 0) < 0){
		return -1;
	}

	instancia->compactSemaphore = compactSem;

	return 0;
}

Instancia* createInstancia(int socket, int spaceUsed, char* name){
	Instancia* instancia = malloc(sizeof(Instancia));
	instancia->socket = socket;
	instancia->spaceUsed = spaceUsed;
	instancia->storedKeys = list_create();
	instancia->isFallen = INSTANCIA_ALIVE;
	instancia->name = strdup(name);
	instancia->actualCommand = 0;

	if(addSemaphoreToInstancia(instancia) < 0){
		return NULL;
	}

	return instancia;
}

Instancia* createNewInstancia(int instanciaSocket, char* name){
	Instancia* newInstancia = createInstancia(instanciaSocket, 0, name);

	if(newInstancia){
		list_add(instancias, newInstancia);
	}

	return newInstancia;
}

/*-----------------------------------------------------*/

/*
 * TEST FUNCTIONS
 */

void showStoredKey(char* key){
	log_info(logger, "%s", key);
}

void showStoredKeys(Instancia* instancia){
	if(instancia->storedKeys != NULL){
		printf("Keys = \n");
		list_iterate(instancia->storedKeys, (void*) showStoredKey);
	}else{
		log_info(logger, "storedKeys cannot be showed");
	}
}

char* instanciaState(Instancia* instancia){
	if(instancia->isFallen == INSTANCIA_ALIVE){
		return "alive";
	}
	return "fallen";
}

void showInstancia(Instancia* instancia){
	if(instancia != NULL){

		log_info(logger,
				"\nName = %s\nSocket = %d\nSpace used = %d\nState = %s",
				instancia->name, instancia->socket, instancia->spaceUsed, instanciaState(instancia));

		showStoredKeys(instancia);

		printf("----------\n");
	}else{
		log_info(logger, "Instance cannot be showed");
	}
}

void showInstancias(t_list* instanciasList){
	printf("----- INSTANCIAS -----\n");
	if(list_size(instanciasList) != 0){
		list_iterate(instanciasList, (void*) &showInstancia);
	}
	else{
		log_info(logger, "Actual instancias list is empty");
	}
}
/*
 * TEST FUNCTIONS
 */

/*
 * instanciaFunctions.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "instanciaFunctions.h"

int recieveInstanciaName(char** arrivedInstanciaName, int instanciaSocket, t_log* logger){
	if(recieveString(arrivedInstanciaName, instanciaSocket) == CUSTOM_FAILURE){
		log_error(logger, "Couldn't recieve instancia's name");
		free(*arrivedInstanciaName);
		return -1;
	}else if(strlen(*arrivedInstanciaName) == 0){
		log_error(logger, "An Instancia must have name");
		free(*arrivedInstanciaName);
		return -1;
	}
	return 0;
}

int sendInstanciaConfiguration(int instanciaSocket, int cantEntry, int entrySize, t_log* logger){
	InstanciaConfiguration config;
	config.entriesAmount = cantEntry;
	config.entrySize = entrySize;
	if (send(instanciaSocket, &config, sizeof(InstanciaConfiguration), 0) < 0) {
		log_error(logger, "Couldn't send configuration to instancia");
		return -1;
	}
	return 0;
}

Instancia* existsInstanciaWithName(char* arrivedInstanciaName){
	int instanciaHasName(Instancia* instancia){
		return strcmp(instancia->name, arrivedInstanciaName) == 0;
	}

	return list_find(instancias, (void*) instanciaHasName);
}

int addSemaphoresToInstancia(Instancia* instancia){
	sem_t* sem = malloc(sizeof(sem_t));
	if(sem_init(sem, 0, 0) < 0){
		return -1;
	}

	instancia->executionSemaphore = sem;

	sem_t* compactSem = malloc(sizeof(sem_t));
	if(sem_init(compactSem, 0, 0) < 0){
		return -1;
	}

	instancia->compactSemaphore = compactSem;

	return 0;
}

void instanciaIsBack(Instancia* instancia, int instanciaSocket){
	instancia->isFallen = INSTANCIA_ALIVE;
	instancia->socket = instanciaSocket;
	addSemaphoresToInstancia(instancia);
}

void recieveInstanciaNameDummy(char** arrivedInstanciaName){
	*arrivedInstanciaName = "instanciaDePrueba";
}

//TODO agregar el send de INSTANCIA_DO_OPERATION
char instanciaDoOperation(Instancia* instancia, Operation* operation, t_log* logger){
	if(sendOperation(operation, instancia->socket) == CUSTOM_FAILURE){
		return INSTANCIA_RESPONSE_FALLEN;
	}
	log_info(logger, "Operation sent to instancia");
	return waitForInstanciaResponse(instancia);
}

char instanciaDoOperationDummy(Instancia* instancia, Operation* operation, t_log* logger){
	/*if(sendOperation(operation, instancia->socket) == CUSTOM_FAILURE){
		return INSTANCIA_RESPONSE_FALLEN;
	}*/
	log_info(logger, "Operation sent to instancia");
	return waitForInstanciaResponseDummy(instancia);
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

void keyDestroyer(char* keyToBeDestroyed){
	free(keyToBeDestroyed);
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

void destroyInstanciaSemaphores(Instancia* instancia){
	sem_destroy(instancia->executionSemaphore);
	free(instancia->executionSemaphore);

	sem_destroy(instancia->compactSemaphore);
	free(instancia->compactSemaphore);
}

//TODO donde se use esta funcion, meter tambien mutex de la lista de instancias! (esta instancia es una de esa lista!)
//ese caso podria darse cuando esten activos varios hilos de instancia (compactacion)
void instanciaHasFallen(Instancia* fallenInstancia){
	fallenInstancia->isFallen = INSTANCIA_FALLEN;
	close(fallenInstancia->socket);
	destroyInstanciaSemaphores(fallenInstancia);
}

char waitForInstanciaResponse(Instancia* chosenInstancia){
	char response = 0;
	int recvResult = recv(chosenInstancia->socket, &response, sizeof(char), 0);
	if (recvResult <= 0){
		return INSTANCIA_RESPONSE_FALLEN;
	}
	return response;
}

char waitForInstanciaResponseDummy(){
	return INSTANCIA_NEED_TO_COMPACT;
}

Instancia* createNewInstancia(int instanciaSocket, char* name){
	Instancia* newInstancia = createInstancia(instanciaSocket, 0, 'a', 'z', name);

	if(newInstancia){
		list_add(instancias, newInstancia);
	}

	return newInstancia;
}

Instancia* createInstancia(int socket, int spaceUsed, char firstLetter, char lastLetter, char* name){
	Instancia* instancia = malloc(sizeof(Instancia));
	instancia->socket = socket;
	instancia->spaceUsed = spaceUsed;
	instancia->firstLetter = firstLetter;
	instancia->lastLetter = lastLetter;
	instancia->storedKeys = list_create();
	instancia->isFallen = INSTANCIA_ALIVE;
	instancia->name = strdup(name);

	if(addSemaphoresToInstancia(instancia) < 0){
		return NULL;
	}

	return instancia;
}

/*-----------------------------------------------------*/

/*
 * TEST FUNCTIONS
 */

void showStoredKey(char* key){
	printf("%s\n", key);
}

void showStoredKeys(Instancia* instancia){
	if(instancia->storedKeys != NULL){
		list_iterate(instancia->storedKeys, (void*) showStoredKey);
	}else{
		printf("storedKeys cannot be showed\n");
	}
}

void showInstanciaState(Instancia* instancia){
	if(instancia->isFallen == INSTANCIA_ALIVE){
		printf("State: alive\n");
	}else{
		printf("State: fallen\n");
	}
}

void showInstancia(Instancia* instancia){
	if(instancia != NULL){
		printf("Name = %s\n", instancia->name);
		printf("Socket = %d\n", instancia->socket);
		printf("Space used = %d\n", instancia->spaceUsed);
		printf("First letter = %c\n", instancia->firstLetter);
		printf("Last letter = %c\n", instancia->lastLetter);
		showStoredKeys(instancia);
		showInstanciaState(instancia);
		printf("----------\n");
	}else{
		printf("Instance cannot be showed\n");
	}
}

void showInstancias(t_list* instanciasList){
	printf("----- INSTANCIAS -----\n");
	list_iterate(instanciasList, (void*) &showInstancia);
}
/*
 * TEST FUNCTIONS
 */

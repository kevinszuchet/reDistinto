/*
 * instanciaFunctions.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "instanciaFunctions.h"

Instancia* existsInstanciaWithName(char* arrivedInstanciaName, t_list* instancias){
	int instanciaHasName(Instancia* instancia){
		return strcmp(instancia->name, arrivedInstanciaName) == 0;
	}

	return list_find(instancias, (void*) &instanciaHasName);
}

void instanciaIsBack(Instancia* instancia){
	instancia->isFallen = INSTANCIA_ALIVE;
}

int instanciaDoOperationDummy(){
	return 1;
}

int instanciaDoOperation(Instancia* instancia, Operation* operation){
	if(sendOperation(operation, instancia->socket) == CUSTOM_FAILURE){
		return -1;
	}
	printf("Se pudo enviar la operacion a la instancia\n");
	return waitForInstanciaResponse(instancia);
}

int isLookedKeyGeneric(char* actualKey, char* key){
	if(strcmp(actualKey, key)){
		return 0;
	}
	return 1;
}

Instancia* lookForKey(char* key, t_list* instancias){
	int isLookedKey(char* actualKey){
		return isLookedKeyGeneric(actualKey, key);
	}

	int isKeyInInstancia(Instancia* instancia){
		if(list_any_satisfy(instancia->storedKeys, (void*) &isLookedKey)){
			return 1;
		}
		return 0;
	}

	return list_find(instancias, (void*) &isKeyInInstancia);
}

void removeKeyFromFallenInstancia(char* key, Instancia* instancia){
	int isLookedKey(char* actualKey){
		return isLookedKeyGeneric(actualKey, key);
	}

	list_remove_by_condition(instancia->storedKeys, (void*) &isLookedKey);
}

void addKeyToInstanciaStruct(Instancia* instancia, char* key){
	list_add(instancia->storedKeys, key);
}

//TODO testear esta funcion
//TODO donde se use esta funcion, meter tambien mutex de la lista de instancias! (esta instancia es una de esa lista!)
void instanciaHasFallen(Instancia* fallenInstancia){
	fallenInstancia->isFallen = INSTANCIA_FALLEN;
}

int waitForInstanciaResponse(Instancia* chosenInstancia){
	int response = 0;
	int recvResult = recv(chosenInstancia->socket, &response, sizeof(int), 0);
	if (recvResult <= 0){
		return -1;
	}
	return response;
}

int firstInstanciaBeforeSecond(Instancia* firstInstancia, Instancia* secondInstancia){
	if(firstInstancia->id > secondInstancia->id){
		return 1;
	}
	return 0;
}

//TODO pasa el ultimo id a variable global
int createNewInstancia(int instanciaSocket, t_list* instancias, int* greatesInstanciaId, char* name){
	//TODO evaluar como se va a recibir esta lista, tiene que estar copiada en la instancia
	t_list* storedKeys = list_create();
	Instancia* newInstancia = createInstancia(*greatesInstanciaId, instanciaSocket, 0, 'a', 'z', storedKeys, name);

	//TODO sacar esto, es para que no se ponga esta cadena en todas las instancias
	if(*greatesInstanciaId == 0){
		list_add(newInstancia->storedKeys, "lio:messi");
	}

	list_add(instancias, newInstancia);
	(*greatesInstanciaId)++;

	return 0;
}

Instancia* createInstancia(int id, int socket, int spaceUsed, char firstLetter, char lastLetter, t_list* storedKeys, char* name){
	Instancia* instancia = malloc(sizeof(Instancia));
	instancia->id = id;
	instancia->socket = socket;
	instancia->spaceUsed = spaceUsed;
	instancia->firstLetter = firstLetter;
	instancia->lastLetter = lastLetter;
	instancia->storedKeys = storedKeys;
	instancia->isFallen = INSTANCIA_ALIVE;
	instancia->name = name;
	return instancia;
}

void destroyInstancia(Instancia* instancia){
	//TODO liberar espacio de storedKeys
	free(instancia);
}

/*-----------------------------------------------------*/

/*
 * TEST FUNCTIONS
 */
void initializeSomeInstancias(t_list* instancias){
	list_add(instancias, createInstancia(0, 10, 0, 'a', 'z', NULL, "instancia1"));
	list_add(instancias, createInstancia(5, 10, 0, 'a', 'z', NULL, "instancia2"));
	list_add(instancias, createInstancia(9, 10, 0, 'a', 'z', NULL, "instancia3"));
}

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
		printf("ID = %d\n", instancia->id);
		printf("Socket = %d\n", instancia->socket);
		printf("Space used = %d\n", instancia->spaceUsed);
		printf("First letter = %c\n", instancia->firstLetter);
		printf("Last letter = %c\n", instancia->lastLetter);
		showStoredKeys(instancia);
		showInstanciaState(instancia);
		printf("Name = %s\n", instancia->name);
		printf("----------\n");
	}else{
		printf("Instance cannot be showed\n");
	}
}

void showInstancias(t_list* instancias){
	printf("----- INSTANCIAS -----\n");
	list_iterate(instancias, (void*) &showInstancia);
}
/*
 * TEST FUNCTIONS
 */

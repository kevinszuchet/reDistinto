/*
 * instanciaFunctions.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "instanciaFunctions.h"

void removeKeyFromFallenInstancia(char* key, Instancia* instancia){
	//TODO sacar de esa instancia

	if(instancia != NULL){

	}else{
		//TODO buscar la clave y sacarla de la instancia
	}
}

int addKeyToInstanciaStruct(Instancia* instancia, char* key){
	list_add(instancia->storedKeys, key);
}

//TODO testear esta funcion
void instanciaHasFallen(Instancia* fallenInstancia, t_list* instancias, t_list* fallenInstancias){

	int isFallenInstancia(Instancia* instancia){
		return instancia == fallenInstancia ? 1 : 0;
	}

	list_remove_by_condition(instancias, (void*) &isFallenInstancia);
	list_add(fallenInstancias, fallenInstancia);
}

int waitForInstanciaResponse(Instancia* chosenInstancia){
	int response = 0;
	int recvResult = recv(chosenInstancia->socket, &response, sizeof(int), 0);
	if (recvResult <= 0){
		if(recvResult == 0){
			//se cayo la instancia
			return -1;
		}else{
			//TODO que hacemos aca? que paso?
		}
	}
	return response;
}

int firstInstanciaBeforeSecond(Instancia* firstInstancia, Instancia* secondInstancia){
	if(firstInstancia->id > secondInstancia->id){
		return 1;
	}
	return 0;
}

int createNewInstancia(int instanciaSocket, t_list* instancias){
	Instancia* instanciaWithGreatestId;
	//TODO se puede sacar esto y el showInstancias de abajo
	showInstancias(instancias);

	int greatestId = 0;

	if(list_size(instancias) != 0){
		greatestId = ((Instancia*) list_get(instancias, list_size(instancias) - 1))->id;
		greatestId++;
	}

	//TODO evaluar como se va a recibir esta lista, tiene que estar copiada en la instancia
	t_list* storedKeys = list_create();
	instanciaWithGreatestId = createInstancia(greatestId, instanciaSocket, 0, 'a', 'z', storedKeys);

	//TODO sacar esto, es para que no se ponga esta cadena en todas las instancias
	if(greatestId == 0){
		list_add(instanciaWithGreatestId->storedKeys, "lio:messi");
	}

	list_add(instancias, instanciaWithGreatestId);

	showInstancias(instancias);

	return 0;
}

Instancia* createInstancia(int id, int socket, int spaceUsed, char firstLetter, char lastLetter, t_list* storedKeys){
	Instancia* instancia = malloc(sizeof(Instancia));
	instancia->id = id;
	instancia->socket = socket;
	instancia->spaceUsed = spaceUsed;
	instancia->firstLetter = firstLetter;
	instancia->lastLetter = lastLetter;
	instancia->storedKeys = storedKeys;
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
	list_add(instancias, createInstancia(0, 10, 0, 'a', 'z', NULL));
	list_add(instancias, createInstancia(5, 10, 0, 'a', 'z', NULL));
	list_add(instancias, createInstancia(9, 10, 0, 'a', 'z', NULL));
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

void showInstancia(Instancia* instancia){
	if(instancia != NULL){
		printf("ID = %d\n", instancia->id);
		printf("Socket = %d\n", instancia->socket);
		printf("Space used = %d\n", instancia->spaceUsed);
		printf("First letter = %c\n", instancia->firstLetter);
		printf("Last letter = %c\n", instancia->lastLetter);
		showStoredKeys(instancia);
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

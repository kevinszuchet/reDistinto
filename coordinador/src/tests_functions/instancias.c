/*
 * initializeSomeInstancias.c
 *
 *  Created on: 30 abr. 2018
 *      Author: utnso
 */

#include "instancias.h"

Instancia* createInstancia(int id, int socket, int spaceUsed, char firstLetter, char lastLetter){
	Instancia* instancia = malloc(sizeof(Instancia));
	instancia->id = id;
	instancia->socket = socket;
	instancia->spaceUsed = spaceUsed;
	instancia->firstLetter = firstLetter;
	instancia->lastLetter = lastLetter;
	instancia->storedKeys = list_create();
	return instancia;
}

void destroyInstancia(Instancia* instancia){
	//TODO liberar espacio de storedKeys
	free(instancia);
}

void initializeSomeInstancias(t_list* instancias){
	list_add(instancias, createInstancia(0, 10, 0, 'a', 'z'));
	list_add(instancias, createInstancia(5, 10, 0, 'a', 'z'));
	list_add(instancias, createInstancia(9, 10, 0, 'a', 'z'));
}

void showInstancia(Instancia* instancia){
	if(instancia != NULL){
		printf("ID = %d\n", instancia->id);
		printf("Socket = %d\n", instancia->socket);
		printf("Space used = %d\n", instancia->spaceUsed);
		printf("First letter = %c\n", instancia->firstLetter);
		printf("Last letter = %c\n", instancia->lastLetter);
		printf("----------\n");
	}else{
		printf("Instance cannot be showed\n");
	}
}

void showInstancias(t_list* instancias){
	printf("----- INSTANCIAS -----\n");
	list_iterate(instancias, &showInstancia);
}


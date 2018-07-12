/*
 * tad_esi.c
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#include "tad_esi.h"


void addSentenceCounter(void* esi) {
	((Esi* )esi)->sentenceCounter++;
}

void addWaitingTime(void* esi) {
	((Esi* )esi)->waitingTime++;
}

void addWaitingTimeToAll(t_list* esis) {
	list_iterate(esis, &addWaitingTime);
}

void reduceWaitingTime(Esi** esi) {
	(*esi)->waitingTime--;
}

Esi *createEsi(int id, double initialEstimation, int socketConection) {
	Esi* newEsi = malloc(sizeof(Esi));
	newEsi->id = id;
	newEsi->lastBurst = 0;
	newEsi->socketConection = socketConection;
	newEsi->waitingTime = 0;
	newEsi->lastEstimation = initialEstimation;
	newEsi->sentenceCounter = 0;
	newEsi->lockedKeys = list_create();
	return newEsi;
}

int id(Esi* esi) {
	return esi->id;
}

void addLockedKeyToEsi(char** key, Esi** esi) {
	list_add((*esi)->lockedKeys, (void*) *key);
	printf("Locked key %s in ESI %d \n",*key,(*esi)->id);
}

void removeLockedKey(char* key, Esi* esi) {
	bool keyCompare(void* takenKeys) {
		return string_equals_ignore_case((char*) takenKeys, key);
	}
	list_remove_by_condition(esi->lockedKeys, &keyCompare);
}

void printEsi(void* esiToPrint) {
	Esi* esi = (Esi*) esiToPrint;
	printf("=============\n");
	printf("ID = (%d)\n", esi->id);
	printf("Socket = (%d)\n", esi->socketConection);
	printf("Last burst = (%f)\n", esi->lastBurst);
	printf("Last estimation = (%f)\n", esi->lastEstimation);
	printf("Waiting time = (%d)\n", esi->waitingTime);
	printf("Sentence counter = (%d)\n", esi->sentenceCounter);
	printf("Locked keys =\n");

	if (list_is_empty(esi->lockedKeys)) {
		printf("There are no locked keys \n");
	} else {
		for (int i = 0; i < list_size(esi->lockedKeys); i++) {
			printf("%s\n", (char*) list_get(esi->lockedKeys, i));
		}
	}

	printf("=============\n");
}

void updateLastBurst(Esi** esi) {
	(*esi)->lastBurst = (*esi)->sentenceCounter;
	(*esi)->sentenceCounter = 0;
}

void printEsiList(t_list* esiList) {
	list_iterate(esiList, &printEsi);
}

void destroyEsi(void * voidEsi) {
	Esi * esi = (Esi*)voidEsi;
	if (esi != NULL)
		list_destroy(esi->lockedKeys);

	free(esi);
}

void destroyKey(void* key){
	if ((char *) key != NULL)
		free(key);
}

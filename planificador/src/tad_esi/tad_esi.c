/*
 * tad_esi.c
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#include "tad_esi.h"

void addWaitingTime(void* esi){

	((Esi*)esi)->waitingTime++;
}

void addWaitingTimeToAll(t_list* esis){
	list_iterate(esis,&addWaitingTime);
}

void reduceWaitingTime(Esi** esi){
	(*esi)->waitingTime--;
}

Esi *createEsi(int id,double initialEstimation,int socketConection){
	Esi* newEsi = malloc(sizeof(Esi));
	newEsi->id = id;
	newEsi->lastBurst = 0;
	newEsi->socketConection = socketConection;
	newEsi->waitingTime = 0;
	newEsi->lastEstimation = initialEstimation;
	newEsi->lockedKeys = list_create();
	return newEsi;
}

int id(Esi* esi){
	return esi->id;
}


void addLockedKeyToEsi(char** key, Esi** esi){
	list_add((*esi)->lockedKeys,(void*)*key);
}
void removeLockedKey(char* key, Esi* esi){
	bool keyCompare(void* takenKeys){
		if(string_equals_ignore_case((char*)takenKeys,key)){
			return true;
		}
		return false;
	}
	void destroyer(void* element){
		free(element);
	}
	list_remove_by_condition(esi->lockedKeys,&keyCompare);
}

void printEsi(Esi* esi){
	printf("=============");
	printf("ID = (%d)\n",esi->id);
	printf("Socked = (%d)\n",esi->socketConection);
	printf("Last burst = (%f)\n",esi->lastBurst);
	printf("Last estimation = (%f)\n",esi->lastEstimation);
	printf("Waiting time = (%d)\n",esi->waitingTime);
	printf("Locked keys =\n");
	for(int i = 0;i<list_size(esi->lockedKeys);i++){
		printf("%s\n",(char*)list_get(esi->lockedKeys,i));
	}
	printf("=============");
}

void updateLastBurst(int burst,Esi** esi){
	(*esi)->lastBurst = burst;
}


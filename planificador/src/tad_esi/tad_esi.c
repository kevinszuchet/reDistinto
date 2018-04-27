/*
 * tad_esi.c
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#include "tad_esi.h"

void addWaitingTime(Esi* esi){
	esi->waitingTime++;
}

void addWaitingTimeToAll(Esi** esiList){
	int i = 0;
	while(esiList[i]){
		addWaitingTime(esiList[i]);
		i++;
	}
}

Esi *createEsi(int id,double initialEstimation,int socketConection){
	Esi* newEsi = malloc(sizeof(Esi));
	newEsi->id = id;
	newEsi->lastBurst = 0;
	newEsi->socketConection = socketConection;
	newEsi->waitingTime = 0;
	newEsi->lastEstimation = initialEstimation;
	return newEsi;
}

int id(Esi* esi){
	return esi->id;
}

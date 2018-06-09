/*
 * tad_esi.h
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_TAD_ESI_TAD_ESI_H_
#define SRC_TAD_ESI_TAD_ESI_H_

#include<stdlib.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include<commons/string.h>
#include <stdio.h>

typedef struct Esi{
	int id;
	double lastBurst;
	int waitingTime;
	int socketConection;
	double lastEstimation;
	t_list* lockedKeys;

}Esi;

void printEsi(void* esiToPrint);
void printEsiList(t_list* esiList);

void addWaitingTime(void* esi);

void addWaitingTimeToAll(t_list* esis);
void updateLastBurst(int burst,Esi** esi);
void reduceWaitingTime(Esi** esi);
Esi *createEsi(int id,double initialBurst,int socketConection);

int id(Esi* esi);

void addLockedKeyToEsi(char** key, Esi** esi);
void removeLockedKey(char* key, Esi* esi);

#endif /* SRC_TAD_ESI_TAD_ESI_H_ */

/*
 * planif_algorithm.c
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#include "planif_algorithm.h"

t_log* logger;

typedef bool (*Comparator)(void*,void*); //Used this to not repeat code (high order function)

int alpha;



//If A win or draw B return 1, otherwise 0
bool comparatorSJFSD(void* esi_A,void* esi_B){
	Esi* esiA = (Esi*)esi_A;
	Esi* esiB = (Esi*)esi_B;
	double nextEstimationA = getEstimation(esiA);
	double nextEstimationB = getEstimation(esiB);
	if(nextEstimationA<=nextEstimationB){
		return 1;
	}else{
		return 0;
	}
}

double getEstimation(Esi* esi){
	double estimation = esi->lastBurst*alpha/100 + esi->lastEstimation * (1-alpha)/100;
	return estimation;
}

Esi* nextEsiByAlgorithm(char* algorithm,int alphaReceived, t_list* esiList){
	printf("Starting...\n");
	alpha = alphaReceived;
	Comparator comparatorToUse;
	printf("Algorithm to use =%s\n",algorithm);
	if(strcmp(algorithm,"SJF-SD")==0){
		printf("Use SJF-SD\n");
		comparatorToUse= &comparatorSJFSD;
	}else if(strcmp(algorithm,"SJF-CD")==0){

	}else{
		//HRRN

	}
	list_sort(esiList,comparatorToUse);
	printf("List sorted\n");
	Esi* nextEsi = list_get(esiList,0);
	nextEsi->lastEstimation = getEstimation(nextEsi); //When an ESI is selected to run, override last estimation
	return nextEsi;
}

/*
 * planif_algorithm.c
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#include "planif_algorithm.h"


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

bool comparatorHRRN(void* esi_A,void* esi_B){
	Esi* esiA = (Esi*)esi_A;
	Esi* esiB = (Esi*)esi_B;
	double nextResponseRatioA =(esiA->waitingTime+ getEstimation(esiA))/esiA->waitingTime;
	double nextResponseRatioB =(esiB->waitingTime+ getEstimation(esiB))/esiB->waitingTime;
	if(nextResponseRatioA>=nextResponseRatioB){
		return 1;
	}else{
		return 0;
	}
}


double getEstimation(Esi* esi){
	double estimation = esi->lastBurst*alpha/100 + esi->lastEstimation * (100-alpha)/100;
	return estimation;
}

Esi* simulateAlgoithm(char* algorithm,int alphaReceived, t_list* esiList){
	printf("Starting simulation...\n");
	alpha = alphaReceived;
	Comparator comparatorToUse;
	printf("Algorithm to use =%s\n",algorithm);
	if(strcmp(algorithm,"SJF-SD")==0){
		comparatorToUse= &comparatorSJFSD;
	}else if(strcmp(algorithm,"SJF-CD")==0){
		comparatorToUse= &comparatorSJFSD;
	}else{
		comparatorToUse= &comparatorHRRN;
	}
	list_sort(esiList,comparatorToUse);
	printf("List sorted\n");
	printEsiList(esiList);
	Esi* nextEsi = list_get(esiList,0);
	return nextEsi;
}

Esi* nextEsiByAlgorithm(char* algorithm,int alphaReceived, t_list* esiList){

	Esi* nextEsi = simulateAlgoithm(algorithm,alphaReceived,esiList);

	printf("ID selected = %d\n",nextEsi->id);
	return nextEsi;
}

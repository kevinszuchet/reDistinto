/*
 * planif_algorithm.h
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_PLANIF_ALGORITHM_PLANIF_ALGORITHM_H_
#define SRC_PLANIF_ALGORITHM_PLANIF_ALGORITHM_H_

#include "../tad_esi/tad_esi.h"
#include <commons/collections/list.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <commons/log.h>

bool comparatorSJFSD(void* esi_A, void* esi_B);
bool comparatorHRRN(void* esi_A, void* esi_B);

double getResponseRatio(Esi* esi);
double getEstimation(Esi* esi);

Esi* simulateAlgoithm(char* algorithm, int alphaReceived, t_list* esiList);
Esi* nextEsiByAlgorithm(char* algorithm, int alphaReceived, t_list* esiList);

#endif /* SRC_PLANIF_ALGORITHM_PLANIF_ALGORITHM_H_ */

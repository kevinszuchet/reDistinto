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

Esi* nextEsiByAlgorithm(char* algorithm,int alphaReceived, t_list* esiList);
double getEstimation(Esi* esi);

#endif /* SRC_PLANIF_ALGORITHM_PLANIF_ALGORITHM_H_ */

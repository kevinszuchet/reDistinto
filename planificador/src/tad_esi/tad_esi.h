/*
 * tad_esi.h
 *
 *  Created on: 23 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_TAD_ESI_TAD_ESI_H_
#define SRC_TAD_ESI_TAD_ESI_H_

typedef struct Esi{
	int id;
	double lastBurst;
	int waitingTime = 0;
}Esi;

void addWaitingTime(Esi*);



#endif /* SRC_TAD_ESI_TAD_ESI_H_ */

/*
 * semaphores.h
 *
 *  Created on: 1 jun. 2018
 *      Author: utnso
 */

#ifndef SEMAPHORES_SEMAPHORES_H_
#define SEMAPHORES_SEMAPHORES_H_

//#include <sys/types.h>
	//#include <sys/stat.h>
	//#include <sys/ipc.h>
#include <sys/sem.h>
#include <errno.h>
#include <stdlib.h>

void initSem(int semid, int numSem, int valor);
void doWait(int semid, int numSem);
void doSignal(int semid, int numSem);

#endif /* SEMAPHORES_SEMAPHORES_H_ */

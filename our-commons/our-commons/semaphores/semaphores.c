/*
 * semaphores.c
 *
 *  Created on: 1 jun. 2018
 *      Author: utnso
 */
#include "semaphores.h"

void doSignal(int semid, int numSem) {
    struct sembuf sops; //Signal
    sops.sem_num = numSem;
    sops.sem_op = 1;
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) == -1) {

        printf("Couldn't do signal");
    }
}

void doWait(int semid, int numSem) {
    struct sembuf sops;
    sops.sem_num = numSem; /* Sobre el primero, ... */
    sops.sem_op = -1; /* ... un wait (resto 1) */
    sops.sem_flg = 0;

    if (semop(semid, &sops, 1) == -1) {
    	printf("Couldn't do wait");
    }
}

void initSem(int semid, int numSem, int valor) { //iniciar un semaforo

    if (semctl(semid, numSem, SETVAL, valor) < 0) {

    	printf("Couldn't initialize semaphore");
    }
}

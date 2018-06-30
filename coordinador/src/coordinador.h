/*
 * coordinador.h
 *
 *  Created on: 21 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_COORDINADOR_H_
#define SRC_COORDINADOR_H_

	#include <stdbool.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/socket.h>
	#include <unistd.h>
	#include <our-commons/sockets/server.h>
	#include <our-commons/modules/names.h>
	#include <our-commons/messages/operation_codes.h>
	#include <our-commons/messages/serialization.h>
	#include <our-commons/tads/tads.h>
	#include <commons/config.h>
	#include <commons/collections/list.h>
	#include <commons/log.h>
	#include <pthread.h>
	#include <semaphore.h>
	#include "submodules/instancia/instanciaFunctions.h"

	#define  CFG_FILE "../coordinador.cfg"

	typedef struct EsiRequest{
		int id;
		int socket;
		Operation* operation;
	}EsiRequest;

	t_log* logger;
	int cantEntry;
	int entrySize;

	EsiRequest* actualEsiRequest;
	t_list* instancias;

	pthread_mutex_t instanciasListMutex;
	pthread_mutex_t lastInstanciaChosenMutex;
	char instanciaResponseStatus;
	sem_t instanciaResponse;
	char* valueFromKey;
	char instanciaStatusFromValueRequest;
	sem_t valueFromKeyInstanciaSemaphore;

	char* getOperationName(Operation* operation);
	void showConfig(int listeningPort);
	void getConfig(int* listeningPort);

	int welcomePlanificador(int coordinadorSocket, int planificadorSocket);
	void freeResources();
	void planificadorFell();

#endif /* SRC_COORDINADOR_H_ */

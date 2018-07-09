/*
 * esi.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_ESI_H_
#define SRC_ESI_H_

	#include <our-commons/sockets/client.h>
	#include <our-commons/modules/names.h>
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <parsi/parser.h>
	#include <our-commons/tads/tads.h>
	#include <our-commons/messages/serialization.h>

	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>

	#define  CFG_FILE "../esi.cfg"
	void getConfig(char** ipCoordinador, char** ipPlanificador, int* portCoordinador, int* portPlanificador);
	void waitPlanificadorOrders(int planificadorSocket, char * script, int coordinadorSocket);
	void tryToExecute(int planificadorSocket, char * line, int coordinadorSocket, int * esiPC, size_t len);
	void interpretateOperation(Operation * operation, char * line);
	void initializeOperation(Operation * operation, char operationCode, char * key, char * value);
	void initializeOperationResponse(OperationResponse * operationResponse, char coordinadorResponse, char status);
	void destroy_operation(Operation * operation);
	char * getLoggerPath(char * argv[]);
	int numberOfElementsOnArray(char ** array) {
		int size = 0;
		for (; array[size] != NULL; size++);
		return size;
	}

	int empty_string(char * str) {
		return (str != NULL && strcmp(str, "") != 0 ? 0 : 1);
	}
#endif /* SRC_ESI_H_ */

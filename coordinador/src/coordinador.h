/*
 * coordinador.h
 *
 *  Created on: 21 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_COORDINADOR_H_
#define SRC_COORDINADOR_H_

	#include <our-commons/sockets/server.h>
	#include <our-commons/modules/names.h>
	#include <our-commons/messages/operation_codes.h>
	#include <our-commons/tads/tads.h>
	#include <commons/config.h>
	#include <commons/collections/list.h>
	#include <commons/log.h>

	#define  CFG_FILE "../coordinador.cfg"

	#define EXECUTION_ERROR -1

	typedef struct Instancia{
		int id;
		int socket;
		int spaceUsed;
		char firstLetter;
		char lastLetter;
	}Instancia;

	typedef struct EsiRequest{
		Operation* operation;
		int socket;
	}EsiRequest;

	void getConfig(int* listeningPort, char** algorithm, int* cantEntry, int* entrySize, int* delay);

	int welcomePlanificador(int coordinadorSocket);
	int clientHandler(int clientSocket);

#endif /* SRC_COORDINADOR_H_ */

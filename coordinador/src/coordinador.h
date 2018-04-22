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
	#include <commons/config.h>

	#define  CFG_FILE "../coordinador.cfg"

	void getConfig(int* listeningPort, char** algorithm, int* cantEntry, int* entrySize, int* delay);

	int welcomePlanificador(int coordinadorSocket);
	int clientHandler(int clientSocket);

#endif /* SRC_COORDINADOR_H_ */

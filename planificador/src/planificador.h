/*
 * planificador.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_PLANIFICADOR_H_
#define SRC_PLANIFICADOR_H_

	#include <our-commons/sockets/client.h>
	#include <our-commons/sockets/server.h>
	#include <our-commons/modules/names.h>
	#include "console/console.h"
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/collections/dictionary.h>
<<<<<<< HEAD
	#include <pthread.h>
=======
	#include <commons/collections/queue.h>
>>>>>>> dab17bdb35f211cc108bf330bf3f1c789388d459

	#define  CFG_FILE "../planificador.cfg"
	#define USERBLOCKED -1

	t_dictionary* blockedKeysDic;

	int listeningPort;
	char* algorithm;
	int initialEstimation;
	char* ipCoordinador;
	int portCoordinador;
	char** blockedKeys;

	void getConfig(int* listeningPort, char** algorithm, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);

	int welcomeEsi();
	int welcomeCoordinador();
	void addConfigurationBlockedKeys(char**);
#endif /* SRC_PLANIFICADOR_H_ */

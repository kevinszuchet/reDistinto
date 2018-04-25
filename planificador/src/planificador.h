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
	#include <commons/collections/queue.h>
	#include <commons/collections/list.h>
	#include <pthread.h>
	#include "tad_esi/tad_esi.h" //Necessary to control ESIs
	#include "planif_algorithm/planif_algorithm.h" //Necessary to delegate planification algorithm stuff

	#define  CFG_FILE "../planificador.cfg"
	#define USERBLOCKED -1


	t_dictionary* blockedEsiDic;
	t_list* readyEsis;
	t_list* finishedEsis;
	Esi* runningEsi;
	int listeningPort;
	char* algorithm;
	int alphaEstimation;
	int initialEstimation;
	char* ipCoordinador;
	int portCoordinador;
	char** blockedKeys;

	void generateTestEsi();
	void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation,int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);


	int welcomeEsi();
	int welcomeCoordinador();
	void addConfigurationBlockedKeys(char**);
#endif /* SRC_PLANIFICADOR_H_ */

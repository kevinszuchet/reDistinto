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
	#include <our-commons/messages/operation_codes.h>
	#include "console/console.h"
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/collections/dictionary.h>
	#include <commons/collections/queue.h>
	#include <commons/collections/list.h>
	#include <pthread.h>
	#include "tad_esi/tad_esi.h" //Necessary to control ESIs
	#include "planif_algorithm/planif_algorithm.h" //Necessary to delegate planification algorithm stuff
	#include <commons/log.h>


	#define  CFG_FILE "../planificador.cfg"
	#define CONFIG_BLOCKED 0


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

	typedef struct{
		char* key;
		int operation;
	}key_operation;

	typedef struct{
		t_queue* blockedEsis;
		int resourceTakerID;
	}blocked_queue;

	void handleEsiInformation(int esiExecutionInformation,key_operation keyOp);
	void receiveCoordinadorMessage(key_operation* keyOp);
	int waitEsiInformation(int esiSocket);
	void sendKeyStatusToCoordinador(int status);
	void sendMessageExecuteToEsi(Esi* nextEsi);

	void generateTestEsi();
	void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation,int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);

	void blockKey(char* keyToBlock, int esiBlocked);
	int checkKeyBlocked(char* keyRecieved);
	int isTakenResource(char* key);

	int welcomeEsi();
	int welcomeNewClients();
	void addConfigurationBlockedKeys(char**);
	int handleConcurrence(int listenerPort);
#endif /* SRC_PLANIFICADOR_H_ */

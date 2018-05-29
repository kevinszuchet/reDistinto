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
	#include <our-commons/messages/serialization.h>
	#include <our-commons/tads/tads.h>

	#define  CFG_FILE "../planificador.cfg"
	#define CONSOLE_BLOCKED 0
	#define KEYFREE 1
	#define KEYBLOCKED 0

	t_dictionary* blockedEsiDic;
	t_list* readyEsis;
	t_list* finishedEsis;
	Esi* runningEsi;

	t_dictionary* takenResources;

	int listeningPort;
	char* algorithm;
	int alphaEstimation;
	int initialEstimation;
	char* ipCoordinador;
	int portCoordinador;
	char** blockedKeys;




	//DUMMIE FUNCTIONS
	void sendKeyStatusToCoordinadorDummie(char status);
	void sendMessageExecuteToEsiDummie(Esi* nextEsi);
	char waitEsiInformationDummie(int esiSocket);
	void sendEsiIdToCoordinador(int esiID);

	void executionProcedure();

	void handleEsiInformation(OperationResponse* esiExecutionInformation,char* keyOp);

	OperationResponse *waitEsiInformation(int esiSocket);
	void sendKeyStatusToCoordinador(char* key);
	void sendMessageExecuteToEsi(Esi* nextEsi);

	void generateTestEsi();
	void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation,int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);

	void moveFromRunningToReady(Esi* esi);

	void freeResource(char* key,Esi* esiTaker);
	void takeResource(char* keyToLock, int esiTaker);

	void blockEsi(char* lockedResource, int esiBlocked);
	int checkKeyBlocked(char* keyRecieved);
	char isTakenResource(char* key);
	void removeFromReady(Esi* esi);

	void addEsiToReady(Esi* esi);
	int welcomeEsi();
	int welcomeNewClients();
	void addConfigurationLockedKeys(char**);
	int handleConcurrence(int listenerPort);
#endif /* SRC_PLANIFICADOR_H_ */

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
	#include <semaphore.h>

	#define  CFG_FILE "../planificador.cfg"
	#define CONSOLE_BLOCKED 0
	#define KEYFREE 1
	#define KEYBLOCKED 0
	#define PAUSE 0
	#define CONTINUE 1

	#define INREADYLIST 'a'
	#define INFINISHEDLIST 's'
	#define INRUNNING 'd'
	#define INBLOCKEDDIC 'e'
	#define NOWHERE 'f'

	int pauseState;

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

	t_list* instruccionsByConsoleList;
	sem_t pauseStateSemaphore;

	int coordinadorSocket;

	//DUMMIE FUNCTIONS
	void sendKeyStatusToCoordinadorDummie(char status);
	void sendMessageExecuteToEsiDummie(Esi* nextEsi);
	char waitEsiInformationDummie(int esiSocket);
	void sendEsiIdToCoordinador(int esiID);

	void executionProcedure();

	void executeConsoleInstruccions();
	void handleEsiInformation(OperationResponse* esiExecutionInformation,char* keyOp);

	void abortEsi(Esi* esi);
	void removeFdFromSelect(int socket);
	Esi* getEsiBySocket(int socket);
	char getEsiPlaceBySocket(int socket);
	OperationResponse *waitEsiInformation(int esiSocket);
	void sendKeyStatusToCoordinador(char* key);
	void sendMessageExecuteToEsi(Esi* nextEsi);

	void generateTestEsi();
	void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation,int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);

	void deleteEsiFromSystemBySocket(int socket);
	void moveFromRunningToReady(Esi* esi);

	void freeTakenKeys(Esi* esi);
	void freeKey(char* key,Esi* esiTaker);
	void lockKey(char* keyToLock, int esiTaker);

	void exitPlanificador();
	Esi* getEsiById(int id);
	bool mustDislodgeRunningEsi();
	void dislodgeEsi(Esi* esi,bool moveToReady);

	//Keys Functions
	void blockEsi(char* lockedKey, int esiBlocked);
	char isLockedKey(char* key);
	void addKeyToGeneralKeys(char* key);
	void unlockEsi(char* key);

	//Place in system functions
	void removeFromReady(Esi* esi);
	void addEsiToReady(Esi* esi);

	//Other functions
	void addConfigurationLockedKeys(char**);
	void destroyer(void* element);

	//Conection Functions
	void welcomeEsi();
	int welcomeNewClients();
	int handleConcurrence();
#endif /* SRC_PLANIFICADOR_H_ */

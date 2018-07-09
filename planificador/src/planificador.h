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

	t_log * logger;

	int pauseState;

	t_dictionary* blockedEsiDic;
	t_list* readyEsis;
	t_list* finishedEsis;
	Esi* runningEsi;

	t_list* allSystemTakenKeys;
	t_list* allSystemKeys;
	t_list* allSystemEsis;

	int listeningPort;
	char* algorithm;
	int alphaEstimation;
	int initialEstimation;
	char* ipCoordinador;
	int portCoordinador;
	char** blockedKeys;

	pthread_t threadConsole;
	pthread_t threadConsoleInstructions;

	pthread_mutex_t executionMutex;




	int sentenceCounter;

	int actualID;

	int coordinadorSocket;

	bool finishedExecutingInstruccion;

	char* globalKey;
	Esi* globalEsi;

	void initializePlanificador();
	void getConfig(int* listeningPort, char** algorithm, int* alphaEstimation, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys);

	// Console functions
	void executeConsoleInstruccions();

	void executeInstruccion();

	// REVIEW donde se usa esta funcion? es necesario el prototipo aca?
	void removeFdFromSelect(int socket);
	void showBlockedEsisInKey(char* key);

	// Algorithms (dispatcher) functions
	void addEsiToReady(Esi* esi);
	void removeFromReady(Esi* esi);
	void moveFromRunningToReady(Esi* esi);
	void moveEsiToRunning(Esi* esiToRun);
	void takeRunningEsiOut();

	void dislodgeEsi(Esi* esi, bool addToReady);
	bool mustDislodgeRunningEsi();

	void addToFinishedList(Esi* finishedEsi);
	void finishEsi(Esi* esiToFinish);

	// Coordinador functions
	void sendKeyStatusToCoordinador(char* key);
	void sendEsiIdToCoordinador(int esiId);

	// ESI functions
	Esi* getEsiById(int id);
	Esi* generateEsiStruct(int esiSocket);
	void sendMessageExecuteToEsi(Esi* nextEsi);
	OperationResponse *recieveEsiInformation(int esiSocket);

	Esi* getEsiBySocket(int socket);
	void deleteEsiFromSystem(Esi* esiToDelete);
	void abortEsi(Esi* esi);
	void handleEsiInformation(OperationResponse* esiExecutionInformation, char* key);
	void handleEsiStatus(char esiStatus);
	Esi* getNextEsi();

	// Keys Functions
	void blockEsi(char* lockedKey, int esiBlocked);
	char isLockedKey(char* key);
	void addKeyToGeneralKeys(char* key);
	void unlockEsi(char* key,bool isConsoleInstruccion);
	void freeTakenKeys(Esi* esi);
	void freeKey(char* key, Esi* esiTaker);
	void lockKey(char* key, int esiID);
	bool isValidEsiId(int esiID);

	// Connection Functions
	void welcomeEsi(int clientSocket);
	int welcomeNewClients(int newCoordinadorSocket);
	int handleConcurrence();
	int clientMessageHandler(char clientMessage, int clientSocket);

	// Other functions
	void addConfigurationLockedKeys(char** blockedKeys);
	void exitPlanificador();

	// Destroy functions
	void destroyEsiQueue(void * queueVoid);

#endif /* SRC_PLANIFICADOR_H_ */

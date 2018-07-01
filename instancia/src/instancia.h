/*
 * instancia.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_INSTANCIA_H_
#define SRC_INSTANCIA_H_

	#define IS_EMPTY 0
	#define IS_SET 1
	#define ENTRY_START_ERROR -1
	#define I_NEED_TO_COMPACT -2

	#define MAX_KEY_SIZE 40

	#define CFG_FILE "../instancia.cfg"

	#include <our-commons/sockets/client.h>
	#include <our-commons/modules/names.h>
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/collections/list.h>
	#include "tadEntryTable/tadEntryTable.h"
	#include <our-commons/messages/operation_codes.h>
	#include <our-commons/tads/tads.h>
	#include <our-commons/messages/serialization.h>
	#include "replaceAlgorithms/replaceAlgorithms.h"
	#include "ourList/ourList.h"
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <sys/mman.h>
	#include <sys/stat.h>
	#include <fcntl.h>
	#include <unistd.h>
	#include <pthread.h>

	// Global vars
	int entriesAmount;
	int entrySize;
	t_list * entryTable; // Takes record of the key + how many entraces the value occupies

	char * storage;
	int * biMap;
	char * algorithm;
	char* path;
	int dumpDelay;
	char * name;
	char* ipCoordinador;

	t_log * logger;
	t_log * replaceAlgorithmsLogger;

	void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char** path, char** name, int* dump);

	// Functions
	void sendMyNameToCoordinador(char * name, int coordinadorSocket);
	void receiveCoordinadorConfiguration(int coordinadorSocket);
	void initialize(int entraces, int entryStorage);
	void waitForCoordinadorStatements(int coordinadorSocket);
	char interpretateStatement(Operation * operation);
	void showStorage();
	int finish();

	void biMapInitialize(int entraces);
	void emptyBiMap(int entraces);
	void biMapUpdate(int valueStart, int entriesForValue, int value);

	char set(char *key, char *value);
	int updateKey(char *key, char *value);
	int getStartEntryToSet(int amountOfEtries);
	void storageSet(int initialEntry,  char * value);

	char compact();
	int getTotalSettedEntries();
	void getValue(char * value, int valueStart, int valueSize);
	int getValueStartEntry(char * key);

	void handleDump();
	char dump();
	char store(char *key);
	char * getValueForCoordinador(char * key);
	char getKeyByFile(char * key);

	int wholeUpperDivision(int x, int y);

#endif /* SRC_INSTANCIA_H_ */

/*
 * instancia.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_INSTANCIA_H_

	#define SRC_INSTANCIA_H_
	#define SENTINEL_VALUE "\0"
	#define ENTRY_START_ERROR -1

	#include <our-commons/sockets/client.h>
	#include <our-commons/modules/names.h>
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <commons/collections/dictionary.h>
	#define  CFG_FILE "../instancia.cfg"
	#include "tadEntryTable/tadEntryTable.h"
	void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char** path, char** name, int* dump);

	//Functions
	int initialize(int entraces, int entryStorage);
	int set(char *key, char *value);
	int notifyCoodinador(char *key, char *value, char *operation);
	int dump();
	int compact();
	int updateKey(char *key, char *value);
	int store(char *key);
	int finish();
	void autoCompleteSentinelValue(int amount, char **s);
	int getValueStartEntry(char * key);
	int getStartEntryToSet(int amountOfEtries);
	int wholeUpperDivision(int x, int y);
	int getTotalSettedEntries();
	void storageSet(int initialEntry,  char * value);

#endif /* SRC_INSTANCIA_H_ */

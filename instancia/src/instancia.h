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
	int finish();

	void biMapInitialize(int entraces);
	void emptyBiMap(int entraces);
	void biMapUpdate(int valueStart, int entriesForValue);

	int set(char *key, char *value);
	int updateKey(char *key, char *value);
	int getStartEntryToSet(int amountOfEtries);
	void storageSet(int initialEntry,  char * value);

	int compact();
	int getTotalSettedEntries();
	void getValue(char ** value, int valueStart, int valueSize);
	int getValueStartEntry(char * key);

	int notifyCoodinador(char *key, char *value, char *operation);
	int dump();
	int store(char *key);

	int wholeUpperDivision(int x, int y);

#endif /* SRC_INSTANCIA_H_ */

/*
 * tadEntryTable.h
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#ifndef SRC_TADENTRYTABLE_TADENTRYTABLE_H_
#define SRC_TADENTRYTABLE_TADENTRYTABLE_H_

	#include <stdlib.h>
	#include <string.h>
	#include <stdio.h>
	#include <stdbool.h>

	typedef struct entryTableInfo{
		char * key;
		int valueStart;
		int valueSize;
		int lastReference;
	} entryTableInfo;

	int currentReference;

	void createTableInfo(entryTableInfo * entryInfo, char * key, int valueEntryStart, int valueTotalSize);

	char * getKey(entryTableInfo * entryInfo);
	void setKey(entryTableInfo * entryInfo, char * key);

	int getValueStart(entryTableInfo *entryInfo);
	void setValueStart(entryTableInfo * entryInfo, int valueStart);

	int getValueSize(entryTableInfo *entryInfo);

	int getLastReference(entryTableInfo *entryInfo);
	void increaseLastReference(entryTableInfo *entryInfo);

	void destroyTableInfo(entryTableInfo * entryInfo);

	bool hasKey(void * entryInfo, void * key);

#endif /* SRC_TADENTRYTABLE_TADENTRYTABLE_H_ */

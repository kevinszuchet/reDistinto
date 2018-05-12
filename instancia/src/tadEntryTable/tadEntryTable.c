/*
 * tadEntryTable.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "tadEntryTable.h"

void createTableInfo(entryTableInfo * entryInfo, int valueEntryStart, int valueTotalSize){
	entryInfo->valueStart = valueEntryStart;
	entryInfo->valueSize = valueTotalSize;
}

int getValueStart(entryTableInfo * entryInfo){
	return entryInfo->valueStart;
}

void setValueStart(entryTableInfo * entryInfo, int valueStart){
	entryInfo->valueStart = valueStart;
}

int getValueSize(entryTableInfo * entryInfo){
	return entryInfo->valueSize;
}

void destroyTableInfo(entryTableInfo * entryInfo){
	free(entryInfo);
}

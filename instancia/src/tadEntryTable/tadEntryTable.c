/*
 * tadEntryTable.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "tadEntryTable.h"

entryTableInfo *createTableInfo(int valueEntryStart, int valueTotalSize){
	entryTableInfo * entryInfo = malloc(sizeof(entryTableInfo));
	entryInfo->valueStart = valueEntryStart;
	entryInfo->valueSize = valueTotalSize;
	return entryInfo;
}
int getValueStart(entryTableInfo * entryInfo){
	return entryInfo->valueStart;
}
int getValueSize(entryTableInfo * entryInfo){
	return entryInfo->valueSize;
}

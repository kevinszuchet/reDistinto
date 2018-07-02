/*
 * tadEntryTable.c
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#include "tadEntryTable.h"

void createTableInfo(entryTableInfo * entryInfo, char * key, int valueEntryStart, int valueTotalSize) {
	setKey(entryInfo, key);
	entryInfo->valueStart = valueEntryStart;
	entryInfo->valueSize = valueTotalSize;
	entryInfo->lastReference = 0;
}

char * getKey(entryTableInfo * entryInfo) {
	return entryInfo->key;
}

void setKey(entryTableInfo * entryInfo, char * key) {
	entryInfo->key = malloc(strlen(key) + 1);
	strcpy(entryInfo->key, key);
}

int getValueStart(entryTableInfo * entryInfo) {
	return entryInfo->valueStart;
}

void setValueStart(entryTableInfo * entryInfo, int valueStart) {
	entryInfo->valueStart = valueStart;
}

int getValueSize(entryTableInfo * entryInfo) {
	return entryInfo->valueSize;
}

int getLastReference(entryTableInfo *entryInfo) {
	return entryInfo->lastReference;
}

void increaseLastReference(entryTableInfo *entryInfo) {
	entryInfo->lastReference = currentReference;
	currentReference++;
}

bool hasKey(void * entryInfo, void * key) {
	return strcmp(getKey((entryTableInfo *) entryInfo), (char *) key) == 0;
}

void destroyTableInfo(entryTableInfo * entryInfo) {
	free(entryInfo->key);
	free(entryInfo);
}

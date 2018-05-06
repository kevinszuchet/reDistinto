/*
 * tadEntryTable.h
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#ifndef SRC_TADENTRYTABLE_TADENTRYTABLE_H_
#define SRC_TADENTRYTABLE_TADENTRYTABLE_H_

#include<stdlib.h>

typedef struct entryTableInfo{
	int valueStart;
	int valueSize;
}entryTableInfo;

entryTableInfo * createTableInfo(int valueEntryStart, int valueTotalSize);
int getValueStart(entryTableInfo *entryInfo);
int getValueSize(entryTableInfo *entryInfo);

#endif /* SRC_TADENTRYTABLE_TADENTRYTABLE_H_ */

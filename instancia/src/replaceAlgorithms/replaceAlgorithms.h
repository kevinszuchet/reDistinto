/*
 * replaceAlgorithms.h
 *
 *  Created on: 20 may. 2018
 *      Author: utnso
 */

#ifndef SRC_REPLACEALGORITHMS_H_
#define SRC_REPLACEALGORITHMS_H_

	#include "../instancia.h"
	#include "../tadEntryTable/tadEntryTable.h"
	#include "../ourList/ourList.h"

	void updateAccodringToAlgorithm(char * key);
	void deleteAccodringToAlgorithm();

	void deleteKey(entryTableInfo * toBeDeletedEntryInfo);

	void updateUsage(char * key);

	bool leastRecentlyUsedComparator(void * currentData, void * selectedData);
	bool biggestSpaceUsedComparator(void * currentData, void * selectedData);

	bool atomicEntry(void * entryInfo);

#endif /* SRC_REPLACEALGORITHMS_H_ */

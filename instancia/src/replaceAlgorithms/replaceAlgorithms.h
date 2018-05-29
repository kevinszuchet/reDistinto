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

	entryTableInfo * getPointedKey();

	void updateUsage(char * key);

	bool leastRecentlyUsedComparator(entryTableInfo * currentData, entryTableInfo * selectedData);
	bool biggestSpaceUsedComparator(entryTableInfo * currentData, entryTableInfo * selectedData);

	void findElementBy(entryTableInfo ** toBeDeletedElement, bool (*comparator)(void*, void*));

	bool atomicEntry(entryTableInfo * entryInfo);
	bool bothEntriesAreAtomics(entryTableInfo * currentData, entryTableInfo * selectedData);

#endif /* SRC_REPLACEALGORITHMS_H_ */

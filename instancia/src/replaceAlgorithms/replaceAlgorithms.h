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

	int initializeAccordingToAlgorithm();
	int updateAccodringToAlgorithm(char * key);
	int deleteAccodringToAlgorithm();

	void deleteKey(t_hash_element *elem);

	void findNextValidPointer(t_hash_element * elem, int * index);
	int initializePointer(t_hash_element * elem, int * index);

	void pointToNextKey();
	t_hash_element * getPointedKey();

	void updateUsage(char * key);

	t_hash_element * getLeastRecentlyUsedKey();
	t_hash_element * getBiggestSpaceUsedKey();

	// global vars

	t_hash_element * entryTableElement;
	int entryTableIndex;

#endif /* SRC_REPLACEALGORITHMS_H_ */

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

	// global vars

	t_hash_element * entryTableElement;
	int entryTableIndex;

#endif /* SRC_REPLACEALGORITHMS_H_ */

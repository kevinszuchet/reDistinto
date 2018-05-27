/*
 * replaceAlgorithms.c
 *
 *  Created on: 20 may. 2018
 *      Author: utnso
 */

#include "replaceAlgorithms.h"

void updateAccodringToAlgorithm(char * key) {

	if (strcmp(algorithm, "LRU") == 0) {
		log_info(replaceAlgorithmsLogger, "Update: Least Recently Used Algorithm");
		updateUsage(key);
	}

	//CIRC doesn't need Updates
	//BSU doesn't need Updates
}

void deleteAccodringToAlgorithm() {
	entryTableInfo * toBeDeletedElement;

	if (strcmp(algorithm, "CIRC") == 0) {
		toBeDeletedElement = getPointedKey();
	}

	// TODO passing argument 2 of ‘findKeyBy’ from incompatible pointer type [-Wincompatible-pointer-types]

	else if (strcmp(algorithm, "LRU") == 0) {
		findElementBy(&toBeDeletedElement, leastRecentlyUsedComparator);
	}

	else {
		findElementBy(&toBeDeletedElement, biggestSpaceUsedComparator);
	}

	deleteKey(toBeDeletedElement);
}

void deleteKey(entryTableInfo * toBeDeletedEntryInfo) {

	log_info(replaceAlgorithmsLogger, "the key %s is about to be deleted", toBeDeletedEntryInfo->key);

	biMapUpdate(toBeDeletedEntryInfo->valueStart, wholeUpperDivision(toBeDeletedEntryInfo->valueSize, entrySize), IS_EMPTY);
	list_remove_and_destroy_by_condition_with_param(entryTable, (void *) hasKey, (void *) destroyTableInfo, toBeDeletedEntryInfo->key);

	// REVIEW se libera cuando hago el remove?

	log_info(replaceAlgorithmsLogger, "the key was successfully deleted");
}


/************************************************************Round Algorithm******************************************************/
entryTableInfo * getPointedKey() {
	t_list * atomicEntriesList = list_filter(entryTable, atomicEntry);
	entryTableInfo * data = atomicEntriesList->head->data;
	list_destroy(atomicEntriesList);
	return data;
}
/************************************************************Round Algorithm******************************************************/

/*****************************************************Least Recently Used Algorithm***********************************************/

void updateUsage(char * key) {

	t_link_element *element = entryTable->head;

	while (element != NULL) {

		if (strcmp(getKey(element->data), key) == 0) {

			setUsageToZero(element->data);
			log_info(replaceAlgorithmsLogger, "");
		}
		else {
			increaseKeyUsage(element->data);
		}
		element = element->next;
	}
}

bool leastRecentlyUsedComparator(entryTableInfo * currentData, entryTableInfo * selectedData) {
	return getKeyUsage(currentData) > getKeyUsage(selectedData) && bothEntriesAreAtomics(currentData, selectedData);
}

/*****************************************************Least Recently Used Algorithm***********************************************/

/******************************************************Biggest Space Used Algorithm***********************************************/

bool biggestSpaceUsedComparator(entryTableInfo * currentData, entryTableInfo * selectedData) {
	return getValueSize(currentData) > getValueSize(selectedData) && bothEntriesAreAtomics(currentData, selectedData);
}
/*****************************************************Biggest Space Used Algorithm************************************************/

void findElementBy(entryTableInfo ** toBeDeletedElement, bool (*comparator)(void*, void*)) {
	t_list * auxList = list_duplicate(entryTable);
	list_sort(auxList, comparator);
	*toBeDeletedElement = list_get(entryTable, 0);
	// REVIEW hace falta destruir todos sus elementos o esto destruiria los elementos de la lista original?
	list_destroy(auxList);
}

bool atomicEntry(entryTableInfo * entryInfo) {
	return entryInfo->valueSize <= entrySize;
}

bool bothEntriesAreAtomics(entryTableInfo * oneEntryInfo, entryTableInfo * otherEntryInfo) {
	return atomicEntry(oneEntryInfo) && atomicEntry(otherEntryInfo);
}

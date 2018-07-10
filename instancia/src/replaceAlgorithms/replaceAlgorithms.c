/*
 * replaceAlgorithms.c
 *
 *  Created on: 20 may. 2018
 *      Author: utnso
 */

#include "replaceAlgorithms.h"

void updateAccodringToAlgorithm(char * key) {

	if (strcmp(algorithm, "LRU") == 0) {
		log_info(logger, "Update: Least Recently Used Algorithm");
		updateUsage(key);
	}

	//CIRC doesn't need Updates
	//BSU doesn't need Updates
}

void deleteAccodringToAlgorithm() {
	entryTableInfo * toBeDeletedElement;
	t_list * atomicEntriesList = list_filter(entryTable, atomicEntry);

	if (strcmp(algorithm, "CIRC") == 0) {
		//toBeDeletedElement = atomicEntriesList->head->data;
	}

	else {

		if (strcmp(algorithm, "LRU") == 0) {
			list_sort(atomicEntriesList, leastRecentlyUsedComparator);
		}

		else {
			list_sort(atomicEntriesList, biggestSpaceUsedComparator);
		}
	}

	if (atomicEntriesList->head != NULL) {
		toBeDeletedElement = atomicEntriesList->head->data;
		deleteKey(toBeDeletedElement);
	}
	else{
		log_warning(logger, "There are no atomic entires to be deleted");
	}
	list_destroy(atomicEntriesList);
}

void deleteKey(entryTableInfo * toBeDeletedEntryInfo) {

	log_info(logger, "the key %s is about to be deleted", toBeDeletedEntryInfo->key);

	biMapUpdate(toBeDeletedEntryInfo->valueStart, wholeUpperDivision(toBeDeletedEntryInfo->valueSize, entrySize), IS_EMPTY);

	// REVIEW se libera cuando hago el remove?
	list_remove_and_destroy_by_condition_with_param(entryTable, toBeDeletedEntryInfo->key, (void *) hasKey, (void *) destroyTableInfo);
	log_info(logger, "the key was successfully deleted");
}

void updateUsage(char * key) {
	t_link_element * findedElement = list_find_with_param(entryTable, key, hasKey);

	if (findedElement != NULL) {
		increaseLastReference(findedElement->data);
	}
}

bool leastRecentlyUsedComparator(void * currentData, void * selectedData) {
	return getLastReference((entryTableInfo *) currentData) < getLastReference((entryTableInfo *) selectedData);
}

bool biggestSpaceUsedComparator(void * currentData, void * selectedData) {
	return getValueSize((entryTableInfo *) currentData) > getValueSize((entryTableInfo *) selectedData);
}

bool atomicEntry(void * voidEntryInfo) {
	entryTableInfo * entryInfo = (entryTableInfo *) voidEntryInfo;
	return entryInfo->valueSize <= entrySize;
}

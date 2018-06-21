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

	// REVIEW hace falta destruir todos sus elementos o esto destruiria los elementos de la lista original?
	list_destroy(atomicEntriesList);
}

void deleteKey(entryTableInfo * toBeDeletedEntryInfo) {

	log_info(replaceAlgorithmsLogger, "the key %s is about to be deleted", toBeDeletedEntryInfo->key);

	biMapUpdate(toBeDeletedEntryInfo->valueStart, wholeUpperDivision(toBeDeletedEntryInfo->valueSize, entrySize), IS_EMPTY);
	list_remove_and_destroy_by_condition_with_param(entryTable, toBeDeletedEntryInfo->key, (void *) hasKey, (void *) destroyTableInfo);

	// REVIEW se libera cuando hago el remove?

	log_info(replaceAlgorithmsLogger, "the key was successfully deleted");
}

void updateUsage(char * key) {

	t_link_element *element = entryTable->head;
	int found = 0;
	while (element != NULL && found == 0) {

		if (strcmp(getKey(element->data), key) == 0) {

			log_info(replaceAlgorithmsLogger, "The usage from key: %s is: %d", getKey(element->data), getKeyUsage(element->data));
			increaseKeyUsage(element->data);
			log_info(replaceAlgorithmsLogger, "The usage from key: %s has been updated. Usage: %d", getKey(element->data), getKeyUsage(element->data));

			found = 1;
		}

		element = element->next;
	}
}

bool leastRecentlyUsedComparator(void * currentData, void * selectedData) {
	return getKeyUsage((entryTableInfo *) currentData) < getKeyUsage((entryTableInfo *) selectedData);
}

bool biggestSpaceUsedComparator(void * currentData, void * selectedData) {
	return getValueSize((entryTableInfo *) currentData) > getValueSize((entryTableInfo *) selectedData);
}

bool atomicEntry(void * voidEntryInfo) {
	entryTableInfo * entryInfo = (entryTableInfo *) voidEntryInfo;
	return entryInfo->valueSize <= entrySize;
}

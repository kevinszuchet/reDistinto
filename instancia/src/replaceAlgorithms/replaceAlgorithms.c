/*
 * replaceAlgorithms.c
 *
 *  Created on: 20 may. 2018
 *      Author: utnso
 */

#include "replaceAlgorithms.h"

int initializeAccordingToAlgorithm() {

	log_info(replaceAlgorithmsLogger, "Initiliaze: Round Algorithm");
	initializePointer(entryTableElement, &entryTableIndex);

	//BSU doesn't need initialization
	//LRU doesn't need initialization
	//But they need the CIRC algorithm.
	return 0;
}

int updateAccodringToAlgorithm(char * key) {

	if (strcmp(algorithm, "LRU") == 0) {

		log_info(replaceAlgorithmsLogger, "Update: Least Recently Used Algorithm");
		updateUsage(key);
	}

	//CIRC doesn't need Updates
	//BSU doesn't need Updates

	return 0;

}

int deleteAccodringToAlgorithm() {
	t_hash_element * toBeDeletedElement;

	if (entryTableElement == NULL) {

		initializePointer(entryTableElement, &entryTableIndex);
	}

	if (strcmp(algorithm, "CIRC") == 0) {
		toBeDeletedElement = getPointedKey();
	}

	else if (strcmp(algorithm, "LRU") == 0) {
		toBeDeletedElement = getLeastRecentlyUsedKey();
	}

	else {
		toBeDeletedElement = getBiggestSpaceUsedKey();
	}


	deleteKey(toBeDeletedElement->data, toBeDeletedElement->key);

	return 0;
}

void deleteKey(entryTableInfo * data, char * key) {

	log_info(replaceAlgorithmsLogger, "the key %d is about to be deleted", key);

	biMapUpdate(getValueStart(data), wholeUpperDivision(getValueSize(data), entrySize), IS_EMPTY);
	dictionary_remove_and_destroy(entryTable, key, (void *) destroyTableInfo);
	pointToNextKey();

	log_info(replaceAlgorithmsLogger, "the key was successfully deleted");
}

void findNextValidPointer(t_hash_element * elem, int * index) {

	while (elem == NULL && (*index) < entryTable->table_max_size) {
		elem = entryTable->elements[*index];
		(*index)++;
	}
}


int initializePointer(t_hash_element * elem, int * index) {

	if (entryTable->elements_amount == 0) {

		log_error(replaceAlgorithmsLogger, "There are no elements in the entryTable so the pointer can't be initialized\n");
		return -1;
	}

	index = 0;

	findNextValidPointer(elem, index);

	log_info(replaceAlgorithmsLogger, "The pointer was successfully initialized\n");
	return 0;
}

/************************************************************Round Algorithm******************************************************/

void pointToNextKey() {

	findNextValidPointer(entryTableElement, &entryTableIndex);

	if (entryTableElement == NULL) {
		initializePointer(entryTableElement, &entryTableIndex);
	}

	/*
	 * Lo que ví en el dictionary, debuggeandolo, es que no se agregan los elementos de forma contigua.
	 * De repente tenes 1 elemento y despues vienen 3 Null y después el siguiente y así. Por eso está medio fea esta función
	 *
	 */
}


t_hash_element * getPointedKey() {

	return entryTableElement;
}
/************************************************************Round Algorithm******************************************************/

/*****************************************************Least Recently Used Algorithm***********************************************/

void updateUsage(char * key) {

	for (int i = 0; i < entryTable->table_max_size; i++) {
		t_hash_element *element = entryTable->elements[i];

		while (element != NULL) {

			if (strcmp(element->key,key) == 0) {

				setUsageToZero(element->data);
				log_info(replaceAlgorithmsLogger, "");
			}
			else {

				increaseKeyUsage(element->data);

			}
			element = element->next;

		}
	}
}

t_hash_element * getLeastRecentlyUsedKey() {

	t_hash_element * selectedElem = NULL;
	t_hash_element * currentElem = NULL;
	int index;

	initializePointer(currentElem, &index);
	selectedElem = currentElem;

	while (index < entryTable->table_max_size) {

		if (currentElem == NULL) {

			currentElem = currentElem->next;
		}
		else if (getKeyUsage(currentElem->data) > getKeyUsage(selectedElem->data)) {

			selectedElem = currentElem;
			currentElem = currentElem->next;
		}

		index++;
	}


	return selectedElem;
}

/*****************************************************Least Recently Used Algorithm***********************************************/
//TODO CHEQUEAR EL TEMA DEL CODIGO REPETIDO EN LAS FUNCIONES getBiggestSpaceUsedKey(), getLeastRecentlyUsedKey()
/******************************************************Biggest Space Used Algorithm***********************************************/
t_hash_element * getBiggestSpaceUsedKey() {

	t_hash_element * selectedElem = NULL;
	t_hash_element * currentElem = NULL;
	int index;

	initializePointer(currentElem, &index);
	selectedElem = currentElem;

	while (index < entryTable->table_max_size) {

		if (currentElem == NULL) {

			currentElem = currentElem->next;
		}
		else if (getValueSize(currentElem->data) > getValueSize(selectedElem->data)) {

			selectedElem = currentElem;
			currentElem = currentElem->next;
		}

		index++;
	}


	return selectedElem;
}


/*****************************************************Biggest Space Used Algorithm************************************************/


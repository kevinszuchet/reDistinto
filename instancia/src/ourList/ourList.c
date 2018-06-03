/*
 * ourList.c
 *
 *  Created on: 27 may. 2018
 *      Author: utnso
 */

#include "ourList.h"

void * list_find_with_param(t_list * self, void * param, bool(*condition)(void*, void*)) {
	t_link_element * element = list_find_element_with_param(self, param, condition, NULL);
	return element != NULL ? element : NULL;
}

t_link_element * list_find_element_with_param(t_list * self, void * param, bool(*condition)(void*, void*), int * index) {
	t_link_element * element = self->head;
	int position = 0;

	while (element != NULL && !condition(element->data, param)) {
		element = element->next;
		position++;
	}

	if (index != NULL) {
		*index = position;
	}

	return element;
}

void * list_remove_by_condition_with_param(t_list *self, void * param, bool(*condition)(void*, void*)) {
	int index = 0;

	t_link_element* element = list_find_element_with_param(self, param, condition, &index);
	if (element != NULL) {
		return list_remove(self, index);
	}

	return NULL;
}

void list_remove_and_destroy_by_condition_with_param(t_list *self, void * param, bool(*condition)(void*, void*), void(*element_destroyer)(void*)) {
	void* data = list_remove_by_condition_with_param(self, param, condition);
	if (data)
		element_destroyer(data);
}

bool compareByKey(entryTableInfo * entryInfo, char * key) {

	return strcmp(entryInfo->key, key) == 0;
}

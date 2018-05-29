/*
 * ourList.c
 *
 *  Created on: 27 may. 2018
 *      Author: utnso
 */

#include "ourList.h"

static t_link_element * list_find_element_by_condition(t_list * self, bool(*condition)(void*, void*), int * index, void * param);

void * list_find_by_condition(t_list * self, bool(*condition)(void*, void*), void * param) {
	t_link_element * element = list_find_element_by_condition(self, condition, NULL, param);
	return element != NULL ? element : NULL;
}

static t_link_element * list_find_element_by_condition(t_list * self, bool(*condition)(void*, void*), int * index, void * param) {
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

void* list_remove_by_condition_with_param(t_list *self, bool(*condition)(void*, void*), void * param) {
	int index = 0;

	t_link_element* element = list_find_element_by_condition(self, condition, &index, param);
	if (element != NULL) {
		return list_remove(self, index);
	}

	return NULL;
}

void list_remove_and_destroy_by_condition_with_param(t_list *self, bool(*condition)(void*, void*), void(*element_destroyer)(void*), void * param) {
	void* data = list_remove_by_condition_with_param(self, condition, param);
	if (data)
		element_destroyer(data);
}

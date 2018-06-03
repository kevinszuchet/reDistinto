/*
 * ourList.h
 *
 *  Created on: 27 may. 2018
 *      Author: utnso
 */

#ifndef SRC_OURLIST_OURLIST_H_
#define SRC_OURLIST_OURLIST_H_

	#include <commons/collections/list.h>
	#include "../tadEntryTable/tadEntryTable.h"

	t_link_element * list_find_element_with_param(t_list * self, void * param, bool(*condition)(void*, void*), int * index);
	void * list_find_with_param(t_list * self, void * param, bool(*condition)(void*, void*));

	void * list_remove_by_condition_with_param(t_list *self, void * param, bool(*condition)(void*, void*));
	void list_remove_and_destroy_by_condition_with_param(t_list *self, void * param, bool(*condition)(void*, void*), void(*element_destroyer)(void*));

#endif /* SRC_OURLIST_OURLIST_H_ */

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

	void * list_find_by_condition(t_list * self, bool(*condition)(void*, void*), void* param);

	void* list_remove_by_condition_with_param(t_list *self, bool(*condition)(void*, void*), void * param);
	void list_remove_and_destroy_by_condition_with_param(t_list *self, bool(*condition)(void*, void*), void(*element_destroyer)(void*), void * param);

#endif /* SRC_OURLIST_OURLIST_H_ */

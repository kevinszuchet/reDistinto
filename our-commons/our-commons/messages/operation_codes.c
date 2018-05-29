/*
 * operation_codes.c
 *
 *  Created on: 28 may. 2018
 *      Author: utnso
 */

#include "operation_codes.h"

char* getCoordinadorResponseName(char coordinadorResponse){
	switch(coordinadorResponse){
		case BLOCK:
			return "BLOCK";
			break;
		case LOCK:
			return "LOCK";
			break;
		case SUCCESS:
			return "SUCCESS";
			break;
		case ABORT:
			return "ABORT";
			break;
		default:
			return "UNKNOWN KEY STATUS";
			break;
	}
}

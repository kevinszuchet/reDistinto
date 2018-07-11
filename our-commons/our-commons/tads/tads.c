/*
 * tads.c
 *
 *  Created on: 28 may. 2018
 *      Author: utnso
 */

#include "tads.h"

char* getOperationName(Operation* operation){
	switch(operation->operationCode){
		case OURSET:
			return "SET";
			break;
		case OURSTORE:
			return "STORE";
			break;
		case OURGET:
			return "GET";
			break;
		default:
			return "UNKNOWN OPERATION";
			break;
	}
}

char* getValueIfPossible(Operation* operation){
	if(operation->value){
		return operation->value;
	}
	return "Operation with no value";
}

void showOperation(Operation* operation, t_log* logger){
	log_info(logger,
			"\nOperation key = %s\nKey = %s\nValue = %s",
			getOperationName(operation), operation->key, getValueIfPossible(operation));
}

char* getKeyStatusName(char keyStatus){
	switch(keyStatus){
		case LOCKED:
			return "LOCKED";
			break;
		case NOTBLOCKED:
			return "NOTBLOCKED";
			break;
		case BLOCKED:
			return "BLOCKED";
			break;
		default:
			return "UNKNOWN KEY STATUS";
			break;
	}
}

void destroyOperation(Operation* operation){
	free(operation->key);
	free(operation->value);
	free(operation);
}

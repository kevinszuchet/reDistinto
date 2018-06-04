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

void showOperation(Operation* operation){
	printf("Operation key = %s\n", getOperationName(operation));
	printf("Key = %s\n", operation->key);
	operation->value ? printf("Value = %s\n", operation->value) : printf("Operation with no value\n");
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

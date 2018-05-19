/*
 * serialization.h
 *
 *  Created on: 1 may. 2018
 *      Author: utnso
 */

#ifndef SERIALIZATION_H_
#define SERIALIZATION_H_

	#include "operation_codes.h"
	#include "../tads/tads.h"

	#include "../sockets/sockets.h"
	#include <string.h>

int sendInt(int value, int sendSocket);
int recieveInt(int** value, int recvSocket);

int sendString(char* value, int sendSocket);
int recieveString(char** string, int recvSocket);

int sendOperation(Operation* operation, int sendSocket);
int recieveOperation(Operation** operation, int recvSocket);

#endif /* SERIALIZATION_H_ */

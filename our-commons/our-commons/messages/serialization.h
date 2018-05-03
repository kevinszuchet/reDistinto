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

int sendOperation(Operation* operation, int sendSocket);

int recieveOperation(Operation * operation, int recvSocket);

#endif /* SERIALIZATION_H_ */

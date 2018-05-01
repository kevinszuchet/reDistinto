/*
 * serialization.c
 *
 *  Created on: 1 may. 2018
 *      Author: utnso
 */

#include "serialization.h"

void sendOperation(Operation * operation, int sendSocket) {
	//serializeOperation(operation);
	//send(sendSocket, buffer, strlen(buffer), NULL);
}

void recieveOperation(Operation * operation, int recvSocket) {
	//unserializeOperation(operation);
	//recv(recvSocket, buffer, strlen(buffer), NULL);
}

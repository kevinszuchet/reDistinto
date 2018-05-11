/*
 * serialization.c
 *
 *  Created on: 1 may. 2018
 *      Author: utnso
 */

#include "serialization.h"

int send_all(int socket, void* package, int length)
{
    char *auxPointer = (char*) package;
    while (length > 0)
    {
        int i = send(socket, auxPointer, length, 0);
        if (i < 1) return 0;
        auxPointer += i;
        length -= i;
    }
    return 1;
}

int recv_all(int socket, void* package, int length)
{
    char *auxPointer = (char*) package;
    while (length > 0)
    {
        int i = recv(socket, auxPointer, length, 0);
        if (i < 1) return 0;
        auxPointer += i;
        length -= i;
    }
    return 1;
}

int addToPackageGeneric(void* package, void* value, int size, int* offset) {
		package = realloc(package, *offset + size);
		memcpy(package + *offset, value, size);
		*offset += size;
		return 1;
	}

int sendOperation(Operation* operation, int sendSocket) {
	void* package = NULL;
	int offset = 0;

	int addToPackage(void* value, int size) {
		return addToPackageGeneric(package, value, size, &offset);
	}

	int sizeKey = strlen(operation->key) + 1;
	int sizeValue = operation->value != NULL ? strlen(operation->value) + 1 : 0;

	addToPackage(&operation->operationCode, sizeof(operation->operationCode));
	addToPackage(&sizeKey, sizeof(sizeKey));
	addToPackage(&sizeValue, sizeof(sizeValue));
	addToPackage(&operation->key, sizeKey);
	if (sizeValue != 0) {addToPackage(&operation->value, sizeValue);}

	int result = send_all(sendSocket, package, offset);
	free(package);
	return result;
}

int recieveOperation(Operation * operation, int recvSocket) {
	int sizeKey;
	int sizeValue;

	return
		recv_all(recvSocket, &operation->operationCode, sizeof(char)) *
		recv_all(recvSocket, &sizeKey, sizeof(int)) *
		recv_all(recvSocket, &sizeValue, sizeof(int)) *
		recv_all(recvSocket, &operation->key, sizeKey) *
		(sizeValue != 0) ? recv_all(recvSocket, &operation->value, sizeValue) : 1;
}

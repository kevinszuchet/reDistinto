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
        if (i < 1) return -1;
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
        if (i < 1) return -1;
        auxPointer += i;
        length -= i;
    }
    return 1;
}

int sendOperation(Operation* operation, int sendSocket) {
	void* package;
	int offset = 0;

	int addToPackage(void* value, int size) {
		if((package = realloc(package, offset + size)) == NULL) return -1;
		memcpy(package + offset, value, size);
		offset += size;
		return 1;
	}

	int sizeKey = strlen(operation->key);
	int sizeValue = strlen(operation->value);

	//TODO como solucionar el manejo de errores
	//Muy feo los ifs estos
	if((addToPackage(&operation->operationCode, sizeof(operation->operationCode))) == -1) return -1;
	if((addToPackage(&sizeKey, sizeof(sizeKey))) == -1) return -1;
	if((addToPackage(&sizeValue, sizeof(sizeValue))) == -1) return -1;
	if((addToPackage(&operation->key, sizeKey)) == -1) return -1;
	if((addToPackage(&operation->value, sizeValue)) == -1) return -1;

	return send_all(sendSocket, package, offset);
}

//Operacion tiene que llegar inicializada por ahora
int recieveOperation(Operation * operation, int recvSocket) {
	int* sizeKey = malloc(sizeof(int));
	int* sizeValue = malloc(sizeof(int));

	//TODO manejo de errores
	recv_all(recvSocket, &operation->operationCode, sizeof(char));
	recv_all(recvSocket, sizeKey, sizeof(int));
	recv_all(recvSocket, sizeValue, sizeof(int));
	recv_all(recvSocket, &operation->key, *sizeKey);
	recv_all(recvSocket, &operation->value, *sizeValue);

	return 1;
}

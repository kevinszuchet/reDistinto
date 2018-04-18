/*
 * client.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef CLIENT_H_
#define CLIENT_H_

	#include "sockets.h"
	#include <string.h>

	int connectToServer(char* serverIP, int serverPort, const char* serverName, const char* clientName);
	int handshakeWithServer(int serverSocket, int handshakeValue, const char* serverName, const char* clientName);

#endif /* CLIENT_H_ */

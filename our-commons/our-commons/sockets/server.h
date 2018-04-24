/*
 * server.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef SERVER_H_
#define SERVER_H_

	#include "sockets.h"
	#include <string.h>

	int openConnection(int listenerPort, const char* serverName, const char* clientName);
	int handshakeWithClient(int clientSocket, int clientHandshakeValue, const char* serverName, const char* clientName);
	int welcomeClient(int listenerPort, const char* serverName, const char* clientName, int handshakeValue, int (*welcomeProcedure)(int coordinadorSocket));
	int acceptUnknownClient(int serverSocket, const char* serverName);
	int recieveClientId(int clientSocket, const char* serverName);

#endif /* SERVER_H_ */

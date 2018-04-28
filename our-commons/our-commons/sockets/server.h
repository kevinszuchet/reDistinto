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
	#include <commons/log.h>

	int openConnection(int listenerPort, const char* serverName, const char* clientName, t_log* logger);
	int handshakeWithClient(int clientSocket, int clientHandshakeValue, const char* serverName, const char* clientNamem, t_log* logger);
	int welcomeClient(int listenerPort, const char* serverName, const char* clientName, int handshakeValue, int (*welcomeProcedure)(int coordinadorSocket), t_log* logger);
	int acceptUnknownClient(int serverSocket, const char* serverName, t_log* logger);
	int recieveClientId(int clientSocket, const char* serverName, t_log* logger);

#endif /* SERVER_H_ */

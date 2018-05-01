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
	#include <commons/log.h>

	int connectToServer(char* serverIP, int serverPort, const char* serverName, const char* clientName, t_log* logger);
	int welcomeServer(const char* serverIp, int serverPort, const char* serverName, const char* clientName, int handshakeValue, int (*welcomeProcedure)(int serverSocket), t_log* logger);
	int sendMyIdToServer(int serverSocket, int clientId, const char* clientName, t_log* logger);

#endif /* CLIENT_H_ */

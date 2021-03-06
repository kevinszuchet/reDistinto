/*
 * client.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "client.h"



int connectToServer(const char* serverIP, int serverPort, const char* serverName, const char* clientName, t_log* logger){
	struct sockaddr_in serverAddress;
	serverAddress.sin_family  = AF_INET;
	serverAddress.sin_addr.s_addr = inet_addr(serverIP);
	serverAddress.sin_port = htons(serverPort);
	//log_info(logger, "");
	int serverSocket = 0;

	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		log_error(logger, "%s couldn't create the socket to connect to %s: %s", clientName, serverName, strerror(errno));
		return -1;
	}

	if (connect(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
		log_error(logger, "%s couldn't connect to %s: %s", clientName, serverName, strerror(errno));
		return -1;
	}

	log_info(logger, "%s could connect to %s", clientName, serverName);
	return serverSocket;
}

int handshakeWithServer(int serverSocket, char handshakeValue, const char* serverName, const char* clientName, t_log* logger) {
	char response;
	if(recv(serverSocket, &response, sizeof(char), 0) <= 0){
		log_error(logger, "recv failed on %s, while trying to connect with server %s: %s", clientName, serverName, strerror(errno));
		return -1;
	}

	if(response == handshakeValue){
		log_info(logger, "%s could handshake with %s!", clientName, serverName);
	}else{
		log_error(logger, "%s couldn't handshake with server %s, since the response was %d != %d", clientName, serverName, response, handshakeValue);
		return -1;
	}

	char clientHandshakeValue = response;
	if (send(serverSocket, &clientHandshakeValue, sizeof(char), 0) < 0){
		log_error(logger, "Something was wrong with send: %s", strerror(errno));
		return -1;
	}

	return 0;
}

int welcomeServer(const char* serverIp, int serverPort, const char* serverName, const char* clientName, char handshakeValue,
		int (*welcomeProcedure)(int serverSocket), t_log* logger){

	int serverSocket = connectToServer(serverIp, serverPort, serverName, clientName, logger);
	if (serverSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeResult = handshakeWithServer(serverSocket, handshakeValue, serverName, clientName, logger);
	if(handshakeResult < 0){
		//que pasa si falla el handshake?
		return -1;
	}

	welcomeProcedure(serverSocket);

	return 0;
}

int sendMyIdToServer(int serverSocket, char clientId, const char* clientName, t_log* logger){
	if (send(serverSocket, &clientId, sizeof(char), 0) < 0){
		log_error(logger,"Something was wrong with send from %s: %s", clientName, strerror(errno));
		return -1;
	}
	return 0;
}


/*
 * server.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "server.h"

int openConnection(int listenerPort, const char* serverName, const char* clientName, t_log* logger){
	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	serverAddress.sin_port = htons(listenerPort);

	int serverSocket = 0;
	if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		log_error(logger, "%s couldn't create socket for client %s: %s", serverName, clientName, strerror(errno));
		return -1;
	}

	int activated = 1;
	if(setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &activated, sizeof(activated)) == -1){
		log_error(logger, "%s had an error in setsockopt: %s", serverName, strerror(errno));
		close(serverSocket);
		return -1;
	}

	if (bind(serverSocket, (void*) &serverAddress, sizeof(serverAddress)) != 0) {
		log_error(logger, "%s couldn't bind the port %d: %s", serverName, listenerPort, strerror(errno));
		close(serverSocket);
		return -1;
	}

	if(listen(serverSocket, 100) == -1){
		log_error(logger,"%s couldn't start to listen in port %d: %s", serverName, listenerPort, strerror(errno));
		close(serverSocket);
		return -1;
	}

	log_info(logger, "%s could create the socket %d to listen to %s", serverName, serverSocket, clientName);
	log_info(logger, "listening...");

	return serverSocket;
}

int acceptUnknownClient(int serverSocket, const char* serverName, t_log* logger){

	if(serverSocket < 0){
		log_error(logger, "The socket (%d) where %s is listening is not a valid one", serverSocket, serverName);
		return -1;
	}

	struct sockaddr_in clientAddress;
	unsigned int len = sizeof(clientAddress);
	int clientSocket = accept(serverSocket, (void*) &clientAddress, &len);
	if (clientSocket == -1){
		log_error(logger, "%s couldn't accept a new connection: %s", serverName, strerror(errno));
		return -1;
	}

		log_info(logger, "%s could accept a new connection and it's set in socket: %d", serverName, clientSocket);

	return clientSocket;
}

int handshakeWithClient(int clientSocket, int clientHandshakeValue, const char* serverName, const char* clientName, t_log* logger){

	if(clientSocket < 0){
		log_error(logger, "The socket (%d) where %s is trying to connect to %s is not a valid one", clientSocket, serverName, clientName);
		return -1;
	}

	if (send(clientSocket, &clientHandshakeValue, sizeof(int), 0) < 0){
		log_error(logger, "The socket (%d) where %s is trying to connect to %s is not a valid one", clientSocket, serverName, clientName);
		close(clientSocket);
		return -1;
	}

	int response = 0;
	int resultRecv = recv(clientSocket, &response, sizeof(int), 0);
	if(resultRecv <= 0){
		log_error(logger, "recv failed on %s, while trying to connect with client %s: %s", serverName, clientName, strerror(errno));
		close(clientSocket);
		return -1;
	}

	if(response == clientHandshakeValue){
		log_info(logger, "%s could handshake with %s!", serverName, clientName);
	}else{
		log_error(logger, "%s couldn't handshake with client %s, since the response was %d != %d", serverName, clientName, response, clientHandshakeValue);
		close(clientSocket);
		return -1;
	}

	return 0;
}

int welcomeClient(int listenerPort, const char* serverName, const char* clientName, int handshakeValue,
	int (*welcomeProcedure)(int serverSocket, int clientSocket), t_log* logger){

	int serverToClientSocket = 0;
	if((serverToClientSocket = openConnection(listenerPort, serverName, clientName, logger)) < 0){
		close(serverToClientSocket);
		return -1;
	}

	int clientSocket = 0;

	if((clientSocket = acceptUnknownClient(serverToClientSocket, serverName, logger)) < 0){
		close(clientSocket);
		close(serverToClientSocket);
		return -1;
	}

	int handshakeResult = handshakeWithClient(clientSocket, handshakeValue, serverName, clientName, logger);
	if(handshakeResult < 0){
		close(clientSocket);
		close(serverToClientSocket);
		return -1;
	}

	welcomeProcedure(serverToClientSocket, clientSocket);

	close(clientSocket);
	close(serverToClientSocket);

	return 0;
}

int recieveClientId(int clientSocket,  const char* serverName, t_log* logger){
	int id = 0;
	int resultRecv = recv(clientSocket, &id, sizeof(int), 0);
	if(resultRecv <= 0){
		log_error(logger, "%s couldn't receive client id, from client socket %d: %s", serverName, clientSocket, strerror(errno));
		close(clientSocket);
		return -1;
	}

	return id;
}

/*
 * client.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef CLIENT_H_
#define CLIENT_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <errno.h>
	#include <unistd.h>

	int handshakeWithServer(char* serverIP, int serverPort, int handshakeValue, const char* serverName);

#endif /* CLIENT_H_ */

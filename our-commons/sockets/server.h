/*
 * server.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef SERVER_H_
#define SERVER_H_

	#include <stdlib.h>
	#include <stdio.h>
	#include <arpa/inet.h>
	#include <sys/socket.h>
	#include <errno.h>
	#include <unistd.h>
	#include <commons/config.h>

	int handshakeWithClient(int listenerPort, int clientHandshakeValue, const char* clientName);

#endif /* SERVER_H_ */

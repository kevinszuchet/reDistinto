/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>


int main(void) {
	int handshakeResult = handshakeWithServer("127.0.0.1", 8080, 10, "coordinador");
	printf("%d\n", handshakeResult);

	return 0;
}



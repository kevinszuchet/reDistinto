/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/server.h>

int main(){
	int handshakeResult = handshakeWithClient(8080, 10, "planificador");
	printf("%d\n", handshakeResult);

	return 0;
}

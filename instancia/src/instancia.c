/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>
#include <../our-commons/modules/names.h>

int main(){
	/*
	 * Handshake between instancia and coordinador
	 * */
	int coordinadorSocket = connectToServer("127.0.0.1", 8081, COORDINADOR, INSTANCIA);
	if (coordinadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithCoordinador = handshakeWithServer(coordinadorSocket, 11, COORDINADOR, INSTANCIA);
	if(handshakeWithCoordinador < 0){
		//que pasa si falla el handshake?
		return -1;
	}
	/*
	 * Handshake between instancia and coordinador
	 * */

	return 0;
}

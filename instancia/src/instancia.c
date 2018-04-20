/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>
#include <../our-commons/modules/names.h>
#include <commons/string.h>
#include <commons/config.h>

#define  CFG_FILE "../instancia.cfg"

void getConfig(int* listeningPort,char** algorithm, int* cantEntry,int* entrySize,int* delay){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
}

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

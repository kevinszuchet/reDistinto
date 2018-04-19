/*
 * instancia.c
 *
 *  Created on: 16 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/client.h>
#include <../our-commons/modules/names.h>
#include <commons/string.h>
#include <commons/config.h>

#define  CFG_FILE "esi.cfg"


int main(void) {
	/*char* ipCoordinador;
	char* ipPlanificador;
	int puertoCoordinador;
	int puertoPlanificador;
	t_config* config;
	config = config_create(CFG_FILE);
	config_set_value(config, "ipCoordinador","127.0.0.1");*/

	/*
	 * Handshake between esi and planificador
	 * */
	int planificadorSocket = connectToServer("127.0.0.1", 8082, PLANIFICADOR, ESI);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithPlanificador = handshakeWithServer(planificadorSocket, 12, PLANIFICADOR, ESI);
	if(handshakeWithPlanificador < 0){
		//que pasa si falla el handshake?
		return -1;
	}
	/*
	 * Handshake between esi and planificador
	 * */

	/*
	 * Handshake between esi and coordinador
	 * */
	int coordinadorSocket = connectToServer("127.0.0.1", 8083, COORDINADOR, ESI);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithCoordinador = handshakeWithServer(coordinadorSocket, 13, COORDINADOR, ESI);
	if(handshakeWithCoordinador < 0){
		//que pasa si falla el handshake?
		return -1;
	}
	/*
	 * Handshake between esi and coordinador
	 * */

	return 0;
}

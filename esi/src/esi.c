/*
 * instancia.c
 *
 *  Created on: 16 abr. 2018
 *      Author: utnso
 */

#include "esi.h"

int main(void) {
	char* ipCoordinador;
	char* ipPlanificador;
	int portCoordinador;
	int portPlanificador;

	getConfig(&ipCoordinador, &ipPlanificador, &portCoordinador, &portPlanificador);

	/*
	 * Handshake between esi and planificador
	 * */
	/*int planificadorSocket = connectToServer("127.0.0.1", 8082, PLANIFICADOR, ESI);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithPlanificador = sendMyIdToServer(planificadorSocket, 13, ESI);
	if(handshakeWithPlanificador < 0){
		//que pasa si falla el handshake?
		return -1;
	}*/

	int planificadorSocket = connectToServer(ipPlanificador, portPlanificador, PLANIFICADOR, ESI);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	sendMyIdToServer(planificadorSocket, 13, ESI);

	/*
	 * Handshake between esi and planificador
	 * */



	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, ESI);
	if (coordinadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, 12, ESI);

	return 0;
}

void getConfig(char** ipCoordinador,char** ipPlanificador, int* portCoordinador,int* portPlanificador) {

	t_config* config;
	config = config_create(CFG_FILE);
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*ipPlanificador = config_get_string_value(config, "IP_PLANIFICADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*portPlanificador = config_get_int_value(config, "PORT_PLANIFICADOR");
}

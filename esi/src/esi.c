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

	printf("IP Coordinador = %s\n", ipCoordinador);
	printf("IP Planificador = %s\n", ipPlanificador);
	printf("Port Coordinador = %d\n", portCoordinador);
	printf("Port Planificador= %d\n", portPlanificador);

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
	int coordinadorSocket = connectToServer("127.0.0.1", 8083, COORDINADOR,ESI);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	int handshakeWithCoordinador = handshakeWithServer(coordinadorSocket, 13,COORDINADOR, ESI);
	if(handshakeWithCoordinador < 0){
		//que pasa si falla el handshake?
		return -1;
	}
	/*
	 * Handshake between esi and coordinador
	 * */

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

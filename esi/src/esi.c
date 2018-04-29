/*
 * instancia.c
 *
 *  Created on: 16 abr. 2018
 *      Author: utnso
 */

#include "esi.h"

t_log* logger;

int main(int argc, char* argv[]) {

	logger = log_create("../esi.log", "tpSO", true, LOG_LEVEL_INFO);
	FILE* scriptFile;

	/*if (argc != 2) {
		printf("ESI cannot execute: you must enter a script file to read\n");
		return -1;
	}

	if ((scriptFile = fopen(argv[1], "r")) == NULL) {
		printf("ESI cannot execute: the script file cannot be opened\n");
		return -1;
	}*/

	char* ipCoordinador;
	char* ipPlanificador;
	int portCoordinador;
	int portPlanificador;

	getConfig(&ipCoordinador, &ipPlanificador, &portCoordinador, &portPlanificador);

	/*
	 * Handshake between esi and planificador
	 * */

	int planificadorSocket = connectToServer(ipPlanificador, portPlanificador, PLANIFICADOR, ESI, logger);
	if (planificadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	sendMyIdToServer(planificadorSocket, 12, ESI, logger);

	/*
	 * Handshake between esi and coordinador
	 * */

	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, ESI, logger);
	if (coordinadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, 12, ESI, logger);

	//permite probar concurrencia
	sleep(20);

	return 0;
}

/*
 * Configuration
 * */
void getConfig(char** ipCoordinador, char** ipPlanificador, int* portCoordinador, int* portPlanificador) {

	t_config* config;
	config = config_create(CFG_FILE);
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*ipPlanificador = config_get_string_value(config, "IP_PLANIFICADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*portPlanificador = config_get_int_value(config, "PORT_PLANIFICADOR");
}

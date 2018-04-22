/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"

int main(void) {

	int listeningPort;
	char* algorithm;
	int initialEstimation;
	char* ipCoordinador;
	int portCoordinador;
	char** blockedKeys;

	getConfig(&listeningPort, &algorithm, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	int welcomeResponse = welcomeServer("127.0.0.1", 8080, COORDINADOR, PLANIFICADOR, 10, &welcomeCoordinador);
	if (welcomeResponse < 0){
		//reintentar?
	}
	/*
	 * Planificador console
	 * */
	//crear el hilo!
	openConsole();
	/*
	 *  Planificador console
	 * */

	return 0;
}

void getConfig(int* listeningPort, char** algorithm, int* initialEstimation, int* ipCoordinador, int* portCoordinador, char*** blockedKeys){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*initialEstimation = config_get_int_value(config, "ESTIMATION");
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*blockedKeys = config_get_string_value(config, "BLOCKED_KEYS");
}

int welcomeEsi(){
	for(;;);

	return 0;
}

int welcomeCoordinador(){
	printf("Probando recepcion en el coordinador\n");
	/*int welcomePlanificador = welcomeClient(8082, COORDINADOR, ESI, 12, &welcomeEsi);
	if(welcomePlanificador < 0){
		//TODO: que pasa en este caso?
		return -1;
	}*/

	return 0;
}

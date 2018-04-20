/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include <../our-commons/sockets/server.h>
#include <../our-commons/modules/names.h>
#include <commons/string.h>
#include <commons/config.h>

#define  CFG_FILE "../coordinador.cfg"

void getConfig(int* listeningPort,char** algorithm, int* cantEntry,int* entrySize,int* delay){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
}

int welcomeInstancia();

int main(){

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	int delay;
	getConfig(&listeningPort,&algorithm,&cantEntry,&entrySize,&delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);

	int welcomePlanificador = welcomeClient(8080, COORDINADOR, PLANIFICADOR, 10, &welcomeInstancia);

	if(welcomePlanificador < 0){
		//TODO: que pasa en este caso?
		return -1;
	}

	return 0;
}

int startProcedure(){
	for(;;);
	return 0;
}

int welcomeEsi(){
	int messageResult = welcomeClient(8083, COORDINADOR, ESI, 13, &startProcedure);
	return 0;
}

int welcomeInstancia(){
	int messageResult = welcomeClient(8081, COORDINADOR, INSTANCIA, 11, &welcomeEsi);
	return 0;
}

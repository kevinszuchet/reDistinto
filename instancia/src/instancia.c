/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include "instancia.h"

t_log * logger;

int main(void) {
	logger = log_create("../instancia.log", "tpSO", true, LOG_LEVEL_INFO);
	char* ipCoordinador;
	int portCoordinador;
	char* algorithm, * path, * name;
	int dump;

	getConfig(&ipCoordinador, &portCoordinador, &algorithm, &path, &name, &dump);

	printf("IP coord = %s\n", ipCoordinador);
	printf("Puerto = %d\n", portCoordinador);
	printf("Algoritmo = %s\n", algorithm);
	printf("Path = %s\n", path);
	printf("Name= %s\n", name);
	printf("Dump= %d\n", dump);

	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, INSTANCIA, logger);
	if (coordinadorSocket < 0){
		//reintentar conexion?
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, 11, INSTANCIA, logger);

	return 0;
}

void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char**path, char** name, int* dump) {

	t_config* config;
	config = config_create(CFG_FILE);
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*path = config_get_string_value(config, "PATH");
	*name = config_get_string_value(config, "NAME");
	*dump = config_get_int_value(config, "DUMP");
}

void initialize(int entryAmount, int entrySize){
}

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

	if (argc != 2) {
		log_error(logger, "ESI cannot execute: you must enter a script file to read\n");
		return -1;
	}

	if ((scriptFile = fopen(argv[1], "r")) == NULL) {
		log_error(logger, "ESI cannot execute: the script file cannot be opened\n");
		return -1;
	}

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

	/*
	 * ESI wait to planificador, who will order to execute
	 * */
	waitPlanificadorOrders(planificadorSocket, scriptFile, coordinadorSocket);

	fclose(scriptFile);

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

/*
 * Interaction with coordinador and planificador
 * */

void waitPlanificadorOrders(int planificadorSocket, FILE * scriptFile, int coordinadorSocket) {

	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int esiPC = 0; // ESI program counter

	int response = RUN;

	while ((read = getline(&line, &len, scriptFile)) != -1) {
		/*if (recv(planificadorSocket, &response, sizeof(int), MSG_WAITALL)) {
			log_error(logger, "recv failed on, while esi trying to connect with planificador %s\n", strerror(errno));
			exit(-1);
		}*/

		if (response == RUN) {
			log_info(logger, "recv an order from planificador\n");
			tryToExecute(planificadorSocket, line, coordinadorSocket, &esiPC);
		}

		//fseek(scriptFile, esiPC, NULL);
	}

	if (line) {
		free(line);
	}
}

void tryToExecute(int planificadorSocket, char * line, int coordinadorSocket, int * esiCP) {

	/*
	 * Parser tries to understand each line, one by one
	 * */

	/*
	 *  Try to read the line that correspond to esiCP
	 *  Send the line to coordinador
	 *  recv response from coordinador
	 *  	success response: esiCP++
	 *  	error response: nothing with esiCP
	 *  send my response to planificador (ending or in process)
	 */

	Operation * operation = malloc(sizeof(Operation));
	interpretateOperation(operation, line);

	/*
	 * sendOperation(operation, coordinadorSocket): send serialized operation to coordinador
	 * recv(coordinadorSocket, response, sizeof(int), MSG_WAITALL) wait operation response from coordinador
	 *		if (response == SUCCESS): success response: iterate esiCP: esiCP += len
	 *		else if (response == FAILURE): failure response: try to interpretate the same line
	 */

	free(operation);
}

void interpretateOperation(Operation * operation, char * line) {
	t_esi_operacion parsedLine = parse(line);

	if (!parsedLine.valido) {
		log_error(logger, "Parsi cannot understand the line %s", line);
		exit(-1);
	}

	switch (parsedLine.keyword) {
		case GET:
			operation->operationCode = GET;
			operation->key = malloc(strlen(parsedLine.argumentos.GET.clave) + 1);
			strcpy(operation->key, parsedLine.argumentos.GET.clave);
			log_info(logger, "GET\tclave: <%s>\n", parsedLine.argumentos.GET.clave);
			break;

		case SET:
			operation->operationCode = SET;
			operation->key = malloc(strlen(parsedLine.argumentos.SET.clave) + 1);
			operation->value = malloc(strlen(parsedLine.argumentos.SET.valor) + 1);
			strcpy(operation->key, parsedLine.argumentos.SET.clave);
			strcpy(operation->value, parsedLine.argumentos.SET.valor);
			log_info(logger, "SET\tclave: <%s>\tvalor: <%s>\n", parsedLine.argumentos.SET.clave, parsedLine.argumentos.SET.valor);
			break;

		case STORE:
			operation->operationCode = STORE;
			operation->key = malloc(strlen(parsedLine.argumentos.STORE.clave) + 1);
			strcpy(operation->key, parsedLine.argumentos.STORE.clave);
			log_info(logger, "STORE\tclave: <%s>\n", parsedLine.argumentos.STORE.clave);
			break;

		default:
			log_error(logger, "Parsi could not understand the keyowrd %s", line);
			exit(-1);
	}

	// Hay que hacer free de key y value?
	//free(operation->key);

	// Rompe y tira segmentation fault
	/*if (strlen(operation->value) > 0) {
	 	 Rompe y tira segmentation fault
		free(operation->value);
	}*/

	destruir_operacion(parsedLine);
}

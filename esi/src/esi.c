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
	waitPlanificadorOrder(planificadorSocket, scriptFile);

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

void waitPlanificadorOrder(int planificadorSocket, FILE * scriptFile) {

	while (1) {
		int response = 0;
		if (recv(planificadorSocket, &response, sizeof(int), 0)) {
			log_error(logger, "recv failed on, while esi trying to connect with planificador %s\n", strerror(errno));
			exit(-1);
		} else {
			log_info(logger, "recv a RUN order from planificador\n");
		}

		if (response == RUN) {
			tryToExecute(scriptFile);
		}
	}
}

void tryToExecute(FILE * scriptFile) {

	char * line = NULL;
	size_t len = 0;
	ssize_t read;
	int esiPC = 0; // ESI program counter

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

	if ((read = getline(&line, &len, scriptFile)) != -1) {

		do {
			Operation * operation = malloc(sizeof(Operation));
			interpretateOperation(operation, line);

			sendOperation(operation);

			/*
			 * send serialized operation to coordinador
			 * wait operation response from coordinador
			 *		success response: iterate esiCP: esiCP += len
			 *		failure response: try to interpretate the same line
			 */

			fseek(scriptFile, esiPC, NULL);

		} while ((read = getline(&line, &len, scriptFile)) != -1);
	}

	if (line) {
		free(line);
	}
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
			strcpy(operation->key, parsedLine.argumentos.GET.clave);
			log_info(logger, "GET\tclave: <%s>\n", parsedLine.argumentos.GET.clave);
			break;

		case SET:
			operation->operationCode = SET;
			strcpy(operation->key, parsedLine.argumentos.SET.clave);
			strcpy(operation->value, parsedLine.argumentos.SET.valor);
			log_info(logger, "SET\tclave: <%s>\tvalor: <%s>\n", parsedLine.argumentos.SET.clave, parsedLine.argumentos.SET.valor);
			break;

		case STORE:
			operation->operationCode = STORE;
			strcpy(operation->key, parsedLine.argumentos.STORE.clave);
			log_info(logger, "STORE\tclave: <%s>\n", parsedLine.argumentos.STORE.clave);
			break;

		default:
			log_error(logger, "Parsi could not understand the keyowrd %s", line);
			exit(-1);
	}

	destruir_operacion(parsedLine);
}

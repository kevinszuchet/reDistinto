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

	if (argc != 2) {
		log_error(logger, "ESI cannot execute: you must enter a script file to read\n");
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
	 * Script handle (with mmap)
	 * */

	int scriptFd;
	char *script;
	struct stat sb;

	if ((scriptFd = open(argv[1], O_RDONLY)) == -1) {
		log_error(logger, "The script file cannot be opened\n");
		return -1;
	}

	if (fstat(scriptFd, &sb) == -1) {
		log_error(logger, "It is not possible to determinate the script file size\n");
		return -1;
	}

	script = mmap(NULL, sb.st_size, PROT_READ, MAP_SHARED, scriptFd, 0);

	if (script == MAP_FAILED) {
		log_error(logger, "mmap failed\n");
		return -1;
	}

	close(scriptFd);

	/*
	 * ESI wait to planificador, who will order to execute
	 * */
	waitPlanificadorOrders(planificadorSocket, script, coordinadorSocket);

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

void waitPlanificadorOrders(int planificadorSocket, char * script, int coordinadorSocket) {

	char * line = NULL;
	char ** scriptsSplitted = string_split(script, "\n");
	size_t len = sizeof(scriptsSplitted);
	int esiPC = 0; // ESI program counter

	int response;

	while (esiPC < len && empty_string(line = scriptsSplitted[esiPC]) == 0) {

		/*
		 * Parser tries to understand each line, one by one (when planificador says)
		 * */

		if (recv(planificadorSocket, &response, sizeof(int), MSG_WAITALL) <= 0) {
			log_error(logger, "recv failed on trying to connect with planificador %s\n", strerror(errno));
			exit(-1);
		}

		if (response == RUN) {
			log_info(logger, "recv an order from planificador\n");
			tryToExecute(planificadorSocket, line, coordinadorSocket, &esiPC, len);
		}
	}

	if (line) {
		free(line);
	}
}

void tryToExecute(int planificadorSocket, char * line, int coordinadorSocket, int * esiPC, size_t len) {

	char coordinadorResponse, status;

	/*
	 *  Try to read the line that correspond to esiCP
	 *  Send serialized operation to coordinador
	 *  Wait (recv) operation response from coordinador
	 *  	Success response: iterate esiCP
	 *  	Error response: try to execute same line when planificador says
	 *  Send my response to planificador (ending, success, failure)
	 */

	Operation * operation = malloc(sizeof(Operation));
	interpretateOperation(operation, line);

	if (sendOperation(operation, coordinadorSocket) == 0) {
		log_error(logger, "ESI cannot send the serialized operation to coordinador", line);
		exit(-1);
	}

	destroy_operation(operation);

	if (recv(coordinadorSocket, &coordinadorResponse, sizeof(int), MSG_WAITALL) <= 0) {
		log_error(logger, "recv failed on try to get the coordinador operation response", line);
		exit(-1);
	}

	*esiPC += (coordinadorResponse == SUCCESS ? 1 : 0);

	status = (*esiPC == (len - 1) ? FINISHED : NOTFINISHED);

	// Create and send the operationResponse
	OperationResponse operationResponse;
	operationResponse.coordinadorResponse = coordinadorResponse;
	operationResponse.esiStatus = status;

	if (send(planificadorSocket, &operationResponse, sizeof(int), 0) <= 0) {
		log_error(logger, "ESI cannot send the operation response to planificador", line);
		exit(-1);
	}
}

void interpretateOperation(Operation * operation, char * line) {
	t_esi_operacion parsedLine = parse(line);

	if (!parsedLine.valido) {
		log_error(logger, "Parsi cannot understand the line %s", line);
		exit(-1);
	}

	operation->value = "";

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

	destruir_operacion(parsedLine);
}

void destroy_operation(Operation * operation) {
	if (empty_string(operation->key) == 0)
		free(operation->key);

	if (empty_string(operation->value) == 0)
		free(operation->value);

	free(operation);
}

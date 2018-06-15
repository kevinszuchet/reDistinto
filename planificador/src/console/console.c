/*
 * consola.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "console.h"

t_log* logger;
t_list* instruccionsByConsoleList;

void openConsole() {
	char* line, ** parameters;
	printf("Listado de comandos\n1.Pausar\n2.Continuar\n3.Bloquear <clave> <id>\n4.Desbloquear <clave>\n5.Listar <recurso>\n6.Kill <ID>\n7.Status <clave>\n8.Deadlock\n");
	instruccionsByConsoleList = list_create();

	while(1) {
		line = readline("> ");
		string_to_lower(line);
		parameters = string_split(line, " ");

		// TODO Borrar estos printf cuando funcione completamente
		printf("Parameter quantity = %d\n", parameterQuantity(parameters));
		for (int i = 0; i < parameterQuantity(parameters); i++) {
			printf("Parametro %d = %s\n", i + 1, parameters[i]);
		}

		/*if (line) {
			add_history(line);
		}*/

		if (!strncmp(line, "exit", 4)) {
			free(line);
			exitPlanificador();
			break;
		}

		if (validCommand(parameters)) {
			log_info(logger, "Instruccion added to pending instruccion List");
			list_add(instruccionsByConsoleList, parameters);
		} else {
			int i = 0;
			while(parameters[i]) {
				free(parameters[i]);
				i++;
			}
		}

		free(parameters);
		free(line);
	}
}

void executeConsoleInstruccions() {
	void validateAndexecuteComand(void* parameters) {
		if (validCommand((char**) parameters)) {
			execute((char**) parameters);
		}
	}

	if (list_size(instruccionsByConsoleList) > 0) {
		log_info(logger, "Hay (%d) instrucciones de consola para ejecutar", list_size(instruccionsByConsoleList));
		list_iterate(instruccionsByConsoleList, &validateAndexecuteComand);

		list_clean_and_destroy_elements(instruccionsByConsoleList, destroyConsoleParam);
	}
}

void execute(char** parameters) {
	char* command = parameters[0];
	int commandNumber = getCommandNumber(command);
	// TODO Borrar cuando funcione completamente
	log_info(logger, "Execute command : %s\n", command);
	char* key = malloc(40);
	int esiID;
	int* esiIDpointer;
	t_queue* blockedEsis;

	switch(commandNumber) {
		case PAUSAR:
			pauseState = PAUSE;
			log_info(logger, "Execution paused by console");
		break;

		case CONTINUAR:
			pauseState = CONTINUE;
			log_info(logger, "Execution continued by console");
		break;

		case BLOQUEAR:
		    key = parameters[1];
		    esiID = atoi(parameters[2]);
		    if (!isLockedKey(key))
		    	lockKey(key, CONSOLE_BLOCKED);
			blockEsi(key,esiID);

			log_info(logger, "ESI (%d) was blocked in (%s) key:\n", esiID, key);
		break;

		case DESBLOQUEAR:
			 key = parameters[1];
			unlockEsi(key);
		break;

		case LISTAR:
			 key = parameters[1];
			 blockedEsis = (t_queue*) dictionary_get(blockedEsiDic, key);

			 if (queue_is_empty(blockedEsis))
				 printf("There are no blocked esis in key (%s)\n", key);

			 for (int i = 0; i < queue_size(blockedEsis); i++) {
				 printf("ID BEFORE POP = %d\n", *((int*) queue_peek(blockedEsis)));
				 esiIDpointer = (int*) queue_pop(blockedEsis);
				 printf("ID BEFORE PRINT = %d\n", *esiIDpointer);
				 printEsi(getEsiById(*esiIDpointer));
				 queue_push(blockedEsis, esiIDpointer);
			 }
		break;

		case KILL:
		break;

		case STATUS:
		break;

		case DEADLOCK:
		break;

		default:
			printf("%s: command not found\n", command);
		break;
	}
}

int parameterQuantity(char** parameters) {
	int i = 0;
	while (parameters[i] != NULL) {
		i++;
	}
	return i;
}

int validCommand(char** parameters) {
	if (parameterQuantity(parameters) == 0) {
		printf("No command was written\n");
		return 0;
	}

	char* command = parameters[0];
	int commandNumber = getCommandNumber(command), cantExtraParameters = parameterQuantity(parameters) - 1;

	char* key;
	int id;

	switch(commandNumber) {
		case PAUSAR:
			return parameterQuantityIsValid(cantExtraParameters, 0);
		break;

		case CONTINUAR:
			return parameterQuantityIsValid(cantExtraParameters, 0);
		break;

		case BLOQUEAR:
			if (parameterQuantityIsValid(cantExtraParameters, 2)) {
				if (validKey(parameters[1])) {
					key = malloc(40);
					strcpy(key, parameters[1]);
					id = atoi(parameters[2]); // TODO Falta validar que sea un numero
					if (validateBloquear(key, id)) {
						free(key);
						return 1;
					}
				}
			}
			printf("Invalid command\n");
			return 0;
		break;

		case DESBLOQUEAR:
			if (parameterQuantityIsValid(cantExtraParameters, 1)) {
				if (validKey(parameters[1])) {
					key = malloc(40);
					strcpy(key, parameters[1]);
					if (validateDesbloquear(key)) {
						free(key);
						return 1;
					}
				}
			}
			printf("Invalid command\n");
			return 0;
		break;

		case LISTAR:
			if (parameterQuantityIsValid(cantExtraParameters, 1)) {
				if (validKey(parameters[1])) {
					key = malloc(40);
					strcpy(key, parameters[1]);
					if (keyExists(key)) {
						free(key);
						return 1;
					}
				}
			}
			printf("Invalid command\n");
			return 0;
		break;

		case KILL:
			return parameterQuantityIsValid(cantExtraParameters, 1);
		break;

		case STATUS:
			return parameterQuantityIsValid(cantExtraParameters, 1);
		break;

		case DEADLOCK:
			return parameterQuantityIsValid(cantExtraParameters, 0);
		break;

		default:
			printf("%s: command not found\n", command);
			return 0;
		break;
	}
}

int validKey(char* key) {
	if (strlen(key) > 40) {
		printf("Invalid key lenght\n");
		return 0;
	}
	return 1;
}

int validateDesbloquear(char* key) {
	return (keyExists(key) ? 1 : 0);
}

int validateBloquear(char* key,int id) {
	return (isReady(id) || isRunning(id) ? 1 : 0);
}

int keyExists(char* key) {
	return (dictionary_has_key(blockedEsiDic, key) ? 1 : 0);
}

int isReady(int idEsi) {
	if (list_is_empty(readyEsis)) {
		return 0;
	}

	for (int i = 0; i < list_size(readyEsis); i++) {
		if (((Esi*) list_get(readyEsis, i))->id == idEsi) {
			return 1;
		}
	}
	return 0;
}

int isRunning(int idEsi) {
	return (runningEsi != NULL && runningEsi->id == idEsi ? 1 : 0);
}

int getCommandNumber(char* command) {
	if (!strcmp(command, "pausar")) {
		return PAUSAR;
	} else if (!strcmp(command, "continuar")) {
		return CONTINUAR;
	} else if (!strcmp(command, "bloquear")) {
		return BLOQUEAR;
	} else if (!strcmp(command, "desbloquear")) {
		return DESBLOQUEAR;
	} else if (!strcmp(command, "listar")) {
		return LISTAR;
	} else if (!strcmp(command, "kill")) {
		return KILL;
	} else if (!strcmp(command, "status")) {
		return STATUS;
	} else if (!strcmp(command, "deadlock")) {
		return DEADLOCK;
	} else if (atoi(command) >= 1 && atoi(command) <= 8) {
		return atoi(command);
	} else {
		return INVALID_COMMAND;
	}
}

int parameterQuantityIsValid(int cantExtraParameters, int necessaryParameters) {
	if (cantExtraParameters != necessaryParameters) {
		printf("Invalid parameter quantity\n");
		return 0;
	}
	return 1;
}

// Destroy functions
void destroyConsoleParam(void * param) {
	if (param)
		free(param);
}

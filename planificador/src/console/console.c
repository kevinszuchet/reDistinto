/*
 * consola.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "console.h"

t_list* instruccionsByConsoleList = NULL;

void openConsole() {
	char* line, ** parameters;
	printf("Listado de comandos\n1.Pausar\n2.Continuar\n3.Bloquear <clave> <id>\n4.Desbloquear <clave>\n5.Listar <recurso>\n6.Kill <ID>\n7.Status <clave>\n8.Deadlock\n");
	instruccionsByConsoleList = list_create();

	while(1) {
		line = readline("> ");
		string_to_lower(line);
		parameters = string_split(line, " ");

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
			free(parameters);
		}

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

		//list_clean_and_destroy_elements(instruccionsByConsoleList, destroyConsoleParam);
		list_clean(instruccionsByConsoleList);
	}
}

void execute(char** parameters) {
	char* command = parameters[0];
	int commandNumber = getCommandNumber(command);

	char* key = malloc(40);
	int esiID;



	char requestToCoordinador;

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

			log_info(logger, "ESI (%d) was blocked in key (%s)\n", esiID, key);
		break;

		case DESBLOQUEAR:
			 key = parameters[1];
			unlockEsi(key,true);
		break;

		case LISTAR:
			 key = parameters[1];
			showBlockedEsisInKey(key);
		break;

		case KILL:
		    esiID = atoi(parameters[1]);

		    if (!isValidEsiId(esiID)) {
		    	printf("The entered esiID (%d) does not belong to the system.\n", esiID);
		    } else {
		    	Esi * esi = getEsiById(esiID);
		    	int message = KILLESI;
		    	if (sendInt(message, esi->socketConection) == CUSTOM_FAILURE) {
				   log_error(logger, "Coultn't send message to ESI %d", esiID);
				} else {
					log_info(logger, "Send kill message to ESI %d in socket %d", esiID, esi->socketConection);
				}

		    	// REVIEW se aborta en los 2 casos? o si no le puedo mandar el kill no hago nada?
		    	abortEsi(esi);
		    }
		break;

		case STATUS:
			requestToCoordinador = PLANIFICADOR_STATUS_REQUEST;

			//TODO nico chequear
			if (send_all(coordinadorSocket, &requestToCoordinador, sizeof(requestToCoordinador)) == CUSTOM_FAILURE) {
				log_error(logger, "I cannot send the request from status coordinador");
				exitPlanificador();
			}

			key = parameters[1];
			if (sendString(key, coordinadorSocket) == CUSTOM_FAILURE) {
				log_error(logger, "I cannot send the key to resolve status to coordinador");
				exitPlanificador();
			}
		break;

		case DEADLOCK:
			executeDeadlockAlgorithm();
		break;

		default:
			printf("%s: command not found\n", command);
		break;
	}
}

void executeDeadlockAlgorithm(){
	int esiCount = list_size(allSystemEsis);
	int i, j;

	int *asignationMatrix[esiCount];
	for (i=0; i<esiCount; i++)
		asignationMatrix[i] = (int *)malloc(esiCount * sizeof(int));



	for (i = 0; i <  esiCount; i++)
	  for (j = 0; j < esiCount; j++)
		  asignationMatrix[i][j] = -1;


	t_queue* blockedEsis;
	int actualEsiID;
	int takerEsiID;
	for (int i = 0; i < list_size(allSystemKeys); i++) {
		char* key = list_get(allSystemKeys, i);
		blockedEsis = dictionary_get(blockedEsiDic, key);
		for (int j = 0; j < queue_size(blockedEsis); j++) {
			actualEsiID = *((int*) queue_pop(blockedEsis));
			takerEsiID = getEsiTakerIDByKeyTaken(key);
			log_info(logger, "ACTUAL ID = %d", actualEsiID);
			log_info(logger, "TAKER ID = %d", takerEsiID);
			if(takerEsiID ==-1){
				log_info(logger, "Esi (%d) blocked at key (%s), but its not taken", actualEsiID, key);
			}else{
				log_info(logger, "Esi (%d) blocked at key (%s), taken by Esi (%d)", actualEsiID, key, takerEsiID);
				log_info(logger, "Fila (%d)",getEsiIndexByID(actualEsiID));
				log_info(logger, "Columna (%d)",getEsiIndexByID(takerEsiID));

				asignationMatrix[getEsiIndexByID(actualEsiID)][getEsiIndexByID(takerEsiID)] = 1;
			}


			queue_push(blockedEsis, &actualEsiID);

		}
	}

	printf("\n");
		for (i = 0; i <  esiCount; i++)
		{
			 printf("\n");
			  for (j = 0; j < esiCount; j++)
				 printf("%d ", asignationMatrix[i][j]);
		}
	printf("\n");


}
int getEsiIndexByID(int id){
	Esi* esi;
	for(int i = 0;i<list_size(allSystemEsis);i++){
		esi = list_get(allSystemEsis,i);
		log_info(logger, "Esi (%d) is in position (%d)", esi->id, i);
	}
	int index = -1;
	for(int i = 0;i<list_size(allSystemEsis);i++){
		esi = list_get(allSystemEsis,i);
		if(esi->id == id){
			log_info(logger, "Esi (%d) position is (%d)", esi->id, i);
			index = i;
		}
	}
	return index;
	/*exitPlanificador();
	exit(-1);*/
}
int getEsiTakerIDByKeyTaken(char* key){
	bool itemIsKey(void* item) {
		return strcmp(key, (char*) item) == 0;
	}
	Esi* esi;

	for(int i = 0;i<list_size(allSystemEsis);i++){
		esi = list_get(allSystemEsis,i);
		if(list_any_satisfy(esi->lockedKeys,&itemIsKey)){
			return esi->id;
		}
	}
	return -1;
}

int parameterQuantity(char** parameters) {
	int size = 0;
	for (; parameters[size] != NULL; size++);
	return size;
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
	// char** parameters = (char**) param;
	// free(parameters);
	// TODO liberar los parametros que incresan por consola, rompe como estaba
}

void destroyConsole() {
	if (instruccionsByConsoleList)
		list_destroy_and_destroy_elements(instruccionsByConsoleList, destroyConsoleParam);
}

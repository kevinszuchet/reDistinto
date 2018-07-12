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
	pthread_mutex_lock(&mutexInstruccionsByConsole);
	instruccionsByConsoleList = list_create();
	pthread_mutex_unlock(&mutexInstruccionsByConsole);

	while(1) {
		line = readline("> ");
		string_to_lower(line);
		parameters = string_split(line, " ");

		/*if (line) {
			add_history(line);
		}*/

		/*if (!strncmp(line, "exit", 4)) {
			free(line);
			exitPlanificador();
			break;
		}*/

		if (validCommand(parameters)) {
			pthread_mutex_lock(&mutexFinishedExecutingInstruccion);
			if(finishedExecutingInstruccion){

				sem_wait(&executionSemaphore);
				execute(parameters);
				sem_post(&executionSemaphore);

			}else{
				pthread_mutex_lock(&mutexInstruccionsByConsole);
				list_add(instruccionsByConsoleList, parameters);
				log_info(logger, "Instruccion added to pending instruccion List");
				pthread_mutex_unlock(&mutexInstruccionsByConsole);
			}
			pthread_mutex_unlock(&mutexFinishedExecutingInstruccion);
			executeInstruccion();

		} else {

		}
		int i = 0;
		while(parameters[i]) {
			 free(parameters[i]);
			i++;
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
	pthread_mutex_lock(&mutexInstruccionsByConsole);
	if (list_size(instruccionsByConsoleList) > 0) {
		log_info(logger, "Hay (%d) instrucciones de consola para ejecutar", list_size(instruccionsByConsoleList));
		list_iterate(instruccionsByConsoleList, &validateAndexecuteComand);

		//list_clean_and_destroy_elements(instruccionsByConsoleList, destroyConsoleParam);
		list_clean(instruccionsByConsoleList);
	}
	pthread_mutex_unlock(&mutexInstruccionsByConsole);
}

void execute(char** parameters) {
	char* command = parameters[0];
	int commandNumber = getCommandNumber(command);

	char* key;
	int esiID;



	char requestToCoordinador;

	switch(commandNumber) {
		case PAUSAR:
			pthread_mutex_lock(&mutexPauseState);
			pauseState = PAUSE;
			pthread_mutex_unlock(&mutexPauseState);
			log_info(logger, "Execution paused by console");
		break;

		case CONTINUAR:
			pthread_mutex_lock(&mutexPauseState);
			pauseState = CONTINUE;
			pthread_mutex_unlock(&mutexPauseState);
			log_info(logger, "Execution continued by console");
		break;

		case BLOQUEAR:
			key = malloc(40);
			strcpy(key, parameters[1]);
		    esiID = atoi(parameters[2]);
		    if (!isLockedKey(key))
		    	lockKey(key, CONSOLE_BLOCKED);
			blockEsi(key,esiID);

			log_info(logger, "ESI (%d) was blocked in key (%s)\n", esiID, key);
		break;

		case DESBLOQUEAR:
			key = malloc(40);
			strcpy(key, parameters[1]);
			unlockEsi(key,true);
			free(key);
		break;

		case LISTAR:
			key = malloc(40);
			strcpy(key, parameters[1]);
			showBlockedEsisInKey(key);
			free(key);
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
		    }
		break;

		case STATUS:
			key = malloc(40);
			requestToCoordinador = PLANIFICADOR_STATUS_REQUEST;


			if (send_all(coordinadorSocket, &requestToCoordinador, sizeof(requestToCoordinador)) == CUSTOM_FAILURE) {
				log_error(logger, "I cannot send the request from status coordinador");
				free(key);
				exitPlanificador();
			}

			strcpy(key, parameters[1]);
			globalKey = strdup(key);
			if (sendString(key, coordinadorSocket) == CUSTOM_FAILURE) {
				log_error(logger, "I cannot send the key to resolve status to coordinador");
				free(key);
				exitPlanificador();
			}
			free(key);



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


	int **asignationMatrix = (int **)malloc(esiCount * sizeof(int*));
	for (i=0; i<esiCount; i++)
		asignationMatrix[i] = (int *)malloc(esiCount * sizeof(int));



	for (i = 0; i <  esiCount; i++)
	  for (j = 0; j < esiCount; j++)
		  asignationMatrix[i][j] = -1;


	t_queue* blockedEsis;
	int* actualEsiID;
	int takerEsiID;
	for (int i = 0; i < list_size(allSystemKeys); i++) {
		char* key = list_get(allSystemKeys, i);
		blockedEsis = dictionary_get(blockedEsiDic, key);
		for (int j = 0; j < queue_size(blockedEsis); j++) {
			actualEsiID = (int*) queue_pop(blockedEsis);
			takerEsiID = getEsiTakerIDByKeyTaken(key);

			if(takerEsiID ==-1){

			}else{
				asignationMatrix[getEsiIndexByID(*actualEsiID)][getEsiIndexByID(takerEsiID)] = 1;
			}


			queue_push(blockedEsis, actualEsiID);

		}
	}

	printf("\n");
		for (i = 0; i <  esiCount; i++)
		{
			 printf("\n");
			  for (j = 0; j < esiCount; j++)
				 printf("%d ", asignationMatrix[i][j]);
		}
	printf("\n"); //todo borrar despues de testear deadlocks mas complejos, sirve para ver la matriz de espera y retencion

	t_list* totalDeadlockEsis = list_create();
	t_list* actualDeadlockEsis = list_create();
	bool foundPosibleDeadlock;
	int indexToContinue;
	int** indexCopy = malloc(sizeof(int*)*esiCount);
	for(int i = 0;i<esiCount;i++){
		indexCopy[i] = malloc(sizeof(int));
	}

	void showEsiIdByIndex(void* element){
		int* index = (int*)element;
		log_info(logger,"Esi (%d)",(getEsiByIndex(*index))->id);
	}

	for(int indexFila = 0;indexFila<esiCount;indexFila++){
		//printf("Chequeando deadlock desde fila (%d)\n",indexFila);
		list_clean(actualDeadlockEsis);
		foundPosibleDeadlock = true;
		indexToContinue = indexFila;
		bool actualIndexInList(void* element) {
				//printf("Chequeo contra index (%d)\n",indexToContinue);
				int* elementToIndex = (int*)element;
				//printf("Elemento existente en posible deadlock (%d)\n",*elementToIndex);
				if(*elementToIndex== indexToContinue){
					return true;
				}else{
					return false;
				}
		}
		if(!list_any_satisfy(totalDeadlockEsis,&actualIndexInList)){
			while(foundPosibleDeadlock){
				foundPosibleDeadlock = false;
				if(list_any_satisfy(actualDeadlockEsis,&actualIndexInList)){
					//printf("El primer elemento de la lista de deadlock actual es (%d)",*((int*)list_get(actualDeadlockEsis,0)));
					//printf("La fila (%d) llega a deadlock\n",indexFila);
					log_info(logger,"Found deadlock between ");
					list_iterate(actualDeadlockEsis,&showEsiIdByIndex);
					list_add_all(totalDeadlockEsis,actualDeadlockEsis);
				}else{
					*(indexCopy[indexToContinue]) = indexToContinue;
					list_add(actualDeadlockEsis,indexCopy[indexToContinue]);
					//printf("Agrego index (%d) a la lista de deadlock actual\n",indexToContinue);
					//printf("El primer elemento de la lista de deadlock actual es (%d)",*((int*)list_get(actualDeadlockEsis,0)));
					for(int indexColumna = 0;indexColumna<esiCount;indexColumna++){
						if(asignationMatrix[indexToContinue][indexColumna]==1){
							foundPosibleDeadlock=true;
							indexToContinue = indexColumna;
							//printf("Encontre un 1 en fila (%d) y columna (%d)\n",indexFila,indexColumna);

						}
					}
				}
			}
			//printf("La fila (%d) no llega a deadlock\n",indexFila);
		}else{
			//printf("La fila (%d) ya esta contemplada en deadlock\n",indexFila);
		}
	}
	if(list_size(totalDeadlockEsis)==0){
		log_info(logger,"There is no deadlock");
	}
	//todo borrar todos los comentarios despues de testear deadlocks mas complejos

	for (i=0; i<esiCount; i++)
		free(asignationMatrix[i]);

	free(asignationMatrix);
	for(i = 0;i<esiCount;i++){
		free(indexCopy[i]);
	}
	free(indexCopy);

	list_destroy(actualDeadlockEsis);
	list_destroy(totalDeadlockEsis);

}
int getEsiIndexByID(int id){
	Esi* esi;

	int index = -1;
	for(int i = 0;i<list_size(allSystemEsis);i++){
		esi = list_get(allSystemEsis,i);
		if(esi->id == id){

			index = i;
		}
	}
	return index;
	/*exitPlanificador();
	exit(-1);*/
}
Esi* getEsiByIndex(int index){
	Esi* esi;
	esi = (Esi*)list_get(allSystemEsis,index);
	return esi;

}
int getEsiTakerIDByKeyTaken(char* key){
	bool itemIsKey(void* item) {
		return strcmp(key, (char*) item) == 0;
	}
	Esi* esi;
	int idCopy;
	for(int i = 0;i<list_size(allSystemEsis);i++){
		esi = list_get(allSystemEsis,i);
		if(list_any_satisfy(esi->lockedKeys,&itemIsKey)){
			idCopy = esi->id;
			return idCopy;
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
					}else{
						printf("Esi isn't ready or running\n");
					}
				}else{
					printf("Invalid key lenght\n");

				}
			}
			free(key);
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
					}else{
						printf("Key doesn't exists\n");
					}
				}else{
					printf("Invalid lenght\n");
				}
			}
			printf("Invalid command\n");
			free(key);
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
					}else{
						printf("Key doesn't exists\n");
					}
				}
			}
			printf("Invalid command\n");
			free(key);
			return 0;
		break;

		case KILL:
			return parameterQuantityIsValid(cantExtraParameters, 1);
		break;

		case STATUS:
			if (parameterQuantityIsValid(cantExtraParameters, 1)) {
				if (validKey(parameters[1])) {
					return 1;
				}
			}
			printf("Invalid command\n");
			return 0;
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
	pthread_mutex_lock(&mutexReadyList);
	if (list_is_empty(readyEsis)) {
		pthread_mutex_unlock(&mutexReadyList);
		return 0;
	}

	for (int i = 0; i < list_size(readyEsis); i++) {
		if (((Esi*) list_get(readyEsis, i))->id == idEsi) {
			pthread_mutex_unlock(&mutexReadyList);
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
	pthread_mutex_lock(&mutexInstruccionsByConsole);
	if (instruccionsByConsoleList)
		list_destroy_and_destroy_elements(instruccionsByConsoleList, destroyConsoleParam);
	pthread_mutex_unlock(&mutexInstruccionsByConsole);
}

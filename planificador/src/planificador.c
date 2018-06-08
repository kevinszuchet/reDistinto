/*
 * planificador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"

t_log* logger;

fd_set master;

int pauseState = CONTINUE; //1 is running, 0 is paussed

t_dictionary* blockedEsiDic;
t_list* readyEsis;
t_list* finishedEsis;
Esi* runningEsi;

t_list* allSystemTakenKeys;
t_list* allSystemEsis;

t_list* instruccionsByConsoleList;

int listeningPort;
char* algorithm;
int alphaEstimation;
int initialEstimation;
char* ipCoordinador;
int portCoordinador;
char** blockedKeys;
pthread_t threadConsole;
pthread_t threadExecution;
pthread_t threadConsoleInstructions;

pthread_mutex_t mutexReadyList = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexEsiReady = PTHREAD_MUTEX_INITIALIZER;


sem_t executionSemaphore;
sem_t keyRecievedFromCoordinadorSemaphore;
sem_t esiInformationRecievedSemaphore;
sem_t readyEsisSemaphore;

sem_t consoleInstructionSemaphore;

int sentenceCounter = 0;

int actualID = 1; //ID number for ESIs, when a new one is created, this number increases by 1

int coordinadorSocket;

char* keyRecieved;
OperationResponse* esiInformation = NULL;
Esi* nextEsi;




int welcomeNewClients();

int main(void) {
	logger = log_create("../planificador.log", "tpSO", true, LOG_LEVEL_INFO);
	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	allSystemTakenKeys = list_create();
	blockedEsiDic = dictionary_create();
	addConfigurationLockedKeys(blockedKeys);

	allSystemEsis = list_create();
	readyEsis = list_create();
	finishedEsis = list_create();
	runningEsi = NULL;

	instruccionsByConsoleList = list_create();



	sem_init(&keyRecievedFromCoordinadorSemaphore, 0, 0);
	sem_init(&esiInformationRecievedSemaphore, 0, 0);
	sem_init(&readyEsisSemaphore, 0, 0);
	sem_init(&executionSemaphore, 0, 0);

	sem_init(&consoleInstructionSemaphore, 0, 1);



	pthread_create(&threadExecution,NULL,(void *)executionProcedure,NULL);
	pthread_create(&threadConsoleInstructions,NULL,(void*)executeConsoleInstruccions,NULL);
	int welcomeCoordinadorResult = welcomeServer(ipCoordinador, portCoordinador, COORDINADOR, PLANIFICADOR, COORDINADORID, &welcomeNewClients, logger);
	if(welcomeCoordinadorResult < 0){
		log_error(logger, "Couldn't handhsake with coordinador, quitting...");
		exitPlanificador();
	}

	/*int *aux = 0;
	pthread_join(threadExecution,(void**)&aux);
	pthread_join(threadConsoleInstructions,(void**)&aux);*/
	return 0;
}

//General execute ESI functions

void getNextEsi(){
	if(runningEsi==NULL){
		pthread_mutex_lock(&mutexReadyList);
		nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		printf("ID = %d\n",nextEsi->id);
		pthread_mutex_unlock(&mutexReadyList);
		runningEsi = nextEsi;
		removeFromReady(nextEsi);

	}else{
		if(strcmp(algorithm,"SJF-CD")==0){
			if(mustDislodgeRunningEsi())
			{
				dislodgeEsi(runningEsi);
				pthread_mutex_lock(&mutexReadyList);
				nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
				runningEsi = nextEsi;
				pthread_mutex_unlock(&mutexReadyList);
				removeFromReady(nextEsi);
			}else{
				nextEsi = runningEsi;
			}
		}else{
			nextEsi = runningEsi;
		}
	}
}

bool mustDislodgeRunningEsi(){
	//todo ver si tengo que desalojar
	return false;
}

//Core execute function
void executionProcedure(){
	log_info(logger,"Starting to check if an ESI is ready to run");

	//NEW SELECT EXECUTION
	while(1){


		sem_wait(&executionSemaphore);
		pthread_mutex_lock(&mutexReadyList);
		if((list_size(readyEsis)>0 ||runningEsi!=NULL)&&pauseState==CONTINUE){
			pthread_mutex_unlock(&mutexReadyList);

			getNextEsi();

			log_info(logger,"Executing ESI (%d)",runningEsi->id);

			sendEsiIdToCoordinador(runningEsi->id);
			sendMessageExecuteToEsi(runningEsi);
			log_info(logger,"Waiting coordinador request\n");
			sem_wait(&keyRecievedFromCoordinadorSemaphore);

			log_info(logger,"Key received = %s\n",keyRecieved);
			sendKeyStatusToCoordinador(keyRecieved);

			log_info(logger,"Waiting esi information\n");
			sem_wait(&esiInformationRecievedSemaphore);
			if(esiInformation==NULL){
				log_warning(logger,"Darling, you didn't recieve info because esi has been aborted keep trying with anotherone");
			}else{
				log_info(logger,"Going to handle Esi execution info.CoordinadoResponse = (%s) ,esiStatus = (%s)",getCoordinadorResponseName(esiInformation->coordinadorResponse),getEsiInformationResponseName(esiInformation->esiStatus));
				handleEsiInformation(esiInformation,keyRecieved);
				log_info(logger,"Finish executing one instruction from ESI %d\n",nextEsi->id);
			}
		}else{
			pthread_mutex_unlock(&mutexReadyList);

		}
		sem_post(&consoleInstructionSemaphore);


	}
}

void executeConsoleInstruccions(){
	void validateAndexecuteComand(void* parameters){
		if(validCommand((char**)parameters)){
			execute((char**)parameters);
		}
	}
	log_info(logger,"Console instruccion thread created");
	while(1){
		sem_wait(&consoleInstructionSemaphore);

		if(list_size(instruccionsByConsoleList)>0){
			log_info(logger,"Hay (%d) instrucciones de consola para ejecutar",list_size(instruccionsByConsoleList));
			list_iterate(instruccionsByConsoleList,&validateAndexecuteComand);
			list_clean(instruccionsByConsoleList);
		}

		sem_post(&executionSemaphore);
	}


}

void unlockEsi(char* key){
	//Saca al esi del diccionario de bloqueados y lo pasa a listos
	log_info(logger,"An ESI was unlocked from key %s (NOT IMPLEMENTED)\n",key);
}

void finishRunningEsi(){
	list_add(finishedEsis,runningEsi);
	freeTakenKeys(runningEsi);
	log_info(logger,"Esi (%d) succesfully finished",runningEsi->id);
	runningEsi = NULL;
}

void freeTakenKeys(Esi* esi){
	for(int i = 0;i<list_size(esi->lockedKeys);i++){
		char* keyToFree = (char*)list_get(esi->lockedKeys,i);
		freeKey(keyToFree,esi);

	}
}

void addToFinishedList(Esi* finishedEsi){
	list_add(finishedEsis,finishedEsi);
	log_info(logger,"Esi (%d) agregado a finalizados",finishedEsi->id);
}

void removeFromReady(Esi* esi){

	pthread_mutex_lock(&mutexReadyList);
	sem_wait(&readyEsisSemaphore);
	Esi* esiFromReady;
	int idToRemove =-1;
	for(int i = 0;i<list_size(readyEsis);i++){
		esiFromReady = list_get(readyEsis,i);
		if(esiFromReady->id==esi->id){
			idToRemove = i;
		}
	}
	list_remove(readyEsis, idToRemove);
	pthread_mutex_unlock(&mutexReadyList);
}

void sendEsiIdToCoordinador(int id){
	if (send(coordinadorSocket, &id, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about ESI id");
	   exitPlanificador();
	}else{
		log_info(logger,"Send esi ID = %d to coordinador",id);
	}
}

void dislodgeEsi(Esi* esi){
	addEsiToReady(runningEsi);
	updateLastBurst(sentenceCounter,&esi);
	runningEsi = NULL;
}

void handleEsiInformation(OperationResponse* esiExecutionInformation,char* key){
	sentenceCounter++;
	addWaitingTimeToAll(readyEsis);
	reduceWaitingTime(&runningEsi);

	switch(esiExecutionInformation->coordinadorResponse){
		case SUCCESS:
			log_info(logger,"Operation succeded, nothing to do");
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
				break;
			}
		break;

		case LOCK:
			lockKey(key,runningEsi->id);
			log_info(logger,"Key (%s) locked",key);
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
					log_info(logger,"Esi finished execution");
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
					if(strcmp(algorithm,"SJF-CD")==0){
						dislodgeEsi(runningEsi);
					}
				break;
			}

		break;
		case BLOCK:
			log_info(logger,"Operation didn't succed, esi (%d) blocked in key (%c)",runningEsi->id,key);
			//todo ADD ESI TO BLOCKED DIC
			//todo REMOVE ESI FROM RUNNING

		break;
		case FREE:
			freeKey(key,runningEsi);
			log_info(logger,"Operation succeded, key (%s) freed",key);
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
					log_info(logger,"Esi finished execution");
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
					if(strcmp(algorithm,"SJF-CD")==0){
						dislodgeEsi(runningEsi);
					}
				break;
			}
		break;
		case ABORT:
			runningEsi = NULL;
		break;


	}
	esiInformation = NULL;
}

void abortEsi(Esi* esi){
	bool isEsiById(void* element){
		return ((Esi*) element)->id == esi->id;
	}
	sleep(1);
	log_info(logger,"Aborting esi (%d)", esi->id);
	freeTakenKeys(esi);
	if(runningEsi!=NULL&& runningEsi->id==esi->id){
		sem_post(&esiInformationRecievedSemaphore);

	}
	deleteEsiFromSystemBySocket(esi->socketConection);
	list_remove_by_condition(allSystemEsis,&isEsiById);
	log_info(logger,"Esi (%d) succesfully aborted", esi->id);
}



void deleteEsiFromSystemBySocket(int socket){
	bool isEsiBySocket(void* esi){
			return ((Esi*)esi)->socketConection == socket;
		}
		t_queue* blockedEsis;
		Esi* actualEsi;
		t_list* filteredList;
		switch(getEsiPlaceBySocket(socket)){
			case INREADYLIST:
				pthread_mutex_lock(&mutexReadyList);
				actualEsi = list_remove_by_condition(readyEsis,&isEsiBySocket);
				pthread_mutex_unlock(&mutexReadyList);
			break;
			case INFINISHEDLIST:
				//actualEsi = list_remove_by_condition(finishedEsis,&isEsiBySocket);
				filteredList= list_filter(finishedEsis,&isEsiBySocket);
				actualEsi = list_get(filteredList,list_size(filteredList)-1);
			break;
			case INRUNNING:
				 runningEsi = NULL;
			break;
			case INBLOCKEDDIC:

				for(int i = 0;i<list_size(allSystemTakenKeys);i++){
					blockedEsis = malloc(sizeof(t_queue));
					blockedEsis = dictionary_get(blockedEsiDic,list_get(allSystemTakenKeys,i));
					for(int j = 0;j<queue_size(blockedEsis);j++){
						actualEsi = (Esi*)queue_pop(blockedEsis);
						if(actualEsi->socketConection!=socket){
							queue_push(blockedEsis,actualEsi);
						}else{
							//free(actualEsi);
						}
					}

				}

			break;
			default:
				log_error(logger,"Couldn't remove ESI with socket (%d)",socket);
				exitPlanificador();
			break;
		}
}

Esi* getEsiBySocket(int socket){
	bool isEsiBySocket(void* esi){
		return ((Esi*)esi)->socketConection == socket;
	}
	t_queue* blockedEsis;
	Esi* actualEsi;
	Esi* targetEsi = NULL;
	switch(getEsiPlaceBySocket(socket)){
		case INREADYLIST:
			return list_get(list_filter(readyEsis,&isEsiBySocket),0);
		break;
		case INFINISHEDLIST:
			return NULL;
		break;
		case INRUNNING:
			return runningEsi;
		break;
		case INBLOCKEDDIC:

			for(int i = 0;i<list_size(allSystemTakenKeys);i++){
				blockedEsis = dictionary_get(blockedEsiDic,list_get(allSystemTakenKeys,i));
				for(int j = 0;j<queue_size(blockedEsis);j++){
					actualEsi = (Esi*)queue_pop(blockedEsis);
					if(actualEsi->socketConection==socket){
						*targetEsi = *actualEsi;
					}
					queue_push(blockedEsis,actualEsi);
				}
			}
			free(blockedEsis);
			return targetEsi;
		break;
		default:
			log_error(logger,"Couldn't find ESI by socket (%d)",socket);
			exitPlanificador();
			exit(-1);
		break;
	}
}

char getEsiPlaceBySocket(int socket){
	bool isEsiBySocket(void* esi){
		return ((Esi*)esi)->socketConection == socket;
	}
	if(list_size(list_filter(readyEsis,&isEsiBySocket))>0){
		return INREADYLIST;
	}

	if(runningEsi!=NULL && runningEsi->socketConection == socket){
		return INRUNNING;
	}
	t_queue* blockedEsis;
	Esi* actualEsi;

	for(int i = 0;i<list_size(allSystemTakenKeys);i++){
		blockedEsis = dictionary_get(blockedEsiDic,list_get(allSystemTakenKeys,i));
		for(int j = 0;j<queue_size(blockedEsis);j++){
			actualEsi = (Esi*)queue_pop(blockedEsis);
			if(actualEsi->socketConection==socket){
				return INBLOCKEDDIC;
			}
			queue_push(blockedEsis,actualEsi);
		}
	}

	if(list_size(list_filter(finishedEsis,&isEsiBySocket))>0){
		return INFINISHEDLIST;
	}
	return NOWHERE;

}

void moveFromRunningToReady(Esi* esi){
	addEsiToReady(runningEsi);
	runningEsi = NULL;
}

OperationResponse *waitEsiInformation(int esiSocket){

	OperationResponse* finishInformation = malloc(sizeof(OperationResponse));
	int resultRecv = recv(esiSocket, finishInformation, sizeof(OperationResponse), 0);
	if(resultRecv <= 0){
		log_error(logger, "recv failed on %s, while waiting ESI message %s\n", ESI, strerror(errno));
		exitPlanificador();
		exit(-1);
	}else{
		return finishInformation;
	}
}

char waitEsiInformationDummie(int esiSocket){

	return SUCCESS;
}

void sendKeyStatusToCoordinador(char* key){
	bool keyCompare(void* takenKey){
		if(strcmp((char*)takenKey,key)==0){
			return true;
		}
		return false;
	}
	char keyStatus = isLockedKey(key);
	if(keyStatus==BLOCKED){
		if(list_filter(runningEsi->lockedKeys,&keyCompare)){
			keyStatus = LOCKED;
		}
	}
	if (send(coordinadorSocket, &keyStatus, sizeof(char), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	}else{
		log_info(logger,"Send key status to coordinador (%s)", getKeyStatusName(keyStatus));
	}
}
void sendKeyStatusToCoordinadorDummie(char status){

}
void sendMessageExecuteToEsi(Esi* nextEsi){
	int socketEsi = nextEsi->socketConection;
	int message = RUN;
    if (send(socketEsi, &message, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to ESI %d", nextEsi->id);
    }else{
    	log_info(logger,"Send execute message to ESI %d in socket %d",nextEsi->id,nextEsi->socketConection);
    }
}

//General use functions


void addKeyToGeneralKeys(char* key){
	bool itemIsKey(void* item){
		return strcmp(key,(char*)item)==0;
	}
	if(!list_any_satisfy(allSystemTakenKeys,&itemIsKey))
		list_add(allSystemTakenKeys,key);
}

void blockEsi(char* lockedResource, int esiBlocked){
	t_queue* esiQueue;

	if(!dictionary_has_key(blockedEsiDic,lockedResource)){
		log_warning(logger,"Trying to block an ESI in a key that is not already in the dictionary");
		esiQueue= queue_create();
		queue_push(esiQueue,(void*)esiBlocked);
		dictionary_put(blockedEsiDic,lockedResource,esiQueue);
		log_info(logger,"Added ESI (%d) to blocked dictionary in new key (%s)",esiBlocked,lockedResource);

	}else{

		esiQueue = dictionary_get(blockedEsiDic,lockedResource);
		queue_push(esiQueue,(void*)esiBlocked);
		log_info(logger,"Added ESI (%d) to blocked dictionary in existing key (%s)",esiBlocked,lockedResource);
	}
	free(esiQueue);

}

void lockKey(char* key, int esiID){

	addKeyToGeneralKeys(key);

	if(esiID != CONSOLE_BLOCKED){
		addLockedKeyToEsi(&key,&runningEsi);
	}
	if(!dictionary_has_key(blockedEsiDic,key)){
		t_queue* esiQueue = queue_create();
		dictionary_put(blockedEsiDic,key,esiQueue);

	}
	if(isLockedKey(key)==NOTBLOCKED){
		list_add(allSystemTakenKeys,key);
	}


}

Esi* getEsiById(int id){
	bool isId(void* element){
		printf("Holiiis");
		if(((Esi*)element)->id==id)
			return 1;
		return 0;
	}
	Esi* esi =list_get(list_filter(allSystemEsis,&isId),0);
	return esi;
}

void destroyer(void* element){
	free(element);
}

void freeKey(char* key,Esi* esiTaker){
	bool keyCompare(void* takenKey){
		if(string_equals_ignore_case((char*)takenKey,key)){
			return true;
		}
		return false;
	}
	list_remove_by_condition(allSystemTakenKeys,&keyCompare);
	removeLockedKey(key,esiTaker);
	t_queue* blockedEsisQueue = dictionary_get(blockedEsiDic,key);
	Esi* unblockedEsi = NULL;
	if(!queue_is_empty(blockedEsisQueue)){
		unblockedEsi = (Esi*)queue_pop(blockedEsisQueue);
		log_info(logger,"Unblocked ESI %d from key (%s)",unblockedEsi->id,key);
		addEsiToReady(unblockedEsi);
	}
}

char isLockedKey(char* key){
	bool itemIsKey(void* item){
			return strcmp(key,(char*)item)==0;
		}
	if(list_any_satisfy(allSystemTakenKeys,&itemIsKey)){
		return BLOCKED;
	}else{
		return NOTBLOCKED;
	}
}



void addEsiToReady(Esi* esi){
	pthread_mutex_lock(&mutexReadyList);
	list_add(readyEsis,(void*)esi);
	pthread_mutex_unlock(&mutexReadyList);
	sem_post(&readyEsisSemaphore);
}


void exitPlanificador(){
	dictionary_destroy(blockedEsiDic);
	list_destroy(readyEsis);
	list_destroy(finishedEsis);
	list_destroy(allSystemTakenKeys);
	list_destroy(instruccionsByConsoleList);
	sem_destroy(&executionSemaphore);
	sem_destroy(&keyRecievedFromCoordinadorSemaphore);
	sem_destroy(&esiInformationRecievedSemaphore);
	sem_destroy(&readyEsisSemaphore);
	sem_destroy(&consoleInstructionSemaphore);
	log_destroy(logger);
	pthread_cancel(threadConsole);
	pthread_cancel(threadConsoleInstructions);
	pthread_cancel(threadExecution);
	exit(-1);

}

//Planificador setup functions
void addConfigurationLockedKeys(char** blockedKeys){
	int i = 0;

	while(blockedKeys[i]){
		lockKey(blockedKeys[i],CONSOLE_BLOCKED);
		addKeyToGeneralKeys(blockedKeys[i]);
		i++;
	}
	log_info(logger,"All the configuration keys where locked");
}

void getConfig(int* listeningPort, char** algorithm,int* alphaEstimation, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	*alphaEstimation = config_get_int_value(config, "ALPHA_ESTIMATION");
	*initialEstimation = config_get_int_value(config, "ESTIMATION");
	*ipCoordinador = strdup(config_get_string_value(config, "IP_COORDINADOR"));
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*blockedKeys = config_get_array_value(config, "BLOCKED_KEYS");

	config_destroy(config);
}


//New esi functions
Esi* generateEsiStruct(int esiSocket){
	Esi* newEsi = createEsi(actualID,initialEstimation,esiSocket);
	actualID++;
	return newEsi;
}

void welcomeEsi(int clientSocket){
	log_info(logger, "I received a new ESI");
	Esi* newEsi = generateEsiStruct(clientSocket);
	addEsiToReady(newEsi);
	list_add(allSystemEsis,newEsi);

}

int clientHandler(char clientMessage, int clientSocket){

	if (clientMessage == ESIID){
		welcomeEsi(clientSocket);
	}else if(clientMessage == KEYSTATUSMESSAGE){
		log_info(logger,"I recieved a key status message");
		if(recieveString(&keyRecieved,coordinadorSocket)==CUSTOM_FAILURE){
			log_error(logger,"Couldn't recieve key to check from coordinador, quitting...");
			exitPlanificador();

		}
		sem_post(&keyRecievedFromCoordinadorSemaphore);
	}else if(clientMessage == ESIINFORMATIONMESSAGE){
		log_info(logger,"I recieved a esi information message\n");

		esiInformation = waitEsiInformation(nextEsi->socketConection);

		sem_post(&esiInformationRecievedSemaphore);
	}else if(clientMessage == CORDINADORCONSOLERESPONSEMESSAGE){
		log_info(logger,"I recieved a coordinador console response message\n");

	}else{
		log_info(logger, "I received a strange in socket %d", clientSocket);
		//NICO aca esta el problema, esta llegando el 9 que es lock (lo que manda el esi)
		printf("Lo que me llego es %c\n", clientMessage);
		//TODO sacar este exit, esta para probar
		exitPlanificador();
	}

	return 0;
}

int welcomeNewClients(int newCoordinadorSocket){

	coordinadorSocket = newCoordinadorSocket;



	/*
	 * Planificador console
	 * */
	pthread_create(&threadConsole, NULL, (void *) openConsole, NULL);
	/*
	 *  Planificador console
	 * */

	handleConcurrence();



	return 0;
}

int handleConcurrence(){

	fd_set readfds;
	int fdmax, i;
	char clientMessage = 0;
	int resultRecv = 0, serverSocket = 0, clientSocket = 0;

	//revisar el hardcodeo
	serverSocket = openConnection(listeningPort, PLANIFICADOR, "UNKNOWN_CLIENT", logger);
	if(serverSocket < 0){
		//no se pudo conectar!
		return -1;
	}

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&readfds);

	// add the listener to the master set
	FD_SET(serverSocket, &master);
	FD_SET(coordinadorSocket, &master);
	fdmax = serverSocket;


	while(1){
		readfds = master; // copy it
		if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1){
			perror("select");
		}

		// run through the existing connections looking for data to read
		for(i = 0; i <= fdmax; i++){
			if (FD_ISSET(i, &readfds)){ // we got one!!
				if (i == serverSocket){

					clientSocket = acceptUnknownClient(serverSocket, PLANIFICADOR, logger);

					if (clientSocket == -1){
						perror("accept");
					}else{
						FD_SET(clientSocket, &master); // add to master set
						if (clientSocket > fdmax){    // keep track of the max
							fdmax = clientSocket;
						}
					}
				}else{
					clientSocket = i;
					// handle data from a client
					resultRecv = recv(clientSocket, &clientMessage, sizeof(char), 0);
					if(resultRecv <= 0){
						if(resultRecv == 0){
							if(clientSocket == coordinadorSocket){
								log_error(logger, "Coordinador disconnected my planet needs me. Bye bye");
								exitPlanificador();
							}else{
								log_warning(logger, "ESI disconnected. I dont need you anymore");
								abortEsi(getEsiBySocket(clientSocket));
							}

						}else{
							log_error(logger, "Error in recv from %s select: %s\n", PLANIFICADOR, strerror(errno));
							//TODO NICO sacar este exit, no deberia morir el planificador en este caso
							exitPlanificador();
						}

						close(clientSocket);
						FD_CLR(clientSocket, &master);
					}else{
						clientHandler(clientMessage, clientSocket);
					}
				}
			}
		}
	}

	return 0;
}

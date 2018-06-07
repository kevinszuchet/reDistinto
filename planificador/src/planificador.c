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

t_dictionary* blockedEsiDic; //Key->esiQueue  if queue is empty, no one have take the resource
t_list* readyEsis;
t_list* finishedEsis;
Esi* runningEsi;

t_list* allKeys;

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



int actualID = 1; //ID number for ESIs, when a new one is created, this number increases by 1

int coordinadorSocket;

char* keyRecieved;
OperationResponse* esiInformation = NULL;
Esi* nextEsi;


t_list* instruccionsByConsoleList;

int welcomeNewClients();

int main(void) {
	logger = log_create("../planificador.log", "tpSO", true, LOG_LEVEL_INFO);
	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	takenResources=dictionary_create();
	allKeys = list_create();

	blockedEsiDic = dictionary_create();
	addConfigurationLockedKeys(blockedKeys);
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
		exit(-1);
	}



	return 0;
}

//General execute ESI functions

//Core execute function
void executionProcedure(){
	log_info(logger,"Starting to check if an ESI is ready to run");

	//NEW SELECT EXECUTION
	while(1){


		sem_wait(&executionSemaphore);
		pthread_mutex_lock(&mutexReadyList);
		if((list_size(readyEsis)>0 ||runningEsi!=NULL)&&pauseState==CONTINUE){

			if(runningEsi==NULL){
				log_info(logger,"At least an ESI is ready to run\n");

				nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);

				runningEsi = nextEsi;
				pthread_mutex_unlock(&mutexReadyList);
				removeFromReady(nextEsi);
			}else{
				log_info(logger,"Running ESI is ready to run\n");
				nextEsi = runningEsi;
				pthread_mutex_unlock(&mutexReadyList);
			}

			log_info(logger,"Esi %d was selected to execute\n",nextEsi->id);
			sendEsiIdToCoordinador(nextEsi->id);

			sendMessageExecuteToEsi(nextEsi);
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
			sem_post(&consoleInstructionSemaphore);
		}else{
			pthread_mutex_unlock(&mutexReadyList);
			sem_post(&consoleInstructionSemaphore);
		}



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

	log_info(logger,"Finishing esi (%d) \n",runningEsi->id);
	list_add(finishedEsis,runningEsi);
	freeTakenKeys(runningEsi);
	log_info(logger,"Esi (%d) succesfully finished \n",runningEsi->id);
	runningEsi = NULL;


}

void freeTakenKeys(Esi* esi){
	for(int i = 0;i<list_size(esi->lockedKeys);i++){
		char* keyToFree = (char*)list_get(esi->lockedKeys,i);
		freeResource(keyToFree,esi);
		log_info(logger,"Key (%s) was freed by Esi (%d)  \n",keyToFree, esi->id);
	}
	list_clean(esi->lockedKeys);
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
	list_remove(readyEsis,idToRemove);
	pthread_mutex_unlock(&mutexReadyList);
	log_info(logger,"ESI %d removed from ready\n",esi->id);
}

void sendEsiIdToCoordinador(int id){
	if (send(coordinadorSocket, &id, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about ESI id");
	}else{
		log_info(logger,"Send esi ID = %d to coordinador",id);
	}
}

void handleEsiInformation(OperationResponse* esiExecutionInformation,char* key){
	switch(esiExecutionInformation->coordinadorResponse){
		case SUCCESS:
			//Se hizo un SET y salio bien
			log_info(logger,"Operation succeded, nothing to do");
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
					log_info(logger,"Esi finished execution");
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
					if(strcmp(algorithm,"SJF-CD")==0){
						//todo Pasar el esi a ready
						//todo Sacar el esi de running
					}
				break;
			}
		break;

		case LOCK:
			log_info(logger,"Operation succeded");
			takeResource(key,runningEsi->id);
			addKeyToGeneralKeys(key);
			log_info(logger,"Key (%s) locked",key);
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
					log_info(logger,"Esi finished execution");
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
					if(strcmp(algorithm,"SJF-CD")==0){
						moveFromRunningToReady(runningEsi);
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
			//Liberar la clave
			log_info(logger,"Operation succeded, key (%s) freed",key);
			switch(esiExecutionInformation->esiStatus){
				case FINISHED:
					finishRunningEsi();
					log_info(logger,"Esi finished execution");
				break;
				case NOTFINISHED:
					log_info(logger,"Esi didn't finish execution");
					if(strcmp(algorithm,"SJF-CD")==0){
						//todo Pasar el esi a ready
						//todo Sacar el esi de running
					}
				break;
			}
		break;
		case ABORT:
			log_info(logger,"Aborting esi");
			abortEsi(runningEsi);
			runningEsi = NULL;


		break;


	}
	esiInformation = NULL;
}

void abortEsi(Esi* esi){
	log_info(logger,"Waiting to abort esi (%d)", esi->id);
	sleep(3);
	log_info(logger,"Aborting esi (%d)", esi->id);
	freeTakenKeys(esi);

	if(runningEsi!=NULL&& runningEsi->id==esi->id){
		log_info(logger,"Trying to abort running esi, lets save the execution mi lord");
		sem_post(&esiInformationRecievedSemaphore);

	}
	deleteEsiFromSystemBySocket(esi->socketConection);
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
				log_info(logger,"Esi with id (%d) deleted from ready list", actualEsi->id);
			break;
			case INFINISHEDLIST:
				//actualEsi = list_remove_by_condition(finishedEsis,&isEsiBySocket);
				filteredList= list_filter(finishedEsis,&isEsiBySocket);
				actualEsi = list_get(filteredList,list_size(filteredList)-1);
				log_info(logger,"Esi with id (%d) was on finished list lets keep it there", actualEsi->id);
			break;
			case INRUNNING:
				 log_info(logger,"Esi with id (%d) deleted from running", runningEsi->id);
				 runningEsi = NULL;
			break;
			case INBLOCKEDDIC:

				for(int i = 0;i<list_size(allKeys);i++){
					blockedEsis = dictionary_get(blockedEsiDic,list_get(allKeys,i));
					for(int j = 0;j<queue_size(blockedEsis);j++){
						actualEsi = (Esi*)queue_pop(blockedEsis);
						if(actualEsi->socketConection!=socket){
							queue_push(blockedEsis,actualEsi);
						}else{
							log_info(logger,"Esi with id (%d) deleted from blocked dic", actualEsi->id);
						}
					}
				}

			break;
			default:
				log_error(logger,"Couldn't remove ESI with socket (%d)",socket);
				exit(-1);
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
			//return list_get(list_filter(finishedEsis,&isEsiBySocket),0);
			return NULL;
		break;
		case INRUNNING:
			return runningEsi;
		break;
		case INBLOCKEDDIC:

			for(int i = 0;i<list_size(allKeys);i++){
				blockedEsis = dictionary_get(blockedEsiDic,list_get(allKeys,i));
				for(int j = 0;j<queue_size(blockedEsis);j++){
					actualEsi = (Esi*)queue_pop(blockedEsis);
					if(actualEsi->socketConection==socket){
						*targetEsi = *actualEsi;
					}
					queue_push(blockedEsis,actualEsi);
				}
			}
			return targetEsi;
		break;
		default:
			log_error(logger,"Couldn't find ESI by socket (%d)",socket);
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

	for(int i = 0;i<list_size(allKeys);i++){
		blockedEsis = dictionary_get(blockedEsiDic,list_get(allKeys,i));
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
		//Que pasa si recibo mal el mensaje del ESI?
		exit(-1);
	}else{

		//free(finishInformation);
		return finishInformation;
	}
}

char waitEsiInformationDummie(int esiSocket){

	return SUCCESS;
}

void sendKeyStatusToCoordinador(char* key){
	bool keyCompare(void* takenKeys){
		if(strcmp((char*)takenKeys,key)==0){
			return true;
		}
		return false;
	}
	char keyStatus = isTakenResource(key);
	if(keyStatus==BLOCKED){
		log_info(logger,"Key is blocked, lets check if its taken by the ESI");
		if(list_filter(runningEsi->lockedKeys,&keyCompare)){
			keyStatus = LOCKED;
		}
	}
	if (send(coordinadorSocket, &keyStatus, sizeof(char), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	}else{
		log_info(logger,"Send key status to coordinador: %s", getKeyStatusName(keyStatus));
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
void sendMessageExecuteToEsiDummie(Esi* nextEsi){

}


//General use functions

void addKeyToGeneralKeys(char* key){
	bool itemIsKey(void* item){
		return strcmp(key,(char*)item)==0;
	}
	if(!list_any_satisfy(allKeys,&itemIsKey))
		list_add(allKeys,key);
}

void blockEsi(char* lockedResource, int esiBlocked){
	t_queue* esiQueue;// = malloc(sizeof(t_queue));

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

void takeResource(char* key, int esiID){

	if(esiID != CONSOLE_BLOCKED){
		addLockedKey(&key,&runningEsi);
		dictionary_put(takenResources,key,(void*)esiID);
		t_queue* esiQueue = queue_create();
		dictionary_put(blockedEsiDic,key,esiQueue);
		log_info(logger,"Resource (%s) was taken by ESI (%d)",key,esiID);
	}else{
		dictionary_put(takenResources,key,(void*)CONSOLE_BLOCKED);
		t_queue* esiQueue = queue_create();
		dictionary_put(blockedEsiDic,key,esiQueue);
		log_info(logger,"Resource (%s) was taken Console",key);
	}

}

void freeResource(char* key,Esi* esiTaker){
	removeLockedKey(key,esiTaker);

	if(!dictionary_has_key(takenResources,key)){
		log_error(logger,"Trying to free a resource (%s) that is not taken",key);
	}else{
		dictionary_remove(takenResources,key);
		log_info(logger,"Freed resource (%s)",key);

	}

	if(dictionary_has_key(blockedEsiDic,key)){
		t_queue* blockedEsisQueue;
		blockedEsisQueue = dictionary_get(blockedEsiDic,key);
		Esi* unblockedEsi = NULL;
		if(!queue_is_empty(blockedEsisQueue)){
			unblockedEsi = (Esi*)queue_pop(blockedEsisQueue);
			log_info(logger,"Unblocked ESI %d from resource %s",unblockedEsi->id,key);
			addEsiToReady(unblockedEsi);
			log_info(logger,"Added ESI %d to ready",unblockedEsi->id);
		}
	}else{
		log_warning(logger,"Can't release an esi from key (%s), key isn't in the dictionary",key);
	}



}

char isTakenResource(char* key){
	if(dictionary_has_key(takenResources,key)){
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
	log_info(logger, "Added ESI with id=%d and socket=%d to ready list \n",esi->id,esi->socketConection);
}


//Planificador setup functions
void addConfigurationLockedKeys(char** blockedKeys){
	int i = 0;

	while(blockedKeys[i]){
		takeResource(blockedKeys[i],CONSOLE_BLOCKED);
		addKeyToGeneralKeys(blockedKeys[i]);
		i++;
	}
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

int welcomeEsi(int clientSocket){
	log_info(logger, "I received an esi\n");
	Esi* newEsi = generateEsiStruct(clientSocket);
	addEsiToReady(newEsi);

	return 0;
}

int clientHandler(char clientMessage, int clientSocket){

	if (clientMessage == ESIID){
		welcomeEsi(clientSocket);
	}else if(clientMessage == KEYSTATUSMESSAGE){
		log_info(logger,"I recieved a key status message\n");
		if(recieveString(&keyRecieved,coordinadorSocket)==CUSTOM_FAILURE){
			log_error(logger,"Couldn't recieve key to check from coordinador, quitting...");
			exit(-1);
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
		exit(-1);
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
								exit(-1);
							}else{
								log_warning(logger, "ESI disconnected. I dont need you anymore");
								abortEsi(getEsiBySocket(clientSocket));
							}

						}else{
							log_error(logger, "Error in recv from %s select: %s\n", PLANIFICADOR, strerror(errno));
							//TODO NICO sacar este exit, no deberia morir el planificador en este caso
							exit(-1);
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

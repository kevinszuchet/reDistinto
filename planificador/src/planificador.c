/*
 * planificador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"

t_log* logger;

int pauseState = 1; //1 is running, 0 is paussed

t_dictionary* blockedEsiDic; //Key->esiQueue  if queue is empty, no one have take the resource
t_list* readyEsis;
t_list* finishedEsis;
Esi* runningEsi;

int listeningPort;
char* algorithm;
int alphaEstimation;
int initialEstimation;
char* ipCoordinador;
int portCoordinador;
char** blockedKeys;
pthread_t threadConsole;
pthread_t threadExecution;

pthread_mutex_t mutexReadyList = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutexEsiReady = PTHREAD_MUTEX_INITIALIZER;

int executionSemaphore;
int keyRecievedFromCoordinadorSemaphore;
int esiInformationRecievedSemaphore;
int readyEsisSemaphore;

int actualID = 1; //ID number for ESIs, when a new one is created, this number increases by 1

int coordinadorSocket;

char* keyRecieved;
OperationResponse* esiInformation;
Esi* nextEsi;

int welcomeNewClients();

int main(void) {
	logger = log_create("../planificador.log", "tpSO", true, LOG_LEVEL_INFO);
	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	takenResources=dictionary_create();

	blockedEsiDic = dictionary_create();
	addConfigurationLockedKeys(blockedKeys);
	readyEsis = list_create();
	finishedEsis = list_create();
	runningEsi = NULL;



	if((executionSemaphore=semget(IPC_PRIVATE,1,IPC_CREAT | 0700))<0) {
		log_error(logger,"Couln't create semaphore");
	}
	if((keyRecievedFromCoordinadorSemaphore=semget(IPC_PRIVATE,1,IPC_CREAT | 0700))<0) {
		log_error(logger,"Couln't create semaphore");
	}
	if((esiInformationRecievedSemaphore=semget(IPC_PRIVATE,1,IPC_CREAT | 0700))<0) {
		log_error(logger,"Couln't create semaphore");
	}
	if((readyEsisSemaphore=semget(IPC_PRIVATE,1,IPC_CREAT | 0700))<0) {
		log_error(logger,"Couln't create semaphore");
	}
	initSem(keyRecievedFromCoordinadorSemaphore,0,0);
	initSem(esiInformationRecievedSemaphore,0,0);
	initSem(readyEsisSemaphore,0,0);
	initSem(executionSemaphore,0,0);
	pthread_create(&threadExecution,NULL,(void *)executionProcedure,NULL);

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
		while(pauseState){
			doWait(executionSemaphore,0); //POR AHORA ESTA CON UN MUTEX, PASARLO A SEMAFORO DE ESTADO
			log_info(logger,"Time to execute\n");


			if(runningEsi==NULL){
				doWait(readyEsisSemaphore,0);
				doSignal(readyEsisSemaphore,0);

				log_info(logger,"At least an ESI is ready to run\n");
				pthread_mutex_lock(&mutexReadyList);
				nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
				pthread_mutex_unlock(&mutexReadyList);
				runningEsi = nextEsi;
				removeFromReady(nextEsi);
			}else{
				log_info(logger,"Running ESI is ready to run\n");
				nextEsi = runningEsi;
			}
			log_info(logger,"Esi %d was selected to execute\n",nextEsi->id);
			sendEsiIdToCoordinador(nextEsi->id);

			sendMessageExecuteToEsi(nextEsi);
			log_info(logger,"Waiting coordinador request\n");
			doWait(keyRecievedFromCoordinadorSemaphore,0);

			log_info(logger,"Key received = %s\n",keyRecieved);
			sendKeyStatusToCoordinador(keyRecieved);

			log_info(logger,"Waiting esi information\n");
			doWait(esiInformationRecievedSemaphore,0);
			log_info(logger,"Going to handle Esi execution info.CoordinadoResponse = (%s) ,esiStatus = (%s)",getCoordinadorResponseName(esiInformation->coordinadorResponse),getEsiInformationResponseName(esiInformation->esiStatus));
			handleEsiInformation(esiInformation,keyRecieved);
			log_info(logger,"Finish executing one instruction from ESI %d\n",nextEsi->id);
			doSignal(executionSemaphore,0);

		}
	}


	/* OLD EXECUTION
	while(pauseState){ //En vez de esto se puede hacer un semaforo?

		//WAIT AT LEAST ONE ESI TO BE IN READY LIST

		//Obtaining next esi to execute
		if(list_size(readyEsis)>0 || runningEsi != NULL){
			log_info(logger,"At least an ESI is ready to run\n");
			Esi* nextEsi;
			if(runningEsi==NULL){
				pthread_mutex_lock(&mutexReadyList);
				nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
				pthread_mutex_unlock(&mutexReadyList);
				runningEsi = nextEsi;
			}else{
				nextEsi = runningEsi;
			}
			log_info(logger,"Esi %d was selected to execute\n",nextEsi->id);
			sendEsiIdToCoordinador(nextEsi->id);
			removeFromReady(nextEsi);
			sendMessageExecuteToEsi(nextEsi);
			//sendMessageExecuteToEsiDummie(nextEsi);
			char* keyRecieved = malloc(40);;
			log_info(logger,"Waiting coordinador request\n");
			log_info(logger,"...\n");

			if(recieveString(&keyRecieved,coordinadorSocket)==CUSTOM_FAILURE){
				log_error(logger,"Couldn't recieve key to check from coordinador, quitting...");
				exit(-1);
			}
			log_info(logger,"Key received = %s\n",keyRecieved);
			sendKeyStatusToCoordinador(keyRecieved);

			log_info(logger,"Waiting esi information\n");
			OperationResponse* esiInformation = waitEsiInformation(nextEsi->socketConection);

			log_info(logger,"Going to handle Esi execution info.CoordinadoResponse = (%c) ,esiStatus = (%c)",esiInformation->coordinadorResponse,esiInformation->esiStatus);
			handleEsiInformation(esiInformation,keyRecieved);
			log_info(logger,"Finish executing ESI %d\n",nextEsi->id);



		}
	}*/

}


int checkKeyBlocked(char* keyRecieved){
	t_queue* blockedEsis = dictionary_get(blockedEsiDic,keyRecieved);
	if(queue_size(blockedEsis)==0){
		log_info(logger,"Key %s free to use\n");
		return KEYFREE;
	}else{
		return KEYBLOCKED;
	}
}

void unlockEsi(char* key){
	//Saca al esi del diccionario de bloqueados y lo pasa a listos
	log_info(logger,"An ESI was unlocked from key %s (NOT IMPLEMENTED)\n",key);
}

void finishRunningEsi(){
	//Pasa el esi a finalizados
	log_info(logger,"Finishing esi (%d) \n",runningEsi->id);
	list_add(finishedEsis,runningEsi);
	for(int i = 0;i<list_size(runningEsi->lockedKeys);i++){
		char* keyToFree = (char*)list_get(runningEsi->lockedKeys,i);
		freeResource(keyToFree,runningEsi);
		log_info(logger,"Key (%s) was freed by Esi (%d)  \n",keyToFree, runningEsi->id);
	}
	list_clean(runningEsi->lockedKeys);
	runningEsi = NULL;
	log_info(logger,"Esi (%d) succesfully finished \n",runningEsi->id);

}

void removeFromReady(Esi* esi){

	pthread_mutex_lock(&mutexReadyList);
	doWait(readyEsisSemaphore,0);
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
			log_info(logger,"Operation succeded, key (%c) freed",key);
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
}

void abortEsi(Esi* esi){
	//todo liberar todos sus recursos
	//sacar el esi del select
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
void blockEsi(char* lockedResource, int esiBlocked){
	t_queue* esiQueue = malloc(sizeof(esiQueue));

	if(!dictionary_has_key(blockedEsiDic,lockedResource)){
		queue_create(esiQueue);
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
	printf("log1\n");
	if(esiID != CONSOLE_BLOCKED){
		printf("Key (%s) runningEsiID (%d)\n",key,runningEsi->id);
		addLockedKey(&key,&runningEsi);
		dictionary_put(takenResources,key,(void*)esiID);
		log_info(logger,"Resource (%s) was taken by ESI (%d)",key,esiID);
	}else{
		dictionary_put(takenResources,key,(void*)CONSOLE_BLOCKED);
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

	t_queue* blockedEsisQueue;
	blockedEsisQueue = dictionary_get(blockedEsiDic,key);
	Esi* unblockedEsi = NULL;
	if(!queue_is_empty(blockedEsisQueue)){
		unblockedEsi = (Esi*)queue_pop(blockedEsisQueue);
		dictionary_remove(blockedEsiDic,key);
		dictionary_put(blockedEsiDic,key,blockedEsisQueue);
		log_info(logger,"Unblocked ESI %d from resource %s",unblockedEsi->id,key);
		addEsiToReady(unblockedEsi);
		log_info(logger,"Added ESI %d to ready",unblockedEsi->id);
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
	doSignal(readyEsisSemaphore,0);
	log_info(logger, "Added ESI with id=%d and socket=%d to ready list \n",esi->id,esi->socketConection);
}


//Planificador setup functions
void addConfigurationLockedKeys(char** blockedKeys){
	int i = 0;

	while(blockedKeys[i]){
		takeResource(blockedKeys[i],CONSOLE_BLOCKED);
		i++;
	}
}

void getConfig(int* listeningPort, char** algorithm,int* alphaEstimation, int* initialEstimation, char** ipCoordinador, int* portCoordinador, char*** blockedKeys){

	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*alphaEstimation = config_get_int_value(config, "ALPHA_ESTIMATION");
	*initialEstimation = config_get_int_value(config, "ESTIMATION");
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*blockedKeys = config_get_array_value(config, "BLOCKED_KEYS");
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

		//Start planificador task
		log_info(logger,"Start to execute ESIs\n");
		//executionProcedure();
	}else if(clientMessage == KEYSTATUSMESSAGE){
		log_info(logger,"I recieved a key status message\n");
		if(recieveString(&keyRecieved,coordinadorSocket)==CUSTOM_FAILURE){
			log_error(logger,"Couldn't recieve key to check from coordinador, quitting...");
			exit(-1);
		}
		doSignal(keyRecievedFromCoordinadorSemaphore,0);
	}else if(clientMessage == ESIINFORMATIONMESSAGE){
		log_info(logger,"I recieved a esi information message\n");

		esiInformation = waitEsiInformation(nextEsi->socketConection);

		doSignal(esiInformationRecievedSemaphore,0);
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

	log_info(logger,"Starting the execution");
	doSignal(executionSemaphore,0);
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
	fd_set master;
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
							log_error(logger, "The client disconnected from server %s\n", PLANIFICADOR);
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

/*
 * coordinador.c
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

int actualID = 1; //ID number for ESIs, when a new one is created, this number increases by 1

int coordinadorSocket;

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



	pthread_create(&threadExecution,NULL,(void *)executionProcedure,NULL);
	//int coordinadorSocket = ...    y cambiar lo que devuelve welcomeServer por el numero de socket
	//cambio lo que devuelve welcomeServer, para que quede como antes
	welcomeServer(ipCoordinador, portCoordinador, COORDINADOR, PLANIFICADOR, 10, &welcomeNewClients, logger);
	//Te comento esto porque no se usa mas. mira la funcion welcomeNewClients
	/*if (coordinadorSocket < 0){
		//reintentar?
	}*/

	//Start planificador task
	log_info(logger,"Start to execute ESIs\n");
	executionProcedure();

	return 0;
}

//General execute ESI functions

//Core execute function
void executionProcedure(){


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
			Operation* operationRecieved = malloc(sizeof(Operation));
			recieveOperation(operationRecieved,coordinadorSocket);
			//operationRecieved->key = "jugador";
			//operationRecieved->operationCode = OURGET;
			log_info(logger,"Key received = %s , Op received = %c\n",operationRecieved->key,operationRecieved->operationCode);
			int keyStatus = isTakenResource(operationRecieved->key);
			sendKeyStatusToCoordinador(keyStatus);
			//sendKeyStatusToCoordinadorDummie(keyStatus);
			int esiExecutionInformation = waitEsiInformation(nextEsi->socketConection);
			//char esiExecutionInformation = waitEsiInformationDummie(nextEsi->socketConection);
			log_info(logger,"Going to handle Esi execution info = %c",esiExecutionInformation);
			handleEsiInformation(esiExecutionInformation,operationRecieved);
			log_info(logger,"Finish executing ESI %d\n",nextEsi->id);
			free(operationRecieved);


		}
	}

}

//Returns 1 if key is blocked, otherwise return 0
int checkKeyBlocked(char* keyRecieved){
	t_queue* blockedEsis = dictionary_get(blockedEsiDic,keyRecieved);
	if(queue_size(blockedEsis)==0){
		log_info(logger,"Key %s free to use\n");
		return 0;
	}else{
		return 1;
	}
}

void unlockEsi(char* key){
	//Saca al esi del diccionario de bloqueados y lo pasa a listos
	log_info(logger,"An ESI was unlocked from key %s (NOT IMPLEMENTED)\n",key);
}



void finishRunningEsi(){
	//Pasa el esi a finalizados
	//Elimina el esi de running
	log_info(logger,"An ESI was moved to finish list (NOT IMPLEMENTED)\n");

}

void removeFromReady(Esi* esi){
	pthread_mutex_lock(&mutexReadyList);
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
		log_info(logger,"Send esi ID = (%d) to coordinador %d",id);
	}
}

void handleEsiInformation(char esiExecutionInformation,Operation* keyOp){
	switch(esiExecutionInformation){
		case SUCCESS:
			//Se hizo un SET y salio bien
			if(strcmp(algorithm,"SJF-CD")==0){
				//Pasar el esi a ready
				//Sacar el esi de running
			}
		break;
		case FINISHED:
			finishRunningEsi();
		break;
		case LOCK:
			if(strcmp(algorithm,"SJF-CD")==0){
				//Pasar el esi a ready
				//Sacar el esi de running
			}
			//Lockear la clave
		break;
		case BLOCK:
			//ADD ESI TO BLOCKED DIC
			//REMOVE ESI FROM RUNNING
		break;
		case FREE:
			if(strcmp(algorithm,"SJF-CD")==0){
				//Pasar el esi a ready
				//Sacar el esi de running
			}
			//Liberar la clave
		break;


	}
}



char waitEsiInformation(int esiSocket){

	char finishInformation = malloc(sizeof(char));
	int resultRecv = recv(esiSocket, &finishInformation, sizeof(int), 0);
	if(resultRecv <= 0){
		log_error(logger, "recv failed on %s, while waiting ESI message %s\n", ESI, strerror(errno));
		//Que pasa si recibo mal el mensaje del ESI?
		return -1;
	}else{

		free(finishInformation);
		log_info(logger,"Recieved esi finish information (%c)",finishInformation);
		return finishInformation;
	}
}

char waitEsiInformationDummie(int esiSocket){

	return SUCCESS;
}

void sendKeyStatusToCoordinador(char status){
	if (send(coordinadorSocket, &status, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	}else{
		log_info(logger,"Send key status (%d) to coordinador %d",status);
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
	if(esiID == CONSOLE_BLOCKED){
		addLockedKey(key,runningEsi);
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

int isTakenResource(char* key){
	if(dictionary_has_key(takenResources,key)){
		return 1;
	}else{
		return 0;
	}
}



void addEsiToReady(Esi* esi){
	pthread_mutex_lock(&mutexReadyList);
	list_add(readyEsis,(void*)esi);
	pthread_mutex_unlock(&mutexReadyList);
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

int clientHandler(int clientId, int clientSocket){

	if (clientId == 12){
		welcomeEsi(clientSocket);
	}else{
		log_info(logger, "I received a strange\n");
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

	handleConcurrence(listeningPort);



	return 0;
}

int handleConcurrence(int listenerPort){
	fd_set master;
	fd_set readfds;
	int fdmax, i;
	int clientId = 0, resultRecv = 0, serverSocket = 0, clientSocket = 0;

	//revisar el hardcodeo
	serverSocket = openConnection(listenerPort, PLANIFICADOR, "UNKNOWN_CLIENT", logger);
	if(serverSocket < 0){
		//no se pudo conectar!
		return -1;
	}

	FD_ZERO(&master);    // clear the master and temp sets
	FD_ZERO(&readfds);

	// add the listener to the master set
	FD_SET(serverSocket, &master);

	fdmax = serverSocket;

	while(1){
		readfds = master; // copy it
		if (select(fdmax+1, &readfds, NULL, NULL, NULL) == -1) {
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
					resultRecv = recv(clientSocket, &clientId, sizeof(int), 0);
					if(resultRecv <= 0){
						if(resultRecv == 0){
							log_error(logger, "The client disconnected from server %s\n", PLANIFICADOR);
						}else{
							log_error(logger, "Error in recv from %s select: %s\n", PLANIFICADOR, strerror(errno));
							exit(-1);
						}

						close(clientSocket);
						FD_CLR(clientSocket, &master);
					}else{
						clientHandler(clientId, clientSocket);
					}
				}
			}
		}
	}

	return 0;
}

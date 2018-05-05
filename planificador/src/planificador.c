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

	blockedEsiDic = dictionary_create();
	addConfigurationBlockedKeys(blockedKeys);
	readyEsis = list_create();
	finishedEsis = list_create();

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
		if(list_size(readyEsis)>0){
			log_info(logger,"At least an ESI is ready to run\n");
			Esi* nextEsi;
			if(runningEsi==NULL){
				pthread_mutex_lock(&mutexReadyList);
				nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
				pthread_mutex_unlock(&mutexReadyList);
			}else{
				nextEsi = runningEsi;
			}
			log_info(logger,"Esi %d was selected to execute\n",nextEsi->id);
			removeFromReady(nextEsi);
			sendMessageExecuteToEsi(nextEsi);
			Operation operationRecieved;
			recieveOperation(&operationRecieved,coordinadorSocket);
			log_info(logger,"Key received = %s , Op received = %s\n",operationRecieved.key,operationRecieved.operationCode);
			int keyStatus = checkKeyBlocked(operationRecieved.key);
			sendKeyStatusToCoordinador(keyStatus);
			int esiExecutionInformation = waitEsiInformation(nextEsi->socketConection);
			handleEsiInformation(esiExecutionInformation,operationRecieved);
			executionProcedure();

		}
	}

}

//Returns 1 if key is blocked, otherwise return 0
int checkKeyBlocked(char* keyRecieved){
	t_queue* blockedEsis = dictionary_get(blockedEsiDic,keyRecieved);
	if(queue_size(blockedEsis)==0){
		return 0;
	}else{
		return 1;
	}
}

void unlockEsi(char* key){
	//Saca al esi del diccionario de bloqueados y lo pasa a listos
}

void takeResource(char* key, Esi* esi){
	//Pone este esi como taker del diccionario en esa clave
}

void finishRunningEsi(){
	//Pasa el esi a finalizados
	//Elimina el esi de running

}

void removeFromReady(Esi* esi){
	pthread_mutex_lock(&mutexReadyList);
	pthread_mutex_unlock(&mutexReadyList);
}

void handleEsiInformation(int esiExecutionInformation,Operation keyOp){
	switch(esiExecutionInformation){
		case EXITO:
			switch(atoi(&(keyOp.operationCode))){
				case OURSET:
					//Nothing to be done
				break;
				case OURGET:
					takeResource(keyOp.key,runningEsi);
				break;
				case OURSTORE:
					unlockEsi(keyOp.key);
				break;

			}
			if(strcmp(algorithm,"SJF-CD")==0){
				//Pasar el esi a ready
				//Sacar el esi de running
			}
		break;
		case FINALIZADO:
			switch(atoi(&(keyOp.operationCode))){
				case OURSET:
					//Nothing to be done
				break;
				case OURGET:
					takeResource(keyOp.key,runningEsi); //No deberia pasar que un esi que finaliza haga get, pero mejor contemplarlo
				break;
				case OURSTORE:
					unlockEsi(keyOp.key);
				break;

			}
			finishRunningEsi();
		break;
		case FALLA:
			switch(atoi(&(keyOp.operationCode))){
				case OURSET:
					//Nothing to be done
				break;
				case OURGET:
					blockKey(keyOp.key,runningEsi->id);

				break;
				case OURSTORE:
					blockKey(keyOp.key,runningEsi->id);
				break;

			}
		break;
		runningEsi = NULL;

	}
}



int waitEsiInformation(int esiSocket){

	int finishInformation = 0;
	int resultRecv = recv(esiSocket, &finishInformation, sizeof(int), 0);
	if(resultRecv <= 0){
		log_error(logger, "recv failed on %s, while waiting ESI message %s\n", ESI, strerror(errno));
		//Que pasa si recibo mal el mensaje del ESI?
		return -1;
	}else{
		return finishInformation;
	}
}

void sendKeyStatusToCoordinador(int status){
	if (send(coordinadorSocket, &status, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to Coordinador about key status");
	}else{
		log_info(logger,"Send key status (%d) to coordinador %d",status);
	}
}
void sendMessageExecuteToEsi(Esi* nextEsi){
	int socketEsi = nextEsi->id;
	int message = RUN;
    if (send(socketEsi, &message, sizeof(int), 0) < 0){
	   log_error(logger, "Coultn't send message to ESI %d", nextEsi->id);
    }else{
    	log_info(logger,"Send execute message to ESI %d in socket %d",nextEsi->id,nextEsi->socketConection);
    }
}

/* Function to test SJF
void testAlgorithm(){ //Use this to test SJF
	log_info(logger, "Ready esis size = %d\n",list_size(readyEsis));
	if(list_size(readyEsis)>=3){
		log_info(logger, "Run algorithm\n");
		Esi* proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		proxEsi->lastBurst = 5;
		log_info(logger, "Selected ESI to run has id %d\n",proxEsi->id);
		log_info(logger, "Run algorithm 2\n");
		proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		proxEsi->lastBurst = 3;
		log_info(logger, "Selected ESI to run has id %d\n",proxEsi->id);
		log_info(logger, "Run algorithm 3\n");
		proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		log_info("Selected ESI to run has id %d\n",proxEsi->id);
	}
}
*/

//General use functions
void blockKey(char* keyToBlock, int esiBlocked){
	t_queue* esiQueue;
	blocked_queue* blockedQueue;
	if(!dictionary_has_key(blockedEsiDic,keyToBlock)){
		queue_create(esiQueue);
		blockedQueue->blockedEsis = esiQueue;

		blockedQueue->resourceTakerID = -1;
		dictionary_put(blockedEsiDic,keyToBlock,blockedQueue);

	}else{
		if(isTakenResource(keyToBlock)){
			blockedQueue = dictionary_get(blockedEsiDic,keyToBlock);
			dictionary_remove(blockedEsiDic,keyToBlock);
			esiQueue = blockedQueue->blockedEsis;
			queue_push(esiQueue,(void*)esiBlocked);
			blockedQueue->blockedEsis = esiQueue;
			dictionary_put(blockedEsiDic,keyToBlock,blockedQueue);
			log_info(logger, "Blocked esi %d in resource %s that already was taken \n",esiBlocked,keyToBlock);
		}else{
			//If resource isn't taken, its beeing taken by the running esi or by console
			queue_create(esiQueue);
			blockedQueue->blockedEsis = esiQueue;
			if(esiBlocked == CONFIG_BLOCKED){

				blockedQueue->resourceTakerID = CONFIG_BLOCKED;
			}else{
				blockedQueue->resourceTakerID = runningEsi->id;
			}

			dictionary_put(blockedEsiDic,keyToBlock,blockedQueue);
			log_info(logger, "Esi %d has taken resource %s \n",esiBlocked,keyToBlock);
		}
	}

}

int isTakenResource(char* key){
	blocked_queue* blockedQueue;
	if(dictionary_has_key(blockedEsiDic,key)){
		blockedQueue = dictionary_get(blockedEsiDic,key);
		if(blockedQueue->resourceTakerID!=-1){
			return 1;
		}else{
			return 0;
		}
	}else{
		return -1; //Key doesn't exists
	}
}



void addEsiToReady(Esi* esi){
	pthread_mutex_lock(&mutexReadyList);
	list_add(readyEsis,(void*)esi);
	pthread_mutex_unlock(&mutexReadyList);
	log_info(logger, "Added ESI with id=%d and socket=%d to ready list \n",esi->id,esi->socketConection);
}

//Planificador setup functions
void addConfigurationBlockedKeys(char** blockedKeys){
	int i = 0;

	while(blockedKeys[i]){
		blockKey(blockedKeys[i],CONFIG_BLOCKED);
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

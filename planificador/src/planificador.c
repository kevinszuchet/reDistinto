/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"



int pauseState = 1; //1 is running, 0 is paussed

t_dictionary* blockedEsiDic;
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

int actualID = 1; //ID number for ESIs, when a new one is created, this number aumments by 1

int welcomeNewClients();

int main(void) {

	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);

	blockedEsiDic = dictionary_create();
	addConfigurationBlockedKeys(blockedKeys);
	readyEsis = list_create();
	finishedEsis = list_create();

	int welcomeResponse = welcomeServer(ipCoordinador, portCoordinador, COORDINADOR, PLANIFICADOR, 10, &welcomeNewClients);
	if (welcomeResponse < 0){
		//reintentar?
	}

	return 0;
}

void executeEsi(int esiID){
	//Obtengo el socket del ESI
	Esi* nextEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
	int socketEsi = nextEsi->id;
	//Le mando un mensaje al socket

	//Lanzo un hilo para esperar la respuesta del ESI

	//Puedo obtener que se ejecuto correctamente, que se ejecuto correctamente Y FINALIZO o un FALLO en la operacion
}

void testAlgorithm(){ //Use this to test SJF
	printf("Ready esis size = %d\n",list_size(readyEsis));
	if(list_size(readyEsis)>=3){
		printf("Run algorithm\n");
		Esi* proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		proxEsi->lastBurst = 5;
		printf("Selected ESI to run has id %d\n",proxEsi->id);
		printf("Run algorithm 2\n");
		proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		proxEsi->lastBurst = 3;
		printf("Selected ESI to run has id %d\n",proxEsi->id);
		printf("Run algorithm 3\n");
		proxEsi = nextEsiByAlgorithm(algorithm,alphaEstimation,readyEsis);
		printf("Selected ESI to run has id %d\n",proxEsi->id);
	}
}

void blockKey(char* keyToBlock, int esiBlocked){
	t_queue* esiQueue = queue_create();
	if(dictionary_has_key(blockedEsiDic,keyToBlock)){
		esiQueue = dictionary_get(blockedEsiDic,keyToBlock);
		dictionary_remove(blockedEsiDic,keyToBlock);
		queue_push(esiQueue,(void*)esiBlocked);
		dictionary_put(blockedEsiDic,keyToBlock,esiQueue);
		printf("Blocked esi %d in resource %s that already was taken \n",esiBlocked,keyToBlock);
	}else{
		queue_push(esiQueue,(void*)esiBlocked);
		dictionary_put(blockedEsiDic,keyToBlock,esiQueue);
		printf("Blocked esi %d in resource %s \n",esiBlocked,keyToBlock);
	}
}



Esi* generateEsiStruct(int esiSocket){

	Esi* newEsi = createEsi(actualID,initialEstimation,esiSocket);
	actualID++;
	return newEsi;
}

void addEsiToReady(Esi* esi){
	list_add(readyEsis,(void*)esi);
	printf("Added ESI with id=%d and socket=%d to ready list \n",esi->id,esi->socketConection);
}

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

int welcomeEsi(int clientSocket){
	printf("I received an esi\n");
	Esi* newEsi = generateEsiStruct(clientSocket);
	addEsiToReady(newEsi);

	testAlgorithm();

	return 0;
}

int clientHandler(int clientId, int clientSocket){

	if (clientId == 13){
		welcomeEsi(clientSocket);
	}else{
		printf("I received a strange\n");
	}

	return 0;
}

int welcomeNewClients(){

	/*
	 * Planificador console
	 * */
	pthread_create(&threadConsole, NULL, (void *) openConsole, NULL);
	/*
	 *  Planificador console
	 * */

	handleConcurrence(listeningPort, &clientHandler, PLANIFICADOR);



	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"




int listeningPort;
char* algorithm;
int alphaEstimation;
int initialEstimation;
char* ipCoordinador;
int portCoordinador;
char** blockedKeys;



int main(void) {

	getConfig(&listeningPort, &algorithm,&alphaEstimation, &initialEstimation, &ipCoordinador, &portCoordinador, &blockedKeys);


	blockedEsiDic = dictionary_create();
	addConfigurationBlockedKeys(blockedKeys);
	readyEsis = list_create();
	finishedEsis = list_create();
	printf("Genero los esi para testear\n");
	generateTestEsi();
	int welcomeResponse = welcomeServer("127.0.0.1", 8080, COORDINADOR, PLANIFICADOR, 10, &welcomeCoordinador);
	if (welcomeResponse < 0){
		//reintentar?
	}
	/*
	 * Planificador console
	 * */
	//crear el hilo!
	openConsole();
	/*
	 *  Planificador console
	 * */

	return 0;
}

void generateTestEsi(){
	Esi* esi1 = createEsi(1,initialEstimation,1);
	Esi* esi2 = createEsi(2,initialEstimation,2);
	Esi* esi3 = createEsi(3,initialEstimation,3);
	list_add(readyEsis,(void*)esi1);
	printf("Agregue el esi con id=%d\n",esi1->id);
	list_add(readyEsis,(void*)esi2);
	printf("Agregue el esi con id=%d\n",esi2->id);
	runningEsi = esi3;


}

void addConfigurationBlockedKeys(char** blockedKeys){
	int i = 0;
	t_queue* newBlockedKey = queue_create();
	while(blockedKeys[i]){
		dictionary_put(blockedEsiDic,blockedKeys[i],(void*)newBlockedKey);
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

int welcomeEsi(){
	for(;;);

	return 0;
}

int welcomeCoordinador(){
	printf("Probando recepcion en el coordinador\n");
	/*int welcomePlanificador = welcomeClient(8082, COORDINADOR, ESI, 12, &welcomeEsi);
	if(welcomePlanificador < 0){
		//TODO: que pasa en este caso?
		return -1;
	}*/

	return 0;
}

/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"
//TODO cambiar esta por la posta
#define CLAVE_PROVISORIA_ERROR_A_PLANIFICADOR '30'
#define CLAVE_PROVISORIA_CLABE_BLOQUEADA_DE_PLANIFICADOR '31'

t_log* logger;
t_log* operationsLogger;

int planificadorSocket;
void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;
t_list* fallenInstancias;
int delay;
int lastInstanciaChosen = 0;
int firstAlreadyPass = 0;

int main(void) {
	logger = log_create("../coordinador.log", "tpSO", true, LOG_LEVEL_INFO);
	operationsLogger = log_create("../logOperaciones.log", "tpSO", true, LOG_LEVEL_INFO);

	int listeningPort;
	char* algorithm;
	int cantEntry;
	int entrySize;
	getConfig(&listeningPort, &algorithm, &cantEntry, &entrySize, &delay);

	printf("Puerto = %d\n", listeningPort);
	printf("Algoritmo = %s\n", algorithm);
	printf("Cant entry = %d\n", cantEntry);
	printf("Entry size= %d\n", entrySize);
	printf("delay= %d\n", delay);

	setDistributionAlgorithm(algorithm);

	instancias = list_create();
	fallenInstancias = list_create();

	welcomeClient(listeningPort, COORDINADOR, PLANIFICADOR, 10, &welcomePlanificador, logger);

	return 0;
}

void getConfig(int* listeningPort, char** algorithm, int* cantEntry, int* entrySize, int* delay){
	t_config* config;
	config = config_create(CFG_FILE);
	*listeningPort = config_get_int_value(config, "LISTENING_PORT");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*cantEntry = config_get_int_value(config, "CANT_ENTRY");
	*entrySize = config_get_int_value(config, "ENTRY_SIZE");
	*delay = config_get_int_value(config, "DELAY");
}

int instanciaIsNextToActual(Instancia* instancia){
	if(lastInstanciaChosen == 0 && firstAlreadyPass == 0){
		firstAlreadyPass = 1;
		return 1;
	}
	return instancia->id >= lastInstanciaChosen + 1;
}

Instancia* equitativeLoad(char* keyToBeBlocked){
	Instancia* instancia = list_find(instancias, (void*) &instanciaIsNextToActual);
	if(instancia == NULL){
		instancia = list_get(instancias, 0);
	}
	lastInstanciaChosen = instancia->id;
	return instancia;
}

Instancia* leastSpaceUsed(char* keyToBeBlocked){
	//TODO
	return list_get(instancias, 0);
}

Instancia* keyExplicit(char* keyToBeBlocked){
	//TODO
	return list_get(instancias, 0);
}

void setDistributionAlgorithm(char* algorithm){
	if(strcmp(algorithm, "EL") == 0){
		distributionAlgorithm = &equitativeLoad;
	}else if(strcmp(algorithm, "LSU") == 0){
		distributionAlgorithm = &leastSpaceUsed;
	}else if(strcmp(algorithm, "KE") == 0){
		distributionAlgorithm = &keyExplicit;
	}else{
		printf("Couldn't determine the distribution algorithm\n");
		//loggear el error
		//matar al coordinador
	}
}

Instancia* chooseInstancia(char* keyToBeBlocked){
	if(list_size(instancias) != 0){
		return (*distributionAlgorithm)(keyToBeBlocked);
	}
	return NULL;
}

void informPlanificador(Operation* operation, char operationCode){
	//TODO aca tambien hay que reintentar hasta que se mande todo?
	//TODO que pasa cuando se pasa una constante por parametro? vimos que hubo drama con eso
	if(send(planificadorSocket, operationCode, sizeof(char), 0) < 0){

	}
}

void sendResponseToEsi(int esiSocket, int response){

}

int getActualEsiID(){
	int esiId = 0;
	int recvResult = recv(planificadorSocket, &esiId, sizeof(int), 0);
	if(recvResult <= 0){
		if(recvResult == 0)
		log_error(logger, "Planificador disconnected from coordinador, quitting...");
		//TODO decidamos: de aca en mas hacemos exit si muere la conexion con el planificador?
		//Misma decision que en recieveKeyStatusFromPlanificador
		exit(-1);
	}

	return esiId;
}

void logOperation(char* stringToLog){
	log_info(operationsLogger, stringToLog);
}

Instancia* lookForKey(char* key){
	int isLookedKey(char* actualKey){
		if(strcmp(actualKey, key)){
			return 0;
		}
		return 1;
	}

	int isKeyInInstancia(Instancia* instancia){
		if(list_any_satisfy(instancia->storedKeys, (void*) &isLookedKey)){
			return 1;
		}
		return 0;
	}

	//TODO hay que castear el return del find? Supongo que no por el tipo de retorno de esta func
	return list_find(instancias, (void*) &isKeyInInstancia);
}

Instancia* lookForOrChoseInstancia(int esiSocket, char* key){
	Instancia* chosenInstancia;
	chosenInstancia = lookForKey(key);

	showInstancia(chosenInstancia);

	if(chosenInstancia == NULL){
		chosenInstancia = chooseInstancia(key);
		//TODO sacar este show
		showInstancia(chosenInstancia);
		if (chosenInstancia == NULL){
			return NULL;
		}
	}

	return chosenInstancia;
}

int instanciaDoStore(Instancia* instancia, EsiRequest* esiRequest){
	return 0;
}

int doSet(EsiRequest* esiRequest, char** stringToLog){
	Instancia* chosenInstancia = lookForOrChoseInstancia(esiRequest->socket, esiRequest->operation->key);

	if(chosenInstancia == NULL){
		informPlanificador(esiRequest->operation->key, CLAVE_PROVISORIA_ERROR_A_PLANIFICADOR);
		sendResponseToEsi(esiRequest->socket, FALLA);
		return -1;
	}

	if (sendOperation(esiRequest->operation, chosenInstancia->socket) == 0){
		//que pasa aca?
		return -1;
	}

	//TODO important ojo con este recv, ya que se esta escuchando a la instancia en su hilo..
	int response = waitForInstanciaResponse(chosenInstancia);
	if(response < 0){
		informPlanificador(esiRequest->operation->key, CLAVE_PROVISORIA_ERROR_A_PLANIFICADOR);
		sendResponseToEsi(esiRequest->socket, FALLA);
		return -1;
	}

	//TODO chequear que lo que nos devuelve la instancia es lo mismo que espera el ESI
	sendResponseToEsi(esiRequest->socket, response);

	//5 es el tamanio de "ESI X" necesario para loggear, siendo X el id de esi. el +1 del medio...
	//es por el espacio entre key y value
	*stringToLog = malloc(5 + strlen(esiRequest->operation->key) + 1 + strlen(esiRequest->operation->value) + 1);
	//TODO hacer el string necesario
	//sprintf();
	//strcpy();

	return 0;
}

int doStore(EsiRequest* esiRequest, char** stringToLog){
	Instancia* chosenInstancia = NULL;
	chosenInstancia = lookForKey(esiRequest->operation->key);
	if(chosenInstancia == NULL){
		//TODO avisar al planificador que no se puede hacer store de clave inexistente
		informPlanificador(esiRequest->operation, ERROR_CLAVE_NON_EXISTENT);
		return -1;
	}

	if (instanciaDoStore(chosenInstancia, esiRequest) < 0){
		//que pasa aca?
		return -1;
	}

	int response = waitForInstanciaResponse(chosenInstancia);
	if(response < 0){
		//TODO habria que buscar con otra? es medio rebuscado creo
		instanciaHasFallen(chosenInstancia, instancias, fallenInstancias);
		informPlanificador(esiRequest->operation, ERROR_CLAVE_NON_EXISTENT);
		sendResponseToEsi(esiRequest->socket, FALLA);
		return -1;
	}

	//TODO chequear que lo que nos devuelve la instancia es lo mismo que espera el ESI
	sendResponseToEsi(esiRequest->socket, response);

	*stringToLog = malloc(5 + strlen(esiRequest->operation->key) + 1);
	//TODO hacer el string necesario

	return 0;
}

int doGet(EsiRequest* esiRequest, char** stringToLog){
	if(lookForKey(esiRequest->operation->key) == NULL){
		informPlanificador(esiRequest->operation, ERROR_CLAVE_NON_EXISTENT);
	}

	//TODO se manda un lock o ourget? estan ambos en nuestras commons
	informPlanificador(esiRequest->operation, EXITO);

	//TODO hay que considerar los tamanios de int para crear el string?
	//*stringToLog = malloc(5 + strlen(value) + 1);
	//sprintf(*stringToLog, "ESI %d - GET %s", esiId, value);
	return 0;
}

char recieveKeyStatusFromPlanificador(){
	char response = 0;
	int recvResult = recv(planificadorSocket, &response, sizeof(char), 0);
	if(recvResult <= 0){
		if(recvResult == 0){
			log_error(logger, "Planificador disconnected from coordinador, quitting...");
			//TODO decidamos: de aca en mas hacemos exit si muere la conexion con el planificador?
			exit(-1);
		}
	}
	return response;
}

int recieveStentenceToProcess(int esiSocket){
	printf("Voy a recibir el id del planificador\n");
	int esiId = getActualEsiID();
	char* stringToLog;

	//TODO fijarse con el planificador si la clave esta bloqueada
	char keyStatus = recieveKeyStatusFromPlanificador();
	if(keyStatus == CLAVE_PROVISORIA_CLABE_BLOQUEADA_DE_PLANIFICADOR){
		//TODO ojo, no siempre: creo que si es el mismo esi que la bloqueo, se puede hacer
		//habria que hablar con el planificador para que nos diga si en ese caso manda error, no deberia
		return -1;
	}

	Operation* operation = malloc(sizeof(Operation));
	if(recieveOperation(operation, esiSocket) < 1){
		//TODO en este caso se mata al esi?
		informPlanificador(operation, FALLA);
	}

	EsiRequest esiRequest;
	esiRequest.id = esiId;
	esiRequest.operation = operation;
	esiRequest.socket = esiSocket;

	//TODO validar los siguientes casos, los 3, por si es necesario matar al hilo
	//(y cada uno de ellos avisara al planificador/esi lo que corresponda)
	switch (esiRequest.operation->operationCode){
		case OURSET:
			if(doSet(&esiRequest, &stringToLog) < 0){
				return -1;
			}
			break;
		case OURSTORE:
			doStore(&esiRequest, &stringToLog);
			break;
		case OURGET:
			doGet(&esiRequest, &stringToLog);
			break;
		default:
			//deberiamos matar al esi?
			//TODO se puede hacer informPlanificador con un error, pero hay que ver si esta esperando eso
			break;
	}

	//TODO descomentar
	//logOperation(stringToLog);

	free(operation);
	free(stringToLog);
	//lo pide la consigna
	//sleep(delay);
	return 0;
}

int handleInstancia(int instanciaSocket){
	log_info(logger, "An instancia thread was created\n");
	//TODO hay que meter un semaforo para evitar conflictos de los diferentes hilos

	//TODO que pasa si una instancia se recupera? como la distinguimos?
	//si seguimos este camino, se va a crear una nueva y no queremos
	createNewInstancia(instanciaSocket, instancias);

	int recvResult = 0;
	while(1){
		int instanciaResponse = 0;
		recvResult = recv(instanciaSocket, &instanciaResponse, sizeof(int), 0);
		if(recvResult <= 0){
			if(recvResult == 0){
				printf("Instancia on socket %d has fallen\n", instanciaSocket);

				//handlear que pasa en este caso. podriamos guardar el id de la instancia caida, sacar la instancia de la lista...
				//de instancias. Despues, cuando se reincorpore, levantarla de ahi

				close(instanciaSocket);

				//si se cae la instancia, se cae su hilo
				break;
			}
		}else{
			//TODO handlear la respuesta normal de ejecucion en una instancia
		}
	}
	return 0;
}

int handleEsi(int esiSocket){
	log_info(logger,"An esi thread was created\n");
	//TODO descomentar esto, esta porque el esi cae y los recv dan 0 y todavia...
	//...no se valida lo de aca abajo
	while(1){
		//TODO validar el retorno del recieve para que el hilo muera en caso de fallar algo
		recieveStentenceToProcess(esiSocket);
	}
	return 0;
}

void pthreadInitialize(void* clientSocket){
	int castedClientSocket = *((int*) clientSocket);
	int id = recieveClientId(castedClientSocket, COORDINADOR, logger);

	if (id == 11){
		handleInstancia(castedClientSocket);
	}else if(id == 12){
		handleEsi(castedClientSocket);
	}else{
		log_info(logger,"I received a strange\n");
	}

	free(clientSocket);
}

int clientHandler(int clientSocket){
	pthread_t clientThread;
	int* clientSocketPointer = malloc(sizeof(int));
	*clientSocketPointer = clientSocket;
	if(pthread_create(&clientThread, NULL, (void*) &pthreadInitialize, clientSocketPointer)){
		log_error(logger, "Error creating thread\n");
		return -1;
	}

	if(pthread_detach(clientThread) != 0){
		log_error(logger,"Couldn't detach thread\n");
		return -1;
	}

	return 0;
}

int welcomePlanificador(int coordinadorSocket, int newPlanificadorSocket){
	planificadorSocket = newPlanificadorSocket;
	log_info(logger, "%s recieved %s, so it'll now start listening esi/instancia connections\n", COORDINADOR, PLANIFICADOR);
	while(1){
		int clientSocket = acceptUnknownClient(coordinadorSocket, COORDINADOR, logger);
		//TODO validar el retorno
		clientHandler(clientSocket);
	}

	return 0;
}

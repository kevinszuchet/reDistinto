/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "coordinador.h"

t_log* logger;
t_log* operationsLogger;

int planificadorSocket;
void setDistributionAlgorithm(char* algorithm);
Instancia* (*distributionAlgorithm)(char* keyToBeBlocked);
t_list* instancias;
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

void informPlanificador(char* key){

}

void sendResponseToEsi(int esiSocket, int response){

}

int getActualEsiID(){
	int esiId = 0;
	if(recv(planificadorSocket, &esiId, sizeof(int), 0) <= 0){

		return -1;
	}

	return esiId;
}

//TODO estaria bueno manejarnos con otro tad de esi que conozca todos los componentes recibidos...
//esto es: los dos tamanios y los dos valores, para evitar pasar tantos parametros
int sendRequest(Instancia* chosenInstancia, char* key, char* value){
	return 0;
}

void logOperation(char* stringToLog){
	log_info(operationsLogger, stringToLog);
}

/* TODO A DEFINIR
 *
 * Con este metodo estas suponiendo que te va a venir el payload seguido del size
 *
 * Con solo un parametro no hay problema pero si usas esta funcion en el get
 * por como tenemos definido el mensaje no te va a funcionar
 * Necesitas: size - value - size  - value
 * Tenemos  : size - size  - value - value
 *
 * Se puede cambiar el protocolo ya que este metodo parece que nos simplifica bastante
 * Se puede pasar a las commons para que todos lo aprovechen
 */
char* recieveAccordingToSize(int socket){
	int size = 0;
	if(recv(socket, &size, sizeof(int), 0) <= 0){
		return NULL;//para que no te joda los tipos, devolve null en estos casos
	}

	char* recieved = malloc(size);
	if (recv(socket, &recieved, size, 0) <= 0){
		return NULL;
	}

	return recieved;
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
	//TODO descomentar esto, es para probar el algoritmo de distribucion
	chosenInstancia = NULL;

	showInstancia(chosenInstancia);

	if(chosenInstancia == NULL){
		chosenInstancia = chooseInstancia(key);
		//TODO sacar este show
		showInstancia(chosenInstancia);
		if (chosenInstancia == NULL){
			//TODO chequear que se le manda al planificador
			informPlanificador(key);
			sendResponseToEsi(esiSocket, EXECUTION_ERROR);
			return NULL;
		}
	}

	return chosenInstancia;
}

//TODO hay un modulo de serializacion vacio en las commons
/*
 * Hay que definir bien con el encargado de las instancias el formato de los mensajes
 * y usar el modulo de las commons
 */
void* addToPackage(void* package, void* value, int size, int* offset) {
	package = realloc(package, size);
	memcpy(package + *offset, value, size);
	*offset += size;
}

//TODO Mover a commons
//Usar send all en vez de send para aseguranos que se mande todo el package
bool send_all(int socket, char *package, int length)
{
    char *auxPointer = package;
    while (length > 0)
    {
        int i = send(socket, auxPointer, length, 0);
        if (i < 1) return false;
        auxPointer += i;
        length -= i;
    }
    return true;
}

int instanciaDoSet(Instancia* instancia, EsiRequest* esiRequest){
	int offset = 0;
	void* package;

	int sizeKey = strlen(esiRequest->operation->key);
	int sizeValue = strlen(esiRequest->operation->value);

	addToPackage(package, OURSET, sizeof(OURSET), &offset);
	addToPackage(package, sizeKey, sizeof(sizeKey), &offset);
	addToPackage(package, sizeValue, sizeof(sizeValue), &offset);
	addToPackage(package, esiRequest->operation->key, sizeKey, &offset);
	addToPackage(package, esiRequest->operation->value, sizeValue, &offset);

	send_all(instancia->socket, package, offset);

	return 0;

	/*
	//agrego operationCode
	int operationCode = OURSET;
	char* package = malloc(sizeof(operationCode));
	memcpy(package, &operationCode, sizeof(OURSET));
	offset += sizeof(OURSET);
	//agrego sizeClave
	int sizeKey = strlen(key) + 1;//Para incluir \0 pongo +1
	package = realloc(package, offset + sizeof(sizeKey));
	memcpy(package + offset, &sizeKey, sizeof(sizeKey));
	offset += sizeof(sizeKey);
	//agrego sizeValue
	int sizeValue = strlen(value) + 1;//Para incluir \0 pongo +1
	package = realloc(package, offset + sizeof(sizeValue));
	memcpy(package + offset, &sizeValue, sizeof(sizeValue));
	offset += sizeof(sizeValue);
	//agrego clave
	package = realloc(package, offset + sizeKey);
	memcpy(package + offset, key, sizeKey);
	offset += sizeKey;
	//agrego value
	package = realloc(package, offset + sizeValue);
	memcpy(package + offset, value, sizeValue);
	offset += sizeValue;
	*/
}

int instanciaDoStore(Instancia* instancia, EsiRequest* esiRequest){
	return 0;
}

int doSet(EsiRequest* esiRequest, char** stringToLog){

	//TODO fijarse con el planificador si la clave esta bloqueada

	Instancia* chosenInstancia = lookForOrChoseInstancia(esiRequest->socket, esiRequest->operation->key);

	//TODO descomentar esto, es para probar el algoritmo de distribucion
	if(chosenInstancia == NULL){
		return -1;
	}

	//TODO mariano: instanciaDoSet no se podria reemplazar directamente por la funcion de serialization.c?
	if (instanciaDoSet(chosenInstancia, esiRequest) < 0){
		//que pasa aca?
		return -1;
	}

	int response = waitForInstanciaResponse(chosenInstancia);
	if(response < 0){
		informPlanificador(esiRequest->operation->key);
		sendResponseToEsi(esiRequest->socket, EXECUTION_ERROR);
		return -1;
	}

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

	//TODO fijarse con el planificador si la clave esta bloqueada

	Instancia* chosenInstancia = NULL;
	//TODO la funcion lookForKey
	//chosenInstancia = lookForKey(key);
	if(chosenInstancia == NULL){
		//TODO avisar al planificador que no se puede hacer store de clave inexistente
	}

	if (instanciaDoStore(chosenInstancia, esiRequest) < 0){
		//que pasa aca?
		return -1;
	}

	int response = waitForInstanciaResponse(chosenInstancia);
	if(response < 0){
		informPlanificador(esiRequest->operation->key);
		sendResponseToEsi(esiRequest->socket, EXECUTION_ERROR);
		return -1;
	}

	sendResponseToEsi(esiRequest->socket, response);

	*stringToLog = malloc(5 + strlen(esiRequest->operation->key) + 1);
	//TODO hacer el string necesario

	return 0;
}

int doGet(EsiRequest* esiRequest, char** stringToLog){
	//TODO ver como se le va avisar al planificador
	informPlanificador(esiRequest->operation->value);

	//TODO hay que considerar los tamanios de int para crear el string?
	//*stringToLog = malloc(5 + strlen(value) + 1);
	//sprintf(*stringToLog, "ESI %d - GET %s", esiId, value);
	return 0;
}

int recieveStentenceToProcess(int esiSocket){
	int esiId = getActualEsiID();
	char* stringToLog;

	//TOOD mariano recibir la estructura operacion (sacar este NULL)
	EsiRequest* esiRequest = NULL;

	//TODO validar los siguientes casos, los 3, por si es necesario matar al hilo
	//(y cada uno de ellos avisara al planificador/esi lo que corresponda)
	switch (esiRequest->operation->operationCode){
		case OURSET:
			if(doSet(esiRequest, &stringToLog) < 0){
				return -1;
			}
			break;
		case OURSTORE:
			doStore(esiRequest, &stringToLog);
			break;
		case OURGET:
			doGet(esiRequest, &stringToLog);
			break;
		default:
			//deberiamos matar al esi?
			break;
	}

	//TODO descomentar
	//logOperation(stringToLog);

	//TODO liberar todo lo necesario en esiRequest. se podria delegar en una funcion

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
	//while(1){
		//TODO validar el retorno del recieve para que el hilo muera en caso de fallar algo
		recieveStentenceToProcess(esiSocket);
	//}
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

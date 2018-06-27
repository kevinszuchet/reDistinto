/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include "instancia.h"

t_log * logger;
char* path;

int main(void) {

	logger = log_create("../instancia.log", "tpSO", true, LOG_LEVEL_INFO);
	initSerializationLogger(logger);
	replaceAlgorithmsLogger = log_create("../replaceAlgorithms.log", "tpSO", true, LOG_LEVEL_INFO);

	char* ipCoordinador;
	int portCoordinador;
	char * name;
	int dump;

	getConfig(&ipCoordinador, &portCoordinador, &algorithm, &path, &name, &dump);

	printf("IP coord = %s\n", ipCoordinador);
	printf("Puerto = %d\n", portCoordinador);
	printf("Algoritmo = %s\n", algorithm);
	printf("Path = %s\n", path);
	printf("Name= %s\n", name);
	printf("Dump= %d\n", dump);
	log_info(logger, "trying to connect to coordinador...");

	/*
	 * Creates path directory with mkdir (if it does not exists)
	 * S_IRWXU: User mode to Read, Write and eXecute
	 * */

	mkdir(path, S_IRWXU);

	// TODO Tratar de levantar la instancia si existe una carpeta con el mismo nombre
	// void tryToComeAliveAgain(name)

	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, INSTANCIA, logger);

	if (coordinadorSocket < 0) {
		//reintentar conexion?
		log_error(logger, "An error has occurred while trying to connect to coordinador\n socket number: %d", coordinadorSocket);
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, INSTANCIAID, INSTANCIA, logger);

	sendMyNameToCoordinador(name, coordinadorSocket);
	receiveCoordinadorConfiguration(coordinadorSocket);
	waitForCoordinadorStatements(coordinadorSocket);

	free(ipCoordinador);
	free(algorithm);
	free(path);
	free(name);

	finish();
	return 0;
}

void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char**path, char** name, int* dump) {

	t_config* config;
	config = config_create(CFG_FILE);
	*ipCoordinador = strdup(config_get_string_value(config, "IP_COORDINADOR"));
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	*path = strdup(config_get_string_value(config, "PATH"));
	*name = strdup(config_get_string_value(config, "NAME"));
	*dump = config_get_int_value(config, "DUMP");
	config_destroy(config);
}

// Functions

void sendMyNameToCoordinador(char * name, int coordinadorSocket) {
	if (sendString(name, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send my name to coordinador");
		exit(-1);
	}
	log_info(logger, "I send my name to coordinador");
}

char* recieveKeyFromCoordinador(int coordinadorSocket){
	char* key;
	if(recieveString(&key, coordinadorSocket) == CUSTOM_FAILURE){
		log_warning(logger, "Couldn't receive key from coordinador");
		exit(-1);
	}
	return key;
}

void recieveKeysFromCoordinador(int coordinadorSocket){
	int keysAmount;
	if(recieveInt(&keysAmount, coordinadorSocket) == CUSTOM_FAILURE){
		log_error(logger, "Cannot recieve number of keys from coordinador");
		exit(-1);
	}

	if(keysAmount == 0){
		log_info(logger, "There are no keys to raise");
		return;
	}

	for(int i = 0 ; i < keysAmount ; i++){
		//TODO kiwo fijate si hay que liberar esta clave
		//y fijarse de levantar de archivos solamente las que recibas, y tal vez deberias borrar los archivos de las que no (pensar si sirve,
		//tal vez el hecho de no borrarlos nos sirve para chequear algo)
		char* recievedKey = recieveKeyFromCoordinador(coordinadorSocket);
		log_info(logger, "I recieved key %s", recievedKey);
	}
}

void receiveCoordinadorConfiguration(int coordinadorSocket) {
	InstanciaConfiguration instanciaConfiguration;

	log_info(logger, "I am waiting for the last details (sent by coordinador)");

	if (recv_all(coordinadorSocket, &instanciaConfiguration, sizeof(InstanciaConfiguration)) == CUSTOM_FAILURE) {
		log_error(logger, "Cannot recieve configuration from coordinador");
		exit(-1);
	}

	recieveKeysFromCoordinador(coordinadorSocket);

	log_info(logger, "I recieve the coordinador configuration, so I can work");

	initialize(instanciaConfiguration.entriesAmount, instanciaConfiguration.entrySize);
}

void initialize(int entraces, int entryStorage){

	// REVIEW Que casos de error puede haber?? Pensarlo.

	entriesAmount = entraces;
	entrySize = entryStorage;
	entryTable = list_create();

	// REVIEW Hace falta un +1 para el \0?
	storage = calloc(1, entraces * entryStorage);
	biMapInitialize(entraces);

	log_info(logger, "Instancia was intialized correctly");
}

void handleOperationRequest(int coordinadorSocket){
	Operation * operation = NULL;
	char response;

	log_info(logger, "Gonna recieve operation from coordinador");

	if (recieveOperation(&operation, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "recv failed on trying to recieve statement from coordinador");
		exit(-1);
	}

	response = interpretateStatement(operation);

	if(response == INSTANCIA_COMPACT_REQUEST){
		if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
			log_error(logger, "I cannot send my response to coordinador");
			exit(-1);
		}

		//TODO mandar a compactar. chequear que no se haga nada del compactar dos veces!!!
		//con alvarez estamos suponiendo que no va a mandar dos need to compact seguidos, siempre como maixmo uno solo

		response = interpretateStatement(operation);
	}

	char typeOfResponse = INSTANCIA_DID_OPERATION;
	if(send_all(coordinadorSocket, &typeOfResponse, sizeof(char)) == CUSTOM_FAILURE){
		log_error(logger, "I cannot send the type of response to coordinador");
		exit(-1);
	}

	if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send my response to coordinador");
		exit(-1);
	}

	if (operation->operationCode == OURSET) {
		//TODO calcular spaceUsed
		int spaceUsed = 10;//valueHardcodeado
		if (sendInt(spaceUsed, coordinadorSocket) == CUSTOM_FAILURE) {
			log_error(logger, "I cannot send my spaceUsed to coordinador");
			exit(-1);
		}
	}

	log_info(logger, "The operation was successfully notified to coordinador");
}

void waitForCoordinadorStatements(int coordinadorSocket) {
	while (1) {
		log_info(logger, "Wait coordinador statement to execute");

		char command;
		if(recv_all(coordinadorSocket, &command, sizeof(command)) == CUSTOM_FAILURE){
			log_error(logger, "Couldn't receive execution command from coordinador");
			exit(-1);
		}

		switch(command){
			case INSTANCIA_DO_OPERATION:

				handleOperationRequest(coordinadorSocket);

				break;

			case INSTANCIA_DO_COMPACT:

				log_info(logger, "Gonna do compact");

				//TODO kiwo aca tambien hay que mandar a compactar (lo que esta sobre compactar en handleOperationRequest es para
				//la instancia que mando a compactar)

				char response = INSTANCIA_DID_COMPACT;
				if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
					log_error(logger, "I cannot tell coordinador that my compactation finished");
					exit(-1);
				}

				break;

			case INSTANCIA_CHECK_KEY_STATUS:

				//TODO kiwo. aca se recibe un string que es el valor de la clave, y vos tenes que devolver su valor.
				//devolves NULL si no tenes la clave

				break;

			default:
				log_error(logger, "Couldn't understand execution command from coordinador");
				exit(-1);
				break;
		}
	}
}

char interpretateStatement(Operation * operation) {
	showOperation(operation);

	switch (operation->operationCode) {
		case OURSET:
			return set(operation->key, operation->value);
			break;

		case OURSTORE:
			return store(operation->key);
			break;
	}

	destroyOperation(operation);

	return INSTANCIA_RESPONSE_FAILED;
}

void showStorage() {
	int position = 0;
	t_link_element * element = entryTable->head;
	char * value = malloc((entrySize * sizeof(char)) + 1);

	while (element != NULL) {
		getValue(value, getValueStart(element->data) * entrySize, getValueSize(element->data));

		printf("Value of entry %d is: %s\n", position, value);

		element = element->next;
		position++;

	}

	free(value);
}

int finish() {

	list_destroy_and_destroy_elements(entryTable, (void *) destroyTableInfo);
	free(storage);
	free(biMap);
	log_info(logger, "Instancia was finished correctly, bye bye, it was a pleasure!!");
	log_destroy(replaceAlgorithmsLogger);
	log_destroy(logger);

	return 0;
}

/* BiMap */
void biMapInitialize(int entraces) {
	biMap = malloc(entraces * sizeof(int));
	emptyBiMap(entraces);
}

void emptyBiMap(int entraces) {
	biMapUpdate(0, entraces, IS_EMPTY);
}

void biMapUpdate(int valueStart, int entriesForValue, int biMapValue) {
	for (int i = valueStart; i < (valueStart + entriesForValue); i++) {
		// TODO resolver warning: assignment makes pointer from integer without a cast [-Wint-conversion]
		biMap[i] = biMapValue;
	}
}

/* Set */
char set(char *key, char *value){

	int entriesForValue;
	int valueStart = ENTRY_START_ERROR;
	int valueSize = strlen(value);
	entryTableInfo * entryInfo;


	log_info(logger, "Size of value: %d", valueSize);

	// Asks if the size of the value can be stored
	if (valueSize > (entriesAmount * entrySize) || valueSize == 0) {
		log_error(logger, "Unable to set the value: %s, due to his size is bigger than the total Instancia storage size", value);
		return INSTANCIA_RESPONSE_FAILED;
	}

	// Get the amount of entries that is needed to store the value
	entriesForValue = wholeUpperDivision(valueSize, entrySize);
	log_info(logger, "Total entries for value: %d", entriesForValue);

	// Get the start position to store the value
	valueStart = getStartEntryToSet(entriesForValue);

	if (valueStart == ENTRY_START_ERROR) {
		log_error(logger, "There was an error trying to set, no valid entry start was found");
		return INSTANCIA_RESPONSE_FAILED;
	}

	else if (valueStart == I_NEED_TO_COMPACT) {
		return INSTANCIA_COMPACT_REQUEST;
	}

	// If the key exists, the value is updated
	if (list_find_with_param(entryTable, key, hasKey) != NULL) {

		/*
		 * Por las dudas guardo la información de la key que voy a borrar para hacerle update por si algo sale mal la reetablesco.
		 * TODO revisarlo bien y pensar bien como y cuando chequear si no se pudo hacer el set para reetablecer la key.
		 * */

		log_info(logger, "The key: %s already exists, so we are about to update it.", key);

		t_link_element * findedElement = list_find_with_param(entryTable, key, hasKey);

		entryInfo = findedElement->data;

		biMapUpdate(entryInfo->valueStart, wholeUpperDivision(entryInfo->valueSize, entrySize), IS_EMPTY);

		entryInfo->valueSize = valueSize;
		entryInfo->valueStart = valueStart;

	} else {

		// Create the entry structure
		entryInfo = malloc(sizeof(entryTableInfo));
		createTableInfo(entryInfo, key, valueStart, valueSize);

		list_add(entryTable, entryInfo);
	}

	storageSet(valueStart, value);
	biMapUpdate(valueStart, entriesForValue, IS_SET);

	log_info(logger, "Set operation for key: %s and value: %s, was successfully done", key, value);
	updateAccodringToAlgorithm(key);

	return INSTANCIA_RESPONSE_SUCCESS;
}

int getStartEntryToSet(int valueNeededEntries) {

	int entryStart = ENTRY_START_ERROR;
	int totalFreeEntries = 0;
	int validEntries = 0;

	while(entryStart == ENTRY_START_ERROR) {

		for (int i = 0; i < entriesAmount; i++) {

			// If the entry value is empty (only a sentinel value)
			if (!biMap[i]) {

				int j = i + 1;
				totalFreeEntries++;
				validEntries++;

				// Get the valid entries (adjacent) and the total free entries
				while (j < entriesAmount && !biMap[j] && validEntries < valueNeededEntries) {
					totalFreeEntries++;
					validEntries++;
					j++;
				}

				if (validEntries == valueNeededEntries) {
					entryStart = i;
					log_info(logger, "Valid start entry to set was found. Entry Number: %d", entryStart);
					break;
				}

				i = j;
				validEntries = 0;
			}
		}

		if (entryStart == ENTRY_START_ERROR) {

			if (valueNeededEntries > totalFreeEntries) {
				// Delete any value considering the replacement algorithm
				log_info(logger, "there is not enough space to set the value, we are about to run the replace algorithm");
				deleteAccodringToAlgorithm();
			} else {
				// A compactation is needed, we notify coordinador so he can order to compact all instancias.
				return I_NEED_TO_COMPACT;
				log_info(logger, "There are not enough contiguous free entries to set the value. A compactation is needed, we are about to notify coordinador");
				break;
			}
		}
	}

	return entryStart;
}

void storageSet(int initialEntry,  char * value) {
	int base = initialEntry * entrySize;
	int j = 0;
	for (int i = base; i < (strlen(value) + base) ; i++) {
		storage[i] = value[j];
		j++;
	}
}

char compact() {

	int totalSettedEntries = getTotalSettedEntries();

	if (totalSettedEntries > 0) {

		int totalUsedMemory = totalSettedEntries * entrySize;
			char * auxStorage = malloc(totalSettedEntries);
			int auxIndex = 0;

			int valueSize, valueStart, j;

			// Iterate all elements of the dictionary
			t_link_element * element = entryTable->head;

			while (element != NULL) {

				 j = 0;

				 valueSize = getValueSize(element->data);
				 valueStart = getValueStart(element->data) * entrySize;
				 char * value = malloc((valueSize * sizeof(char)) + 1);
				 getValue(value, valueStart, valueSize);

				 // Update ValueStart on dictionary(key) element
				 setValueStart(element->data, auxIndex);

				 for (; auxIndex < totalUsedMemory; auxIndex++) {
					 auxStorage[auxIndex] = value[j];
					 j++;
				 }

				 // Get the next able position to store values
				 auxIndex = wholeUpperDivision(valueSize, entrySize) * entrySize;
				 free(value);
			}

			strcpy(storage, auxStorage);
			emptyBiMap(entriesAmount);
			biMapUpdate(0, totalSettedEntries, IS_SET);

			free(auxStorage);



	}

	log_info(logger, "Compactation was successfully done");
	return INSTANCIA_RESPONSE_SUCCESS;
}

void getValue(char * value, int valueStart, int valueSize) {
	char * storageValue = string_substring(storage, valueStart, valueSize);
	strcpy(value, storageValue);
	value[valueSize] = '\0';
	free(storageValue);
}

int getTotalSettedEntries() {

	int totalEntries = 0;

	for (int i = 0; i < entriesAmount; i++) {
		if (!biMap[i]) {
			totalEntries++;
		}
	}

	return totalEntries;
}

int wholeUpperDivision(int x, int y) {
	return (1 + ((x - 1) / y));
}

char storeKeyAndValue(entryTableInfo * selectedEntryByKey) {

	FILE *file;
	char *valueToStore, *filePath = malloc(strlen(path) + strlen(getKey(selectedEntryByKey)) + 1);

	sprintf(filePath, "%s%s", path, getKey(selectedEntryByKey));
	filePath[strlen(path) + strlen(getKey(selectedEntryByKey))] = '\0';

	log_info(logger, "Size of value: %d", getValueSize(selectedEntryByKey));
	log_info(logger, "Value start: %d", getValueStart(selectedEntryByKey));

	valueToStore = malloc((getValueSize(selectedEntryByKey) * sizeof(char)) + 1);
	getValue(valueToStore, getValueStart(selectedEntryByKey) * entrySize, getValueSize(selectedEntryByKey));

	log_info(logger, "Value to store: %s", valueToStore);

	if ((file = fopen(filePath, "w")) != NULL) {

		fprintf(file, "%s", valueToStore);

		log_info(logger, "The key was stored in: %s", filePath);

		free(filePath);
		free(valueToStore);
		fclose(file);

	} else {
		log_error(logger, "Couldn't open the file: %s", strerror(errno));

		free(filePath);
		return INSTANCIA_RESPONSE_FAILED;
	}

	log_info(logger, "The key: %s, was successfully stored", getKey(selectedEntryByKey));

	return INSTANCIA_RESPONSE_SUCCESS;
}

char store(char *key) {

	log_info(logger, "The key: %s, is about to being stored", key);

	t_link_element * selectedElemByKey = list_find_with_param(entryTable, key, hasKey);

	if (selectedElemByKey == NULL) {
		return INSTANCIA_RESPONSE_FAILED;
	}

	entryTableInfo * selectedEntryByKey = selectedElemByKey->data;

	updateAccodringToAlgorithm(key);

	return storeKeyAndValue(selectedEntryByKey);
}

char dump() {

	log_info(logger, "We are about to dump all the keys");

	int position = 0;
	t_link_element * element = entryTable->head;
	while (element != NULL) {

		if(storeKeyAndValue(element->data) == INSTANCIA_RESPONSE_FAILED) {
			//TODO se sigue con el dump o se corta acá si falla??
			log_error(logger, "The store number %d couldn't be done", position);
		}

		log_error(logger, "The store number %d was succesfully done", position);

		element = element->next;
		position++;
	}

	log_info(logger, "Dump was successfully done");
	return INSTANCIA_RESPONSE_SUCCESS;
}

//TODO Esta mal esta función, estoy esperando el refactor del coordinador para hacerla bien
/*char * getValueForCoordinador(char * key, char * value) {

	if (list_find_with_param(entryTable, key, hasKey) != NULL) {

		log_info(logger, "The key: %s exists, so we are about to get its associated value.", key);
		entryTableInfo * entryInfo;
		int valueStart, valueSize;

		t_link_element * findedElement = list_find_with_param(entryTable, key, hasKey);

		entryInfo = findedElement->data;
		valueStart = entryInfo->valueStart;
		valueSize = entryInfo->valueSize;
		value = malloc((valueSize * sizeof(char)) + 1);
		getValue(value, valueStart * entrySize, valueSize);

		return value;
	}

	return INSTANCIA_RESPONSE_FAILED;
}*/


char getKeyByFile(char * key) {

	char *filePath = malloc(strlen(path) + strlen(key) + 1);

	sprintf(filePath, "%s%s", path, key);
	filePath[strlen(path) + strlen(key)] = '\0';
	log_info(logger, "file path to get the value from: %s", filePath);

	int fd = open(filePath, O_RDONLY, S_IRUSR | S_IWUSR);
	struct stat sb;
	free(filePath);

	if(fstat(fd, &sb) == -1) {

		log_error(logger, "Couldn't open the file that contains the key to set");

		return INSTANCIA_RESPONSE_FAILED;
	}

	log_info(logger, "The file was opened successfully");
	char * value = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

	log_info(logger, "the value taken from the file asociated to key %s, is: %s",key, value);

	char response = set(key, value);

	munmap(value, sb.st_size);
	close(fd);

	if (response == INSTANCIA_RESPONSE_SUCCESS) {
		log_info(logger, "The key: %s was successfully set again", key);
	}
	else {
		log_error(logger, "There was an error trying to set again the key %s", key);
	}
	return response;
}


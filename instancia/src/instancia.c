/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include "instancia.h"

pthread_mutex_t dumpMutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {

	//replaceAlgorithmsLogger = log_create("../replaceAlgorithms.log", "tpSO", true, LOG_LEVEL_INFO);

	initSerializationLogger(logger);

	if (argc != 2) {
		log_error(logger, "Instancia cannot execute: you must enter a configuration file");
		return -1;
	}

	CFG_FILE = strdup(argv[1]);

	int portCoordinador;

	getConfig(&ipCoordinador, &portCoordinador, &algorithm, &path, &name, &dumpDelay);

	printf("IP coord = %s\n", ipCoordinador);
	printf("Puerto = %d\n", portCoordinador);
	printf("Algoritmo = %s\n", algorithm);
	printf("Path = %s\n", path);
	printf("Name= %s\n", name);
	printf("Dump= %d\n", dumpDelay);

	char *logPath = malloc(strlen("../") + strlen(name) + strlen(".log")+ 1);

	sprintf(logPath, "%s%s%s", "../", name, ".log");
	logPath[strlen("../") + strlen(name) + strlen(".log")] = '\0';
	logger = log_create(logPath, "tpSO", true, LOG_LEVEL_INFO);
	initSerializationLogger(logger);

	log_info(logger, "trying to connect to coordinador...");
	/*
	 * Creates path directory with mkdir (if it does not exists)
	 * S_IRWXU: User mode to Read, Write and eXecute
	 * */

	mkdir(path, S_IRWXU);

	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, INSTANCIA, logger);

	if (coordinadorSocket < 0) {

		log_error(logger, "An error has occurred while trying to connect to coordinador\n socket number: %d", coordinadorSocket);
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, INSTANCIAID, INSTANCIA, logger);

	sendMyNameToCoordinador(name, coordinadorSocket);
	receiveCoordinadorConfiguration(coordinadorSocket);

	pthread_t dumpThread;
	if (pthread_create(&dumpThread, NULL, (void*) handleDump, NULL) != 0) {
		log_error(logger, "Error creating dump thread, quitting...");
		return -1;
	}

	if (pthread_detach(dumpThread) != 0) {
		pthread_cancel(dumpThread);
		log_error(logger, "Couldn't detach dump thread, quitting...");
		return -1;
	}

	waitForCoordinadorStatements(coordinadorSocket);

	finish();
	return 0;
}

void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char**path, char** name, int* dumpDelay) {

	t_config* config = NULL;
	config = config_create(CFG_FILE);

	free(CFG_FILE);

	if (config == NULL) {
		log_error(logger, "Instancia cannot work because of invalid configuration file");
		exit(-1);
	}

	*ipCoordinador = strdup(config_get_string_value(config, "IP_COORDINADOR"));
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*algorithm = strdup(config_get_string_value(config, "ALGORITHM"));
	*path = strdup(config_get_string_value(config, "PATH"));
	*name = strdup(config_get_string_value(config, "NAME"));
	*dumpDelay = config_get_int_value(config, "DUMP");
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

char* recieveKeyFromCoordinador(int coordinadorSocket) {
	char* key;
	if (recieveString(&key, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "Couldn't receive key from coordinador");
		exit(-1);
	}
	return key;
}

void recieveKeysFromCoordinador(int coordinadorSocket) {
	int keysAmount;
	if (recieveInt(&keysAmount, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "Cannot recieve number of keys from coordinador");
		exit(-1);
	}

	if (keysAmount == 0) {
		log_info(logger, "There are no keys to raise");
		return;
	}

	for (int i = 0 ; i < keysAmount ; i++) {
		char* recievedKey = recieveKeyFromCoordinador(coordinadorSocket);
		log_info(logger, "I recieved key %s", recievedKey);

		char response = getKeyByFile(recievedKey);

		if (response == INSTANCIA_RESPONSE_SUCCESS) {
			log_info(logger, "The key %s was successfully brought back", recievedKey);
		} else {
			log_warning(logger, "The key %s couldn't be brought back", recievedKey);
		}

		if (recievedKey)
			free(recievedKey);
	}
}

void sendSpaceUsedToCoordinador(int coordinadorSocket) {
	int spaceUsed = getTotalSettedEntries();
	if (sendInt(spaceUsed, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send my spaceUsed to coordinador");
		exit(-1);
	}
	log_info(logger, "Sent space used to coordinador, total setted entries: %d", spaceUsed);
}

void receiveCoordinadorConfiguration(int coordinadorSocket) {
	InstanciaConfiguration instanciaConfiguration;

	log_info(logger, "I am waiting for the last details (sent by coordinador)");

	if (recv_all(coordinadorSocket, &instanciaConfiguration, sizeof(InstanciaConfiguration)) == CUSTOM_FAILURE) {
		log_error(logger, "Cannot recieve configuration from coordinador");
		exit(-1);
	}

	log_info(logger, "I recieve the coordinador configuration, so I can work");
	initialize(instanciaConfiguration.entriesAmount, instanciaConfiguration.entrySize);

	log_info(logger, "Starting to receive keys to set from files");
	recieveKeysFromCoordinador(coordinadorSocket);

	sendSpaceUsedToCoordinador(coordinadorSocket);
}

void initialize(int entraces, int entryStorage) {

	entriesAmount = entraces;
	entrySize = entryStorage;
	entryTable = list_create();

	storage = calloc(1, entraces * entryStorage);
	biMapInitialize(entraces);

	currentReference = 0;

	log_info(logger, "Instancia was intialized correctly");
}

void handleOperationRequest(int coordinadorSocket) {
	Operation * operation = NULL;
	char response;

	log_info(logger, "Gonna recieve operation from coordinador");

	if (recieveOperation(&operation, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "recv failed on trying to recieve statement from coordinador");
		exit(-1);
	}

	response = interpretateStatement(operation);

	if (response == INSTANCIA_COMPACT_REQUEST) {
		if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
			log_error(logger, "I cannot send my response to coordinador");
			exit(-1);
		}

		// Only the instancia that send that needed to compact
		compact();
		response = interpretateStatement(operation);
	}

	char typeOfResponse = INSTANCIA_DID_OPERATION;
	if (send_all(coordinadorSocket, &typeOfResponse, sizeof(char)) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send the type of response to coordinador");
		exit(-1);
	}

	if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send my response to coordinador");
		exit(-1);
	}

	if (operation->operationCode == OURSET) {

		sendSpaceUsedToCoordinador(coordinadorSocket);
	}

	log_info(logger, "The operation was successfully notified to coordinador");
	destroyOperation(operation);
}

void checkValueFromKey(int coordinadorSocket) {

	char* keyFromStatus;

	if (recieveString(&keyFromStatus, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "Couldn't receive key from coordinador to check its status");
		exit(-1);
	}

	log_info(logger, "Gonna get value from key %s", keyFromStatus);

	char* valueFromKey = getValueForCoordinador(keyFromStatus);

	char responseKeyStatus = INSTANCIA_DID_CHECK_KEY_STATUS;
	if (send_all(coordinadorSocket, &responseKeyStatus, sizeof(responseKeyStatus)) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot tell coordinador that I'm gonna send the value from key %s to response status", keyFromStatus);
		exit(-1);
	}
	log_info(logger, "Sent coordinador that i'm gonna send status response");

	if (sendString(valueFromKey, coordinadorSocket) == CUSTOM_FAILURE) {
		log_error(logger, "I cannot send the value from key: %s",keyFromStatus);
		exit(-1);
	}
	log_info(logger, "Sent value %s from key %s to response status", valueFromKey, keyFromStatus);
}

void waitForCoordinadorStatements(int coordinadorSocket) {
	while (1) {

		log_info(logger, "Wait coordinador statement to execute");

		char command;
		if (recv_all(coordinadorSocket, &command, sizeof(command)) == CUSTOM_FAILURE) {
			log_error(logger, "Couldn't receive execution command from coordinador");
			exit(-1);
		}

		log_info(logger, "If dump is not executing we can run the statements");
		pthread_mutex_lock(&dumpMutex);

		switch(command) {
			case INSTANCIA_DO_OPERATION:

				handleOperationRequest(coordinadorSocket);

				break;

			case INSTANCIA_DO_COMPACT:

				log_info(logger, "Gonna do compact");
				// All instancias do compact
				compact();
				char response = INSTANCIA_DID_COMPACT;
				if (send_all(coordinadorSocket, &response, sizeof(response)) == CUSTOM_FAILURE) {
					log_error(logger, "I cannot tell coordinador that my compactation finished");
					exit(-1);
				}

				break;

			case INSTANCIA_CHECK_KEY_STATUS:

				checkValueFromKey(coordinadorSocket);

				break;

			default:
				log_error(logger, "Couldn't understand execution command from coordinador");
				exit(-1);
				break;
		}

		showStorage();
		pthread_mutex_unlock(&dumpMutex);
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
	char * value = NULL;

	while (element != NULL) {
		value = malloc((getValueSize(element->data) * sizeof(char)) + 1);
		getValue(value, getValueStart(element->data) * entrySize, getValueSize(element->data));

		log_info(logger, "Value of entry %d is: %s", position, value);

		element = element->next;
		position++;
		free(value);
	}
}

int finish() {

	list_destroy_and_destroy_elements(entryTable, (void *) destroyTableInfo);
	free(storage);
	free(biMap);
	free(ipCoordinador);
	free(algorithm);
	free(path);
	free(name);
	//log_destroy(replaceAlgorithmsLogger);
	log_info(logger, "Instancia was finished correctly, bye bye, it was a pleasure!!");
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

		biMap[i] = biMapValue;
	}
}

/* Set */
char set(char *key, char *value) {

	int entriesForValue;
	int valueStart = ENTRY_START_ERROR;
	int valueSize = strlen(value);
	entryTableInfo * entryInfo;


	log_info(logger, "Size of value: %d", valueSize);

	// Asks if the size of the value can be stored
	if (valueSize > (entriesAmount * entrySize) || valueSize == 0) {
		log_warning(logger, "Unable to set the value: %s, due to his size is bigger than the total Instancia storage size", value);
		return INSTANCIA_RESPONSE_FAILED;
	}

	// Get the amount of entries that is needed to store the value
	entriesForValue = wholeUpperDivision(valueSize, entrySize);
	log_info(logger, "Total entries for value: %d", entriesForValue);

	// Get the start position to store the value

	// If the key exists, the value is updated
	if (list_find_with_param(entryTable, key, hasKey) != NULL) {


		log_info(logger, "The key: %s already exists, so we are about to update it.", key);

		t_link_element * findedElement = list_find_with_param(entryTable, key, hasKey);
		entryInfo = findedElement->data;
		if (wholeUpperDivision(entryInfo->valueSize, entrySize) < entriesForValue) {

			log_warning(logger,"Unable to update the key: %s because the current value occupies less entries than the new one", key);
			return INSTANCIA_RESPONSE_FAILED;
		}
		else {

			log_info	(logger,"The key: %s can be updated because the current value occupies more or equals entries than the new one", key);
			biMapUpdate(entryInfo->valueStart, wholeUpperDivision(entryInfo->valueSize, entrySize), IS_EMPTY);

			valueStart = entryInfo->valueStart;
			entryInfo->valueSize = valueSize;

		}

	}
	else {

		valueStart = getStartEntryToSet(entriesForValue);

		if (valueStart == ENTRY_START_ERROR) {
			log_warning(logger, "There was an error trying to set, no valid entry start was found");
			return INSTANCIA_RESPONSE_FAILED;
		}

		else if (valueStart == I_NEED_TO_COMPACT) {
			log_warning(logger, "I need to compact");
			return INSTANCIA_COMPACT_REQUEST;
		}
		// Create the entry structure
		log_info(logger, "The key: %s doesn't exist, so we are about to set it", key);
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
				log_info(logger, "There are not enough contiguous free entries to set the value. A compactation is needed, we are about to notify coordinador");
				return I_NEED_TO_COMPACT;
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
		char * auxStorage = calloc(1, totalUsedMemory);
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
			 setValueStart(element->data, auxIndex / entrySize);

			 while (j < valueSize && auxIndex < totalUsedMemory) {
				 auxStorage[auxIndex] = value[j];
				 auxIndex++;
				 j++;
			 }

			 // Get the next able position to store values
			 if (valueSize % entrySize != 0) {
				 auxIndex += (valueSize % entrySize) - 1;
			 }
			 free(value);
			 element = element->next;
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
		if (biMap[i] == 1) {
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

		log_info(logger, "The key: %s, was not found, the store couldn't be done", key);
		return INSTANCIA_RESPONSE_FAILED;
	}

	entryTableInfo * selectedEntryByKey = selectedElemByKey->data;

	char response = storeKeyAndValue(selectedEntryByKey);

	if (response == INSTANCIA_RESPONSE_SUCCESS) {
		updateAccodringToAlgorithm(key);
	}

	return response;
}

void handleDump() {
	while(1) {
		sleep(dumpDelay);
		pthread_mutex_lock(&dumpMutex);
		log_info(logger, "DUMP is about to be done, after delay waiting: %d\n", dumpDelay);
		dump();
		pthread_mutex_unlock(&dumpMutex);
	}
}

void dump() {

	log_info(logger, "We are about to dump all the keys");

	int position = 0;
	t_link_element * element = entryTable->head;
	while (element != NULL) {

		if (storeKeyAndValue(element->data) == INSTANCIA_RESPONSE_FAILED) {

			log_warning(logger, "The store number %d couldn't be done", position);
		}

		log_info(logger, "The store number %d was successfully done", position);

		element = element->next;
		position++;
	}

	log_info(logger, "Dump was successfully done");

}

char * getValueForCoordinador(char * key) {

	char * value = NULL;
	if (list_find_with_param(entryTable, key, hasKey) != NULL) {

		log_info(logger, "The key: %s exists, so we are about to get its associated value.", key);
		entryTableInfo * entryInfo;
		int valueStart, valueSize;

		t_link_element * findedElement = list_find_with_param(entryTable, key, hasKey);

		entryInfo = findedElement->data;
		valueStart = entryInfo->valueStart;
		log_info(logger, "Value start: %d", valueStart);
		valueSize = entryInfo->valueSize;
		log_info(logger, "Value size: %d", valueSize);
		value = malloc((valueSize * sizeof(char)) + 1);
		log_info(logger, "The key: %s exists, so we are about to get its associated value.", key);
		getValue(value, valueStart * entrySize, valueSize);
		log_info(logger, "Value: %s", value);
	}
	else {
		log_info(logger, "The key %s doesn't exist", key);
	}

	return value;
}


char getKeyByFile(char * key) {

	char *filePath = malloc(strlen(path) + strlen(key) + 1);

	sprintf(filePath, "%s%s", path, key);
	filePath[strlen(path) + strlen(key)] = '\0';
	log_info(logger, "file path to get the value from: %s", filePath);

	int fd = open(filePath, O_RDONLY, S_IRUSR | S_IWUSR);
	struct stat sb;
	free(filePath);

	if (fstat(fd, &sb) == -1) {
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
		log_warning(logger, "There was an error trying to set again the key %s", key);
	}
	return response;
}

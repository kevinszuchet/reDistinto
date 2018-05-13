/*
 * instancia.c
 *
 *  Created on: 18 abr. 2018
 *      Author: utnso
 */

#include "instancia.h"

t_log * logger;
int entryAmount;
int entrySize;
t_dictionary * entryTable; //Takes record of the key + how many entraces the value occupies
char * storage;
int ** biMap;

int main(void) {

	logger = log_create("../instancia.log", "tpSO", true, LOG_LEVEL_INFO);

	char* ipCoordinador;
	int portCoordinador;
	char* algorithm, * path, * name;
	int dump;

	getConfig(&ipCoordinador, &portCoordinador, &algorithm, &path, &name, &dump);

	printf("IP coord = %s\n", ipCoordinador);
	printf("Puerto = %d\n", portCoordinador);
	printf("Algoritmo = %s\n", algorithm);
	printf("Path = %s\n", path);
	printf("Name= %s\n", name);
	printf("Dump= %d\n", dump);
	log_info(logger, "trying to connect to coordinador...\n");

	int coordinadorSocket = connectToServer(ipCoordinador, portCoordinador, COORDINADOR, INSTANCIA, logger);

	if (coordinadorSocket < 0) {
		//reintentar conexion?
		log_error(logger, "An error has occurred while trying to connect to coordinador\n socket number: %d\n", coordinadorSocket);
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, 11, INSTANCIA, logger);
	initialize(entryAmount, entrySize);
	finish();
	return 0;
}

void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char**path, char** name, int* dump) {

	t_config* config;
	config = config_create(CFG_FILE);
	*ipCoordinador = config_get_string_value(config, "IP_COORDINADOR");
	*portCoordinador = config_get_int_value(config, "PORT_COORDINADOR");
	*algorithm = config_get_string_value(config, "ALGORITHM");
	*path = config_get_string_value(config, "PATH");
	*name = config_get_string_value(config, "NAME");
	*dump = config_get_int_value(config, "DUMP");
}

int initialize(int entraces, int entryStorage){

	//Que casos de error puede haber?? Pensarlo.

	entryAmount = entraces;
	entrySize = entryStorage;
	entryTable = dictionary_create();
	// Se puede hacer dictioanry_resize()? Como se recorre el dictionary?
	// Hace falta un +1 para el \0?
	storage = malloc(entraces * entryStorage);
	biMapInitialize(entraces);
	log_info(logger, "Instancia was intialized correctly\n");

	return 0;
}

int finish() {

	dictionary_destroy_and_destroy_elements(entryTable, (void *) destroyTableInfo);
	free(storage);
	free(biMap);
	log_info(logger, "Instancia was finished correctly, bye bye, it was a pleasure!!\n");

	return 0;
}

/* BiMap */
void biMapInitialize(int entraces) {

	biMap = malloc(entraces * sizeof(int));
	emptyBiMap(entraces);
}

void emptyBiMap(int entraces) {

	for (int i = 0; i < entraces; i++) {
		biMap[i] = IS_EMPTY;
	}
}

void biMapUpdate(int valueStart, int entriesForValue) {
	for(int i = valueStart; i < (valueStart + entriesForValue); i++) {
		// TODO resolver warning: assignment makes pointer from integer without a cast [-Wint-conversion]
		*biMap[i] = IS_SET;
	}
}

/* Set */
int set(char *key, char *value){

	int entriesForValue;
	int valueStart = -1;
	int valueSize = strlen(value);
	entryTableInfo * entryInfo;

	log_info(logger, "Size of value: %d", valueSize);

	// Asks if the size of the value can be stored
	if (valueSize > (entryAmount * entrySize)) {
		log_error(logger, "Unable to set the value: %s, due to his size is bigger than the total Instancia storage size", value);
		return -1;
	}

	// If the key exists, the value is update
	if (dictionary_has_key(entryTable, key)) {
		updateKey(key, value);
	}

	// Get the amount of entries that is needed to store the value
	entriesForValue = wholeUpperDivision(valueSize, entrySize);
	log_info(logger, "Total entries for value: %d", entriesForValue);

	// Get the start position to store the value
	valueStart = getStartEntryToSet(entriesForValue);

	// Create the entry structure
	entryInfo = malloc(sizeof(entryTableInfo));
	createTableInfo(entryInfo, valueStart, valueSize);

	dictionary_put(entryTable, key, entryInfo);
	storageSet(valueStart, value);
	biMapUpdate(valueStart, entriesForValue);

	log_info(logger, "Set operation for key: %s and value: %s, was successfully done\n", key, value);
	return 0;
}

int getStartEntryToSet(int valueNeededEntries) {

	int entryStart = -1;
	int totalFreeEntries;
	int validEntries = 0;

	while(entryStart == ENTRY_START_ERROR) {

		for (int i = 0; i < entryAmount; i++) {

			// If the entry value is empty (only a sentinel value)
			if (!biMap[i]) {

				int j = i + 1;
				totalFreeEntries++;
				validEntries++;

				// Get the valid entries (adjacent) and the total free entries
				while (j < entryAmount && !biMap[j] && validEntries < valueNeededEntries) {
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

		if(entryStart == ENTRY_START_ERROR) {

			if (valueNeededEntries > totalFreeEntries) {
				// Delete any value considering the replacement algorithm
				log_info(logger, "there is not enough space to set the value, we are about to run the replace algorithm");
			} else {
				// compact();
				log_info(logger, "There are not enough contiguous free entries to set the value, we are about to compact the storage");
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

int updateKey(char *key, char *value) {

	log_info(logger, "The key: %s, already exists so it will be updated with value: %s\n", key, value);

	//...

	log_info(logger, "The key: %s, was successfully updated with the value: %s\n", key, value);

	return 0;

}

int compact() {

	int totalSettedEntries = getTotalSettedEntries();
	int totalUsedMemory = totalSettedEntries * entrySize;
	char * auxStorage = malloc(totalSettedEntries);
	int auxIndex = 0;

	int valueSize, valueStart, j;

	// Iterate all elements of the dictionary
	for (int i = 0; i < entryTable->table_max_size; i++) {
		t_hash_element *element = entryTable->elements[i];

		while (element != NULL) {

			 j = 0;

			 valueSize = getValueSize(element->data);
			 valueStart = getValueStart(element->data);
			 char * value = malloc(valueSize);
			 getValue(&value, valueStart, valueSize);

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
	}

	strcpy(storage, auxStorage);
	biMapUpdate(0, totalSettedEntries);

	free(auxStorage);
	log_info(logger, "Compactation was successfully done\n");
	return 0;
}

void getValue(char ** value, int valueStart, int valueSize) {
	int j = 0;
	for (int i = valueStart; i < (valueStart + valueSize); i++) {
		*value[j] = storage[i];
		j++;
	}
}

int getTotalSettedEntries() {

	int totalEntries = 0;

	for (int i = 0; i < entryAmount; i++) {
		if (!biMap[i]) {
			totalEntries++;
		}
	}

	return totalEntries;
}

int wholeUpperDivision(int x, int y) {
	return (1 + ((x - 1) / y));
}
/*int notifyCoodinador(char *key, char *value, char *operation) {

	log_info(logger, "%s operation, with key: %s and value: %s, was successfully notified to coordinador\n", operation, key, value);
	return 0;

}

int dump() {

	log_info(logger, "Dump was successfully done\n");
	return 0;

}

int store(char *key) {

	log_info(logger, "The key: %s, was successfully stored/n", key);

	return 0;

}

int getValueStartEntry(char * key) {

	int entry = 0;

	log_info(logger, "Start entry for value with key: %s, is: %d", key, entry);

	return entry;

}*/

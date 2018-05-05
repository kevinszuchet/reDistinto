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
t_dictionary * keyTable; //Takes record of the key + how many entraces the value occupies
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
	if (coordinadorSocket < 0){
		//reintentar conexion?
		log_error(logger, "An error has occurred while trying to connect to coordinador\n socket number: %d\n", coordinadorSocket);
		return -1;
	}

	sendMyIdToServer(coordinadorSocket, 11, INSTANCIA, logger);

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

int initialize(int entraces, int storage){
	//Que casos de error puede haber?? Pensarlo.
	entryAmount = entraces;
	entrySize = storage;
	keyTable = dictionary_creat();//tendría que tocar las so-commons para hacer un dictionary_create que reciba un
								 //int como parametro y ese int sea la máxima cantidad de entradas así no es por default
	//Pensar bien como se crea la table de clave-valor
	log_info(logger, "Instancia was intialized correctly\n");
	return 0;
}
int set(char *key, char *value){
	int freeStorage;
	int entriesForValue;
	if((dictionary_has_key(keyTable, key),"true")){
		updateKey(key, value);
	}
	log_info(logger, "Total free storage: %d", getFreeStorage());
	log_info(logger, "Total size of value: %d", sizeof(value));
	log_info(logger, "Total free contiguous free entriens: %d", getcontiguousFreeEntries());
	entriesForValue = sizeof(value) / entrySize;
	log_info(logger, "Total entries for value: %d", entriesForValue);
	if(getFreeStorage() < sizeof(value)){
	    log_info(logger, "there is not enough space to set the key: %s", key);
		//borro alguna key según algoritmo y setteo

	}
	if(getContiguousFreeEntries() < entriesForValue){
		//tengo que compactar
	}

	//hago el set.
	log_info(logger, "Set operation for key: %s and value: %s, was successfully done\n", key, value);
	return 0;
}
int notifyCoodinador(char *key, char *value, char *operation){
	log_info(logger, "%s operation, with key: %s and value: %s, was succesffully notified to coordinador\n", operation, key, value);
	return 0;
}
int dump(){
	log_info(logger, "Dump was succesffully done\n");
	return 0;
}
int compact(){
	log_info(logger, "Compactation was successfully done\n");
	return 0;
}
int updateKey(char *key, char *value){
	log_info(logger, "The key: %s, already exists so it will be updated with value: %s\n", key, value);
	//...
	log_info(logger, "The key: %s, was successfully updated with the value: %s\n", key, value);
	return 0;
}
int store(char *key){
	log_info(logger, "The key: %s, was succesfully stored/n", key);
	return 0;
}
int finish(){
	//dictionary_clean_and_destroy_elements(keyTable, ); Averiguar bien que es lo que le tengo que pasar como segundo parametro
	log_info(logger, "Instancia was finished correctly, bye bye, it was a pleasure!!\n");
	return 0;
}
int getAmountOfFreeEntraces(){
	int amount = 0;
	log_info(logger, "There are %d free spaces\n", amount);
	return amount;
}
int getFreeStorage(){
	return getAmountOfFreeEntraces() * entrySize;
}
int getMaxContiguousFreeEtries(){
	int max = 0;
	log_info(logger, "There are %d contiguous free entries\n", max);
	return max;
}


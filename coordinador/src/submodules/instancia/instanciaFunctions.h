/*
 * instanciaFunctions.h
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#ifndef INSTANCIAFUNCTIONS_H_
#define INSTANCIAFUNCTIONS_H_
#define INSTANCIA_ALIVE 0
#define INSTANCIA_FALLEN 1

	#include "../../coordinador.h"
	#include <semaphore.h>

	typedef struct Instancia{
		int id;
		int socket;
		int spaceUsed;
		char firstLetter;
		char lastLetter;
		t_list* storedKeys;
		int isFallen;
		char* name;
		sem_t* semaphore;
	}Instancia;

	int recieveInstanciaName(char** arrivedInstanciaName, int instanciaSocket, t_log* logger);
	int sendInstanciaConfiguration(int instanciaSocket, int cantEntry, int entrySize, t_log* logger);
	void recieveInstanciaNameDummy(char** arrivedInstanciaName);
	Instancia* existsInstanciaWithName(char* arrivedInstanciaName, t_list* instancias);
	void instanciaIsBack(Instancia* instancia, int instanciaSocket);
	void instanciaDoOperation(Instancia* instancia, Operation* operation, t_log* logger);
	void instanciaDoOperationDummy(Instancia* instancia, Operation* operation, t_log* logger);
	Instancia* lookForKey(char* key, t_list* instancias);
	void removeKeyFromFallenInstancia(char* key, Instancia* instancia);
	void addKeyToInstanciaStruct(Instancia* instancia, char* key);
	void instanciaHasFallen(Instancia* fallenInstancia);
	char waitForInstanciaResponse(Instancia* chosenInstancia);
	char waitForInstanciaResponseDummy();
	int firstInstanciaBeforeSecond(Instancia* firstInstancia, Instancia* secondInstancia);
	Instancia* createNewInstancia(int instanciaSocket, t_list* instancias, int* greatesInstanciaId, char* name);
	Instancia* createInstancia(int id, int socket, int spaceUsed, char firstLetter, char lastLetter, char* name);

	/*-----------------------------------------------------*/

	/*
	 * TEST FUNCTIONS
	 */
	void initializeSomeInstancias(t_list* instancias);
	void showInstancia(Instancia* instancia);
	void showInstancias(t_list* instancias);
	/*
	 * TEST FUNCTIONS
	 */


#endif /* INSTANCIAFUNCTIONS_H_ */

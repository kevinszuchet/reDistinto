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

	typedef struct Instancia{
		int socket;
		int spaceUsed;
		char firstLetter;
		char lastLetter;
		t_list* storedKeys;
		int isFallen;
		char* name;
		sem_t* compactSemaphore;
		char actualCommand;
	}Instancia;

	Instancia* lastInstanciaChosen;

	Instancia* initialiceArrivedInstancia(int instanciaSocket);
	Instancia* initialiceArrivedInstanciaDummy(int instanciaSocket);
	t_list* sendCompactRequestToEveryAliveInstaciaButActual(Instancia* compactCausative);
	int handleInstanciaCompact(Instancia* actualInstancia, t_list* instanciasToBeCompactedButCausative);
	int handleInstanciaOperation(Instancia* actualInstancia, t_list** instanciasToBeCompactedButCausative);
	void instanciaExitGracefully(Instancia* instancia);
	void instanciaDestroyer(Instancia* instancia);
	int instanciaIsAlive(Instancia* instancia);
	int recieveInstanciaName(char** arrivedInstanciaName, int instanciaSocket);
	void recieveInstanciaNameDummy(char** arrivedInstanciaName);
	char instanciaDoOperation(Instancia* instancia, Operation* operation);
	Instancia* lookForKey(char* key);
	void removeKeyFromFallenInstancia(char* key, Instancia* instancia);
	void addKeyToInstanciaStruct(Instancia* instancia, char* key);
	void instanciaHasFallen(Instancia* fallenInstancia);
	Instancia* createNewInstancia(int instanciaSocket, char* name);

	/*-----------------------------------------------------*/

	/*
	 * TEST FUNCTIONS
	 */
	void showInstancia(Instancia* instancia);
	void showInstancias();
	/*
	 * TEST FUNCTIONS
	 */


#endif /* INSTANCIAFUNCTIONS_H_ */

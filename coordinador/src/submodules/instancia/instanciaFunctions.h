/*
 * instanciaFunctions.h
 *
 *  Created on: 5 may. 2018
 *      Author: utnso
 */

#ifndef INSTANCIAFUNCTIONS_H_
#define INSTANCIAFUNCTIONS_H_

	#include "../../coordinador.h"

	typedef struct Instancia{
		int id;
		int socket;
		int spaceUsed;
		char firstLetter;
		char lastLetter;
		t_list* storedKeys;
	}Instancia;

	void removeKeyFromFallenInstancia(char* key, Instancia* instancia);
	int instanciaDoOperation(Instancia* instancia, Operation* operation);
	int instanciaDoOperationDummy();
	Instancia* lookForKey(char* key, t_list* instanciasList);
	Instancia* fallenInstanciaThatHasKey(char* key, t_list* fallenInstancias);
	int addKeyToInstanciaStruct(Instancia* instancia, char* key);
	void instanciaHasFallen(Instancia* fallenInstancia, t_list* instancias, t_list* fallenInstancias, char* keyToBeRemoved);
	int waitForInstanciaResponse(Instancia* chosenInstancia);
	int firstInstanciaBeforeSecond(Instancia* firstInstancia, Instancia* secondInstancia);
	int createNewInstancia(int instanciaSocket, t_list* instancias, t_list* fallenInstancias);
	Instancia* createInstancia(int id, int socket, int spaceUsed, char firstLetter, char lastLetter, t_list* storedKeys);
	void destroyInstancia(Instancia* instancia);

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

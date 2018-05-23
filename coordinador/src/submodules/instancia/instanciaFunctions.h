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
		int id;
		int socket;
		int spaceUsed;
		char firstLetter;
		char lastLetter;
		t_list* storedKeys;
		int isFallen;
	}Instancia;

	int instanciaDoOperation(Instancia* instancia, Operation* operation);
	int instanciaDoOperationDummy();
	Instancia* lookForKey(char* key, t_list* instancias);
	void removeKeyFromFallenInstancia(char* key, Instancia* instancia);
	void addKeyToInstanciaStruct(Instancia* instancia, char* key);
	void instanciaHasFallen(Instancia* fallenInstancia);
	int waitForInstanciaResponse(Instancia* chosenInstancia);
	int firstInstanciaBeforeSecond(Instancia* firstInstancia, Instancia* secondInstancia);
	int createNewInstancia(int instanciaSocket, t_list* instancias, int* greatesInstanciaId);
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

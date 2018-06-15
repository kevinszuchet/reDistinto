/*
 * consola.h
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#ifndef CONSOLA_H_
#define CONSOLA_H_

	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <readline/readline.h>
	#include <readline/history.h>
	#include <ctype.h>
	#include <commons/string.h>
	#include "../planificador.h"

	#define PAUSAR 1
	#define CONTINUAR 2
	#define BLOQUEAR 3
	#define DESBLOQUEAR 4
	#define LISTAR 5
	#define KILL 6
	#define STATUS 7
	#define DEADLOCK 8
	#define INVALID_COMMAND 9

	void openConsole();
	void executeConsoleInstruccions();
	void execute(char ** parameters);

	int parameterQuantity(char** parameters);
	int validCommand(char** parameters);
	int validKey(char* key);
	int validateDesbloquear(char* key);
	int validateBloquear(char* key, int id);
	int keyExists(char* key);
	int parameterQuantityIsValid(int cantExtraParameters, int necessaryParameters);

	int isReady(int idEsi);
	int isRunning(int idEsi);

	int getCommandNumber(char* command);

	// Destroy functions
	void destroyConsoleParam(void * param);
#endif /* CONSOLA_H_ */

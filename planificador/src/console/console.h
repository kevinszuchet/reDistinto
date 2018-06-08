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
	void execute(char **);
	int getCommandNumber(char* );
	int validCommand(char** );
	int parameterQuantity(char**);
	int parameterQuantityIsValid(int, int);
	int validateBloquear(char* key,int id);
	int validateDesbloquear(char* key);
	int keyExists(char* key);
	int isReady(int idEsi);
	int isRunning(int idEsi);

#endif /* CONSOLA_H_ */

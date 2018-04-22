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

	void openConsole();
	void execute(char *);

#endif /* CONSOLA_H_ */

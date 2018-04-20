/*
 * consola.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "console.h"
#include "src/planificador.h"

void openConsole() {
	char *line;

	while(1) {
		line = readline("> ");

		if(line) {
			add_history(line);
		}

		if(!strncmp(line, "exit", 4)) {
			free(line);
			break;
		}

		execute(line);

		free(line);
	}
}

void execute(char *command) {
	printf("%s", command);
	switchs(command) {
		icases("pause")
		icases("continue")
		icases("block")
		icases("unblock")
		icases("list")
		icases("kill")
		icases("status")
		icases("deadlock")
			printf("El comando ingresado fue: %s\n", command);
			break;

		defaults
			printf("El comando %s no es valido\n", command);
			break;
	} switchs_end;
}

/*
 * consola.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */



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

	if(!strcmp(command, "pause")){

	} else if(!strcmp(command, "continue")){

	} else if(!strcmp(command, "block")){

	} else if(!strcmp(command, "unblock")){

	} else if(!strcmp(command, "list")){

	} else if(!strcmp(command, "kill")){

	} else if(!strcmp(command, "status")){

	} else if(!strcmp(command, "deadlock")){

	} else {
		printf("%s: command not found.", command);
	}

}


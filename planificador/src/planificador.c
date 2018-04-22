/*
 * coordinador.c
 *
 *  Created on: 17 abr. 2018
 *      Author: utnso
 */

#include "planificador.h"
#include "console/console.h"

int welcomeCoordinador();

int main(void) {

	int welcomeResponse = welcomeServer("127.0.0.1", 8080, COORDINADOR, PLANIFICADOR, 10, &welcomeCoordinador);
	if (welcomeResponse < 0){
		//reintentar?
	}
	/*
	 * Planificador console
	 * */
	//crear el hilo!
	openConsole();
	/*
	 *  Planificador console
	 * */

	return 0;
}

int welcomeEsi(){
	for(;;);

	return 0;
}

int welcomeCoordinador(){
	printf("Probando recepcion en el coordinador\n");
	/*int welcomePlanificador = welcomeClient(8082, COORDINADOR, ESI, 12, &welcomeEsi);
	if(welcomePlanificador < 0){
		//TODO: que pasa en este caso?
		return -1;
	}*/

	return 0;
}



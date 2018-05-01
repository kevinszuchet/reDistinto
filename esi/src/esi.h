/*
 * esi.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_ESI_H_
#define SRC_ESI_H_

	#include <our-commons/sockets/client.h>
	#include <our-commons/modules/names.h>
	#include <commons/string.h>
	#include <commons/config.h>
	#include <commons/log.h>
	#include <parsi/parser.h>
	#include <our-commons/tads/tads.h>
	#include <our-commons/messages/operation_codes.h>

	#define  CFG_FILE "../esi.cfg"
	void getConfig(char** ipCoordinador, char** ipPlanificador, int* portCoordinador, int* portPlanificador);

	void waitToPlanificadorOrder(int planificadorSocket, FILE * scriptFile);
	void tryToExecute(FILE * scriptFile);

#endif /* SRC_ESI_H_ */

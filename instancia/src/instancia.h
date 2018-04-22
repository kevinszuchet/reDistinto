/*
 * instancia.h
 *
 *  Created on: 22 abr. 2018
 *      Author: utnso
 */

#ifndef SRC_INSTANCIA_H_
#define SRC_INSTANCIA_H_

	#include <our-commons/sockets/client.h>
	#include <our-commons/modules/names.h>
	#include <commons/string.h>
	#include <commons/config.h>

	#define  CFG_FILE "../instancia.cfg"

	void getConfig(char** ipCoordinador, int* portCoordinador, char** algorithm, char** path, char** name, int* dump);

#endif /* SRC_INSTANCIA_H_ */

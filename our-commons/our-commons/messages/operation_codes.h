/*
 * operation_codes.h
 *
 *  Created on: 27 abr. 2018
 *      Author: utnso
 */

#ifndef OPERATION_CODES_H_
#define OPERATION_CODES_H_

#define CUSTOM_SUCCESS 1
#define CUSTOM_FAILURE 0

#define SUCCESS '1'
#define FINISHED '2'
#define ABORT '3'
#define OURSET '4'
#define OURGET '5'
#define RUN 6
#define COMPACT '7'
#define OURSTORE '8'
#define LOCK '9'
#define ERROR_CLAVE_SIZE 'q'
#define ERROR_CLAVE_NOT_IDENTIFIED 'w'
#define ERROR_CLAVE_NON_EXISTENT 'e'
#define ERROR_CLAVE_NOT_FOUND 'r'
#define BLOCK 't'
#define NOTBLOCKED 'y'
#define FREE 'u'
#define NOTFINISHED 'i'
#define BLOCKED 'o'
#define LOCKED 'p'
#define INSTANCIA_RESPONSE_FALLEN 'a'
#define INSTANCIA_RESPONSE_SUCCESS 's'
#define INSTANCIA_RESPONSE_FAILED 'b'
#define INSTANCIA_NEED_TO_COMPACT 'd'

#define ESIID 'f'
#define COORDINADORID 'g'
#define PLANIFICADORID 'h'
#define INSTANCIAID 'j'
#define HANDSHAKEESIPLANIFICADOR 'k'

#define KEYSTATUSMESSAGE 'l'
#define ESIINFORMATIONMESSAGE 'z'
#define CORDINADORCONSOLERESPONSEMESSAGE 'x'

char* getCoordinadorResponseName(char coordinadorResponse);
char* getEsiInformationResponseName(char esiInformation);

#endif /* OPERATION_CODES_H_ */

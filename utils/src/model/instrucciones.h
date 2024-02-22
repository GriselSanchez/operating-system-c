#ifndef SRC_MODEL_INSTRUCCIONES_H_
#define SRC_MODEL_INSTRUCCIONES_H_

#include <commons/collections/list.h>
#include <commons/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/string.h>
#include "utils/serializacion.h"

typedef struct instruccion {
    int largo_operacion;
    char* operacion;
    int cant_parametros;
    t_list* parametros;
} t_instruccion;

/**
 * @NAME: crear_instruccion
 * @DESC: Crea una estructura instruccion en base a la operacion
 *        y la lista de parametros que recibe dicha instruccion.
 */
void crear_instruccion(char* operacion, t_list* parametros, t_instruccion* instruccion);

/**
 * @NAME: instructions_deserealize_from_payload
 * @DESC: Convierte de un void* a una estructura de tipo instruccion.
 *        En caso de no ser posible, devuelve NULL.
 */
t_list* deserealize_instructions_from_payload(void*, int*);

/**
 * @NAME: serializar_instrucciones
 * @DESC: Agrega dentro del payload del `t_paquete *paquete` pasado por parametro
 *        las `t_list *instrucciones` serializadas para enviar posteriormente por socket.
 */
void serializar_instrucciones(t_paquete* paquete, t_list* instrucciones);

/**
 * @NAME: instruccion_destroy
 * @DESC: Libera los recursos solicitados para una instruccion.
 */
void instruccion_destroy(void* instruccion);

/**
 * @NAME: imprimir_instrucciones
 * @DESC: Imprime las instrucciones con el logger pasado como parametro.
 */
void imprimir_instrucciones(t_list* instructions);



void serializar_instruccion(t_paquete* paquete, t_instruccion* instruccion);
void deserializar_instruccion(void* payload, t_instruccion* instruccion);
void imprimir_instruccion(t_instruccion* instruccion);

#endif /* SRC_MODEL_INSTRUCCIONES_H_ */

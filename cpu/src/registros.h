#ifndef SRC_REGISTROS_H_
#define SRC_REGISTROS_H_

#include <commons/log.h>
#include <commons/string.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 
#include "model/pcb.h"
#include "model/contexto-ejecucion.h"

#define CANT_REGISTROS 4

uint32_t* obtener_registro(char indice_char);
void imprimir_registros();
void actualizar_registros_ctx(t_exec_context* ctx);
void actualizar_registros_cpu(t_exec_context* ctx);

#endif /* SRC_REGISTROS_H_ */

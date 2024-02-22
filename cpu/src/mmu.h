#ifndef SRC_MMU_H_
#define SRC_MMU_H_

#include <commons/log.h>
#include <commons/string.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h> 
#include <math.h>
#include "cpu.h"
#include "utils/serializacion.h"

int obtener_numero_pagina(int dir_logica);
int obtener_desplazamiento(int dir_logica);
int traduccion_logica_a_fisica(int dir_logica, int nro_marco);
int obtener_marco(int pagina, int pid, tipo_operacion_t operacion);

uint32_t leer_valor_memoria(int dir_fisica, int pid);
int escribir_valor_memoria(int dir_fisica, uint32_t valor, int pid);

#endif /* SRC_MMU_H_ */

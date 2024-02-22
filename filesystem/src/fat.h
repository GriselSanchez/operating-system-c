#ifndef SRC_FAT_H
#define SRC_FAT_H

#include <stdlib.h>
#include <stdint.h>
#include <commons/log.h>
#include <commons/txt.h>
#include <commons/collections/queue.h>
#include <commons/collections/list.h>

#include "filesystem.h"

void inicializar_fat();
void limpiar_fat(FILE* archivo);
uint32_t* obtener_entradas_fat(FILE* archivo);

void asignar_bloques_nuevos(t_fcb* fcb, int cant_bloques_nuevos);
void liberar_bloques_asignados(t_fcb* fcb, int cant_bloques_a_liberar);

uint32_t obtener_bloque_fat_de_archivo(t_fcb* fcb, int nro_bloque);
t_list* obtener_bloques_fat_asignados(t_fcb* fcb, uint32_t* entradas);
int obtener_cant_bloques_asignados(t_fcb* fcb);
void imprimir_bloques_fat_asignados(t_fcb* fcb, uint32_t* entradas);

void acceso_fat_set(uint32_t* entradas, int nro_entrada, uint32_t valor);
uint32_t acceso_fat_get(uint32_t* entradas, int nro_entrada);

#endif /* SRC_FAT_H */

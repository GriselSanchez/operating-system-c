#ifndef SRC_ARCHIVO_H
#define SRC_ARCHIVO_H

#include <stdlib.h>
#include <stdint.h>
#include <commons/log.h>
#include <commons/txt.h>
#include "model/fcb.h"
#include "filesystem.h"
#include "fat.h"

int abrir_archivo(char* nombre_archivo);
void crear_archivo(char* nombre_archivo);
int truncar_archivo(char* nombre_archivo, int tamanio_nuevo);
void escribir_archivo(char* nombre_archivo, int puntero, int dir_fisica, int pid);
void leer_archivo(char* nombre_archivo, int puntero, int dir_fisica, int pid);

void crear_fcb_nuevo(char* nombre_archivo);
char* obtener_directorio_fcb(char* nombre_archivo);
t_fcb* obtener_fcb(char* nombre_archivo);
t_config* obtener_config_fcb(char* nombre_archivo);

void limpiar_bloque_archivo(t_fcb* fcb, int nro_bloque_archivo);

#endif /* SRC_ARCHIVO_H */

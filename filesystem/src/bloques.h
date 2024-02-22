#ifndef SRC_BLOQUES_H
#define SRC_BLOQUES_H

#include "filesystem.h"
#include "model/fcb.h"
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

void crear_archivo_de_bloques();

void escribir_bloque(int nro_bloque, uint32_t* data);
uint32_t* leer_bloque(int nro_bloque);
void limpiar_bloque(int nro_bloque);

#endif /* SRC_BLOQUES_H */

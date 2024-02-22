#ifndef SRC_MODEL_FCB_H_
#define SRC_MODEL_FCB_H_

#include <stdlib.h>
#include <stdint.h>

typedef struct t_fcb {
  char* identificador; // unico
  int tamanio; // bytes
  uint32_t bloque_inicial; // primer bloque de datos
} t_fcb;

#endif /* SRC_MODEL_FCB_H_ */

#ifndef SRC_SWAP_H_
#define SRC_SWAP_H_

#include "filesystem.h"
#include "commons/bitarray.h"

t_bitarray* inicializar_swap();
void imprimir_swap();

t_list* asignar_bloques_swap(int cant_bloques_nuevos);
void liberar_bloques_swap(t_list* bloques_a_liberar);
void swap_out(int nro_bloque, uint32_t* data);
uint32_t* swap_in(int nro_bloque);
void limpiar_bloque_swap(int nro_bloque);
#endif /* SRC_SWAP_H_ */
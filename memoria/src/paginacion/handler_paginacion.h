#ifndef SRC_HANDLER_PAGINACION_H_
#define SRC_HANDLER_PAGINACION_H_

#include <stdlib.h>
#include <commons/string.h>
#include <commons/collections/list.h>

#include "paginacion/tabla_paginas.h"
#include "paginacion/espacio_usuario.h"
#include "utils/diccionario_monitor.h"
#include "utils/lista_monitor.h"
#include "utils/serializacion.h"

#include "../memoria.h"

extern t_config_memoria* configs;

typedef enum {
    FIFO,
    LRU
} t_reemplazo;
typedef struct {
    t_diccionario_mutex* pid_tablas;
    t_espacio_usuario* espacio_usuario;
    t_lista_mutex* paginas_pendientes_modificacion;
    t_reemplazo algoritmo_reemplazo;
    int tamanio_pagina;
} t_handler_paginacion;


t_handler_paginacion* create_handler_paginacion(int tamanio_memoria, int tamanio_pagina, char* algoritmo_reemplazo);

/**
 * Crea una tabla para el proceso indicado, se guarda en el diccionario de tablas
*/
t_tabla_paginas* generar_tabla_paginas(t_handler_paginacion* handler, int pid, int tamanio_proceso);

int obtener_marco(t_handler_paginacion* handler, int pid, int nro_pagina, tipo_operacion_t tipo_operacion);

uint32_t leer_valor(t_handler_paginacion* handler, int dir_fisica, int pid);

/*  En caso satisfactorio se responderá un 1.*/ //TODO: ver en que caso no seria satisfactorio
int escribir_valor(t_handler_paginacion* handler, int dir_fisica, uint32_t valor, int pid);

/*Finaliza proceso, hay que liberar memoria */
void remover_proceso(t_handler_paginacion* handler, int pid);

void handler_paginacion_destroy(t_handler_paginacion* handler);


/** ARCHIVOS */
// en el emplo de conf, la cantidad de elementos por pagina deberian ser maximo 4
uint32_t* leer_valor_pagina(t_handler_paginacion* handler, int dir_fisica, int cant_elementos, int pid);

void escribir_valor_pagina(t_handler_paginacion* handler, int dir_fisica, int cant_elementos, uint32_t* valores, int pid);

/** ALGORITMOS DE REEMPLAZO **/

t_pagina* remover_pagina_memoria(t_handler_paginacion* handler);
/* obtiene la pagina mas antigua que se encuentra cargada en memoria */
t_pagina* encontrar_pagina_FIFO(t_handler_paginacion* handler);
/* obtiene la pagina que fue usada hace mas tiempo, que se encuentra cargada en memoria */
t_pagina* encontrar_pagina_LRU(t_handler_paginacion* handler);

/** SWAP **/
int* obtener_posiciones_swap(int cantidad_paginas, int pid);
void liberar_posiciones_swap(t_handler_paginacion* handler, int pid);
void swap_out(t_handler_paginacion* handler, t_tabla_paginas* tabla, int nro_pagina);
void swap_in(t_handler_paginacion* handler, t_tabla_paginas* tabla, int nro_pagina, int nuevo_frame);


int cargar_en_memoria(t_handler_paginacion* handler, int pid, int nro_pagina);

/** AUX **/
//esta funcion no necesita ser publica, la agrego en el .h para testear
t_pagina* encontrar_pagina_a_modificar(t_lista_mutex* paginas_pendientes_modificacion, int nro_frame);

void imprimir_tablas_de_paginas(t_handler_paginacion* handler);

#endif /* SRC_HANDLER_PAGINACION_H_ */
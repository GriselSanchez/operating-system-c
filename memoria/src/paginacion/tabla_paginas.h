#ifndef SRC_TABLA_PAGINAS_H_
#define SRC_TABLA_PAGINAS_H_

#include <stdlib.h>

#include <commons/log.h>
#include <commons/temporal.h>
#include <stdint.h>
#include <pthread.h>

#define CANT_PAGINAS 1000


extern t_log* logger;

// posicion swap, deberia recibirse al crear la tabla.


typedef struct {
    int pid;
    int nro_pagina;
    int nro_frame;
    int bit_presencia;
    int bit_modificado;
    int pos_swap;
    t_temporal* creacion;
    t_temporal* ultima_lectura;
} t_pagina;

//posicion en la lista = numero de pagina
typedef struct {
    int pid;
    int cant_paginas;
    t_pagina paginas[CANT_PAGINAS];
} t_tabla_paginas;

/* el array debe tener la misma cantidad de elementos como cant_paginas
*/
t_tabla_paginas* create_tabla_paginas(int pid, int cant_paginas, int posiciones_swap[]);

/** Devuelve si la pagina esta en memoria (bit de presencia en 1)
 */
bool pagina_esta_en_memoria(t_tabla_paginas* tabla, int numero_pagina);

/** Devuelve informacion de la pagina (t_pagina)
 *  Actualiza la fecha de instante de referencia para LRU
 *  Antes de leer_pagina, chequear que este en memoria
*/
t_pagina* leer_pagina(t_tabla_paginas* tabla, int numero_pagina);

/** en la posicion dada por la pagina, guarda el numero de frame
 * y setea el bit de presencia a 1
 */
void cargar_pagina_en_memoria(t_tabla_paginas* tabla, int numero_pagina, int numero_frame);

/** en la posicion dada por la pagina, setea el bit de presencia a 0
 */
void tabla_remover_pagina_de_memoria(t_tabla_paginas* tabla, int numero_pagina);

/** en la posicion dada por la pagina, setea el bit de modificado a 1
 */
void marcar_dato_modificado(t_tabla_paginas* tabla, int numero_pagina);



/** GETTERS
 * cuidado con obtener estos datos cuando el bit de presencia no es 1
 */

t_temporal* pagina_obtener_creacion(t_tabla_paginas* tabla, int numero_pagina);

t_temporal* pagina_obtener_ultima_lectura(t_tabla_paginas* tabla, int numero_pagina);

bool pagina_fue_modificada(t_tabla_paginas* tabla, int numero_pagina);

int pagina_obtener_frame(t_tabla_paginas* tabla, int numero_pagina);

int pagina_obtener_pos_swap(t_tabla_paginas* tabla, int numero_pagina);

//usar solo para algoritmos de reemplazo, este getter no actualiza ningun dato de la pagina
t_pagina* pagina_obtener_pagina(t_tabla_paginas* tabla, int numero_pagina);

/** DESTROY **/
void destroy_tabla_paginas(t_tabla_paginas* tabla);

void log_acceso_tabla_de_paginas(t_tabla_paginas* tabla, int numero_pagina);

#endif
#include "paginacion/tabla_paginas.h"
#include "tabla_paginas.h"

extern pthread_mutex_t pagina_logger;

t_tabla_paginas* create_tabla_paginas(int pid, int cant_paginas, int posiciones_swap[])
{
    t_tabla_paginas* tabla = malloc(sizeof(t_tabla_paginas));

    tabla->pid = pid;
    tabla->cant_paginas = cant_paginas;
    for (int i = 0; i < cant_paginas; i++) {
        t_pagina fila;
        fila.pid = pid;
        fila.nro_pagina = i;
        fila.nro_frame = 0;
        fila.bit_presencia = 0;
        fila.bit_modificado = 0;
        fila.pos_swap = posiciones_swap[i];
        fila.creacion = NULL;
        fila.ultima_lectura = NULL;
        tabla->paginas[i] = fila;
    }

    return tabla;
}

void log_acceso_tabla_de_paginas(t_tabla_paginas* tabla, int numero_pagina) {
    t_pagina pagina = tabla->paginas[numero_pagina];
    log_info(logger, "PID: %i - Pagina %i - Marco %i", pagina.pid, numero_pagina, pagina.nro_frame);
}

bool pagina_esta_en_memoria(t_tabla_paginas* tabla, int numero_pagina)
{
    return tabla->paginas[numero_pagina].bit_presencia == 1;
}

t_pagina* leer_pagina(t_tabla_paginas* tabla, int numero_pagina)
{
    if (tabla->paginas[numero_pagina].ultima_lectura != NULL) {
        temporal_destroy(tabla->paginas[numero_pagina].ultima_lectura);
    }
    tabla->paginas[numero_pagina].ultima_lectura = temporal_create();

    return &tabla->paginas[numero_pagina];

}

void cargar_pagina_en_memoria(t_tabla_paginas* tabla, int numero_pagina, int numero_frame)
{
    tabla->paginas[numero_pagina].bit_presencia = 1;
    tabla->paginas[numero_pagina].nro_frame = numero_frame;
    tabla->paginas[numero_pagina].bit_modificado = 0;
    tabla->paginas[numero_pagina].creacion = temporal_create();
    tabla->paginas[numero_pagina].ultima_lectura = temporal_create();
}

void tabla_remover_pagina_de_memoria(t_tabla_paginas* tabla, int numero_pagina)
{
    log_acceso_tabla_de_paginas(tabla, numero_pagina);
    tabla->paginas[numero_pagina].bit_presencia = 0;

    if (tabla->paginas[numero_pagina].creacion != NULL) {
        temporal_destroy(tabla->paginas[numero_pagina].creacion);
        tabla->paginas[numero_pagina].creacion = NULL;
    }
    if (tabla->paginas[numero_pagina].ultima_lectura != NULL) {
        temporal_destroy(tabla->paginas[numero_pagina].ultima_lectura);
        tabla->paginas[numero_pagina].ultima_lectura = NULL;
    }
}

void marcar_dato_modificado(t_tabla_paginas* tabla, int numero_pagina)
{
    tabla->paginas[numero_pagina].bit_modificado = 1;
}


/** getters **/

t_temporal* pagina_obtener_creacion(t_tabla_paginas* tabla, int numero_pagina)
{
    return tabla->paginas[numero_pagina].creacion;
}

t_temporal* pagina_obtener_ultima_lectura(t_tabla_paginas* tabla, int numero_pagina)
{
    return tabla->paginas[numero_pagina].ultima_lectura;
}

bool pagina_fue_modificada(t_tabla_paginas* tabla, int numero_pagina)
{
    return tabla->paginas[numero_pagina].bit_modificado == 1;
}

int pagina_obtener_frame(t_tabla_paginas* tabla, int numero_pagina)
{
    log_acceso_tabla_de_paginas(tabla, numero_pagina);
    return tabla->paginas[numero_pagina].nro_frame;
}


int pagina_obtener_pos_swap(t_tabla_paginas* tabla, int numero_pagina)
{
    log_acceso_tabla_de_paginas(tabla, numero_pagina);
    return tabla->paginas[numero_pagina].pos_swap;
}

t_pagina* pagina_obtener_pagina(t_tabla_paginas* tabla, int numero_pagina)
{
    log_acceso_tabla_de_paginas(tabla, numero_pagina);
    return &tabla->paginas[numero_pagina];
}

/** DESTROY **/

void destroy_tabla_paginas(t_tabla_paginas* tabla) {
    for (int i = 0; i < tabla->cant_paginas; i++) {
        if (tabla->paginas[i].creacion != NULL) temporal_destroy(tabla->paginas[i].creacion);
        if (tabla->paginas[i].ultima_lectura != NULL) temporal_destroy(tabla->paginas[i].ultima_lectura);
    }
    free(tabla);
}

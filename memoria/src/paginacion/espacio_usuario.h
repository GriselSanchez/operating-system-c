#ifndef SRC_ESPACIO_USUARIO_H_
#define SRC_ESPACIO_USUARIO_H_

#include <stdlib.h>
#include <stdint.h> 
#include <string.h>
#include <math.h>
#include <commons/log.h>
#include <pthread.h>

extern t_log* logger;

typedef struct {
    int tamanio_memoria;
    int cantidad_marcos;
    int marcos_ocupados[10000];
    pthread_mutex_t* mutex;
    void* espacio;
} t_espacio_usuario;

//deberian ser los marcos esto? o accesderse a traves de la direccion generada con el marco?
//definicion de memoria: vector de bytes, uno al lado del otro, con direccion unica -> bajo nivel
/*Un espacio contiguo de memoria (representado por un void*). Este representará el espacio
de usuario de la misma, donde los procesos podrán leer y/o escribir.*/

t_espacio_usuario* create_espacio_usuario(int tamanio, int tamanio_marco);

uint32_t espacio_leer_valor(t_espacio_usuario* espacio_usuario, int dir_fisica, int pid);

void espacio_escribir_valor(t_espacio_usuario* espacio_usuario, int dir_fisica, uint32_t valor, int pid);

/**
 * Devuelve el primer frame libre que encuentra.
 * Si no hay frames libres, devuelve -1.
 */
int obtener_frame_libre(t_espacio_usuario* espacio_usuario);

void marcar_frame_ocupado(t_espacio_usuario* espacio_usuario, int marco);

void liberar_frame(t_espacio_usuario* espacio_usuario, int marco);

int espacio_obtener_frame(t_espacio_usuario* espacio_usuario, int dir_fisica);


void espacio_destroy(t_espacio_usuario* espacio_usuario);

void imprimir_espacio_usuario(t_espacio_usuario* espacio_usuario);

#endif /* SRC_ESPACIO_USUARIO_H_ */
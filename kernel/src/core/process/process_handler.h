#ifndef SRC_CORE_PROCESS_HANDLER_H_
#define SRC_CORE_PROCESS_HANDLER_H_

#include <commons/log.h>
#include <commons/string.h>
#include <semaphore.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>

#include "kernel.h"
#include "utils/cola_monitor.h"
#include "utils/lista_monitor.h"
#include "process.h"
#include "model/instrucciones.h"
#include "model/contexto-ejecucion.h"
#include "core/resources/resources.h"
#include "core/filesystem/filesystem.h"
#include "core/planners/planners.h"
#include "core/memory/memory.h"
#include "deadlock/deadlock.h"

extern t_log* logger;
extern t_kernel* kernel;

/**
* @NAME: start_short_term_planner
* @DESC: Ejecuta un planifiador de corto plazo, en un hilo aparte, de forma no bloqueante.
*        Recibe por parametro los sockets de dispatch e interrupt de la CPU y el planificador de corto plazo.
*/
void start_short_term_planner(int cpu_disp_fd, t_kernel_memory* memoria, t_planner* planner, t_kernel_filesystem* filesystem);

/**
* @NAME: start_long_term_planner
* @DESC: Ejecuta un planifiador de largo plazo, en un hilo aparte, de forma no bloqueante.
*        Recibe por parametro el valor de maxima multiprogramacion y el planificador de corto plazo.
*/
void start_long_term_planner(int max_multiprogramming, t_planner* planner, t_kernel_memory* memoria);

/**
* @NAME: start_interruptions_handler
* @DESC: Ejecuta el handler de interrupciones, en un hilo aparte, de forma no bloqueante.
*        Recibe por parametro el socket para comunicarse con la CPU.
*/
void start_interruptions_handler(int cpu_interruption_fd);

/**
* @NAME: handle_new_process
* @DESC: Recibe la prioridad de un proceso y el path de ubicacion de las instrucciones.
*        Se encarga de crear un nuevo proceso con su PCB, y agregarlo en el planificador de
*        largo plazo. Ademas lanza un evento de "nuevo proceso".
*/
void handle_new_process(char* path, int size, int priority);

/**
* @NAME: destroy_proccess_handler
* @DESC: Libera las estructuras creadas por los distintos planificadores.
*/
void destroy_proccess_handler();

/**
* @NAME: find_process
* @DESC: Busca un proceso segun un id recibido entre todos los procesos en memoria.
*/
t_pcb* find_process(int pid);

/**
* @NAME: handle_force_process_exit
* @DESC: Recibe un PID y fuerza la salida de un proceso.
*/
void handle_force_process_exit(t_planner* short_term_planner, t_kernel_memory* memoria, int pid);

/**
* @NAME: update_max_multi_programming
* @DESC: Cambia el valor de maxima multiprogramacion del planificador.
*/
void update_max_multi_programming(int new_max_mp);

void _simulate_process_block(void* param);

/*** @DESC: Bloquea un proceso durante el tiempo indicado, devolviendolo a READY luego.
*        Si `block_time <= 0` el proceso se bloquea indeterminadamente (solamente se marca como BLOCK).
*/
void process_block_sleep(t_pcb* process_context, t_planner* planner, int block_time);

void _simulate_process_block(void* param);

#endif /* SRC_CORE_PROCESS_HANDLER_H_ */
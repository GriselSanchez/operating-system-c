#ifndef SRC_CORE_PROCESS_H_
#define SRC_CORE_PROCESS_H_

#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/socket.h>
#include <unistd.h> //sleep
#include <commons/log.h>
#include <commons/collections/list.h>
#include <commons/collections/queue.h>

#include "utils/serializacion.h"
#include "utils/sockets.h"
#include "model/instrucciones.h"
#include "model/pcb.h"
#include "utils/cola_monitor.h"
#include "utils/lista_monitor.h"
#include "../planners/planners.h"


extern t_log* logger;
extern t_cola_mutex* cola_bloqueados;
extern t_cola_mutex* cola_bloqueados_io;
extern t_cola_mutex* cola_bloqueados_excepcion;
extern t_cola_mutex* cola_bloqueados_sleep;
extern t_cola_mutex* cola_bloqueados_eventos;
extern sem_t sem_stp_paused;

typedef struct {
  int pid;
  sem_t sem_execution;
} t_process;

typedef struct {
  t_pcb* pcb;
  t_planner* planner;
  int block_time;
} t_block_params;

/**
*  Estas variables son externas y se utilizan para el manejo de cambio
*  de estado de los procesos, se inicializacion en el `process_handler`
*  con la función `initialize_proccess_handler`.
*/
extern sem_t max_concurrent_multiprograming;
extern sem_t ready_process_event;
//extern t_cola_mutex* new_process_contexts;
//extern t_lista_mutex* ready_process_contexts;
//extern t_resources resources;

/**
* @NAME: process_new
* @DESC: Recibe el path del archivo de instrucciones para un nuevo proceso y su prioridad.
*        Crea el PCB (process context block) para el nuevo proceso y le asigna el PID correspondiente.
*/
t_pcb* process_new(char* path, int size, int priority);

/**
* @NAME: process_execute
* @DESC: Marca un proceso en estado EXEC.
*/
void process_execute(t_pcb*);

/**
* @NAME: process_block
* @DESC: Marca un proceso en estado BLock
*/
void process_block(t_pcb* process_context);


/**
* @NAME: process_ready
* @DESC: Marca un proceso en estado READY.
*        Recibe como parametros el planificador de corto plazo que se este utlizando en ese momento
*        //La CPU debe haber actualizado la ultima rafaga de ejecucion del proceso.
*/
void process_ready(t_pcb* process_context, t_planner* planner);

/**
* @NAME: process_release
* @DESC: Una vez finalizado un proceso la funcion se encarga de liberar los recursos correspondientes
*        al PCB del mismo y notificar a la consola con el estado de ejecucion del proceso, el cual debe
*        asignarse previamente en el `pcb`. Emite un evento para avisar que se disponde de la capacidad
*        de procesamiento concurrente previamente asignada para este proceso ya finalizado.
*/
int process_release(t_pcb* process_context);

/**
* @NAME: process_assign_resource
* @DESC: Se agrega el recurso asignado dentro de la estructura PCB de un proceso.
*        Se reciben por parametros el proceso y el ID del recurso, correspondientes a la operacion.
*/
void process_assign_resource(t_pcb* pcb, char* resource_name);

/**
* @NAME: process_remove_resource
* @DESC: Se elimina un recurso asignado previamente dentro de la estructura PCB de un proceso.
*        Se reciben por parametros el proceso y el ID del recurso, correspondientes a la operacion.
*/
void process_remove_resource(t_pcb* pcb, char* resource_name);

/**
* @NAME: process_assign_open_file
* @DESC: Se agrega el archivo abierto dentro de la estructura PCB de un proceso.
*        Se reciben por parametros el proceso, el ID del archivo (nombre) y el modo de apertura.
*/
void process_assign_open_file(t_pcb* pcb, char* file_name, modo_apertura mode);

/**
* @NAME: process_remove_open_file
* @DESC: Se elimina un archivo asignado previamente dentro de la estructura PCB de un proceso.
*        Se reciben por parametros el proceso y el ID del archivo (nombre).
*/
void process_remove_open_file(t_pcb* pcb, char* file_name);

/**
* @NAME: process_has_open_file
* @DESC: Devuelve si un archivo se encuentra abierto para un proceso.
*        Se reciben por parametros el proceso, el ID del archivo (nombre).
*        En `mode` se devuelve el modo de apertura del archivo para el proceso.
*/
bool process_has_open_file(t_pcb* pcb, char* file_name, modo_apertura* mode);

/**
* @NAME: process_kill
* @DESC: Luego de que un proceso haya finalizado con `process_release` se debe llamar a esta funcion 
*        con la direccion de memoria del pcb del proceso terminado.
*        Al finalizar, devuelve el parametro apuntando a NULL.
*/
void process_kill(t_pcb** pcb_pointer);

/**
* @NAME: process_find_open_file
*/
t_file* _process_find_open_file(t_pcb* pcb, char* file_name);


void remover_de_cola_bloqueados(t_cola_mutex* cola_bloqueados, void* elemento);

void process_assign_pending_file(t_pcb* pcb, char* file_name, modo_apertura mode);

#endif /* SRC_CORE_PROCESS_H_ */

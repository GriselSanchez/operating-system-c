/*
 *
 *  Created on: May 12, 2023
 *      Author: utnso
 */

#ifndef SRC_CORE_PLANIFICADORES_H_
#define SRC_CORE_PLANIFICADORES_H_

#include <commons/temporal.h>
#include "utils/lista_monitor.h"
#include "utils/cola_monitor.h"
#include <unistd.h>
#include "model/pcb.h"

extern t_log* logger;

typedef struct planner {
  char* name;
  void* planner;
  t_pcb* (*next_process)(struct planner*, t_lista_mutex*);
  void (*process_on_ready)(struct planner*, t_pcb*);
  void (*init_quantum)(struct planner*, t_pcb*);
  void (*remove_process)(struct planner*, t_pcb*);
} t_planner;

typedef struct {
  t_cola_mutex* fifo;
} t_planner_fifo;

typedef struct {
  t_cola_mutex* rr;
  int quantum;
  pthread_t thread_quantum_rr;
} t_planner_rr;

typedef struct {
  int quantum;
  t_pcb* pcb;
  t_planner* planner;
} t_thread_quantum_arg;

typedef struct {
  t_lista_mutex* procesos;
  t_pcb* proceso_actual;
} t_planner_prioridades;

t_planner* create_planner(char* name);

void planner_destroy(t_planner* planner);
extern sem_t sem_interruption;

/*--------- Planificador FIFO ---------*/
/**
 * @NAME: planner_set_fifo
 * @DESC: Configura al planificador que se pasa por parametros para funcionar con FIFO.
 */
void planner_set_fifo(t_planner* planner);

/**
 * @NAME: fifo_next_process
 * @DESC: Devuelve el proximo proceso a ejecutar segun el algoritmo FIFO.
 *        No es requerido pasarle el parametro `procesos_en_ready`.
 */
t_pcb* fifo_next_process(t_planner* planner, t_lista_mutex* procesos_en_ready);

/**
 * @NAME: fifo_process_on_ready
 * @DESC: Se debe llamar antes de enviar un proceso a READY utilizando el algoritmo de FIFO.
 */
void fifo_process_on_ready(t_planner* planner, t_pcb* proceso);

/*--------- Planificador RR ---------*/
/**
 * @NAME: planner_set_rr
 * @DESC: Configura al planificador que se pasa por parametros para funcionar con RR.
 */
void planner_set_rr(t_planner* planner, int quantum);

/**
 * @NAME: rr_next_process
 * @DESC: Devuelve el proximo proceso a ejecutar segun el algoritmo RR.
 *        No es requerido pasarle el parametro `procesos_en_ready`.
 */
t_pcb* rr_next_process(t_planner* planner, t_lista_mutex* procesos_en_ready);

/**
 * @NAME: rr_process_on_ready
 * @DESC: Se debe llamar antes de enviar un proceso a READY utilizando el algoritmo de RR.
 */
void rr_process_on_ready(t_planner* planner, t_pcb* proceso);

/**
 * @NAME: handle_quantum
 * @DESC: Se debe llamar al solicitar nuevo proceso para iniciarlizar quantum.
 */
void handle_quantum(t_planner* planner, t_pcb* pcb);

/**
 * @NAME: watch_quantum_rr
 * @DESC: Contador de quantum que marca cuando debe enviar interrupcion.
 */
void watch_quantum_rr(t_thread_quantum_arg* args);


/*--------- Planificador FIFO ---------*/
/**
 * @NAME: planner_set_planner_prioridades
 * @DESC: Configura al planificador que se pasa por parametros para funcionar con algoritmo por prioridades.
 */
void planner_set_planner_prioridades(t_planner* planner);

/**
 * @NAME: planner_prioridades_next_process
 * @DESC: Devuelve el proximo proceso a ejecutar segun el algoritmo por prioridades.
 *
 */
t_pcb* planner_prioridades_next_process(t_planner* planner, t_lista_mutex* procesos_en_ready);

/**
 * @NAME: planner_prioridades_add_process_on_ready
 * @DESC: Se debe llamar antes de enviar un proceso a READY utilizando el algoritmo de prioridades.
 * Si el nuevo proceso tiene mas prioridad que el actual, se envia senal de desalojo.
 */
void planner_prioridades_add_process_on_ready(t_planner* planner, t_pcb* nuevo_proceso);

void fifo_remove_process(t_planner* planner, t_pcb* proceso);
void rr_remove_process(t_planner* planner, t_pcb* proceso);
void planner_prioridades_remove_process(t_planner* planner, t_pcb* nuevo_proceso);

#endif /* SRC_CORE_PLANIFICADORES_H_ */

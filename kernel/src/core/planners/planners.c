#include "planners.h"

extern pthread_mutex_t pcb_status_mutex;

void handle_planner_evict_process(t_planner* short_term_planner, t_pcb* pcb, op_code int_reason);
void process_ready(t_pcb* process_context, t_planner* planner);

t_planner* create_planner(char* name) {
  t_planner* planner = malloc(sizeof(t_planner));
  planner->name = strdup(name);
  return planner;
}

void planner_destroy(t_planner* planner) {
  // TODO: agregar destroy de lista de procesos en prioridades
  free(planner->planner);
  free(planner);
}

/*--------- Planificador FIFO ---------*/

void planner_set_fifo(t_planner* planner) {
  t_planner_fifo* planner_fifo = malloc(sizeof(t_planner_fifo));
  planner_fifo->fifo = queue_init_mutex();

  planner->next_process = &fifo_next_process;
  planner->process_on_ready = &fifo_process_on_ready;
  planner->remove_process = &fifo_remove_process;
  planner->init_quantum = NULL;
  planner->planner = (void*)planner_fifo;
}

t_pcb* fifo_next_process(t_planner* planner, t_lista_mutex* _) {
  t_planner_fifo* fifo_planner = (t_planner_fifo*)planner->planner;
  return queue_pop_mutex(fifo_planner->fifo);
}

void fifo_process_on_ready(t_planner* planner, t_pcb* proceso) {
  t_planner_fifo* fifo_planner = (t_planner_fifo*)planner->planner;
  queue_push_mutex(fifo_planner->fifo, proceso);
}

void fifo_remove_process(t_planner* planner, t_pcb* proceso) {
  t_planner_fifo* fifo_planner = (t_planner_fifo*)planner->planner;
  queue_remove_element_mutex(fifo_planner->fifo, proceso);
}

/*--------- Planificador RR ---------*/
void planner_set_rr(t_planner* planner, int quantum) {
  t_planner_rr* planner_rr = malloc(sizeof(t_planner_rr));
  pthread_t thread_id;
  planner_rr->rr = queue_init_mutex();
  planner_rr->quantum = quantum;
  planner_rr->thread_quantum_rr = thread_id;

  planner->next_process = &rr_next_process;
  planner->process_on_ready = &rr_process_on_ready;
  planner->remove_process = &rr_remove_process;
  planner->init_quantum = &handle_quantum;
  planner->planner = (void*)planner_rr;
}

t_pcb* rr_next_process(t_planner* planner, t_lista_mutex* procesos_en_ready) {
  t_planner_rr* rr_planner = (t_planner_rr*)planner->planner;

  t_pcb* next_process = queue_pop_mutex(rr_planner->rr);
  //handle_quantum(rr_planner->quantum, planner, next_process, rr_planner->thread_quantum_rr);
  return next_process;
}

void rr_process_on_ready(t_planner* planner, t_pcb* proceso) {
  t_planner_rr* rr_planner = (t_planner_rr*)planner->planner;
  queue_push_mutex(rr_planner->rr, proceso);
}

void rr_remove_process(t_planner* planner, t_pcb* proceso) {
  t_planner_rr* rr_planner = (t_planner_rr*)planner->planner;
  queue_remove_element_mutex(rr_planner->rr, proceso);
}

// Thread for RR quantum
void handle_quantum(t_planner* planner, t_pcb* pcb) {
  t_planner_rr* planner_rr = planner->planner;
  pthread_t thread_id = planner_rr->thread_quantum_rr;
  int quantum = planner_rr->quantum;
  if (thread_id) pthread_cancel(thread_id);

  t_thread_quantum_arg* quantum_arg = malloc(sizeof(t_thread_quantum_arg));
  quantum_arg->quantum = quantum;
  quantum_arg->pcb = pcb;
  quantum_arg->planner = planner;
  pthread_create(&thread_id, NULL, (void*)watch_quantum_rr, (void*)quantum_arg);
  pthread_detach(thread_id);
}

void watch_quantum_rr(t_thread_quantum_arg* args) {
  t_pcb* pcb = args->pcb;
  t_planner* planner = args->planner;
  int quantum = args->quantum;
  int quantum_on_sec = quantum / 1000;
  sleep(quantum_on_sec);
  pthread_mutex_lock(&pcb_status_mutex);
  if (pcb != NULL && pcb->current_status != EXIT && pcb->current_status != BLOCK) {
    // TODO: ver que se está logeando current_status 
    log_info(logger, "PID: %d - Desalojado por fin de Quantum", pcb->pid);
    handle_planner_evict_process(planner, pcb, PROC_EXIT_QUANTUM);
  }
  pthread_mutex_unlock(&pcb_status_mutex);

  return;
}

/**
 * PLANNER PRIORIDADES
 */

bool comparador_prioridades(void* nuevo_proceso, void* proceso_actual) {
  t_pcb* proceso1 = (t_pcb*)nuevo_proceso;
  t_pcb* proceso2 = (t_pcb*)proceso_actual;

  return proceso1->priority < proceso2->priority;
}

void planner_set_planner_prioridades(t_planner* planner)
{
  t_planner_prioridades* planner_prioridades = malloc(sizeof(t_planner_prioridades));
  planner_prioridades->procesos = list_init_mutex();
  planner_prioridades->proceso_actual = NULL;

  planner->next_process = &planner_prioridades_next_process;
  planner->process_on_ready = &planner_prioridades_add_process_on_ready;
  planner->remove_process = &planner_prioridades_remove_process;
  planner->init_quantum = NULL;
  planner->planner = (void*)planner_prioridades;
}

t_pcb* planner_prioridades_next_process(t_planner* planner, t_lista_mutex* procesos)
{
  t_planner_prioridades* planner_prioridades = (t_planner_prioridades*)planner->planner;
  planner_prioridades->proceso_actual = (t_pcb*)list_remove_mutex(planner_prioridades->procesos, 0);
  return planner_prioridades->proceso_actual;
}

//tengo una lista ordenada por prioridad de pcb en la insercion
void planner_prioridades_add_process_on_ready(t_planner* planner, t_pcb* nuevo_proceso) {
  t_planner_prioridades* planner_prioridades = (t_planner_prioridades*)planner->planner;
  if (planner_prioridades->proceso_actual != NULL
    && nuevo_proceso->priority < planner_prioridades->proceso_actual->priority) {

    list_add_in_index_mutex(planner_prioridades->procesos, 0, nuevo_proceso);

    //por consistencia
    //planner_prioridades->proceso_actual = NULL;
    //desalojar proceso
    handle_planner_evict_process(planner, planner_prioridades->proceso_actual, PROC_EXIT_PRIORITY);
  } else {
    list_add_sorted_mutex(planner_prioridades->procesos, nuevo_proceso, &comparador_prioridades);
  }
}

void planner_prioridades_remove_process(t_planner* planner, t_pcb* nuevo_proceso) {
  t_planner_prioridades* planner_prioridades = (t_planner_prioridades*)planner->planner;
  list_remove_element_mutex(planner_prioridades->procesos, nuevo_proceso);
}
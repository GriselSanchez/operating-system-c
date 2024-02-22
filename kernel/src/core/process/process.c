#include "process.h"
#include <pthread.h>
#include <unistd.h>

#define STRING_EQUAL(b, c) strcmp(b, c) == 0

int* recursos;
int recursos_disponibles;

extern pthread_mutex_t pcb_status_mutex;

void _simulate_process_block(void* params);
void _process_change_status(t_pcb* pcb, Status next_status);

t_pcb* process_new(char* path, int size, int priority) {
  pthread_mutex_lock(&pcb_status_mutex);
  t_pcb* new_process = create_new_pcb(path, size, priority);
  log_info(logger, "Se crea el proceso %i en NEW", new_process->pid);
  pthread_mutex_unlock(&pcb_status_mutex);

  //imprimir_instrucciones(instructions);
  return new_process;
}

void process_execute(t_pcb* process_context) {
  _process_change_status(process_context, EXEC);
}

void process_assign_resource(t_pcb* pcb, char* resource_name) {
  pcb->blocked_by_resource_name = NULL;
  char* asigned = strdup(resource_name);
  list_add(pcb->asigned_resources, asigned);
}

void process_remove_resource(t_pcb* pcb, char* resource_name) {
  bool _cond(void* res_name) {
    return string_equals_ignore_case((char*)res_name, resource_name);
  }
  list_remove_and_destroy_by_condition(pcb->asigned_resources, &_cond, &free);
}

t_file* _process_find_open_file(t_pcb* pcb, char* file_name) {
  bool _condition(void* file) {
    return strcmp(((t_file*)file)->nombre_archivo, file_name) == 0;
  }
  return list_find(pcb->open_files, &_condition);
}

void process_remove_open_file(t_pcb* pcb, char* file_name) {
  t_file* open_file = _process_find_open_file(pcb, file_name);
  if (open_file != NULL) {
    list_remove_element(pcb->open_files, open_file);
  }
  file_destroy(open_file);
}

bool process_has_open_file(t_pcb* pcb, char* file_name, modo_apertura* mode) {
  t_file* open_file = _process_find_open_file(pcb, file_name);
  if (open_file != NULL) {
    if (mode != NULL) *mode = open_file->modo_apertura;
    return true;
  }
  return false;
}


t_file* file_create(char* file_name, modo_apertura mode) {
  char* new_file_name = strdup(file_name);
  t_file* new_file = malloc(sizeof(t_file));
  new_file->nombre_archivo = new_file_name;
  new_file->puntero_archivo = 0;
  new_file->modo_apertura = mode;
  return new_file;
}

void file_destroy(t_file* file) {
  free(file->nombre_archivo);
  free(file);
}

void process_assign_open_file(t_pcb* pcb, char* file_name, modo_apertura mode) {
  t_file* open_file = file_create(file_name, mode);
  list_add(pcb->open_files, open_file);
  log_info(logger, "PID: %d - Archivo agregado a tabla de archivos del proceso: %s", pcb->pid, file_name);
  pcb->pending_file = NULL;
}

void process_assign_pending_file(t_pcb* pcb, char* file_name, modo_apertura mode) {
  t_file* file = file_create(file_name, mode);
  pcb->pending_file = file;
  log_info(logger, "PID: %d - Archivo asignado como pendiente de abrir: %s", pcb->pid, file_name);
}

void process_block(t_pcb* process_context) {
  //log_info(logger, "PID: %d - Bloqueado por: <SLEEP | NOMBRE_RECURSO | NOMBRE_ARCHIVO>", 
  //                  process_context->pid);
  _process_change_status(process_context, BLOCK);
}

void process_ready(t_pcb* process_context, t_planner* planner) {
  planner->process_on_ready(planner, process_context);
  _process_change_status(process_context, READY);
}

int process_release(t_pcb* process_context) {
  _process_change_status(process_context, EXIT);
  /**
   * Cuando se reciba un mensaje de CPU con motivo de finalizar el proceso,
   * se deberá pasar al mismo al estado EXIT, liberar todos los recursos que tenga asignados
   */
  int exit_code = process_context->exit_code;
  sem_post(&max_concurrent_multiprograming);

  //Motivo: <SUCCESS / SEG_FAULT / OUT_OF_MEMORY>
  log_info(logger, "Finaliza el proceso %i - Motivo: %s", process_context->pid, OP_CODE_NAME(process_context->exit_code));
  if (process_context != NULL) finish_pcb(process_context);
  return 0;
}

void process_kill(t_pcb** pcb_pointer) {
  t_pcb* pcb = *pcb_pointer;
  free(pcb);
  pcb_pointer = NULL;

}

void _process_change_status(t_pcb* pcb, Status next_status) {
  // we check if planning es paused
  //if (kernel->paused_planning) sem_wait(&sem_stp_paused);
  pthread_mutex_lock(&pcb_status_mutex);
  log_info(logger, "PID: %d - Estado Anterior %s - Estado Actual: %s",
    pcb->pid,
    STATUS_NAME(pcb->current_status),
    STATUS_NAME(next_status));
  pcb->current_status = next_status;
  pthread_mutex_unlock(&pcb_status_mutex);
}



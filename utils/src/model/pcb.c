#include "pcb.h"

#define NOT_SHARED 0

/**
 * @NAME: last_pid
 * @DESC: Se utiliza a la hora de asignarle un 'process id' a un nuevo proceso,
 *        siempre siendo dicha asignacion de forma incremental. Para modificarlo
 *        es requerido usar `last_pid_mutex`.
*/
int last_pid = 0;
pthread_mutex_t last_pid_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @NAME: new_process_id
 * @DESC: Devuelve un nuevo pid de forma thread safe.
*/
int new_process_id() {
  pthread_mutex_lock(&last_pid_mutex);
  int new_pid = ++last_pid;
  pthread_mutex_unlock(&last_pid_mutex);
  return new_pid;
}

t_pcb* create_new_pcb(char* path, int size, int priority) {
  t_pcb* new_process = malloc(sizeof(t_pcb));

  new_process->pid = new_process_id();
  new_process->program_counter = NOT_STARTED;
  new_process->cpu_registries = initialize_registries();
  new_process->priority = priority;
  new_process->path = path;
  new_process->size = size;
  new_process->resource_name = NULL;
  new_process->nombre_archivo = NULL;
  new_process->block_time = 0;
  new_process->page_fault_nro = 0;
  new_process->blocked_by_resource_name = NULL;
  new_process->current_status = NEW;
  new_process->asigned_resources = list_create();
  new_process->open_files = list_create();
  new_process->is_finished = false;

  // Porfa no borrar
  new_process->nombre_archivo = NULL;
  new_process->modo_apertura = -1;
  new_process->puntero_archivo = -1;
  new_process->dir_fisica = -1;
  new_process->tamanio_archivo = -1;

  return new_process;
}

t_registros* initialize_registries() {
  t_registros* cpu_registries = malloc(sizeof(t_registros));
  for (int i = 0; i < 4; i++) {
    cpu_registries->registros_4bytes[i] = 0;
  }
  return cpu_registries;
}

void destroy_registries(t_registros* registries) {
  free(registries);
}

void finish_pcb(t_pcb* pcb) {
  //no liberar el PCB en esta func.

  //TODO: Revisar por que el `instruccion_destroy` no funciona aca adentro.
  destroy_registries(pcb->cpu_registries);
  if (pcb->asigned_resources != NULL) list_destroy(pcb->asigned_resources);
  if (pcb->open_files != NULL) list_destroy(pcb->open_files);
  // if (pcb->pending_file != NULL) list_destroy(pcb->pending_file);

  //list_destroy_and_destroy_elements(pcb->open_files, (void*)assigned_resource);
  //list_destroy_and_destroy_elements(pcb->open_files, (void*)open_file_destroy);
  pcb->is_finished=true;
}

char* STATUS_NAME(Status status) {
  if (status == NEW) {
    return "NEW\0";
  } else if (status == READY) {
    return "READY\0";
  } else if (status == EXEC) {
    return "EXEC\0";
  } else if (status == BLOCK) {
    return "BLOCK\0";
  } else if (status == EXIT) {
    return "EXIT\0";
  } else {
    return "STATUS_UNDEFINED\0";
  }
}
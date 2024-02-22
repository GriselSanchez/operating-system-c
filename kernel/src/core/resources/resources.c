#include "resources.h"

#define NOT_SHARED 0
#define SHARED 1

pthread_mutex_t res_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t instance_mutex = PTHREAD_MUTEX_INITIALIZER;


bool resources_exist_resource(t_resources* resources, char* resource_name) {
  return dictionary_has_key(resources->resources_dict, resource_name);
}

/**
A la hora de recibir de la CPU un Contexto de Ejecución desplazado por WAIT,
el Kernel deberá verificar primero que exista el recurso solicitado y en caso de que exista
restarle 1 a la cantidad de instancias del mismo. En caso de que el número sea estrictamente
menor a 0, el proceso que realizó WAIT se bloqueará en la cola de bloqueados correspondiente
al recurso.

A la hora de recibir de la CPU un Contexto de Ejecución desplazado por SIGNAL, el Kernel
deberá verificar primero que exista el recurso solicitado y luego sumarle 1 a la cantidad
de instancias del mismo. En caso de que corresponda, desbloquea al primer proceso de la cola
de bloqueados de ese recurso. Una vez hecho esto, se devuelve la ejecución al proceso
que peticionó el SIGNAL.

Para las operaciones de WAIT y SIGNAL donde el recurso no exista,
se deberá enviar el proceso a EXIT.
*/

/* Otra solucion podria ser:
 *    - cada recurso tiene un semaforo con la cant. de inst. disponibles.
 *    - agrego el proceso a la cola del recurso.
 *    - intento descontar del semaforo del recurso.
 *      > si pude descontar sigo
 *      > sino, me bloqueo
 *    - ... ¿?
*/

int _get_resource(t_resources* resources, char* resource_name, t_resource** resource) {
  if (!resources_exist_resource(resources, resource_name)) {
    return RESOURCE_NOT_DEFINED;
  }
  pthread_mutex_lock(&res_mutex);
  *resource = (t_resource*)dictionary_get(resources->resources_dict, resource_name);
  pthread_mutex_unlock(&res_mutex);
  return 0;
}

int atomicAvailableIncrementAndGet(t_resource* resource) {
  pthread_mutex_lock(&res_mutex);
  int incremented = ++resource->available_qty;
  pthread_mutex_unlock(&res_mutex);
  return incremented;
}

int atomicAvailableDecrementAndGet(t_resource* resource) {
  pthread_mutex_lock(&res_mutex);
  int decremented = --resource->available_qty;
  pthread_mutex_unlock(&res_mutex);
  return decremented;
}

int resources_request(t_resources* resources, char* resource_name, int pid, t_resource** res) {
  t_resource* resource;
  //  if(res != NULL) *res=NULL;
  if (_get_resource(resources, resource_name, &resource) != 0) {
    return RESOURCE_NOT_DEFINED;
  }
  //si el recurso existe le resto uno.
  int available_qty = atomicAvailableDecrementAndGet(resource);
  bool available = available_qty >= 0;
  //si no esta disponible me guardo el PID que pidio su uso.
  if (!available) {
    int* ppid = malloc(sizeof(int));
    *ppid = pid;
    queue_push_mutex(resource->blocked_pids, ppid);
  } else {
    //esto es solo para loguear

  }
  if (res != NULL) *res = resource;
  return available ? 0 : available_qty * (-1);
}

int resources_release(t_resources* resources, char* resource_name, t_resource** res) {
  t_resource* resource;
  if (_get_resource(resources, resource_name, &resource) != 0) {
    return RESOURCE_NOT_DEFINED;
  }
  //si el recurso existe le sumo uno.
  int available_qty = atomicAvailableIncrementAndGet(resource);
  bool available = available_qty > 0;
  if (res != NULL) *res = resource;
  if (available_qty > resource->max_qty) return ERROR_ON_ATOMIC_ASSIGNMENT;
  return available ? 0 : available_qty * (-1);
}

int resource_next_pid(t_resource* resource) {
  int* next = queue_pop_mutex(resource->blocked_pids);
  return next != NULL ? *next : INEXISTENT_NEXT_PID;
}

int resources_next_pid(t_resources* resources, char* resource_name) {
  t_resource* resource;
  if (_get_resource(resources, resource_name, &resource) != 0) return RESOURCE_NOT_DEFINED;
  //si el recurso existe retorno el proximo pid que espera para utilizarlo.
  return resource_next_pid(resource);
}

t_resource* _resource_create(char* resource_name, int resource_qty) {
  t_cola_mutex* queue = queue_init_mutex();
  t_resource* new_resource = malloc(sizeof(t_resource));
  new_resource->resource_name = strdup(resource_name);
  new_resource->max_qty = resource_qty;
  new_resource->available_qty = resource_qty;
  new_resource->available_instances = resource_qty;
  new_resource->blocked_pids = queue;
  return new_resource;
}

//AGREGO EL PROCESO A LA COLA DE BLOQUEADOS
void resource_add_process_blocked(t_resource* resource, int pid) {
  queue_push_mutex(resource->blocked_pids, pid);
}


t_resources* resources_initialize(t_resources* resources, t_list* recursos, t_list* instancias_recursos) {
  if (list_size(recursos) != list_size(instancias_recursos)) {
    //return DATA_DE_RECURSOS_INCONSISTENTE;
    //explotar !!!
  }
  t_dictionary* resources_dict = dictionary_create();
  char* resource_name;

  for (int index = 0; index < recursos->elements_count; index++) {
    resource_name = list_get(recursos, index);
    //TODO: chequear si ya existe en el diccionario ?
    int* resource_qty = list_get(instancias_recursos, index);
    t_resource* res = _resource_create(resource_name, *resource_qty);
    dictionary_put(resources_dict, resource_name, res);
    log_debug(logger, "Resource '%s' created with %d instances.", resource_name, *resource_qty);
  }

  if (resources == NULL) resources = malloc(sizeof(t_resources));
  resources->resources_dict = resources_dict;
  return NULL;
}

//call when freeing an instance
void add_resource_instances_available(t_resource* resource) {
  pthread_mutex_lock(&instance_mutex);
  resource->available_instances ++;
  pthread_mutex_unlock(&instance_mutex);
}

//call when assinging an instance
void sub_resource_instances_available(t_resource* resource) {
  pthread_mutex_lock(&instance_mutex);
  resource->available_instances --;
  pthread_mutex_unlock(&instance_mutex);
}


void resource_destroy(t_resource* resource) {
  queue_destroy_mutex(resource->blocked_pids);
  free(resource);
}

void resources_destroy(t_resources* resources) {
  dictionary_destroy_and_destroy_elements(resources->resources_dict, &resource_destroy);
  free(resources);
}

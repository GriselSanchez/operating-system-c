#ifndef SRC_CORE_RESOURCES_H_
#define SRC_CORE_RESOURCES_H_

#include <stdio.h>
#include <sys/sem.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <commons/log.h>
#include <commons/collections/dictionary.h>
#include "utils/cola_monitor.h"

#define DATA_DE_RECURSOS_INCONSISTENTE -1 
#define RESOURCE_NOT_DEFINED -2
#define ERROR_ON_ATOMIC_ASSIGNMENT -3
#define INEXISTENT_NEXT_PID -4
#define NO_ERROR 0
#define BUSY 1

typedef struct {
  char* resource_name;
  int max_qty;
  int available_qty;
  int available_instances; //ONLY FOR LOGS, DON'T USE IT ANYWHERE ELSE.
  t_cola_mutex* blocked_pids; //LISTA DE PROCESOS BLOQUEADOS
} t_resource;

typedef struct {
  t_dictionary* resources_dict;
} t_resources;

extern t_log* logger;

/**
 * @NAME: resources_initialize
 * @DESC: Inicializa los recursos en base al archivo de configs.
 */
t_resources* resources_initialize(t_resources* resources, t_list* recursos, t_list* instancias_recursos);

/**
 * @NAME: resources_exist_resource
 * @DESC: Retorna si existe un recurso registrado bajo el nombre de `resource_name`.
 */
bool resources_exist_resource(t_resources* resources, char* resource_name);

/**
 * @NAME: resources_request
 * @DESC: Solicita un recurso disponible, bajo el nombre de `resource_name`.
 *        Retorna:
 *          = 0 - Si el recurso se encuentra disponible luego re solicitarlo.
 *          > 0 - Si el recurso NO se encuntra disponible en ese momento,
 *                  y se encola el PID que lo solicito en el recurso.
 *          < 0 - Si hubo un error al solicitar el recurso (x ej. RESOURCE_NOT_DEFINED).
 *        Si no surgieron errores el recurso se encuentra en `resource`.
 */
int resources_request(t_resources* resources, char* resource_name, int pid, t_resource** resource);

/**
 * @NAME: resources_release
 * @DESC: Libera un recurso disponible, bajo el nombre de `resource_name`.
 *        Retorna:
 *          = 0 - Si el recurso se encuentra disponible luego de liberar.
 *          > 0 - Si el recurso aún NO se encuntra disponible.
 *          < 0 - Si hubo un error al liberar el recurso (x ej. RESOURCE_NOT_DEFINED).
 *        Si no surgieron errores el recurso se encuentra en `resource`.
 */
int resources_release(t_resources* resources, char* resource_name, t_resource** resource);

/**
 * @NAME: resources_next_pid
 * @DESC: Devuelve el proximo proceso al que deberia asignarse dicho recurso, bajo el nombre de `resource_name`.
 *        Retorna:
 *          # > 0 - El proximo valor de PID al que se deberia asignar el recurso.
 *          INEXISTENT_NEXT_PID - Si no existe PID encolado en espera de uso del recurso.
 *          < 0 - Si hubo un error (x ej. RESOURCE_NOT_DEFINED).
 */
int resources_next_pid(t_resources* resources, char* resource_name);

/**
 * @NAME: add_resource_instances_available
 * @DESC: lleva la cuenta de las instancias disponibles. llamar despues de liberar la instancia del recurso
 */
void add_resource_instances_available(t_resource* resource);

/**
 * @NAME: sub_resource_instances_available
 * @DESC: lleva la cuenta de las instancias disponibles. llamar despues de asignar una instancia del recurso
 */
void sub_resource_instances_available(t_resource* resource);

/**
 * DESTROY
 */ 
void resources_destroy(t_resources* resources);


//For test purpose only
t_resource* _resource_create(char* resource_name, int resource_qty);

void resource_add_process_blocked(t_resource* resource, int pid);

#endif //SRC_CORE_RESOURCES_H_
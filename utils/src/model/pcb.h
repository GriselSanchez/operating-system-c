#ifndef SRC_MODEL_PCB_H_
#define SRC_MODEL_PCB_H_

#include "model/instrucciones.h"
#include "utils/serializacion.h"
#include "model/file.h"
#include <commons/collections/list.h>
#include <commons/log.h>
#include <semaphore.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>

#define NOT_STARTED 0

typedef struct registros {
    uint32_t registros_4bytes[4];
} t_registros;

typedef enum {
    NEW = 0,
    READY,
    EXEC,
    BLOCK,
    EXIT
} Status;

typedef enum {
    BLOCK_IO,
    BLOCK_EXCEPCION,
    BLOCK_SLEEP,
    BLOCK_EVENTS

} MotivoDeBLoqueo;

typedef struct {
    int pid;
    int program_counter;
    t_registros* cpu_registries;
    int priority;
    int size;
    char* path;
    Status current_status;
    int exit_code;
    t_list* asigned_resources;
    int block_time;
    char* resource_name;
    char* blocked_by_resource_name;
    int page_fault_nro;

    // FILE SYSTEM
    t_list* open_files; // t_file;
    t_file* pending_file;
    char* nombre_archivo;
    modo_apertura modo_apertura;
    int puntero_archivo;
    int dir_fisica;
    int tamanio_archivo;
    bool is_finished;
} t_pcb;

/**
 * @NAME: STATUS_NAME
 * @DESC: Imprime el nombre de un status.
 */
char* STATUS_NAME(Status status);

/**
 * @NAME: create_new_pcb
 * @DESC: Crea una estructura pcb (Process Control Block) en base a la estimacion inicial
 *         de lo que sera la rafaga de uso de CPU (util para HRRN) y una lista de instrucciones.
 */
t_pcb* create_new_pcb(char* path, int size, int priority);

/**
 * @NAME: finish_pcb
 * @DESC: Libera los recursos solicitados por la estructura pcb.
 */
void finish_pcb(t_pcb* pcb);

/**
 * @NAME: initialize_registries
 * @DESC: Inicializa los registros de la CPU del PCB.
 */
t_registros* initialize_registries();


#endif /* SRC_MODEL_PCB_H_ */

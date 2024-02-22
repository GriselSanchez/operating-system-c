#ifndef _SRC_CORE_MEMORY_H_
#define _SRC_CORE_MEMORY_H_

#include "utils/serializacion.h"
#include "utils/sockets.h"
#include "model/pcb.h"

typedef struct {
    char* memory_host;
    int memory_port;
} t_kernel_memory;

/**
 * @NAME: memory_allocate_new_process
 * @DESC: Solicita al modulo memoria que cree la memoria correspondinte para el proceso enviado por parametros.
 *
 */
int memory_allocate_new_process(t_pcb* pcb, t_kernel_memory* memory);

void memory_terminate_process(t_pcb* pcb, t_kernel_memory* memory);

int memory_load_page(t_pcb* pcb, int page_to_load, t_kernel_memory* memory);

t_kernel_memory* memory_initialize(char* memory_host, int memory_port);

#endif  /* _SRC_CORE_MEMORY_H_ */
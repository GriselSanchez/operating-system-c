#ifndef SRC_MEMORIA_H_
#define SRC_MEMORIA_H_
#define LOG_PATH "memory.log"
#define MODULE_NAME "Memory"

#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>
#include <pthread.h>
#include <unistd.h>

#include "configs/memory_configs.h"
#include "instruccion_proceso.h"
#include "paginacion/handler_paginacion.h"

#include "utils/sockets.h"
#include "utils/serializacion.h"
#include "utils/server.h"
#include "model/pcb.h"

typedef struct {
    t_server* server;
    int response_delay;
    //int kernel_m;
    //int filesystem_m;
    //int cpu_m;
}t_memory;

void* onConnection(void* arg);
t_memory* create_memory();
t_memory* start_memory(t_memory*);
void destroy_memory(t_memory*);
int memory_connect_filesystem(t_memory* memory);
void memory_destroy(t_memory*);

void* atender_kernel(int client_fd);
void* atender_filesystem(int client_fd);
void* atender_cpu(int client_fd);

void lanzar_hilo_para_atender_kernel(int client_fd);
void lanzar_hilo_para_atender_filesystem(int client_fd);
void lanzar_hilo_para_atender_cpu(int client_fd);

void allocate_new_process(t_paquete*, int);
void enviar_handshake_cpu(int cpu_fd);
void mov_in_instruction(t_paquete* paquete, int cpu_fd);
void mov_out_instruction(t_paquete* paquete, int cpu_fd);
void send_next_instruction(t_paquete* paquete, int client_fd);

#endif /* SRC_MEMORIA_H_ */

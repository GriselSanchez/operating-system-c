#ifndef SRC_CPU_H_
#define SRC_CPU_H_
#define MODULE_NAME "CPU"

#include "utils/server.h"
#include "model/contexto-ejecucion.h"
#include "utils/serializacion.h"
#include "registros.h"
//#include "configs/cpu_configs.h"
#include "cpu_configs.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include <commons/log.h>
#include <commons/config.h>

typedef struct {
    t_server* server_dispatch;
    t_server* server_interrupt;
    int memoria_fd;
}t_cpu;

typedef struct {
    int pid;
    op_code motivo;
}t_interrupcion;

t_config* init_config();
void iniciar_ciclo_instruccion(t_exec_context* pcb, int client_fd);
void finish_cpu(t_server* server, t_log* logger, t_config* config);

t_exec_context* recibir_contexto_de_ejecucion();
t_exec_context* actualizar_contexto_de_ejecucion(t_exec_context* pcb);
void enviar_contexto_de_ejecucion(t_exec_context* pcb, op_code cod_operacion, int client_fd);

void on_connection_dispatch(int);
void on_connection_interrupt(int);
t_cpu* create_cpu();
void cpu_start();
void start_cpu_dispatch();
void start_cpu_interrupt();
void destroy_cpu(t_cpu*);

void reiniciar_flag_interrupcion(t_interrupcion*);
void cpu_config_destroy(t_config_cpu* configs);
void cpu_destroy(t_cpu* cpu);

int cpu_connect_memory();

#endif /* SRC_CPU_H_ */
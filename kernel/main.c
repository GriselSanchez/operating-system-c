#include <commons/log.h>
#include <stdlib.h>

#include "core/process/process_handler.h"

t_log* logger;
t_cola_mutex *cola_bloqueados;
t_cola_mutex *cola_bloqueados_io;
t_cola_mutex *cola_bloqueados_excepcion;
t_cola_mutex *cola_bloqueados_sleep;
t_cola_mutex *cola_bloqueados_eventos;



int main(int argc, char* argv[]) {
    logger = log_create("kernel.log", "Kernel", 1, LOG_LEVEL_DEBUG);
    cola_bloqueados= queue_init_mutex();
    cola_bloqueados_io = queue_init_mutex();
    cola_bloqueados_excepcion = queue_init_mutex();
    cola_bloqueados_sleep = queue_init_mutex();
    cola_bloqueados_eventos = queue_init_mutex();
    return 0;
}
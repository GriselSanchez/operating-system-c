#include "registros.h"

extern t_log* logger;
extern pthread_mutex_t registros_mutex;

uint32_t registros_4bytes[CANT_REGISTROS] = { 0, 0, 0, 0 };

uint32_t* obtener_registro(char indice_char) {

    if (indice_char == 'A')
        return &registros_4bytes[0];
    if (indice_char == 'B')
        return &registros_4bytes[1];
    if (indice_char == 'C')
        return &registros_4bytes[2];
    if (indice_char == 'D')
        return &registros_4bytes[3];

    return NULL;
}

void actualizar_registros_ctx(t_exec_context* ctx) {
    pthread_mutex_lock(&registros_mutex);
    for (int i = 0; i < CANT_REGISTROS; i++) {
        ctx->cpu_registries->registros_4bytes[i] = registros_4bytes[i];
    }

    log_info(logger, "Registros del PID %i actualizados: AX: %i | BX: %i | CX: %i | DX: %i.", ctx->pid,
        (int)ctx->cpu_registries->registros_4bytes[0],
        (int)ctx->cpu_registries->registros_4bytes[1],
        (int)ctx->cpu_registries->registros_4bytes[2],
        (int)ctx->cpu_registries->registros_4bytes[3]);
    pthread_mutex_unlock(&registros_mutex);
}

void actualizar_registros_cpu(t_exec_context* ctx) {
    pthread_mutex_lock(&registros_mutex);
    for (int i = 0; i < CANT_REGISTROS; i++) {
        registros_4bytes[i] = ctx->cpu_registries->registros_4bytes[i];
    }

    log_info(logger, "Registros de CPU actualizados: AX: %i | BX: %i | CX: %i | DX: %i.",
        registros_4bytes[0],
        registros_4bytes[1],
        registros_4bytes[2],
        registros_4bytes[3]);
    pthread_mutex_unlock(&registros_mutex);
}

void imprimir_registros() {
    pthread_mutex_lock(&registros_mutex);
    log_info(logger, "AX: %i | BX: %i | CX: %i | DX: %i", registros_4bytes[0], registros_4bytes[1], registros_4bytes[2], registros_4bytes[3]);
    pthread_mutex_unlock(&registros_mutex);
}

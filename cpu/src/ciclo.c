#include "ciclo.h"

extern t_log* logger;
extern t_config_cpu* configs;
extern t_cpu* cpu;
extern int tamanio_pagina;
extern t_interrupcion* interrupcion;
extern pthread_mutex_t interrupcion_mutex;
pthread_mutex_t ciclo_mutex = PTHREAD_MUTEX_INITIALIZER;

int flag_exit;
op_code exit_code;

void iniciar_ciclo_instruccion(t_exec_context* ctx, int client_fd) {
    pthread_mutex_lock(&ciclo_mutex);
    flag_exit = 0;
    exit_code = -1;

    while (!flag_exit) {
        t_instruccion* instruccion = malloc(sizeof(t_instruccion));
        fetch(ctx, instruccion);
        int should_increment_pc = decode(instruccion, ctx);
        if (should_increment_pc != -1) ctx->program_counter += 1;
        check_interrupt(ctx);
        instruccion_destroy(instruccion);
    }

    enviar_contexto_de_ejecucion(ctx, exit_code, client_fd);
    pthread_mutex_unlock(&ciclo_mutex);

    reiniciar_flag_interrupcion(interrupcion);
}

void fetch(t_exec_context* ctx, t_instruccion* instruccion) {
    log_info(logger, "PID: %i - FETCH - Program Counter: %i.", ctx->pid, ctx->program_counter);

    obtener_siguiente_instruccion_de_memoria(ctx, instruccion);
}

int decode(t_instruccion* instruccion, t_exec_context* ctx) {
    int should_increment_pc = 1;
    char* primer_parametro = NULL;
    char* segundo_parametro = NULL;

    if (comparar_operacion(instruccion, INSTRUCTION_SET)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_SET(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_SUM)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_SUM(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_SUB)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_SUB(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_JNZ)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_JNZ(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_SLEEP)) {
        primer_parametro = list_get(instruccion->parametros, 0);

        execute_SLEEP(ctx, primer_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_WAIT)) {
        primer_parametro = list_get(instruccion->parametros, 0);

        execute_WAIT(ctx, primer_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_SIGNAL)) {
        primer_parametro = list_get(instruccion->parametros, 0);

        execute_SIGNAL(ctx, primer_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_MOV_IN)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        should_increment_pc = execute_MOV_IN(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_MOV_OUT)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        should_increment_pc = execute_MOV_OUT(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_OPEN)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_F_OPEN(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_CLOSE)) {
        primer_parametro = list_get(instruccion->parametros, 0);

        execute_F_CLOSE(ctx, primer_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_SEEK)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_F_SEEK(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_READ)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        should_increment_pc = execute_F_READ(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_WRITE)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        should_increment_pc = execute_F_WRITE(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_F_TRUNCATE)) {
        primer_parametro = list_get(instruccion->parametros, 0);
        segundo_parametro = list_get(instruccion->parametros, 1);

        execute_F_TRUNCATE(ctx, primer_parametro, segundo_parametro);
    }

    if (comparar_operacion(instruccion, INSTRUCTION_EXIT)) {
        execute_EXIT(ctx);
        should_increment_pc = -1;
    }

    return should_increment_pc;
}

void execute_SET(t_exec_context* ctx, char* registro, char* nuevo_valor) {
    log_ejecucion_2_param(ctx, INSTRUCTION_SET, registro, nuevo_valor);

    uint32_t* registro_cpu = obtener_registro(registro[0]);

    if (registro_cpu != NULL) {
        *registro_cpu = atoi(nuevo_valor);
    }
}

void execute_SUM(t_exec_context* ctx, char* registro_destino, char* registro_origen) {
    log_ejecucion_2_param(ctx, INSTRUCTION_SUM, registro_destino, registro_origen);

    uint32_t* registro_origen_cpu = obtener_registro(registro_origen[0]);
    uint32_t* registro_destino_cpu = obtener_registro(registro_destino[0]);

    if (registro_destino_cpu != NULL && registro_origen_cpu != NULL) {
        *registro_destino_cpu = *registro_origen_cpu + *registro_destino_cpu;
    }
}

void execute_SUB(t_exec_context* ctx, char* registro_destino, char* registro_origen) {
    log_ejecucion_2_param(ctx, INSTRUCTION_SUB, registro_destino, registro_origen);

    uint32_t* registro_origen_cpu = obtener_registro(registro_origen[0]);
    uint32_t* registro_destino_cpu = obtener_registro(registro_destino[0]);

    if (registro_destino_cpu != NULL && registro_origen_cpu != NULL) {
        *registro_destino_cpu = *registro_destino_cpu - *registro_origen_cpu;
    }
}

void execute_JNZ(t_exec_context* ctx, char* registro, char* nuevo_pc) {
    log_ejecucion_2_param(ctx, INSTRUCTION_JNZ, registro, nuevo_pc);

    uint32_t* registro_cpu = obtener_registro(registro[0]);

    if (*registro_cpu != 0) {
        ctx->program_counter = atoi(nuevo_pc) - 1;
    }
}

void execute_SLEEP(t_exec_context* ctx, char* segundos) {
    log_ejecucion_1_param(ctx, INSTRUCTION_SLEEP, segundos);

    ctx->block_time = atoi(segundos);
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = SLEEP;
    flag_exit = 1;
}

void execute_WAIT(t_exec_context* ctx, char* recurso) {
    log_ejecucion_1_param(ctx, INSTRUCTION_WAIT, recurso);

    char* resource = strdup(recurso);
    ctx->resource_name = resource;
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = WAIT;
    flag_exit = 1;
}

void execute_SIGNAL(t_exec_context* ctx, char* recurso) {
    log_ejecucion_1_param(ctx, INSTRUCTION_SIGNAL, recurso);

    char* resource = strdup(recurso);
    ctx->resource_name = resource;
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = SIGNAL;
    flag_exit = 1;
}

int execute_MOV_IN(t_exec_context* ctx, char* registro, char* dir_logica) {
    log_ejecucion_2_param(ctx, INSTRUCTION_MOV_IN, registro, dir_logica);

    int dir_logica_nro = atoi(dir_logica);
    int pagina = obtener_numero_pagina(dir_logica_nro);
    int marco = obtener_marco(pagina, ctx->pid, LEER);

    if (marco == -1) {
        log_info(logger, "Page Fault PID: %i - Página: %i.", ctx->pid, pagina);

        actualizar_contexto_de_ejecucion(ctx);
        ctx->page_fault_nro = pagina;

        exit_code = PAGE_FAULT;
        flag_exit = 1;
        return -1;
    } else {
        log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i.", ctx->pid, pagina, marco);

        int dir_fisica = traduccion_logica_a_fisica(dir_logica_nro, marco);
        uint32_t valor_memoria = leer_valor_memoria(dir_fisica, ctx->pid);

        if (valor_memoria == -1) {
            log_error(logger, "Error al leer memoria.");
            flag_exit = 1;
        } else {
            log_info(logger, "PID: %i - Acción: LEER - Dirección Física: %i - Valor: %u.", ctx->pid, dir_fisica, valor_memoria);

            uint32_t* registro_destino_cpu = obtener_registro(registro[0]);

            if (registro_destino_cpu != NULL) {
                *registro_destino_cpu = valor_memoria;
            }
        }
        return 1;
    }
}

int execute_MOV_OUT(t_exec_context* ctx, char* dir_logica, char* registro) {
    log_ejecucion_2_param(ctx, INSTRUCTION_MOV_OUT, dir_logica, registro);

    int dir_logica_nro = atoi(dir_logica);
    int pagina = obtener_numero_pagina(dir_logica_nro);
    int marco = obtener_marco(pagina, ctx->pid, ESCRIBIR);

    if (marco == -1) {
        log_info(logger, "Page Fault PID: %i - Página: %i.", ctx->pid, pagina);

        actualizar_contexto_de_ejecucion(ctx);
        ctx->page_fault_nro = pagina;

        exit_code = PAGE_FAULT;
        flag_exit = 1;
        return -1;
    } else {
        log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i.", ctx->pid, pagina, marco);

        int dir_fisica = traduccion_logica_a_fisica(dir_logica_nro, marco);
        uint32_t* registro_destino_cpu = obtener_registro(registro[0]);

        int result = escribir_valor_memoria(dir_fisica, *registro_destino_cpu, ctx->pid);

        if (result == -1) {
            log_error(logger, "Error al escribir en memoria.");
            flag_exit = 1;
        } else {
            log_info(logger, "PID: %i - Acción: ESCRIBIR - Dirección Física: %i - Valor: %u.", ctx->pid, dir_fisica, *registro_destino_cpu);
        }
        return 1;
    }
}

void execute_F_OPEN(t_exec_context* ctx, char* nombre_archivo, char* modo_apertura) {
    log_ejecucion_2_param(ctx, INSTRUCTION_F_OPEN, nombre_archivo, modo_apertura);

    char* name = strdup(nombre_archivo);
    ctx->nombre_archivo = name;
    ctx->modo_apertura = string_contains(modo_apertura, "W") ? W : R;
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = F_OPEN;
    flag_exit = 1;
}

void execute_F_CLOSE(t_exec_context* ctx, char* nombre_archivo) {
    log_ejecucion_1_param(ctx, INSTRUCTION_F_CLOSE, nombre_archivo);

    char* name = strdup(nombre_archivo);
    ctx->nombre_archivo = name;
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = F_CLOSE;
    flag_exit = 1;
}

void execute_F_SEEK(t_exec_context* ctx, char* nombre_archivo, char* posicion) {
    log_ejecucion_2_param(ctx, INSTRUCTION_F_SEEK, nombre_archivo, posicion);

    char* name = strdup(nombre_archivo);
    ctx->nombre_archivo = name;
    ctx->puntero_archivo = atoi(posicion);
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = F_SEEK;
    flag_exit = 1;
}

int execute_F_READ(t_exec_context* ctx, char* nombre_archivo, char* dir_logica) {
    log_ejecucion_2_param(ctx, INSTRUCTION_F_READ, nombre_archivo, dir_logica);

    int dir_logica_nro = atoi(dir_logica);
    int pagina = obtener_numero_pagina(dir_logica_nro);
    int marco = obtener_marco(pagina, ctx->pid, LEER);

    if (marco == -1) {
        log_info(logger, "Page Fault PID: %i - Página: %i.", ctx->pid, pagina);

        actualizar_contexto_de_ejecucion(ctx);
        ctx->page_fault_nro = pagina;

        exit_code = PAGE_FAULT;
        flag_exit = 1;
        return -1;
    } else {
        log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i.", ctx->pid, pagina, marco);

        int dir_fisica = traduccion_logica_a_fisica(dir_logica_nro, marco);

        char* name = strdup(nombre_archivo);
        ctx->nombre_archivo = name;
        ctx->dir_fisica = dir_fisica;
        actualizar_contexto_de_ejecucion(ctx);

        exit_code = F_READ;
        flag_exit = 1;
        return 1;
    }
}

int execute_F_WRITE(t_exec_context* ctx, char* nombre_archivo, char* dir_logica) {
    log_ejecucion_2_param(ctx, INSTRUCTION_F_WRITE, nombre_archivo, dir_logica);

    int dir_logica_nro = atoi(dir_logica);
    int pagina = obtener_numero_pagina(dir_logica_nro);
    int marco = obtener_marco(pagina, ctx->pid, ESCRIBIR);

    if (marco == -1) {
        log_info(logger, "Page Fault PID: %i - Página: %i.", ctx->pid, pagina);

        actualizar_contexto_de_ejecucion(ctx);
        ctx->page_fault_nro = pagina;

        exit_code = PAGE_FAULT;
        flag_exit = 1;
        return -1;
    } else {
        log_info(logger, "PID: %i - OBTENER MARCO - Página: %i - Marco: %i.", ctx->pid, pagina, marco);

        int dir_fisica = traduccion_logica_a_fisica(dir_logica_nro, marco);

        char* name = strdup(nombre_archivo);
        ctx->nombre_archivo = name;
        ctx->dir_fisica = dir_fisica;
        actualizar_contexto_de_ejecucion(ctx);

        exit_code = F_WRITE;
        flag_exit = 1;

        return 1;
    }
}

void execute_F_TRUNCATE(t_exec_context* ctx, char* nombre_archivo, char* tamanio) {
    log_ejecucion_2_param(ctx, INSTRUCTION_F_TRUNCATE, nombre_archivo, tamanio);

    char* name = strdup(nombre_archivo);
    ctx->nombre_archivo = name;
    ctx->tamanio_archivo = atoi(tamanio);
    actualizar_contexto_de_ejecucion(ctx);

    exit_code = F_TRUNCATE;
    flag_exit = 1;
}

void execute_EXIT(t_exec_context* ctx) {
    log_ejecucion_sin_param(ctx, INSTRUCTION_EXIT);

    actualizar_contexto_de_ejecucion(ctx);

    exit_code = PROC_EXIT_SUCCESS;
    flag_exit = 1;
}

int check_interrupt(t_exec_context* ctx) {
    pthread_mutex_lock(&interrupcion_mutex);
    int hay_interrupcion = interrupcion->pid == ctx->pid;

    if (hay_interrupcion) {
        if (flag_exit == 1 && exit_code != -1 && (interrupcion->motivo == PROC_EXIT_QUANTUM || interrupcion->motivo == PROC_EXIT_PRIORITY)) {
            log_info(logger, "PID: %i - Se devuelve el ctx por instruccion: %d", ctx->pid, exit_code);
            ctx->interrupt_motive = interrupcion->motivo;
        } else {
            log_info(logger, "PID: %i - Se devuelve el ctx por interrupcion: %d", ctx->pid, interrupcion->motivo);
            exit_code = interrupcion->motivo;
            flag_exit = 1;
            ctx->interrupt_motive = -1;
            actualizar_contexto_de_ejecucion(ctx);
        }
        pthread_mutex_unlock(&interrupcion_mutex);
        return 1;
    } else {
        ctx->interrupt_motive = -1;
        pthread_mutex_unlock(&interrupcion_mutex);
    }

    return 0;
}

void log_ejecucion_sin_param(t_exec_context* ctx, char* instruccion) {
    log_info(logger, "PID: %i - Ejecutando: %s.", ctx->pid, instruccion);
}

void log_ejecucion_1_param(t_exec_context* ctx, char* instruccion, char* param1) {
    log_info(logger, "PID: %i - Ejecutando: %s - %s.", ctx->pid, instruccion,
        param1);
}

void log_ejecucion_2_param(t_exec_context* ctx, char* instruccion, char* param1, char* param2) {
    log_info(logger, "PID: %i - Ejecutando: %s - %s - %s.", ctx->pid, instruccion,
        param1, param2);
}

int comparar_operacion(t_instruccion* instruccion, char* nombre) {
    return string_equals_ignore_case(instruccion->operacion, nombre);
}

t_instruccion* obtener_siguiente_instruccion_de_memoria(t_exec_context* ctx, t_instruccion* instruccion_deserializada) {
    t_paquete* paquete = crear_paquete(NEXT_INSTRUCTION);
    agregar_payload_a_paquete(paquete, &ctx->pid, sizeof(int));
    agregar_payload_a_paquete(paquete, &ctx->program_counter, sizeof(int));
    enviar_paquete_serializado_por_socket(cpu->memoria_fd, paquete);

    t_paquete* instruccion_paquete = deserealizar_paquete_desde_socket(cpu->memoria_fd);
    deserializar_instruccion(instruccion_paquete->buffer->payload, instruccion_deserializada);

    paquete_destroy(paquete);
    paquete_destroy(instruccion_paquete);
}
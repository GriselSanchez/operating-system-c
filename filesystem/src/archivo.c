#include "archivo.h"

extern t_log* logger;
extern t_filesystem_config* configs;
extern t_filesystem* filesystem;

int abrir_archivo(char* nombre_archivo) {
    log_info(logger, "Abrir Archivo: %s", nombre_archivo);

    char* directorio_fcb = obtener_directorio_fcb(nombre_archivo);
    FILE* archivo_fcb = fopen(directorio_fcb, "r");

    if (archivo_fcb != NULL) {
        t_config* config_fcb = obtener_config_fcb(nombre_archivo);
        int tamanio = config_get_int_value(config_fcb, "TAMANIO_ARCHIVO");

        log_info(logger, "Archivo FCB con tamanio %i.", tamanio);

        free(directorio_fcb);
        fclose(archivo_fcb);
        config_destroy(config_fcb);

        return tamanio;
    } else {
        log_error(logger, "Archivo FCB no encontrado.");
        free(directorio_fcb);

        return -1;
    }
}

void crear_archivo(char* nombre_archivo) {
    log_info(logger, "Crear Archivo: %s", nombre_archivo);

    crear_fcb_nuevo(nombre_archivo);
}

int truncar_archivo(char* nombre_archivo, int tamanio_nuevo) {
    log_info(logger, "Truncar Archivo: %s - Tamanio: %i bytes", nombre_archivo, tamanio_nuevo);

    t_config* config_fcb = obtener_config_fcb(nombre_archivo);
    t_fcb* fcb_actual = malloc(sizeof(t_fcb));

    fcb_actual->tamanio = config_get_int_value(config_fcb, "TAMANIO_ARCHIVO");
    fcb_actual->bloque_inicial = config_get_int_value(config_fcb, "BLOQUE_INICIAL");

    int cant_bloques_asignados = obtener_cant_bloques_asignados(fcb_actual);
    int cant_bloques_nuevos = ((tamanio_nuevo + (configs->tam_bloque - 1)) / configs->tam_bloque) - cant_bloques_asignados;

    if (cant_bloques_nuevos == 0) {
        log_info(logger, "No se modifica la cantidad de bloques.");
    } else if (cant_bloques_nuevos < 0) {
        int cant_bloques_a_liberar = cant_bloques_nuevos * -1;

        log_info(logger, "Intentando liberar %i bloques...", cant_bloques_a_liberar);

        liberar_bloques_asignados(fcb_actual, cant_bloques_a_liberar);
    } else if (cant_bloques_nuevos > 0) {
        int diferencia_tamanio = tamanio_nuevo - fcb_actual->tamanio;

        log_info(logger, "Intentando asignar %i bloques nuevos...", cant_bloques_nuevos);

        // TODO: es necesario limpiar los bloques nuevos?
        asignar_bloques_nuevos(fcb_actual, cant_bloques_nuevos);
    }

    fcb_actual->tamanio = tamanio_nuevo;

    config_set_value(config_fcb, "TAMANIO_ARCHIVO", string_itoa(fcb_actual->tamanio));
    config_set_value(config_fcb, "BLOQUE_INICIAL", string_itoa(fcb_actual->bloque_inicial));
    config_save(config_fcb);
    free(fcb_actual);
    config_destroy(config_fcb);

    return tamanio_nuevo;
}

void escribir_archivo(char* nombre_archivo, int puntero, int dir_fisica, int pid) {
    log_info(logger, "Escribir Archivo: %s - Puntero: %i - Memoria: %i", nombre_archivo, puntero, dir_fisica);

    t_paquete* paquete_memoria = crear_paquete(MEMORY_READ);
    agregar_payload_a_paquete(paquete_memoria, &pid, sizeof(int));
    agregar_payload_a_paquete(paquete_memoria, &dir_fisica, sizeof(int));
    enviar_paquete_serializado_por_socket(configs->memoria_fd, paquete_memoria);
    paquete_destroy(paquete_memoria);

    uint32_t* data_memoria = malloc(configs->tam_bloque);
    t_paquete* result = deserealizar_paquete_desde_socket(configs->memoria_fd);
    op_code code_result = result->codigo_operacion;
    if (code_result == MEMORY_OK) {
        log_info(logger, "Data recibida de memoria - Dir Fisica: %i", dir_fisica);
        memcpy(data_memoria, result->buffer->payload, configs->tam_bloque);
        for (size_t i = 0; i < configs->tam_bloque / sizeof(uint32_t); i++)
        {
            log_info(logger, "%i", data_memoria[i]);
        }
    }
    paquete_destroy(result);

    t_fcb* fcb = obtener_fcb(nombre_archivo);

    int nro_bloque_archivo = puntero / configs->tam_bloque;
    int nro_bloque = obtener_bloque_fat_de_archivo(fcb, nro_bloque_archivo);

    log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque FS: %i", nombre_archivo, nro_bloque_archivo, nro_bloque);
    escribir_bloque(nro_bloque + configs->cant_bloques_swap, data_memoria);

    free(fcb);
    free(data_memoria);
}

void leer_archivo(char* nombre_archivo, int puntero, int dir_fisica, int pid) {
    log_info(logger, "Leer Archivo: %s - Puntero: %i - Memoria: %i", nombre_archivo, puntero, dir_fisica);

    t_fcb* fcb = obtener_fcb(nombre_archivo);

    int nro_bloque_archivo = puntero / configs->tam_bloque;
    int nro_bloque = obtener_bloque_fat_de_archivo(fcb, nro_bloque_archivo);

    log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque FS: %i", nombre_archivo, nro_bloque_archivo, nro_bloque);
    uint32_t* data_bloque = leer_bloque(nro_bloque + configs->cant_bloques_swap);

    t_paquete* paquete_memoria = crear_paquete(MEMORY_WRITE);
    agregar_payload_a_paquete(paquete_memoria, &pid, sizeof(int));
    agregar_payload_a_paquete(paquete_memoria, &dir_fisica, sizeof(int));
    agregar_payload_a_paquete(paquete_memoria, data_bloque, configs->tam_bloque);
    enviar_paquete_serializado_por_socket(configs->memoria_fd, paquete_memoria);
    paquete_destroy(paquete_memoria);

    t_paquete* result = deserealizar_paquete_desde_socket(configs->memoria_fd);
    op_code code_result = result->codigo_operacion;
    if (code_result == MEMORY_OK) {
        log_info(logger, "Data enviada a memoria - Dir Fisica: %i", dir_fisica);
        for (size_t i = 0; i < configs->tam_bloque / sizeof(uint32_t); i++)
        {
            log_info(logger, "%i", data_bloque[i]);
        }
    }
    paquete_destroy(result);

    free(fcb);
    free(data_bloque);
}

void crear_fcb_nuevo(char* nombre_archivo) {
    char* directorio_fcb = obtener_directorio_fcb(nombre_archivo);
    char* contenido_fcb = string_from_format("NOMBRE_ARCHIVO=%s\nTAMANIO_ARCHIVO=0\nBLOQUE_INICIAL=0", nombre_archivo);

    FILE* archivo_fcb = fopen(directorio_fcb, "w");

    if (archivo_fcb == NULL) {
        log_error(logger, "Archivo FCB no encontrado.");
        exit(1);
    }

    txt_write_in_file(archivo_fcb, contenido_fcb);

    fclose(archivo_fcb);
    free(contenido_fcb);
    free(directorio_fcb);
}

char* obtener_directorio_fcb(char* nombre_archivo) {
    return string_from_format("%s/%s.fcb", configs->path_fcb, nombre_archivo);
}

t_fcb* obtener_fcb(char* nombre_archivo) {
    char* directorio_fcb = obtener_directorio_fcb(nombre_archivo);
    t_config* config_fcb = config_create(directorio_fcb);

    t_fcb* fcb_actual = malloc(sizeof(t_fcb));

    fcb_actual->tamanio = config_get_int_value(config_fcb, "TAMANIO_ARCHIVO");
    fcb_actual->bloque_inicial = config_get_int_value(config_fcb, "BLOQUE_INICIAL");

    log_info(logger, "NOMBRE_ARCHIVO=%s - TAMANIO_ARCHIVO=%i - BLOQUE_INICIAL=%i", nombre_archivo, fcb_actual->tamanio, fcb_actual->bloque_inicial);

    config_destroy(config_fcb);
    free(directorio_fcb);

    return fcb_actual;
}

t_config* obtener_config_fcb(char* nombre_archivo) {
    char* directorio_fcb = obtener_directorio_fcb(nombre_archivo);
    t_config* config = config_create(directorio_fcb);
    return config;
}

void limpiar_bloque_archivo(t_fcb* fcb, int nro_bloque_archivo) {
    int nro_bloque = obtener_bloque_fat_de_archivo(fcb, nro_bloque_archivo);

    log_info(logger, "Acceso Bloque - Archivo: %s - Bloque Archivo: %i - Bloque FS: %i", fcb->identificador, nro_bloque_archivo, nro_bloque);
    limpiar_bloque(nro_bloque + configs->cant_bloques_swap);
}

#include "filesystem.h"

t_log* logger;
t_filesystem_config* configs;
t_filesystem* filesystem;
t_bitarray* bitarray;

pthread_mutex_t mutex_archivo;
pthread_mutex_t mutex_swap;

int main(int argc, char* argv[]) {

    logger = log_create("filesystem.log", "Filesystem", 1, LOG_LEVEL_DEBUG);
    configs = cargar_config_de_archivo();
    // crear_directorio();

    inicializar_fat();
    bitarray = inicializar_swap();
    crear_archivo_de_bloques();

    pthread_mutex_init(&mutex_archivo, NULL);
    pthread_mutex_init(&mutex_swap, NULL);

    filesystem = filesystem_create(configs);
    filesystem_start(filesystem);

    filesystem_config_destroy(configs);
    filesystem_destroy(filesystem);
    log_destroy(logger);

    return 0;
}

void* on_connection(void* arg) {
    int cliente_fd = *(int*)arg;

    int response = handshake_server(cliente_fd, HANDSHAKE);
    if (response == -1) {
        close(cliente_fd);
        log_error(logger, "Error en handshake");
        return;
    }
    log_info(logger, "Handshake correcto %i", cliente_fd);

    t_paquete* paquete = deserealizar_paquete_desde_socket(cliente_fd);
    op_code code_op = paquete->codigo_operacion;

    switch (code_op) {
    case ABRIR_ARCHIVO: {
        int offset = 0;
        char* nombre = deserializar_string(paquete->buffer->payload, &offset);

        pthread_mutex_lock(&mutex_archivo);
        int respuesta = abrir_archivo(nombre);
        pthread_mutex_unlock(&mutex_archivo);

        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(nombre);
        break;
    }

    case CREAR_ARCHIVO: {
        int offset = 0;
        char* nombre = deserializar_string(paquete->buffer->payload, &offset);

        pthread_mutex_lock(&mutex_archivo);
        crear_archivo(nombre);
        pthread_mutex_unlock(&mutex_archivo);

        int respuesta = ARCHIVO_OK;
        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(nombre);
        break;
    }

    case TRUNCAR_ARCHIVO: {
        int offset = 0;
        int tamanio = 0;
        char* nombre = deserializar_string(paquete->buffer->payload, &offset);
        memcpy(&tamanio, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);

        pthread_mutex_lock(&mutex_archivo);
        int respuesta = truncar_archivo(nombre, tamanio);
        pthread_mutex_unlock(&mutex_archivo);

        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(nombre);
        break;
    }

    case LEER_ARCHIVO: {
        int offset = 0;
        int puntero = 0;
        int dir_fisica = 0;
        int pid = 0;

        char* nombre = deserializar_string(paquete->buffer->payload, &offset);
        memcpy(&puntero, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);
        memcpy(&dir_fisica, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);
        memcpy(&pid, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);

        pthread_mutex_lock(&mutex_archivo);
        leer_archivo(nombre, puntero, dir_fisica, pid);
        pthread_mutex_unlock(&mutex_archivo);

        int respuesta = ARCHIVO_OK;
        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(nombre);
        break;
    }

    case ESCRIBIR_ARCHIVO: {
        int offset = 0;
        int puntero = 0;
        int dir_fisica = 0;
        int pid = 0;

        char* nombre = deserializar_string(paquete->buffer->payload, &offset);
        memcpy(&puntero, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);
        memcpy(&dir_fisica, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);
        memcpy(&pid, (paquete->buffer->payload + offset), sizeof(int));
        offset += sizeof(int);

        pthread_mutex_lock(&mutex_archivo);
        escribir_archivo(nombre, puntero, dir_fisica, pid);
        pthread_mutex_unlock(&mutex_archivo);

        int respuesta = ARCHIVO_OK;
        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(nombre);
        break;
    }

    case SWAP_NEW_PROCESS: {
        int cant_bloques = 0;
        memcpy(&cant_bloques, (paquete->buffer->payload), sizeof(int));

        pthread_mutex_lock(&mutex_swap);
        t_list* bloques_nuevos = asignar_bloques_swap(cant_bloques);
        pthread_mutex_unlock(&mutex_swap);

        t_paquete* paquete_respuesta = crear_paquete(SWAP_OK);
        for (size_t i = 0; i < cant_bloques; i++)
        {
            int bloque_nuevo = list_get(bloques_nuevos, i);
            int pos_swap = bloque_nuevo * configs->tam_bloque;
            agregar_payload_a_paquete(paquete_respuesta, &pos_swap, sizeof(int));
        }
        enviar_paquete_serializado_por_socket(cliente_fd, paquete_respuesta);

        list_destroy(bloques_nuevos);
        paquete_destroy(paquete_respuesta);
        break;
    }

    case SWAP_END_PROCESS: {
        int offset = 0;
        int cant_bloques = 0;
        t_list* bloques = list_create();

        memcpy(&cant_bloques, (paquete->buffer->payload), sizeof(int));
        offset += sizeof(int);

        for (size_t i = 0; i < cant_bloques; i++)
        {
            int pos_swap = 0;
            memcpy(&pos_swap, (paquete->buffer->payload + offset), sizeof(int));
            list_add(bloques, pos_swap / configs->tam_bloque);
            offset += sizeof(int);
        }

        pthread_mutex_lock(&mutex_swap);
        liberar_bloques_swap(bloques);
        pthread_mutex_unlock(&mutex_swap);

        int respuesta = SWAP_OK;
        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        list_destroy(bloques);
        break;
    }

    case SWAP_IN: {
        int pos_swap = 0;
        memcpy(&pos_swap, (paquete->buffer->payload), sizeof(int));

        int nro_bloque = pos_swap / configs->tam_bloque;
        uint32_t* data = swap_in(nro_bloque);

        t_paquete* paquete = crear_paquete(SWAP_OK);
        agregar_payload_a_paquete(paquete, data, configs->tam_bloque);
        enviar_paquete_serializado_por_socket(cliente_fd, paquete);

        paquete_destroy(paquete);
        free(data);
        break;
    }

    case SWAP_OUT: {
        int offset = 0;
        int pos_swap = 0;
        uint32_t* data = malloc(configs->tam_bloque);

        memcpy(&pos_swap, (paquete->buffer->payload), sizeof(int));
        offset += sizeof(int);
        memcpy(data, (paquete->buffer->payload + offset), configs->tam_bloque);
        offset += configs->tam_bloque;

        int nro_bloque = pos_swap / configs->tam_bloque;
        swap_out(nro_bloque, data);

        int respuesta = SWAP_OK;
        enviar_payload(cliente_fd, &respuesta, sizeof(int));

        free(data);
        break;
    }

    default:
        log_error(logger, "OP CODE INVALIDO: %i.", code_op);
        break;
    }

    paquete_destroy(paquete);
    liberar_conexion(cliente_fd);
}

t_filesystem* filesystem_create(t_filesystem_config* configs) {
    t_filesystem* filesystem = malloc(sizeof(t_filesystem));
    if (filesystem == NULL) {
        log_error(logger, "Error on memory allocation.");
        return NULL;
    }
    log_info(logger, "Puerto: %i", configs->puerto_escucha);
    filesystem->server = multiclient_server_create(configs->puerto_escucha, &on_connection);
    return filesystem;
}

void filesystem_start(t_filesystem* filesystem) {
    log_info(logger, "Levantando servidor.");

    if (filesystem_connect_memoria(filesystem) == -1) {
        log_error(logger, "Cant create connection to memory");
    }

    server_start(filesystem->server);
}

int filesystem_connect_memoria(t_filesystem* filesystem) {
    log_debug(logger, "Creando conexion a memoria: %s : %i", configs->ip_memoria, configs->puerto_memoria);

    configs->memoria_fd = create_conection(configs->ip_memoria, configs->puerto_memoria);
    int result = handshake_client(configs->memoria_fd, HANDSHAKE_FILE_SYSTEM);
    if (result == 0) {
        log_debug(logger, "Handshake con memoria correcto %i.", configs->memoria_fd);

        return result;
    }

    return -1;
}

void crear_directorio() {
    if (mkdir(configs->path_fcb, 0755) == 0) {
        log_info(logger, "Directorio %s creado exitosamente.", configs->path_fcb);
    } else {
        if (errno == EEXIST) {
            log_info(logger, "El directorio: %s ya existe.", configs->path_fcb);
        } else {
            log_error(logger, "Error creando directorio: %s.", configs->path_fcb);
        }
    }
}

void filesystem_destroy(t_filesystem* filesystem) {
    server_stop(filesystem->server);
    server_destroy(filesystem->server);
    free(filesystem);
}

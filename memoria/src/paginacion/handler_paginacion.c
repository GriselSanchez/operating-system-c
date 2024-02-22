#include "handler_paginacion.h"

// TODOs: ver en que momento se carga una pagina en memoria, quien lo pide, cpu?
// cuando se marca una pagina como modificada?
extern t_memory* memory;
extern t_list* procesos_en_memoria;
extern pthread_mutex_t operaciones_bloqueantes_mutex;
// extern pthread_mutex_t pagina_logger;

t_handler_paginacion* create_handler_paginacion(int tamanio_memoria, int tamanio_pagina, char* algoritmo_reemplazo) {

    t_handler_paginacion* handler = malloc(sizeof(t_handler_paginacion));
    handler->pid_tablas = diccionario_create();
    handler->espacio_usuario = create_espacio_usuario(tamanio_memoria, tamanio_pagina);
    handler->paginas_pendientes_modificacion = list_init_mutex();
    handler->algoritmo_reemplazo = string_equals_ignore_case(algoritmo_reemplazo, "FIFO") ? FIFO : LRU;
    handler->tamanio_pagina = tamanio_pagina;
    return handler;
}

t_tabla_paginas* generar_tabla_paginas(t_handler_paginacion* handler, int pid, int tamanio_proceso) {
    int cantidad_paginas = tamanio_proceso / handler->tamanio_pagina;
    int* posiciones_swap = obtener_posiciones_swap(cantidad_paginas, pid);

    t_tabla_paginas* tabla = create_tabla_paginas(pid, cantidad_paginas, posiciones_swap);
    char* pid_key = string_itoa(pid);
    diccionario_put(handler->pid_tablas, pid_key, tabla);

    log_info(logger, "Tabla de Paginas NEW - PID: %i - Tamaño: %i", pid, cantidad_paginas);
    // imprimir_tablas_de_paginas(handler);

    free(posiciones_swap);

    return tabla;
}

int obtener_marco(t_handler_paginacion* handler, int pid, int nro_pagina, tipo_operacion_t tipo_operacion) {
    char* pid_key = string_itoa(pid);
    t_tabla_paginas* tabla = diccionario_get(handler->pid_tablas, pid_key);
    if (tabla != NULL) {
        if (pagina_esta_en_memoria(tabla, nro_pagina)) {
            log_debug(logger, "PID: %i - La pagina %i se encuentra en memoria", pid, nro_pagina);

            t_pagina* pagina = leer_pagina(tabla, nro_pagina);
            log_info(logger, "Obtener marco - PID: %i - Pagina %i - Marco %i", pid, nro_pagina, pagina->nro_frame);

            if (tipo_operacion == ESCRIBIR) {
                list_add_mutex(handler->paginas_pendientes_modificacion, pagina);
            }

            free(pid_key);
            return pagina->nro_frame;
        } else {
            log_debug(logger, "PID: %i - La pagina %i no se encuentra en memoria", pid, nro_pagina);

            free(pid_key);
            return -1;
        }
    }

    free(pid_key);
    log_error(logger, "Obtener marco: no se encontro tabla de paginas para proceso %i", pid);
    return -1;
}

int cargar_en_memoria(t_handler_paginacion* handler, int pid, int nro_pagina) {
    char* pid_key = string_itoa(pid);
    t_tabla_paginas* tabla_pagina_nueva = diccionario_get(handler->pid_tablas, pid_key);
    if (tabla_pagina_nueva == NULL) {
        log_error(logger, "Cargar en memoria: Hubo un error al encontrar la tabla de paginas para  proceso %i", pid);
        free(pid_key);

        return -1;
    }

    pthread_mutex_lock(&operaciones_bloqueantes_mutex);
    int numero_frame_libre = obtener_frame_libre(handler->espacio_usuario);
    if (numero_frame_libre == -1) {
        t_pagina* pagina_a_reemplazar = remover_pagina_memoria(handler);
        if (pagina_a_reemplazar == NULL) {
            log_error(logger, "Cargar en memoria: hubo un error al encontrar la pagina a reemplazar");
            free(pid_key);

            return -1;
        }
        numero_frame_libre = pagina_a_reemplazar->nro_frame;
        log_info(logger, "REEMPLAZO - Marco: %i - Page Out: %i-%i- Page In: %i-%i", numero_frame_libre, pagina_a_reemplazar->pid, pagina_a_reemplazar->nro_pagina, pid, nro_pagina);
    }
    marcar_frame_ocupado(handler->espacio_usuario, numero_frame_libre);
    swap_in(handler, tabla_pagina_nueva, nro_pagina, numero_frame_libre);
    pthread_mutex_unlock(&operaciones_bloqueantes_mutex);

    free(pid_key);
    // imprimir_tablas_de_paginas(handler);
    return numero_frame_libre;
}

uint32_t leer_valor(t_handler_paginacion* handler, int dir_fisica, int pid) {
    uint32_t valor = espacio_leer_valor(handler->espacio_usuario, dir_fisica, pid);
    return valor;
}

int escribir_valor(t_handler_paginacion* handler, int dir_fisica, uint32_t valor, int pid) {
    espacio_escribir_valor(handler->espacio_usuario, dir_fisica, valor, pid);
    int marco = espacio_obtener_frame(handler->espacio_usuario, dir_fisica);
    t_pagina* pagina = encontrar_pagina_a_modificar(handler->paginas_pendientes_modificacion, marco);
    //si encuentra la pagina en pendientes, marcala modificada (tests)
    if (pagina != NULL) {
        log_info(logger, "PID: %i - Pagina %i - Marco %i", pagina->pid, pagina->nro_pagina, pagina->nro_frame);
        pagina->bit_modificado = 1;
        log_debug(logger, "Escribir valor: PID: %i - Pagina %i - Marco %i - Modificado %i", pagina->pid, pagina->nro_pagina, pagina->nro_frame, pagina->bit_modificado);
    }
    return 1;
}


void remover_proceso(t_handler_paginacion* handler, int pid)
{
    liberar_posiciones_swap(handler, pid);
    // TODO: eliminar elemento de la lista de procesos_en_memoria

    //obtiene marcos ocupados de la tabla de paginas y marcar marcos como libres en el espacio de usuario
    char* pid_key = string_itoa(pid);
    t_tabla_paginas* tabla = diccionario_get(handler->pid_tablas, pid_key);

    log_info(logger, "Tabla de Paginas DESTROY - PID: %i - Tamaño: %i", pid, tabla->cant_paginas);

    for (int i = 0; i < tabla->cant_paginas; i++) {
        if (pagina_esta_en_memoria(tabla, i)) {
            liberar_frame(handler->espacio_usuario, pagina_obtener_frame(tabla, i));
        }
    }
    diccionario_remove_and_destroy(handler->pid_tablas, pid_key, &destroy_tabla_paginas);
}

void handler_paginacion_destroy(t_handler_paginacion* handler)
{
    list_destroy_mutex(handler->paginas_pendientes_modificacion);
    diccionario_destroy_and_destroy_elements(handler->pid_tablas, &destroy_tabla_paginas);
    espacio_destroy(handler->espacio_usuario);
    free(handler);
}

uint32_t* leer_valor_pagina(t_handler_paginacion* handler, int dir_fisica, int cant_elementos, int pid)
{
    uint32_t* valor = malloc(cant_elementos * sizeof(uint32_t));
    for (int i = 0; i < cant_elementos; i++) {
        int direccion = dir_fisica + (sizeof(uint32_t) * i);
        valor[i] = espacio_leer_valor(handler->espacio_usuario, direccion, pid);
    }
    return valor;
}

void escribir_valor_pagina(t_handler_paginacion* handler, int dir_fisica, int cant_elementos, uint32_t* valores, int pid)
{
    for (int i = 0; i < cant_elementos; i++) {
        int direccion = dir_fisica + (sizeof(uint32_t) * i);
        espacio_escribir_valor(handler->espacio_usuario, direccion, valores[i], pid);
    }
}

/*Para marcar bit de modificado en una pagina
*no destruir pagina, sigue siendo usada en la tabla de paginas*/
t_pagina* encontrar_pagina_a_modificar(t_lista_mutex* paginas_pendientes_modificacion, int nro_frame) {
    bool find_condition(void* pagina) {
        return (_Bool)(((t_pagina*)pagina)->nro_frame == nro_frame);
    }
    return (t_pagina*)list_remove_by_condition_mutex(paginas_pendientes_modificacion, &find_condition);
}

/** ALGORITMOS DE REEMPLAZO **/
//corre algoritmo de reemplazo, envia pagina que sera reemplazada a filesystem si bit M =1
t_pagina* remover_pagina_memoria(t_handler_paginacion* handler) {
    t_pagina* pagina_a_reemplazar = NULL;

    if (handler->algoritmo_reemplazo == FIFO) {
        pagina_a_reemplazar = encontrar_pagina_FIFO(handler);
    }
    if (handler->algoritmo_reemplazo == LRU) {
        pagina_a_reemplazar = encontrar_pagina_LRU(handler);
    }

    if (pagina_a_reemplazar != NULL) {
        if (pagina_a_reemplazar->bit_modificado == 1) {
            char* pid_key = string_itoa(pagina_a_reemplazar->pid);
            t_tabla_paginas* tabla = diccionario_get(handler->pid_tablas, pid_key);
            tabla_remover_pagina_de_memoria(tabla, pagina_a_reemplazar->nro_pagina);
            swap_out(handler, tabla, pagina_a_reemplazar->nro_pagina);
            // imprimir_tablas_de_paginas(handler);
        }
    }
    return pagina_a_reemplazar;
}

t_pagina* encontrar_pagina_FIFO(t_handler_paginacion* handler) {
    t_list* tablas = diccionario_elements(handler->pid_tablas);

    t_pagina* pagina_mas_antigua = NULL;

    // funcion que se va a aplicar a cada tabla de paginas
    // si pagina_mas_antigua es null, obtiene la primer pagina que encuentre que esta en memoria
    // si encuentra una pagina que fue creada antes que la pagina_mas_antigua, la reemplaza
    void encontrar_pagina_mas_antigua(void* tabla_paginas) {
        t_tabla_paginas* tabla = (t_tabla_paginas*)tabla_paginas;
        for (int i = 0; i < tabla->cant_paginas; i++) {
            if (pagina_esta_en_memoria(tabla, i)) {
                if (pagina_mas_antigua == NULL) {
                    pagina_mas_antigua = pagina_obtener_pagina(tabla, i);
                } else {
                    t_temporal* fecha = pagina_obtener_creacion(tabla, i);
                    t_temporal* fecha_mas_antigua = pagina_mas_antigua->creacion;
                    int64_t diferencia = temporal_diff(fecha_mas_antigua, fecha);
                    if (diferencia < 0) {
                        pagina_mas_antigua = pagina_obtener_pagina(tabla, i);
                    }
                }
            }
        }
    }

    list_iterate(tablas, &encontrar_pagina_mas_antigua);

    list_destroy(tablas);
    return pagina_mas_antigua;
}

// obtiene la pagina que fue usada hace mas tiempo, que se encuentra cargada en memoria
t_pagina* encontrar_pagina_LRU(t_handler_paginacion* handler) {
    t_list* tablas = diccionario_elements(handler->pid_tablas);

    //pagina usada menos recientemente least recently used
    t_pagina* pagina_lru = NULL;

    // funcion que se va a aplicar a cada tabla de paginas
    // si pagina_lru es null, obtiene la primer pagina que encuentre que esta en memoria
    // si encuentra una pagina que fue usada por ultima vez antes que la pagina_lru, la reemplaza
    void encontrar_pagina_lru(void* tabla_paginas) {
        t_tabla_paginas* tabla = (t_tabla_paginas*)tabla_paginas;
        for (int i = 0; i < tabla->cant_paginas; i++) {
            if (pagina_esta_en_memoria(tabla, i)) {
                if (pagina_lru == NULL) {
                    pagina_lru = pagina_obtener_pagina(tabla, i);
                } else {
                    t_temporal* fecha = pagina_obtener_ultima_lectura(tabla, i);
                    t_temporal* fecha_mas_antigua = pagina_lru->ultima_lectura;
                    int64_t diferencia = temporal_diff(fecha_mas_antigua, fecha);
                    if (diferencia < 0) {
                        pagina_lru = pagina_obtener_pagina(tabla, i);
                    }
                }
            }
        }
    }

    list_iterate(tablas, &encontrar_pagina_lru);

    list_destroy(tablas);
    return pagina_lru;
}


/** SWAP **/

int* obtener_posiciones_swap(int cantidad_paginas, int pid) {
    int* posiciones_swap = malloc(cantidad_paginas * sizeof(int));

    int socket_fs = memory_connect_filesystem(memory);
    t_paquete* paquete = crear_paquete(SWAP_NEW_PROCESS);
    agregar_payload_a_paquete(paquete, &cantidad_paginas, sizeof(int));
    enviar_paquete_serializado_por_socket(socket_fs, paquete);
    paquete_destroy(paquete);

    t_paquete* paquete_respuesta = deserealizar_paquete_desde_socket(socket_fs);
    op_code code_result = paquete_respuesta->codigo_operacion;
    if (code_result == SWAP_OK) {
        int offset = 0;
        log_info(logger, "Posiciones SWAP para PID %i:", pid);

        for (size_t i = 0; i < cantidad_paginas; i++)
        {
            memcpy(&posiciones_swap[i], paquete_respuesta->buffer->payload + offset, sizeof(int));
            offset += sizeof(int);
            log_info(logger, "%i", posiciones_swap[i]);
        }
        paquete_destroy(paquete_respuesta);
    }

    liberar_conexion(socket_fs);
    return posiciones_swap;
}

void liberar_posiciones_swap(t_handler_paginacion* handler, int pid) {
    char* pid_key = string_itoa(pid);
    t_tabla_paginas* tabla = diccionario_get(handler->pid_tablas, pid_key);

    int socket_fs = memory_connect_filesystem(memory);
    t_paquete* paquete = crear_paquete(SWAP_END_PROCESS);
    agregar_payload_a_paquete(paquete, &tabla->cant_paginas, sizeof(int));
    for (size_t i = 0; i < tabla->cant_paginas; i++)
    {
        int pos_swap = tabla->paginas[i].pos_swap;
        agregar_payload_a_paquete(paquete, &pos_swap, sizeof(int));
    }
    enviar_paquete_serializado_por_socket(socket_fs, paquete);
    paquete_destroy(paquete);

    op_code respuesta = 0;
    recibir_payload(socket_fs, &respuesta, sizeof(op_code));
    if (respuesta == SWAP_OK) {
        log_info(logger, "SWAP liberada para proceso PID %i", pid);
    }

    liberar_conexion(socket_fs);
    free(pid_key);
}


void swap_out(t_handler_paginacion* handler, t_tabla_paginas* tabla, int nro_pagina) {
    int pid = tabla->pid;
    int pos_swap = pagina_obtener_pos_swap(tabla, nro_pagina);
    int frame = tabla->paginas[nro_pagina].nro_frame;
    int dir_fisica = frame * configs->tam_pagina;
    uint32_t* data = leer_valor_pagina(handler, dir_fisica, configs->tam_pagina / sizeof(uint32_t), pid);

    int socket_fs = memory_connect_filesystem(memory);
    t_paquete* paquete = crear_paquete(SWAP_OUT);
    agregar_payload_a_paquete(paquete, &pos_swap, sizeof(int));
    agregar_payload_a_paquete(paquete, data, configs->tam_pagina);
    enviar_paquete_serializado_por_socket(socket_fs, paquete);
    paquete_destroy(paquete);

    op_code result = 0;
    recibir_payload(socket_fs, &result, sizeof(op_code));
    if (result == SWAP_OK) {
        log_info(logger, "SWAP OUT - PID: %i - Marco: %i - Page Out: %i-%i", pid, frame, pid, nro_pagina);
    } else {
        log_error(logger, "Error en SWAP OUT");
    }

    liberar_conexion(socket_fs);
    free(data);
}

void swap_in(t_handler_paginacion* handler, t_tabla_paginas* tabla, int nro_pagina, int nuevo_frame) {
    int pid = tabla->pid;
    int pos_swap = pagina_obtener_pos_swap(tabla, nro_pagina);
    int dir_fisica = nuevo_frame * configs->tam_pagina;

    int socket_fs = memory_connect_filesystem(memory);
    t_paquete* paquete = crear_paquete(SWAP_IN);
    agregar_payload_a_paquete(paquete, &pos_swap, sizeof(int));
    enviar_paquete_serializado_por_socket(socket_fs, paquete);
    paquete_destroy(paquete);

    t_paquete* paquete_respuesta = deserealizar_paquete_desde_socket(socket_fs);
    op_code code_result = paquete_respuesta->codigo_operacion;
    if (code_result == SWAP_OK) {
        uint32_t* data = malloc(configs->tam_pagina);
        memcpy(data, paquete_respuesta->buffer->payload, configs->tam_pagina);

        log_info(logger, "SWAP IN - PID: %i - Marco: %i - Page In: %i-%i", pid, nuevo_frame, pid, nro_pagina);
        log_info(logger, "Cargar en memoria - PID: %i - Pagina %i - Marco %i", pid, nro_pagina, nuevo_frame);
        cargar_pagina_en_memoria(tabla, nro_pagina, nuevo_frame);

        escribir_valor_pagina(handler, dir_fisica, configs->tam_pagina / sizeof(uint32_t), data, pid);

        free(data);
    } else {
        log_error(logger, "Error en SWAP IN");
    }

    liberar_conexion(socket_fs);
    paquete_destroy(paquete_respuesta);
}


/** LOGS **/
void imprimir_tablas_de_paginas(t_handler_paginacion* handler) {
    t_list* tablas = diccionario_elements(handler->pid_tablas);

    void log_tabla_paginas(void* tabla_paginas) {
        t_tabla_paginas* tabla = (t_tabla_paginas*)tabla_paginas;
        log_info(logger, "Tabla de paginas PID: %i", tabla->pid);

        for (size_t i = 0; i < tabla->cant_paginas; i++)
        {
            t_pagina* pagina = pagina_obtener_pagina(tabla, i);
            log_info(logger, "NRO-%i | FRAME-%i | P-%i | M-%i | SWAP-%i", pagina->nro_pagina, pagina->nro_frame, pagina->bit_presencia, pagina->bit_modificado, pagina->pos_swap);
        }
    }

    list_iterate(tablas, &log_tabla_paginas);
    list_destroy(tablas);
}
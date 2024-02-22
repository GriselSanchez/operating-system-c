#include "instruccion_proceso.h"

extern t_log* logger;
extern t_lista_mutex* procesos_en_memoria;

t_instruccion* obtener_prox_instruccion(int pid, int program_counter) {
    t_proceso_memoria* proceso = obtener_proceso(pid);
    return list_get(proceso->instrucciones, program_counter);
}

t_proceso_memoria* obtener_proceso(int pid) {
    bool _coincide_pid(void* elem) {
        return (_Bool)(((t_proceso_memoria*)elem)->pid == pid);
    }
    t_proceso_memoria* process;
    process = list_find_mutex(procesos_en_memoria, &_coincide_pid);
    return process;
}

t_list* obtener_instrucciones_proceso(char* process_path, char* instructions_path) {
    char* path = string_new();
    string_append(&path, instructions_path);
    string_append(&path, "/");
    string_append(&path, process_path);

    t_list* instrucciones_proceso = obtener_instrucciones_de_archivo(path);
    free(path);

    return instrucciones_proceso;
}

void enviar_instruccion_serializada(int pid, int program_counter, int socket_cpu) {
    t_proceso_memoria* proceso = obtener_proceso(pid);
    t_instruccion* siguiente_instruccion = list_get(proceso->instrucciones, program_counter);

    t_paquete* instruccion_paquete = crear_paquete(INSTRUCCIONES);
    serializar_instruccion(instruccion_paquete, siguiente_instruccion);

    enviar_paquete_serializado_por_socket(socket_cpu, instruccion_paquete);
    paquete_destroy(instruccion_paquete);
}

void enviar_new_process_serialized(t_list* instructions, int socket_kernel) {
    t_paquete* instruccion_paquete = crear_paquete(MEMORY_NEW_PROCESS);
    serializar_instrucciones(instruccion_paquete, instructions);
    enviar_paquete_serializado_por_socket(socket_kernel, instruccion_paquete);
    paquete_destroy(instruccion_paquete);
}

t_list* obtener_instrucciones_de_archivo(char* path) {
    t_list* lista_instrucciones = list_create();
    FILE* archivo;
    char* linea_actual;

    archivo = fopen(path, "r");

    if (archivo == NULL) {
        log_error(logger, "Instrucciones no encontradas.");
        exit(1);
    }


    while (!feof(archivo)) {
        linea_actual = leer_linea(archivo);

        if (linea_actual == NULL) {
            break;
        }

        char** valores_linea = string_split(linea_actual, " ");

        char* operacion = strdup(valores_linea[0]);
        t_list* parametros = list_create();

        for (int i = 1; valores_linea[i] != NULL; i++) {
            char* param = strdup(valores_linea[i]);
            list_add(parametros, param);
        }

        t_instruccion* instruccion = malloc(sizeof(t_instruccion));
        crear_instruccion(operacion, parametros, instruccion);
        list_add(lista_instrucciones, instruccion);
        free(linea_actual);
        string_array_destroy(valores_linea);
    }

    txt_close_file(archivo);

    return lista_instrucciones;
}

char* leer_linea(FILE* archivo) {
    size_t max_buffer = BUFFER_SIZE;
    size_t bytes_totales = 0;
    size_t bytes_leidos = 0;

    char* linea = malloc(max_buffer);

    if (linea == NULL) {
        return NULL;
    }

    int char_actual;

    while ((char_actual = fgetc(archivo)) != EOF) {
        // Duplico el tamanio del buffer si no alcanza
        if (bytes_leidos + 1 >= max_buffer) {
            max_buffer *= 2;
            linea = realloc(linea, max_buffer);
            if (linea == NULL) {
                return NULL;
            }
        }

        if (char_actual != '\n') linea[bytes_leidos++] = (char)char_actual;
        else break;
    }

    if (bytes_leidos == 0) {
        free(linea);
        return NULL;
    }

    linea[bytes_leidos] = '\0';

    return linea;
}
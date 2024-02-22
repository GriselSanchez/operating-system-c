#include "instrucciones.h"

#define STRING_LEN(len) sizeof(char) * (len)

extern t_log* logger;

void crear_instruccion(char* operacion, t_list* parametros, t_instruccion* instruccion) {
    instruccion->operacion = operacion;
    instruccion->parametros = parametros;
    instruccion->largo_operacion = strlen(operacion) + 1;
    instruccion->cant_parametros = list_size(parametros);
}

void imprimir_instrucciones(t_list* instructions) {
    t_instruccion* instruccion;
    char* param;

    for (size_t i = 0; i < instructions->elements_count; i++) {
        instruccion = list_get(instructions, i);
        log_debug(logger, "%s", (char*)instruccion->operacion);

        for (size_t j = 0; j < instruccion->parametros->elements_count; j++) {
            param = list_get(instruccion->parametros, j);
            log_debug(logger, "%s", (char*)param);
        }
    }
}

// FIXES: warning: passing argument 2 of ‘list_destroy_and_destroy_elements’ from incompatible pointer type
// [-Wincompatible-pointer-types]
// note: expected ‘void (*)(void *)’ but argument is of type ‘void (*)(t_instruccion *)’
void instruccion_destroy(void* instruccion) {
    t_instruccion* temp_instruccion = (t_instruccion*)instruccion;

    list_destroy_and_destroy_elements(temp_instruccion->parametros, &free);
    free(temp_instruccion->operacion);
    free(temp_instruccion);
}

// CANT INSTRUCCIONES | LARGO OPERACION | OPERACION | CANT PARAMETROS | LARGO PARAM | PARAM
void serializar_instrucciones(t_paquete* paquete, t_list* instrucciones) {
    int cant_instrucciones = list_size(instrucciones);

    agregar_payload_a_paquete(paquete, &cant_instrucciones, sizeof(cant_instrucciones));
    log_trace(logger, "CANT INSTRUCCIONES: bytes: %li | valor: %i", sizeof(cant_instrucciones), cant_instrucciones);

    for (int i = 0; i < cant_instrucciones; i++) {
        t_instruccion* instruccion = list_get(instrucciones, i);

        agregar_payload_a_paquete(paquete, &(instruccion->largo_operacion), sizeof((instruccion->largo_operacion)));
        log_trace(logger, "LARGO OPERACION: bytes: %li | valor: %i", sizeof((instruccion->largo_operacion)),
            instruccion->largo_operacion);

        agregar_payload_a_paquete(paquete, instruccion->operacion, instruccion->largo_operacion);
        log_trace(logger, "OPERACION: bytes: %i | valor: %s", instruccion->largo_operacion, instruccion->operacion);

        agregar_payload_a_paquete(paquete, &(instruccion->cant_parametros), sizeof((instruccion->largo_operacion)));
        log_trace(logger, "CANT PARAMETROS: bytes: %li | valor: %i", sizeof((instruccion->largo_operacion)),
            instruccion->cant_parametros);

        for (int j = 0; j < instruccion->cant_parametros; j++) {
            char* param = list_get(instruccion->parametros, j);
            int largo_param = strlen(param) + 1;

            agregar_payload_a_paquete(paquete, &largo_param, sizeof(largo_param));
            log_trace(logger, "LARGO PARAM: bytes: %li | valor: %i", sizeof(largo_param), largo_param);

            agregar_payload_a_paquete(paquete, param, largo_param);
            log_trace(logger, "PARAM: bytes: %i | valor: %s", largo_param, param);
        }
    }
}


//--- FROM MAIN MERGE:

// LARGO OPERACION | OPERACION | CANT PARAMETROS | LARGO PARAM | PARAM
void serializar_instruccion(t_paquete* paquete, t_instruccion* instruccion) {
    agregar_payload_a_paquete(paquete, &(instruccion->largo_operacion), sizeof((instruccion->largo_operacion)));
    //printf("LARGO OPERACION: bytes: %li | valor: %i \n", sizeof((instruccion->largo_operacion)), instruccion->largo_operacion);

    agregar_payload_a_paquete(paquete, instruccion->operacion, instruccion->largo_operacion);
    //printf("OPERACION: bytes: %i | valor: %s \n", instruccion->largo_operacion, instruccion->operacion);

    agregar_payload_a_paquete(paquete, &(instruccion->cant_parametros), sizeof((instruccion->largo_operacion)));
    //printf("CANT PARAMETROS: bytes: %li | valor: %i \n", sizeof((instruccion->largo_operacion)), instruccion->cant_parametros);

    for (int j = 0; j < instruccion->cant_parametros; j++) {
        char* param = list_get(instruccion->parametros, j);
        int largo_param = strlen(param) + 1;

        agregar_payload_a_paquete(paquete, &largo_param, sizeof(largo_param));
        //printf("LARGO PARAM: bytes: %li | valor: %i \n", sizeof(largo_param), largo_param);

        agregar_payload_a_paquete(paquete, param, largo_param);
        //printf("PARAM: bytes: %i | valor: %s \n", largo_param, param);
    }
}

void deserializar_instruccion(void* payload, t_instruccion* instruccion) {
    int offset = 0;
    int len_op;

    // Primero obtengo el largo de la operacion.
    memcpy(&len_op, (payload + offset), sizeof(int));
    offset += sizeof(int);

    // Procedo a copiar la operacion.
    char* operacion = malloc(STRING_LEN(len_op));
    memcpy(operacion, (payload + offset), STRING_LEN(len_op));
    offset += STRING_LEN(len_op);

    // Obtengo la cantidad de parametros.
    int cant_params;
    memcpy(&cant_params, (payload + offset), sizeof(int));
    offset += sizeof(int);

    t_list* params = list_create();
    // Copio los params.
    for (int p = 0; p < cant_params; p++) {
        // Obtengo la longitud del parametro.
        int len_param;
        memcpy(&len_param, (payload + offset), sizeof(int));
        offset += sizeof(int);
        // Obtengo el parametro.
        char* param = malloc(STRING_LEN(len_param));
        memcpy(param, (payload + offset), STRING_LEN(len_param));
        list_add(params, strdup(param));
        free(param);
        offset += STRING_LEN(len_param);
    }

    crear_instruccion(operacion, params, instruccion);
}

void imprimir_instruccion(t_instruccion* instruccion) {
    char* param;
    char* buffer = string_new();

    string_append(&buffer, (char*)instruccion->operacion);
    string_append(&buffer, " ");

    for (size_t j = 0; j < instruccion->cant_parametros; j++) {
        param = list_get(instruccion->parametros, j);
        string_append(&buffer, (char*)param);
        string_append(&buffer, " ");
    }

    printf("%s \n", buffer);
}

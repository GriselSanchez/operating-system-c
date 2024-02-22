#ifndef PROJECT_SERIALIZACION_H_
#define PROJECT_SERIALIZACION_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

typedef enum {
    INSTRUCCIONES = 0,
    PROC_EXIT_SEG_FAULT,
    PROC_EXIT_OUT_OF_MEMORY,
    PROC_EXIT_SUCCESS,
    PROC_EXIT_RESOURCE_NOT_FOUND,
    PROC_EXIT_FORCE,
    PROC_EXIT_QUANTUM,
    PROC_EXIT_PRIORITY,
    PROC_EXIT_INVALID_WRITE,
    PROC_EVICTED,
    MEMORY_NEW_PROCESS,
    MEMORY_NEW_PROCESS_OK,
    MEMORY_EXIT_PROCESS,
    MEMORY_WRITE,
    MEMORY_READ,
    MEMORY_OK,
    CONTEXTO_EJECUCION,
    CREATE_SEGMENT,
    DELETE_SEGMENT,
    SLEEP,
    WAIT,
    SIGNAL,
    MOV_IN,
    MOV_OUT,
    F_OPEN,
    F_CLOSE,
    F_SEEK,
    F_READ,
    F_WRITE,
    F_TRUNCATE,
    NEXT_INSTRUCTION,
    HANDSHAKE_CPU,
    HANDSHAKE_FILE_SYSTEM,
    FRAME_REQUEST,
    PAGE_FAULT,
    LOAD_FRAME,
    INTERRUPCION,
    ABRIR_ARCHIVO,
    CREAR_ARCHIVO,
    TRUNCAR_ARCHIVO,
    LEER_ARCHIVO,
    ESCRIBIR_ARCHIVO,
    ARCHIVO_OK,
    SWAP_NEW_PROCESS,
    SWAP_IN,
    SWAP_OUT,
    SWAP_END_PROCESS,
    SWAP_OK,
    SWAP_ERROR
} op_code;

typedef struct buffer {
    int tamanio;
    void* payload;
} t_buffer;

typedef struct paquete {
    op_code codigo_operacion;
    t_buffer* buffer;
} t_paquete;

typedef enum {
    ESCRIBIR,
    LEER
} tipo_operacion_t;

/**
 * @NAME: STATUS_NAME
 * @DESC: Imprime el nombre de un status.
 */
char* OP_CODE_NAME(op_code status);

/**
 * @NAME: crear_paquete
 * @DESC: Crea un paquete asignando los recursos necesarios para el mismo.
 */
t_paquete* crear_paquete(op_code codigo_operacion);

/**
 * @NAME: agregar_payload_a_paquete
 * @DESC: Agrega el nuevo_payload al paquete que recibe por parametros.
 *        Es requerido pasarle en nuevo_tamanio el tamanio del nuevo payload a agregar.
 */
void agregar_payload_a_paquete(t_paquete* paquete, void* nuevo_payload, int nuevo_tamanio);

/**
 * @NAME: serializar_paquete
 * @DESC: Recibe un paquete y devuelve un puntero a void en donde
 *        se encuentra el paquete totalmente serializado, con el formato:
 *        [op_code|tamanio_payload|payload]
 */
void* serializar_paquete(t_paquete* paquete);

/**
 * @NAME: deserealizar_paquete_desde_socket
 * @DESC: Recibe un socket desde el cual deserializar un paquete. Se encarga de asignar
 *        los recursos requeridos para el mismo y devuelve una estructura paquete con el
 *        payload como void*. Es responsabilidad del usuario castear dicho payload a
 *        la estructura deseada.
 */
t_paquete* deserealizar_paquete_desde_socket(int socket);

/**
 * @NAME: enviar_paquete_serializado_por_socket
 * @DESC: Recibe un socket hacia el cual enviar un paquete y un paquete. Se encarga
 *        del serializado de la estructura paquete, pero es responsabilidad del usuario
 *        serializar el contenido del payload previamente.
 */
int enviar_paquete_serializado_por_socket(int socket, t_paquete* pkg);

/**
 * @NAME: paquete_destroy
 * @DESC: Destruye un paquete liberando la memoria asignada previamente.
 */
void paquete_destroy(t_paquete* paquete);

char* deserializar_string(void* payload, int* real_offset);

void* serialize_string(t_paquete* pkg, char* str);

#endif /* PROJECT_SERIALIZACION_H_ */
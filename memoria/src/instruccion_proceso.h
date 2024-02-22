#ifndef SRC_INSTRUCCION_PROCESO_H_
#define SRC_INSTRUCCION_PROCESO_H_

#include <stdlib.h>

#include <commons/log.h>
#include <commons/string.h>
#include <commons/txt.h>

#include "utils/serializacion.h"
#include "model/instrucciones.h"
#include "memoria.h"

#define BUFFER_SIZE 1024;

typedef struct {
    int pid;
    int size;
    t_list* instrucciones;
} t_proceso_memoria;

t_instruccion* obtener_prox_instruccion(int pid, int program_counter);
t_proceso_memoria* obtener_proceso(int pid);
t_list* obtener_instrucciones_proceso(char* process_path, char* instructions_path);

t_list* obtener_instrucciones_de_archivo(char* path);
char* leer_linea(FILE* archivo);

void enviar_instruccion_serializada(int pid, int program_counter, int socket_cpu);
void enviar_new_process_serialized(t_list* instructions, int socket_kernel);

#endif /* SRC_INSTRUCCION_PROCESO_H_ */

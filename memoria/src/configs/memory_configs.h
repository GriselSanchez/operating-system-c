#ifndef SRC_MEMORY_CONFIGS_H_
#define SRC_MEMORY_CONFIGS_H_
#define CONFIG_PATH "./cfg/memoria.config"

#include <stdlib.h>
#include <commons/config.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	int puerto_escucha;
	char* ip_filesystem;
	int puerto_filesystem;
	int socket_fs;
	int tam_memoria;
	int tam_pagina;
	char* path_instrucciones;
	int retardo_respuesta;
	char* algoritmo_reemplazo;
	char* algoritmo_asignacion;
} t_config_memoria;

t_config_memoria* cargar_variables_de_archivo(void);
void memory_config_destroy(t_config_memoria*);

#endif /* SRC_MEMORY_CONFIGS_H_ */
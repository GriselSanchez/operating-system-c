#include "memory_configs.h"

t_config* configuracionMemoria;

t_config_memoria* cargar_variables_de_archivo(){
	t_config_memoria* config_valores = malloc(sizeof(t_config_memoria));
	configuracionMemoria = config_create(CONFIG_PATH);

	if (configuracionMemoria == NULL) {
		perror("Archivo memoria.config NO encontrado");
	} else {
		config_valores->puerto_escucha = config_get_int_value(configuracionMemoria, "PUERTO_ESCUCHA");
		config_valores->ip_filesystem = strdup(config_get_string_value(configuracionMemoria, "IP_FILESYSTEM"));
		config_valores->puerto_filesystem = config_get_int_value(configuracionMemoria, "PUERTO_FILESYSTEM");
		config_valores->tam_memoria = config_get_int_value(configuracionMemoria, "TAM_MEMORIA");
		config_valores->tam_pagina = config_get_int_value(configuracionMemoria, "TAM_PAGINA");
		config_valores->path_instrucciones = strdup(config_get_string_value(configuracionMemoria, "PATH_INSTRUCCIONES"));
		config_valores->retardo_respuesta = config_get_int_value(configuracionMemoria, "RETARDO_RESPUESTA") / 1000;
		config_valores->algoritmo_reemplazo = strdup(config_get_string_value(configuracionMemoria, "ALGORITMO_REEMPLAZO"));
		config_valores->algoritmo_asignacion = strdup(config_get_string_value(configuracionMemoria, "ALGORITMO_ASIGNACION"));
		return config_valores;
	}
}


void memory_config_destroy(t_config_memoria* configs){
	free(configs->algoritmo_asignacion);
	free(configs->algoritmo_reemplazo);
	free(configs->path_instrucciones);
	free(configs->ip_filesystem);
	
	free(configs);
	config_destroy(configuracionMemoria);
}
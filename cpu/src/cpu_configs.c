#include "cpu_configs.h"

t_config* configuracionCPU;

t_config_cpu* cargar_variables_de_archivo(){
	t_config_cpu* config_valores = malloc(sizeof(t_config_cpu));
	configuracionCPU = config_create(CONFIG_PATH);

    if(configuracionCPU == NULL){
        perror("Archivo memoria.config NO encontrado");
    }else{
	    config_valores->ip_memoria = strdup(config_get_string_value(configuracionCPU, "IP_MEMORIA"));
	    config_valores->puerto_memoria = config_get_int_value(configuracionCPU, "PUERTO_MEMORIA");
	    config_valores->puerto_escucha_dispatch = config_get_int_value(configuracionCPU, "PUERTO_ESCUCHA_DISPATCH");
	    config_valores->puerto_escucha_interrupt = config_get_int_value(configuracionCPU, "PUERTO_ESCUCHA_INTERRUPT");

    }
	return config_valores;
}


void cpu_config_destroy(t_config_cpu* configs){
	free(configs);
	config_destroy(configuracionCPU);
}
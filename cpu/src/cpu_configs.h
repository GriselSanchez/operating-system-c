#ifndef SRC_CPU_CONFIGS_H_
#define SRC_CPU_CONFIGS_H_
#define CONFIG_PATH "./cfg/cpu.config"

#include <stdlib.h>
#include <commons/config.h>
#include <string.h>
#include <stdio.h>

typedef struct {
	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha_dispatch;
	int puerto_escucha_interrupt;
} t_config_cpu;

t_config_cpu* cargar_variables_de_archivo(void);
void memory_config_destroy(t_config_cpu*);

#endif /* SRC_MEMORY_CONFIGS_H_ */
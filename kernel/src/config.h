#ifndef SRC_KERNEL_CONFIG_H_
#define SRC_KERNEL_CONFIG_H_

#include <commons/config.h>
#include <commons/collections/list.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <commons/string.h>

#define CONFIG_PATH "./cfg/kernel.config"

typedef struct {
	char* memory_ip;
	int memory_port;
	char* cpu_ip;
	int cpu_dispatch_port;
	int cpu_interrupt_port;
	char* file_system_ip;
	int file_system_port;
	int port;
	int multiprogramming_degree;
	int quantum;
	char* planner;
	t_list* recursos;
	t_list* instancias_recursos;
} t_kernel_config;

/**
* @NAME: load_config
* @DESC: 
*/
t_kernel_config* load_config(char* configs_path);

/**
* @NAME: kernel_config_destroy
* @DESC: 
*/
void kernel_config_destroy(t_kernel_config*);


/**
* @NAME: validate_config
* @DESC: 
*/
bool validate_config(t_config*);

#endif /* SRC_KERNEL_CONFIG_H_ */
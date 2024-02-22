#ifndef SRC_FILESYSTEM_config_H_
#define SRC_FILESYSTEM_config_H_

#include <commons/config.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_PATH "filesystem.config"

typedef struct {
	char* ip_memoria;
	int puerto_memoria;
	int puerto_escucha;
	char* path_fat;
	char* path_bloques;
	char* path_fcb;
	int cant_bloques_total;
	int cant_bloques_swap;
	int tam_bloque;
	int retardo_acceso_bloque;
	int retardo_acceso_fat;
	int memoria_fd;
} t_filesystem_config;

/**
* @NAME: cargar_config_de_archivo
* @DESC:
*/
t_filesystem_config* cargar_config_de_archivo();

/**
* @NAME: filesystem_config_destroy
* @DESC:
*/
void filesystem_config_destroy(t_filesystem_config*);



#endif /* SRC_FILESYSTEM_config_H_ */
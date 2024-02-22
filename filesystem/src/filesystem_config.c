#include "filesystem_config.h"

t_config* configuracion;

t_filesystem_config* cargar_config_de_archivo() {
	t_filesystem_config* config_valores = malloc(sizeof(t_filesystem_config));
	//configuracion = config_create(CONFIG_PATH)
	configuracion = config_create("./cfg/filesystem.config");

	if (configuracion == NULL) {
		perror("Archivo de configuracion filesystem.config NO encontrado");
	}
	config_valores->ip_memoria = strdup(config_get_string_value(configuracion, "IP_MEMORIA"));
	config_valores->puerto_memoria = config_get_int_value(configuracion, "PUERTO_MEMORIA");
	config_valores->puerto_escucha = config_get_int_value(configuracion, "PUERTO_ESCUCHA");
	config_valores->path_fat = strdup(config_get_string_value(configuracion, "PATH_FAT"));
	config_valores->path_bloques = strdup(config_get_string_value(configuracion, "PATH_BLOQUES"));
	config_valores->path_fcb = strdup(config_get_string_value(configuracion, "PATH_FCB"));
	config_valores->cant_bloques_total = config_get_int_value(configuracion, "CANT_BLOQUES_TOTAL");
	config_valores->cant_bloques_swap = config_get_int_value(configuracion, "CANT_BLOQUES_SWAP");
	config_valores->tam_bloque = config_get_int_value(configuracion, "TAM_BLOQUE");
	config_valores->retardo_acceso_bloque = config_get_int_value(configuracion, "RETARDO_ACCESO_BLOQUE");
	config_valores->retardo_acceso_fat = config_get_int_value(configuracion, "RETARDO_ACCESO_FAT");

	return config_valores;
}


void filesystem_config_destroy(t_filesystem_config* config_valores) {
	free(config_valores->ip_memoria);
	free(config_valores->path_fat);
	free(config_valores->path_bloques);
	free(config_valores->path_fcb);
	free(config_valores);
	config_destroy(configuracion);
}
#include "bloques.h"

extern t_log* logger;
extern t_filesystem_config* configs;

void crear_archivo_de_bloques() {
    int TAMANIO = configs->cant_bloques_total * configs->tam_bloque;

    if (access(configs->path_bloques, F_OK) != -1) {
        log_info(logger, "Archivo de bloques ya existe. No es necesario crearlo.");
        return;
    }

    int fd = open(configs->path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        log_error(logger, "Error al abrir el archivo de bloques.");
        exit(1);
    }

    ftruncate(fd, TAMANIO);

    char* bloques = mmap(NULL, TAMANIO, PROT_WRITE, MAP_SHARED, fd, 0);
    if (bloques == MAP_FAILED) {
        log_error(logger, "Error al mapear el archivo del bloques.");
        exit(1);
    }

    memset(bloques, 0, TAMANIO);

    log_info(logger, "Archivo de bloques creado de %i bytes de tamaño.", TAMANIO);

    if (munmap(bloques, TAMANIO) == -1) {
        perror("Error al desmapear el archivo de bloques");
    }

    close(fd);
}

void escribir_bloque(int nro_bloque, uint32_t* data) {
    sleep(configs->retardo_acceso_bloque / 1000);

    int TAMANIO = configs->cant_bloques_total * configs->tam_bloque;
    int fd = open(configs->path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        log_error(logger, "Error al abrir el archivo de bloques.");
        exit(1);
    }

    char* bloques = mmap(NULL, TAMANIO, PROT_WRITE, MAP_SHARED, fd, 0);
    if (bloques == MAP_FAILED) {
        log_error(logger, "Error al mapear el archivo del bloques.");
        exit(1);
    }

    size_t offset = nro_bloque * configs->tam_bloque;
    memcpy(bloques + offset, data, configs->tam_bloque);

    if (munmap(bloques, TAMANIO) == -1) {
        perror("Error al desmapear el archivo de bloques");
    }

    close(fd);
}

uint32_t* leer_bloque(int nro_bloque) {
    sleep(configs->retardo_acceso_bloque / 1000);

    int TAMANIO = configs->cant_bloques_total * configs->tam_bloque;
    int fd = open(configs->path_bloques, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

    if (fd == -1) {
        log_error(logger, "Error al abrir el archivo de bloques.");
        exit(1);
    }

    char* bloques = mmap(NULL, TAMANIO, PROT_WRITE, MAP_SHARED, fd, 0);
    if (bloques == MAP_FAILED) {
        log_error(logger, "Error al mapear el archivo del bloques.");
        exit(1);
    }

    size_t offset = nro_bloque * configs->tam_bloque;
    uint32_t* resultado = malloc(configs->tam_bloque);
    memcpy(resultado, bloques + offset, configs->tam_bloque);

    if (munmap(bloques, TAMANIO) == -1) {
        perror("Error al desmapear el archivo de bloques");
    }

    close(fd);

    return resultado;
}

void limpiar_bloque(int nro_bloque) {
    uint32_t* data = malloc(configs->tam_bloque);

    for (size_t i = 0; i < configs->tam_bloque / sizeof(uint32_t); i++)
    {
        data[i] = 0;
    }

    escribir_bloque(nro_bloque, data);
    free(data);
}

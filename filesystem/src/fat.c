#include "fat.h"

extern t_log* logger;
extern t_filesystem_config* configs;

void inicializar_fat() {
    int TAMANIO = (configs->cant_bloques_total - configs->cant_bloques_swap) * sizeof(uint32_t);

    FILE* archivo = fopen(configs->path_fat, "r");

    if (archivo == NULL) {
        log_info(logger, "No se encuentra el archivo FAT.");

        archivo = fopen(configs->path_fat, "w");

        if (archivo == NULL) {
            log_error(logger, "Error al crear el archivo FAT.");
            exit(1);
        }

        limpiar_fat(archivo);
    } else {
        log_info(logger, "Archivo FAT ya existe. No es necesario crearlo.");
    }

    fclose(archivo);
}

void limpiar_fat(FILE* archivo) {
    int cant_bloques = configs->cant_bloques_total - configs->cant_bloques_swap;

    fseek(archivo, 0, SEEK_SET);

    uint32_t valor_libre = 0;
    for (int i = 0; i < cant_bloques; i++) {
        fwrite(&valor_libre, sizeof(uint32_t), 1, archivo);
    }

    uint32_t bloque_reservado = UINT32_MAX;
    fseek(archivo, 0, SEEK_SET);
    fwrite(&bloque_reservado, sizeof(uint32_t), 1, archivo);
    fflush(archivo);

    log_info(logger, "Archivo FAT creado de %li bytes de tamaño.", cant_bloques * sizeof(uint32_t));
}

void asignar_bloques_nuevos(t_fcb* fcb, int cant_bloques_nuevos) {
    FILE* archivo = fopen(configs->path_fat, "r+");
    if (archivo == NULL) {
        log_error(logger, "No se encuentra el archivo FAT.");
    }

    int cant_bloques = configs->cant_bloques_total - configs->cant_bloques_swap;

    fseek(archivo, 0, SEEK_SET);

    uint32_t entradas[cant_bloques];
    fread(entradas, sizeof(uint32_t), cant_bloques, archivo);

    uint32_t entradas_nuevas[cant_bloques_nuevos];
    int cant_bloques_asignados = 0;

    t_list* bloques_previos = obtener_bloques_fat_asignados(fcb, entradas);
    // log_info(logger, "Bloques previos:");
    // imprimir_bloques_fat_asignados(fcb, entradas);

    for (int i = 0; i < cant_bloques; i++) {
        if (cant_bloques_nuevos == 0) break;
        if (acceso_fat_get(entradas, i) == 0) {
            entradas_nuevas[cant_bloques_asignados] = i;
            cant_bloques_asignados++;
            cant_bloques_nuevos--;
        }
    }

    for (int i = 0; i < cant_bloques_asignados - 1; i++) {
        uint32_t entrada_nueva = entradas_nuevas[i];
        acceso_fat_set(entradas, entrada_nueva, entradas_nuevas[i + 1]);
    }

    acceso_fat_set(entradas, entradas_nuevas[cant_bloques_asignados - 1], UINT32_MAX);

    if (fcb->bloque_inicial == 0) {
        fcb->bloque_inicial = entradas_nuevas[0];
    } else {
        uint32_t ultimo_bloque_previo = list_get(bloques_previos, list_size(bloques_previos) - 1);
        acceso_fat_set(entradas, ultimo_bloque_previo, entradas_nuevas[0]);
    }

    // log_info(logger, "Bloques actualizados:");
    // imprimir_bloques_fat_asignados(fcb, entradas);

    fseek(archivo, 0, SEEK_SET);
    fwrite(entradas, sizeof(uint32_t), cant_bloques, archivo);
    fflush(archivo);
    fclose(archivo);
    list_destroy(bloques_previos);
}

void liberar_bloques_asignados(t_fcb* fcb, int cant_bloques_a_liberar) {
    FILE* archivo = fopen(configs->path_fat, "r+");
    if (archivo == NULL) {
        log_error(logger, "No se encuentra el archivo FAT.");
        return;
    }

    int cant_bloques = configs->cant_bloques_total - configs->cant_bloques_swap;

    fseek(archivo, 0, SEEK_SET);

    uint32_t* entradas = obtener_entradas_fat(archivo);
    fread(entradas, sizeof(uint32_t), cant_bloques, archivo);

    t_list* bloques_previos = obtener_bloques_fat_asignados(fcb, entradas);
    // log_info(logger, "Bloques previos:");
    // imprimir_bloques_fat_asignados(fcb, entradas);

    if (list_size(bloques_previos) < cant_bloques_a_liberar) {
        log_error(logger, "No hay suficientes bloques previos asignados para liberar.");
        fclose(archivo);
        return;
    }

    for (int i = 0; i < cant_bloques_a_liberar; i++) {
        uint32_t bloque_a_liberar = list_remove(bloques_previos, list_size(bloques_previos) - 1);
        acceso_fat_set(entradas, bloque_a_liberar, 0);
    }

    if (list_size(bloques_previos) == 0) fcb->bloque_inicial = 0;
    else {
        uint32_t ultimo_bloque = list_get(bloques_previos, list_size(bloques_previos) - 1);
        acceso_fat_set(entradas, ultimo_bloque, UINT32_MAX);
    }

    // log_info(logger, "Bloques actualizados:");
    // imprimir_bloques_fat_asignados(fcb, entradas);

    fseek(archivo, 0, SEEK_SET);
    fwrite(entradas, sizeof(uint32_t), cant_bloques, archivo);
    fflush(archivo);
    fclose(archivo);
    free(entradas);
    list_destroy(bloques_previos);
}

t_list* obtener_bloques_fat_asignados(t_fcb* fcb, uint32_t* entradas) {
    uint32_t bloque = fcb->bloque_inicial;
    t_list* bloques = list_create();

    while (true) {
        if (bloque == 0 || bloque == UINT32_MAX) break;
        list_add(bloques, (void*)(intptr_t)bloque);
        bloque = acceso_fat_get(entradas, bloque);
    }

    return bloques;
}

uint32_t obtener_bloque_fat_de_archivo(t_fcb* fcb, int nro_bloque) {
    FILE* archivo = fopen(configs->path_fat, "r+");
    if (archivo == NULL) {
        log_error(logger, "No se encuentra el archivo FAT.");
    }

    int cant_bloques = configs->cant_bloques_total - configs->cant_bloques_swap;

    fseek(archivo, 0, SEEK_SET);

    uint32_t entradas[cant_bloques];
    fread(entradas, sizeof(uint32_t), cant_bloques, archivo);

    uint32_t bloque = fcb->bloque_inicial;

    for (size_t i = 0; i < nro_bloque; i++)
    {
        bloque = acceso_fat_get(entradas, bloque);
    }

    return bloque;
}


int obtener_cant_bloques_asignados(t_fcb* fcb) {
    FILE* archivo = fopen(configs->path_fat, "r+");

    uint32_t* entradas = obtener_entradas_fat(archivo);
    t_list* bloques = obtener_bloques_fat_asignados(fcb, entradas);

    int cant_bloques = list_size(bloques);

    fclose(archivo);
    free(entradas);
    list_destroy(bloques);

    return cant_bloques;
}

void imprimir_bloques_fat_asignados(t_fcb* fcb, uint32_t* entradas) {
    t_list* bloques = obtener_bloques_fat_asignados(fcb, entradas);

    for (size_t i = 0; i < list_size(bloques); i++)
    {
        uint32_t entrada = (uint32_t)(intptr_t)list_get(bloques, i);
        log_info(logger, "Acceso FAT - Entrada: %i - Valor: %i", entrada, entradas[entrada]);
    }

    list_destroy(bloques);
}

uint32_t* obtener_entradas_fat(FILE* archivo) {
    if (archivo == NULL) {
        log_error(logger, "No se encuentra el archivo FAT.");
    }
    int cant_bloques = configs->cant_bloques_total - configs->cant_bloques_swap;
    fseek(archivo, 0, SEEK_SET);

    uint32_t* entradas_fat = (uint32_t*)malloc(sizeof(uint32_t) * cant_bloques);
    fread(entradas_fat, sizeof(uint32_t), cant_bloques, archivo);

    return entradas_fat;
}

void acceso_fat_set(uint32_t* entradas, int nro_entrada, uint32_t valor) {
    entradas[nro_entrada] = valor;
    log_info(logger, "Acceso FAT - ESCRITURA - Entrada: %i - Valor: %i", nro_entrada, entradas[nro_entrada]);
}

uint32_t acceso_fat_get(uint32_t* entradas, int nro_entrada) {
    uint32_t entrada = entradas[nro_entrada];
    log_info(logger, "Acceso FAT - LECTURA - Entrada: %i - Valor: %i", nro_entrada, entrada);
    return entrada;
}


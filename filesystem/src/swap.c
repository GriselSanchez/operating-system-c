#include "swap.h"

extern t_log* logger;
extern t_filesystem_config* configs;
extern t_bitarray* bitarray;

t_bitarray* inicializar_swap() {
    int tamanio_bitmap = configs->cant_bloques_swap / 8;
    void* puntero_a_bits = malloc(tamanio_bitmap);

    t_bitarray* bitarray = bitarray_create_with_mode(puntero_a_bits, tamanio_bitmap, LSB_FIRST);

    for (size_t i = 0; i < bitarray_get_max_bit(bitarray); i++)
    {
        bitarray_clean_bit(bitarray, i);
    }

    log_info(logger, "Inicializo bitmap de bloques swap con %li bits.", bitarray_get_max_bit(bitarray));

    return bitarray;
}

void imprimir_swap() {
    char buffer[256];

    int offset = snprintf(buffer, sizeof(buffer), "Bitmap de bloques swap: ");

    for (size_t i = 0; i < bitarray_get_max_bit(bitarray); i++) {
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%u", bitarray_test_bit(bitarray, i));
    }

    log_debug(logger, "%s", buffer);
}

t_list* asignar_bloques_swap(int cant_bloques_nuevos) {
    int cant_bloques = configs->cant_bloques_swap;
    t_list* bloques_nuevos = list_create();

    for (size_t i = 0; i < cant_bloques; i++)
    {
        if (cant_bloques_nuevos == list_size(bloques_nuevos)) break;
        if (bitarray_test_bit(bitarray, i) == 0) {
            bitarray_set_bit(bitarray, i);
            list_add(bloques_nuevos, i);
            limpiar_bloque_swap(i);
        }
    }

    return bloques_nuevos;
}

void liberar_bloques_swap(t_list* bloques_a_liberar) {
    int cant_bloques = list_size(bloques_a_liberar);

    for (size_t i = 0; i < cant_bloques; i++)
    {
        int bloque_a_borrar = list_get(bloques_a_liberar, i);
        bitarray_clean_bit(bitarray, bloque_a_borrar);
    }
}

void swap_out(int nro_bloque, uint32_t* data) {
    log_info(logger, "Acceso SWAP: %i", nro_bloque);

    escribir_bloque(nro_bloque, data);
}

uint32_t* swap_in(int nro_bloque) {
    log_info(logger, "Acceso SWAP: %i", nro_bloque);

    uint32_t* data_bloque = leer_bloque(nro_bloque);
    return data_bloque;
}

void limpiar_bloque_swap(int nro_bloque) {
    log_info(logger, "Acceso SWAP: %i", nro_bloque);

    limpiar_bloque(nro_bloque);
}

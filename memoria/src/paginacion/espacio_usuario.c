#include "espacio_usuario.h"

t_espacio_usuario* create_espacio_usuario(int tamanio_memoria, int tamanio_marco)
{
    int cantidad_marcos = tamanio_memoria / tamanio_marco;
    t_espacio_usuario* espacio_usuario = malloc(sizeof(t_espacio_usuario));
    espacio_usuario->tamanio_memoria = tamanio_memoria;
    espacio_usuario->cantidad_marcos = cantidad_marcos;
    for (int i = 0; i < espacio_usuario->cantidad_marcos; i++) {
        espacio_usuario->marcos_ocupados[i] = 0;
    }
    espacio_usuario->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(espacio_usuario->mutex, NULL);
    espacio_usuario->espacio = malloc(tamanio_memoria);

    return espacio_usuario;
}
//agregar mutex
uint32_t espacio_leer_valor(t_espacio_usuario* espacio_usuario, int dir_fisica, int pid)
{
    uint32_t valor;

    pthread_mutex_lock(espacio_usuario->mutex);
    memcpy(&valor, espacio_usuario->espacio + dir_fisica, sizeof(uint32_t));
    log_debug(logger, "PID: %i - Accion: LEER - Direccion fisica: %i - Valor: %i", pid, dir_fisica, valor);
    pthread_mutex_unlock(espacio_usuario->mutex);

    return valor;
}

void espacio_escribir_valor(t_espacio_usuario* espacio_usuario, int dir_fisica, uint32_t valor, int pid) {
    pthread_mutex_lock(espacio_usuario->mutex);
    log_info(logger, "PID: %i - Accion: ESCRIBIR - Direccion fisica: %i - Valor: %i", pid, dir_fisica, valor);
    memcpy(espacio_usuario->espacio + dir_fisica, &valor, sizeof(uint32_t));
    pthread_mutex_unlock(espacio_usuario->mutex);
}


int obtener_frame_libre(t_espacio_usuario* espacio_usuario) {
    for (int i = 0; i < espacio_usuario->cantidad_marcos; i++) {
        if (espacio_usuario->marcos_ocupados[i] == 0) {
            return i;
        }
    }
    return -1;
}

void marcar_frame_ocupado(t_espacio_usuario* espacio_usuario, int marco) {
    pthread_mutex_lock(espacio_usuario->mutex);
    espacio_usuario->marcos_ocupados[marco] = 1;
    pthread_mutex_unlock(espacio_usuario->mutex);
}

void liberar_frame(t_espacio_usuario* espacio_usuario, int marco) {
    pthread_mutex_lock(espacio_usuario->mutex);
    espacio_usuario->marcos_ocupados[marco] = 0;
    pthread_mutex_unlock(espacio_usuario->mutex);
}

int espacio_obtener_frame(t_espacio_usuario* espacio_usuario, int dir_fisica) {
    int tamanio_marco = espacio_usuario->tamanio_memoria / espacio_usuario->cantidad_marcos;
    return floor(dir_fisica / tamanio_marco);
}

void espacio_destroy(t_espacio_usuario* espacio_usuario) {
    pthread_mutex_destroy(espacio_usuario->mutex);
    free(espacio_usuario->espacio);
    free(espacio_usuario);
}

void imprimir_espacio_usuario(t_espacio_usuario* espacio_usuario) {
    char buffer[256];

    int offset = snprintf(buffer, sizeof(buffer), "Espacio de usuario: ");

    pthread_mutex_lock(espacio_usuario->mutex);
    for (size_t i = 0; i < espacio_usuario->cantidad_marcos; i++) {
        uint32_t valor;
        memcpy(&valor, espacio_usuario->espacio + i * sizeof(uint32_t), sizeof(uint32_t));
        offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%u | ", valor);
    }
    pthread_mutex_unlock(espacio_usuario->mutex);

    log_debug(logger, "%s", buffer);
}

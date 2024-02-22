#include "cola_monitor.h"
#include "model/pcb.h"

t_cola_mutex* queue_init_mutex() {
    t_cola_mutex* cola = malloc(sizeof(t_cola_mutex));
    cola->cola = queue_create();
    cola->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(cola->mutex, NULL);
    return cola;
}

void queue_push_mutex(t_cola_mutex* cola, void* elemento) {
    pthread_mutex_lock(cola->mutex);
    queue_push(cola->cola, elemento);
    pthread_mutex_unlock(cola->mutex);
}

void* queue_pop_mutex(t_cola_mutex* cola) {
    pthread_mutex_lock(cola->mutex);
    void* elemento = NULL;
    if (!queue_is_empty(cola->cola)) {
        elemento = queue_pop(cola->cola);
    }
    pthread_mutex_unlock(cola->mutex);
    return elemento;
}

int queue_size_mutex(t_cola_mutex* cola) {
    pthread_mutex_lock(cola->mutex);
    int size = queue_size(cola->cola);
    pthread_mutex_unlock(cola->mutex);
    return size;
}

void* queue_peek_mutex(t_cola_mutex* cola) {
    pthread_mutex_lock(cola->mutex);
    void* element = queue_peek(cola->cola);
    pthread_mutex_unlock(cola->mutex);
    return element;
}

void queue_destroy_and_destroy_elements_mutex(t_cola_mutex* cola, void(*element_destroyer)(void*)) {
    pthread_mutex_destroy(cola->mutex);
    queue_destroy_and_destroy_elements(cola->cola, element_destroyer);
    free(cola);
}

void queue_destroy_mutex(t_cola_mutex* cola) {
    pthread_mutex_destroy(cola->mutex);
    queue_destroy(cola->cola);
    free(cola);
}

void queue_remove_element_mutex(t_cola_mutex* cola, void* elemento) {
    pthread_mutex_lock(cola->mutex);

    //cola temporal
    t_cola_mutex* nueva_cola = queue_create();

    while (!queue_is_empty(cola->cola)) {
        void* elemento_actual = queue_pop(cola->cola);

        if (elemento_actual != elemento) {
            queue_push(nueva_cola, elemento_actual);
        }

    }
    cola->cola = nueva_cola;
    pthread_mutex_unlock(cola->mutex);


}
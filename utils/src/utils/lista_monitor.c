#include "lista_monitor.h"

t_lista_mutex* list_init_mutex() {
    t_lista_mutex* lista_mutex = malloc(sizeof(t_lista_mutex));
    lista_mutex->lista = list_create();
    lista_mutex->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(lista_mutex->mutex, NULL);
    return lista_mutex;
}

void list_add_mutex(t_lista_mutex* lista, void* elemento) {
    pthread_mutex_lock(lista->mutex);
    list_add(lista->lista, elemento);
    pthread_mutex_unlock(lista->mutex);
}

void list_add_in_index_mutex(t_lista_mutex* lista, int index, void* elemento) {
    pthread_mutex_lock(lista->mutex);
    list_add_in_index(lista->lista, index, elemento);
    pthread_mutex_unlock(lista->mutex);
}

void* list_get_mutex(t_lista_mutex* lista, int index) {
    pthread_mutex_lock(lista->mutex);
    void* valor_obtenido = list_get(lista->lista, index);
    pthread_mutex_unlock(lista->mutex);
    return valor_obtenido;
}

int list_size_mutex(t_lista_mutex* lista) {
    pthread_mutex_lock(lista->mutex);
    int tamanio = list_size(lista->lista);
    pthread_mutex_unlock(lista->mutex);
    return tamanio;
}

void* list_remove_mutex(t_lista_mutex* lista, int index) {
    pthread_mutex_lock(lista->mutex);
    void* valor_removido = list_remove(lista->lista, index);
    pthread_mutex_unlock(lista->mutex);
    return valor_removido;
}

bool list_remove_element_mutex(t_lista_mutex* lista, void* element) {
    pthread_mutex_lock(lista->mutex);
    bool success = list_remove_element(lista->lista, element);
    pthread_mutex_unlock(lista->mutex);
    return success;
}

void* list_remove_by_condition_mutex(t_lista_mutex* lista, bool(*condition)(void*)) {
    pthread_mutex_lock(lista->mutex);
    void* valor_removido = list_remove_by_condition(lista->lista, condition);
    pthread_mutex_unlock(lista->mutex);

    return valor_removido;
}

void list_sort_mutex(t_lista_mutex* lista, bool (*comparator)(void*, void*)) {
    pthread_mutex_lock(lista->mutex);
    list_sort(lista->lista, comparator);
    pthread_mutex_unlock(lista->mutex);
}

void list_remove_and_destroy_element_mutex(t_lista_mutex* lista, int index, void(*element_destroyer)(void*)) {
    pthread_mutex_lock(lista->mutex);
    list_remove_and_destroy_element(lista->lista, index, element_destroyer);
    pthread_mutex_unlock(lista->mutex);
}

void list_destroy_and_destroy_elements_mutex(t_lista_mutex* lista, void(*element_destroyer)(void*)) {
    pthread_mutex_destroy(lista->mutex);
    list_destroy_and_destroy_elements(lista->lista, element_destroyer);
    free(lista);
}

void list_destroy_mutex(t_lista_mutex* lista) {
    pthread_mutex_destroy(lista->mutex);
    list_destroy(lista->lista);
    free(lista);
}

void* list_fold_mutex(t_lista_mutex* monitor, void* seed, void* (*operation)(void*, void*)) {
    pthread_mutex_lock(monitor->mutex);
    void* result = list_fold(monitor->lista, seed, operation);
    pthread_mutex_unlock(monitor->mutex);
    return result;
}

void* list_find_mutex(t_lista_mutex* monitor, bool(*closure)(void*)) {
    pthread_mutex_lock(monitor->mutex);
    void* result = list_find(monitor->lista, closure);
    pthread_mutex_unlock(monitor->mutex);
    return result;
}

t_list* list_filter_mutex(t_lista_mutex* monitor, bool(*condition)(void*)) {
    pthread_mutex_lock(monitor->mutex);
    t_list* filtered = list_filter(monitor->lista, condition);
    pthread_mutex_unlock(monitor->mutex);
    return filtered;
}

void list_add_sorted_mutex(t_lista_mutex* lista, void* elemento, bool(*comparator)(void*, void*))
{
    pthread_mutex_lock(lista->mutex);
    list_add_sorted(lista->lista, elemento, comparator);
    pthread_mutex_unlock(lista->mutex);
}

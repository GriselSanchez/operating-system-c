#include "diccionario_monitor.h"

t_diccionario_mutex* diccionario_create() {
    t_diccionario_mutex* diccionario_mutex = malloc(sizeof(t_diccionario_mutex));
    diccionario_mutex->dictionary = dictionary_create();
    diccionario_mutex->mutex = malloc(sizeof(pthread_mutex_t));
    pthread_mutex_init(diccionario_mutex->mutex, NULL);
    return diccionario_mutex;
}

void diccionario_put(t_diccionario_mutex* diccionario, char* key, void* element)
{
    pthread_mutex_lock(diccionario->mutex);
    dictionary_put(diccionario->dictionary, key, element);
    pthread_mutex_unlock(diccionario->mutex);
}

void* diccionario_get(t_diccionario_mutex* diccionario, char* key)
{
    pthread_mutex_lock(diccionario->mutex);
    void* elemento = dictionary_get(diccionario->dictionary, key);
    pthread_mutex_unlock(diccionario->mutex);
    return elemento;
}

void* diccionario_remove(t_diccionario_mutex* diccionario, char* key)
{
    pthread_mutex_lock(diccionario->mutex);
    void* elemento = dictionary_remove(diccionario->dictionary, key);
    pthread_mutex_unlock(diccionario->mutex);
    return elemento;
}

void diccionario_remove_and_destroy(t_diccionario_mutex* diccionario, char* key, void(*element_destroyer)(void*))
{
    pthread_mutex_lock(diccionario->mutex);
    dictionary_remove_and_destroy(diccionario->dictionary, key, element_destroyer);
    pthread_mutex_unlock(diccionario->mutex);
}

bool diccionario_has_key(t_diccionario_mutex* diccionario, char* key)
{
    pthread_mutex_lock(diccionario->mutex);
    bool contiene_key = dictionary_has_key(diccionario->dictionary, key);
    pthread_mutex_unlock(diccionario->mutex);
    return contiene_key;
}

t_list* diccionario_elements(t_diccionario_mutex *diccionario) {
    pthread_mutex_lock(diccionario->mutex);
    t_list* elements = dictionary_elements(diccionario->dictionary);
    pthread_mutex_unlock(diccionario->mutex);
    return elements;
}


void diccionario_destroy(t_diccionario_mutex* diccionario)
{
    pthread_mutex_lock(diccionario->mutex);
    dictionary_destroy(diccionario->dictionary);
    pthread_mutex_unlock(diccionario->mutex);
    pthread_mutex_destroy(diccionario->mutex);
    free(diccionario);
}

void diccionario_destroy_and_destroy_elements(t_diccionario_mutex* diccionario, void(*element_destroyer)(void*))
{
    pthread_mutex_lock(diccionario->mutex);
    dictionary_destroy_and_destroy_elements(diccionario->dictionary, element_destroyer);
    pthread_mutex_unlock(diccionario->mutex);
    pthread_mutex_destroy(diccionario->mutex);
    free(diccionario);
}

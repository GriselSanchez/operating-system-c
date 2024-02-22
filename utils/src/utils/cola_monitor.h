#ifndef SRC_UTILS_COLA_MONITOR_H
#define SRC_UTILS_COLA_MONITOR_H

#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
    t_queue* cola;
    pthread_mutex_t* mutex;
} t_cola_mutex;

t_cola_mutex* queue_init_mutex();
void queue_push_mutex(t_cola_mutex* cola, void* elemento);
void* queue_pop_mutex(t_cola_mutex* cola);
int queue_size_mutex(t_cola_mutex* cola);
void* queue_peek_mutex(t_cola_mutex* cola);
void queue_destroy_and_destroy_elements_mutex(t_cola_mutex* cola, void(*element_destroyer)(void*));
void queue_destroy_mutex(t_cola_mutex* cola);
void queue_remove_element_mutex(t_cola_mutex* cola, void* elemento);

#endif /* SRC_UTILS_COLA_MONITOR_H */
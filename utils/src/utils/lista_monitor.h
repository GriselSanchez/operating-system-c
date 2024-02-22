#ifndef SRC_UTILS_LISTA_MONITOR_H
#define SRC_UTILS_LISTA_MONITOR_H

#include <commons/collections/list.h>
#include <pthread.h>
#include <stdlib.h>

typedef struct {
  t_list* lista;
  pthread_mutex_t* mutex;
} t_lista_mutex;

t_lista_mutex* list_init_mutex();
void list_add_mutex(t_lista_mutex* lista, void* elemento);
void list_add_in_index_mutex(t_lista_mutex* lista, int index, void* elemento);
void* list_get_mutex(t_lista_mutex* lista, int index);
int list_size_mutex(t_lista_mutex* lista);
void* list_remove_mutex(t_lista_mutex* lista, int index);
bool list_remove_element_mutex(t_lista_mutex* lista, void* element);
void* list_remove_by_condition_mutex(t_lista_mutex* lista, bool(*condition)(void*));
void list_sort_mutex(t_lista_mutex* lista, bool (*comparator)(void*, void*));
void list_remove_and_destroy_element_mutex(t_lista_mutex* lista, int index, void(*element_destroyer)(void*));
void list_destroy_and_destroy_elements_mutex(t_lista_mutex* lista, void(*element_destroyer)(void*));
void list_destroy_mutex(t_lista_mutex* lista);
void* list_fold_mutex(t_lista_mutex* monitor, void* seed, void*(*operation)(void*, void*));
void *list_find_mutex(t_lista_mutex *, bool(*closure)(void*));
t_list* list_filter_mutex(t_lista_mutex*, bool(*condition)(void*));
void list_add_sorted_mutex(t_lista_mutex* lista, void* elemento, bool (*comparator)(void*,void*));

#endif /* SRC_UTILS_LISTA_MONITOR_H */
#ifndef DICCIONARIO_MONITOR_H_
#define DICCIONARIO_MONITOR_H_

#include <commons/collections/dictionary.h>
#include <pthread.h>
#include <stdlib.h>

/**
 * Esto es un wrapper para t_dictionary, no estan wrappeadas todas las funciones, solamente las que fueron necesarias
 * ver dictionary.h
*/

typedef struct {
    t_dictionary* dictionary;
    pthread_mutex_t* mutex;
} t_diccionario_mutex;

	/**
	* @NAME: diccionario_create
	* @DESC: Crea el diccionario
	*/
	t_diccionario_mutex* diccionario_create();

	/**
	* @NAME: diccionario_put
	* @DESC: Inserta un nuevo par (key->element) al diccionario, en caso de ya existir la key actualiza el elemento.
	* [Warning] - Tener en cuenta que esto no va a liberar la memoria del `element` original.
	*/
	void 		  diccionario_put(t_diccionario_mutex *, char *key, void *element);

	/**
	* @NAME: diccionario_get
	* @DESC: Obtiene el elemento asociado a la key.
	*/
	void 		 *diccionario_get(t_diccionario_mutex *, char *key);

	/**
	* @NAME: diccionario_remove
	* @DESC: Remueve un elemento del diccionario y lo retorna.
	*/
	void 		 *diccionario_remove(t_diccionario_mutex *, char *key);

	/**
	* @NAME: diccionario_remove_and_destroy
	* @DESC: Remueve un elemento del diccionario y lo destruye.
	*/
	void 		  diccionario_remove_and_destroy(t_diccionario_mutex *, char *key, void(*element_destroyer)(void*));

	/**
	* @NAME: diccionario_has_key
	* @DESC: Retorna true si key se encuentra en el diccionario
	*/
	bool 		  diccionario_has_key(t_diccionario_mutex *, char* key);

		/**
	* @NAME: dictionary_elements
	* @DESC: Retorna todos los elementos en una lista
	*/
	t_list* diccionario_elements(t_diccionario_mutex *diccionario);

	/**
	* @NAME: diccionario_destroy
	* @DESC: Destruye el diccionario
	*/
	void 		  diccionario_destroy(t_diccionario_mutex *);

	/**
	* @NAME: diccionario_destroy_and_destroy_elements
	* @DESC: Destruye el diccionario y destruye sus elementos
	*/
	void 		  diccionario_destroy_and_destroy_elements(t_diccionario_mutex *, void(*element_destroyer)(void*));

#endif /* DICCIONARIO_MONITOR_H_ */
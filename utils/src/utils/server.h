#ifndef SRC_UTILS_SERVER_H_
#define SRC_UTILS_SERVER_H_

#include "sockets.h"
#include <commons/log.h>

#define MAX_CLIENTS 10

extern t_log* logger;

typedef struct {
  int server_fd;
  void* (*handle_incomming_connection)(void*);
  void(*handle_connection)(int);
} t_server;

t_server* server_create(int listen_port);

/**
* @NAME: multiclient_server_create
* @DESC: Recibe un puerto de escucha, y una funcion para ejecutar con las conexiones entrantes.
*         Devuelve una estructura server que soporta multiples conexiones, utilizano hilos.
*/
t_server* multiclient_server_create(int listen_port, void* (*handle_incomming_connection)(void*));

/**
* @NAME: singleclient_server_create
* @DESC: Recibe un puerto de escucha, y una funcion para ejecutar con las conexiones entrantes.
*         Devuelve una estructura server que soporta una unica conexion.
*/
t_server* singleclient_server_create(int listen_port, void(*handle_incomming_connection)(int));

/**
* @NAME: server_start
* @DESC: Recibe una estructura server y se queda a la escucha de conexiones en dicho server.
*/
void server_start(t_server* server);

/**
* @NAME: server_stop
* @DESC: Recibe una estructura server y frena la escucha de conexiones en dicho server.
*/
void server_stop(t_server* server);

/**
* @NAME: server_destroy
* @DESC: Recibe una estructura server y libera los recursos previamente
*         utilizados porde la estructura.
*/
void server_destroy(t_server* server);

#endif /* SRC_UTILS_SERVER_H_ */
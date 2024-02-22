#ifndef SRC_UTILS_SOCKETS_H_
#define SRC_UTILS_SOCKETS_H_

#include <commons/log.h>
#include <commons/string.h>
#include <netdb.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

extern t_log* logger;

/**
* @NAME: create_server_socket
* @DESC: Recibe un puerto y crea un "file descriptor socket" de escucha 
*         para recibir conexiones. Se utiliza para los "servers". 
*         Retorna dicho "file descriptor socket".
*/
int create_server_socket(int listen_port);

/**
* @NAME: multi_client_server_loop
* @DESC: Recibe un "file descriptor socket" de escucha ya creado y una funcion 
*         para ejecutar cuando entre una nueva conexión. 
*         Se encarga del manejo de multiples conexiones entrantes usando threads (hilos).
*/
void multi_client_server_loop(int server_fd, void *(*handle_connection)(void *));

/**
* @NAME: single_client_server_loop
* @DESC: Recibe un "file descriptor socket" de escucha ya creado 
*         y una funcion para ejecutar cuando entre una nueva conexión.
*         Maneja una unica conexion entrante.
*/
void single_client_server_loop(int server_fd, void (*handle_connection)(int));

/**
* @NAME: create_conection
* @DESC: Recibe una IP y un puerto para crear una conexion hacia un servidor. 
*         Retorna un "file descriptor socket" con dicha conexion creada.
*/
int create_conection(char *ip, int puerto);

/**
* @NAME: liberar_conexion
* @DESC: Recibe un socket, y lo cierra libreando su conexion.
*/
void liberar_conexion(int socket_fd);

int enviar_payload(int socked_fd, void *buffer, size_t tamanio);
int recibir_payload(int socked_fd, void *buffer, size_t tamanio);

/**
* @NAME: handshake_client
* @DESC: Recibe un socket, y un hadshake(numero) y lo envia al servidor.
*/
int handshake_client(int socked_fd, uint32_t handshake);

/**
* @NAME: handshake_server
* @DESC: Recibe un socket, y un handshake esperado.
* Recibe un handshake del cliente.
* Devuelve 0 en caso de que coincida con el esperado, y -1 en caso contrario.
*/
int handshake_server(int socked_fd, uint32_t handshake_esperado);

#endif /* SRC_UTILS_SOCKETS_H_ */
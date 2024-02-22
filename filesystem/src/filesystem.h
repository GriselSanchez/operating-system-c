#ifndef SRC_FILESYSTEM_H_
#define SRC_FILESYSTEM_H_

#include "utils/server.h"
#include "utils/sockets.h"
#include "utils/serializacion.h"
#include "model/fcb.h"
#include "filesystem_config.h"
#include <commons/log.h>
#include "archivo.h"
#include "fat.h"
#include "bloques.h"
#include "swap.h"
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>

#define HANDSHAKE 567

typedef struct {
  t_server* server;
} t_filesystem;


t_filesystem* filesystem_create(t_filesystem_config* configs);

/**
* @NAME: filesystem_start
* @DESC: Recibe una estructura filesystem.
*/
void filesystem_start(t_filesystem* filesystem);

/**
* @NAME: filesystem_destroy
* @DESC: Recibe una estructura filesystem y libera los recursos previamente
*         utilizados por la estructura.
*/
void filesystem_destroy(t_filesystem* filesystem);


int filesystem_connect_memoria(t_filesystem* filesystem);

void* on_connection(void* arg);
void crear_directorio();

#endif /* SRC_FILESYSTEM_H_ */

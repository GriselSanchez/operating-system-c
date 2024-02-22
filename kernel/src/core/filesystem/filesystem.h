#ifndef SRC_CORE_FILESYSTEM__H_
#define SRC_CORE_FILESYSTEM__H_

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <commons/collections/dictionary.h>
#include <commons/collections/list.h>
#include "model/file.h"
#include "utils/serializacion.h"
#include "utils/sockets.h"
#include "core/filesystem/file.h"
#include "utils/diccionario_monitor.h"

//#include "kernel.h"

typedef struct {
    char* fs_host;
    int fs_port;
    t_diccionario_mutex* open_files;
} t_kernel_filesystem;

int _connect_to_filesystem(t_kernel_filesystem* filesystem, char* host, int port);

t_kernel_file* get_file_from_filesystem(t_kernel_filesystem* filesystem, char* file_name);

t_kernel_filesystem* filesystem_initialize(char* filesystem_host, int filesystem_port);

void remove_from_open_files(t_kernel_filesystem* filesystem, char* file_name);

bool _open_file(t_kernel_filesystem* filesystem, char* file_name, int* file_size);
int _create_file(t_kernel_filesystem* filesystem, char* file_name);
int _truncate_file(t_kernel_filesystem* filesystem, char* file_name, int new_size);
bool _write_file(t_kernel_filesystem* filesystem, char* file_name, int puntero, int dir_fisica, int pid);
bool _read_file(t_kernel_filesystem* filesystem, char* file_name, int puntero, int dir_fisica, int pid);

#endif /* SRC_CORE_FILESYSTEM__H_ */
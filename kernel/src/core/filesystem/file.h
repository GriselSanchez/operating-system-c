#ifndef _SRC_CORE_FILESYSTEM_FILE_H_
#define _SRC_CORE_FILESYSTEM_FILE_H_

#include "utils/cola_monitor.h"
#include <string.h>
#include "commons/log.h"

extern t_log* logger;

typedef enum {
    FILE_MODE_READ = 0,
    FILE_MODE_WRITE = 1
} t_mode;

typedef struct {
    t_mode mode;
    int pid;
} t_file_lock_request;

typedef struct {
    pthread_mutex_t mutex;
    int lock;
} t_read_lock;

typedef struct {
    pthread_mutex_t mutex;
    bool lock;
} t_write_lock;


typedef struct {
    char* nombre_archivo;
    int size;
    t_cola_mutex* read_write_queue; // t_file_lock_request
    t_write_lock active_write_lock;
    t_read_lock active_read_lock;
} t_kernel_file;

t_kernel_file* kernel_file_create(char* file_name);

bool file_open_in_read_mode(t_kernel_file* file, int pid);

bool file_open_in_write_mode(t_kernel_file* file, int pid);

void file_close_write_mode(t_kernel_file* file, t_queue* prox_procesos);

void file_close_read_mode(t_kernel_file* file, t_queue* prox_procesos);


#endif  /* _SRC_CORE_FILESYSTEM_FILE_H_ */
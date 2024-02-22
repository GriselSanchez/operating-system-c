#ifndef SRC_KERNEL_H_
#define SRC_KERNEL_H_

#include "utils/server.h"
#include "utils/diccionario_monitor.h"

#include "config.h"
#include "core/resources/resources.h"
#include "core/planners/planners.h"
#include "core/filesystem/filesystem.h"
#include "core/memory/memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct {
    int cpu_dispatch_fd;
    int cpu_interrupt_fd;
    int multiprogramming_degree;
    t_planner* short_term_planner;
    t_resources* resources;
    t_kernel_filesystem* filesystem;
    t_kernel_memory* memory;
    bool paused_planning;
} t_kernel;

void onConnection(int);
t_planner* create_short_term_planner(t_kernel_config* configs);
t_kernel* kernel_create(t_kernel_config*);
void start_planners(t_kernel*, t_kernel_config*);
void kernel_start_connections(t_kernel*, t_kernel_config*);
void finish_kernel(t_kernel*, t_log*, t_kernel_config*);

#endif /* SRC_KERNEL_H_ */
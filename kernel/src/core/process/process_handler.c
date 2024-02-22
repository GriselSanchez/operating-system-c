#include "process_handler.h"

#define NOT_SHARED 0
#define SHARED 1
#define STRING_EQUAL(b, c) strcmp(b, c) == 0
#define BLOCK_4EVER 0

typedef struct {
    t_planner* short_term_planner;
    int cpu_dispatch_fd;
    t_kernel_filesystem* filesystem;
    t_kernel_memory* memory;
} t_stp;

typedef struct {
    t_planner* short_term_planner;
    t_kernel_memory* memory;
} t_ltp;

typedef struct {
    int pid;
    op_code interruption_reason;
} t_interruption;

typedef struct {
    t_stp* stp;
    t_pcb* pcb;
    int page_fault;
} t_page_fault;

extern pthread_mutex_t pcb_status_mutex;

sem_t max_concurrent_multiprograming;
sem_t ready_process_event;
sem_t new_process_event;
sem_t sem_interruption;
sem_t sem_ltp_paused;
sem_t sem_stp_paused;
sem_t sem_page_fault;

pthread_mutex_t pause_planning_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ctx_mutex = PTHREAD_MUTEX_INITIALIZER;

t_cola_mutex* new_process_contexts;
t_lista_mutex* running_process_contexts;
t_cola_mutex* interruptions;
t_cola_mutex* io_events;
t_resources* resources;

// aux functions
void process_block_resource(t_pcb* pcb, t_resource* requested_resource);
void* pids_concat(void* seed, void* elem);
char* list_toString(t_lista_mutex* list);
char* non_mutex_list_toString(t_list*);
t_list* list_processes_at_ready();
t_list* list_processes_at_exec();
t_list* list_processes_at_block();
t_list* list_processes_at_exit();
t_pcb* find_process(int pid);
void exit_process(t_pcb* pcb, int exit_code, t_kernel_memory* memory);
void force_exit_process(t_pcb* pcb, int exit_code, t_kernel_memory* memory, t_planner* short_term_planner);
void ready_process(t_planner* planner, t_pcb* pcb);
bool is_exit_op_code(op_code opc);
bool is_planner_eviction(op_code opc);
void run_in_detached_thread(void* (*func)(void*), void* param);
void wait_for_thread(void* (*func)(void*), t_pcb* process, op_code* result);

void update_max_multi_programming(int new_max_mp) {
    log_info(logger, "Grado Anterior: %d - Grado Actual: %d", kernel->multiprogramming_degree, new_max_mp);
    //Aca accedo a una variable global muy villeramente.
    int max_mp = kernel->multiprogramming_degree;
    int diff_mp = max_mp - new_max_mp;
    if (diff_mp < 0) {
        log_debug(logger, "Incremento el grado de maxima multiprogramación a %i", new_max_mp);
        //incremento semaforo
        for (int i = 0; i < (-1 * diff_mp); i++) {
            sem_post(&max_concurrent_multiprograming);
        }
        //Aca accedo a una variable global muy villeramente.
        kernel->multiprogramming_degree = new_max_mp;
    } else if (diff_mp > 0) {
        log_debug(logger, "Decremento el grado de maxima multiprogramación a %i", new_max_mp);
        //decremento semaforo
        for (int i = 0; i < diff_mp; i++) {
            sem_wait(&max_concurrent_multiprograming);
        }
        //Aca accedo a una variable global muy villeramente.
        kernel->multiprogramming_degree = new_max_mp;
    } else {
        log_debug(logger, "No se modifica el grado de maxima multiprogramación.");
    }
}

void handle_new_process(char* path, int size, int priority) {
    t_pcb* new_process = process_new(path, size, priority);
    queue_push_mutex(new_process_contexts, new_process);
    sem_post(&new_process_event);
}

void pause_planning() {
    pthread_mutex_lock(&pause_planning_mutex);
    if (!kernel->paused_planning) {
        log_info(logger, "PAUSA DE PLANIFICACIÓN");
        kernel->paused_planning = true;
    } else {
        log_info(logger, "PLANIFICACION YA PAUSADA");
    }
    pthread_mutex_unlock(&pause_planning_mutex);
}

void start_planning() {
    pthread_mutex_lock(&pause_planning_mutex);
    if (kernel->paused_planning) {
        log_info(logger, "INICIO DE PLANIFICACIÓN");

        sem_post(&sem_ltp_paused);
        sem_post(&sem_stp_paused);

        kernel->paused_planning = false;
    } else {
        log_info(logger, "PLANIFICACION YA INICIADA");
    }
    pthread_mutex_unlock(&pause_planning_mutex);
}

void listar_procesos_por_estado() {
    log_info(logger, "Estado: NEW - Procesos: %s", non_mutex_list_toString(new_process_contexts->cola->elements));

    t_list* list_readys_to_print = list_processes_at_ready();
    log_info(logger, "Estado: READY - Procesos: %s", non_mutex_list_toString(list_readys_to_print));

    t_list* list_exec_to_print = list_processes_at_exec();
    log_info(logger, "Estado: EXEC - Procesos: %s", non_mutex_list_toString(list_exec_to_print));

    t_list* list_block_to_print = list_processes_at_block();
    log_info(logger, "Estado: BLOCK - Procesos: %s", non_mutex_list_toString(list_block_to_print));

    list_destroy(list_readys_to_print);
    list_destroy(list_exec_to_print);
    list_destroy(list_block_to_print);
}

void assign_resource_to_queued_process(t_planner* planner, t_resources* resources, t_resource* resource) {
    //BUSCA EL RECURSO EN LA LISTA DE RESOURCES Y HACE POP DE LA COLA DE BLOQUEADOS DE ESE RECURSO EL ID DEL PROCESO    
    int pid = resources_next_pid(resources, resource->resource_name);
    if (pid > 0) {
        t_pcb* pcb_to_ready = find_process(pid);
        if (pcb_to_ready != NULL) {
            sub_resource_instances_available(resource);
            process_assign_resource(pcb_to_ready, resource->resource_name);
            ready_process(planner, pcb_to_ready);
            log_debug(logger, "RELEASED %s ASSIGNED TO PID %d", resource->resource_name, pcb_to_ready->pid); //ahora este proceso tiene asignado el recurso
            // log_debug(logger, "Desbloqueado PID %d, que estaba bloqueado por recurso %s", pcb_to_ready->pid, resource->resource_name);
        }
    }
}

void release_resource(t_planner* short_term_planner, t_kernel_memory* memory, t_pcb* pcb, char* resource_name) {
    t_resource* resource;
    //LIBERO UN RECURSO
    int resource_available = resources_release(resources, resource_name, &resource);

    if (resource_available >= 0) {
        log_debug(logger, "release_resource antes de release - PID: %d - Signal: %s - Instancias: %d", pcb->pid, resource->resource_name, resource->available_instances);
        add_resource_instances_available(resource);
        //REMUEVO EL RECURSO DEL PROCESO (busco el pcb y le saco el recurso)
        process_remove_resource(pcb, resource_name);
        log_info(logger, "PID: %d - Signal: %s - Instancias: %d", pcb->pid, resource->resource_name, resource->available_instances);
        if (resource_available == 0) {
            assign_resource_to_queued_process(short_term_planner, resources, resource);
        }
    } else if (resource_available < 0) {
        log_info(logger, "PID: %d - Signal on not defined resource: %s", pcb->pid, resource_name);
        short_term_planner->remove_process(short_term_planner, pcb);
        exit_process(pcb, PROC_EXIT_RESOURCE_NOT_FOUND, memory);
    }
}

void onSignal(t_stp* stp, t_pcb* pcb, char* resource_name) {
    release_resource(stp->short_term_planner, stp->memory, pcb, resource_name);
}

/**
 * A la hora de recibir de la CPU un Contexto de Ejecución desalojado por WAIT, el Kernel deberá
 * verificar primero que exista el recurso solicitado y en caso de que exista restarle 1 a la
 * cantidad de instancias del mismo. En caso de que el número sea estrictamente menor a 0,
 * el proceso que realizó WAIT se bloqueará en la cola de bloqueados correspondiente al recurso.
 * **/


void onWait(t_stp* stp, t_pcb* pcb, char* resource_name) {
    t_resource* requested_resource;
    //process_block(pcb, NULL, BLOCK_4EVER); --> tiene que mandar a la cola de bloqueados solo si el recurso no esta disponible.
    int available_resource = resources_request(resources, resource_name, pcb->pid, &requested_resource);

    // devuelve  0 si el recurso esta disponible
    if (available_resource >= 0) {
        log_debug(logger, "On Wait Antes de asignar - PID: %d - Wait: %s - Instancias: %d", pcb->pid, requested_resource->resource_name, requested_resource->available_instances);

        //recurso existente
        if (available_resource == 0) {
            //recurso disponible
            log_info(logger, "PID: %d - ASSIGNED %s", pcb->pid, requested_resource->resource_name);
            sub_resource_instances_available(requested_resource);
            process_assign_resource(pcb, resource_name);
            log_info(logger, "PID: %d - Wait: %s - Instancias: %d", pcb->pid, requested_resource->resource_name, requested_resource->available_instances);

            ready_process(stp->short_term_planner, pcb);
        } else {
            // recurso no disponible
            process_block(pcb);
            pcb->blocked_by_resource_name = strdup(resource_name);
            log_info(logger, "PID: %d - Wait: %s - Instancias: %d", pcb->pid, requested_resource->resource_name, requested_resource->available_instances);
            checkForDeadlock(pcb, resource_name, running_process_contexts);
            log_info(logger, "PID: %d - Bloqueado por: %s", pcb->pid, resource_name);
        }
    } else {
        log_info(logger, "PID: %d - Wait on not defined resource: %s", pcb->pid, resource_name);
        stp->short_term_planner->remove_process(stp->short_term_planner, pcb);
        exit_process(pcb, PROC_EXIT_RESOURCE_NOT_FOUND, stp->memory);
    }
}

bool open_file(t_kernel_file* file, t_pcb* pcb, modo_apertura mode) {
    if (mode == R) {
        return file_open_in_read_mode(file, pcb->pid);
    } else if (mode == W) {
        return file_open_in_write_mode(file, pcb->pid);
    } else {
        //Este caso no se deberia dar jamas en el TP.
        log_error(logger, "Unknown mode.");
        return false;
    }
}

void onFileOpen(t_stp* stp, t_pcb* pcb, char* file_name, modo_apertura mode) {
    log_info(logger, "PID: %i - Abrir Archivo: %s - Modo: %i", pcb->pid, file_name, mode);
    t_kernel_file* file = get_file_from_filesystem(stp->filesystem, file_name);
    if (file != NULL) {
        if (open_file(file, pcb, mode)) {
            process_assign_open_file(pcb, file_name, mode);
        } else {
            log_info(logger, "PID: %i - Se bloquea el proceso para abrir archivo: %s", pcb->pid, file_name);
            // DUDA: ver si es necesario que sea una lista, creo que como max solo va a haber 1 por proceso
            process_assign_pending_file(pcb, file_name, mode);
            process_block(pcb);
        }
    }
}

void onFileClose(t_stp* stp, t_pcb* pcb, char* file_name) {
    log_info(logger, "PID: %i - Cerrar Archivo: %s", pcb->pid, file_name);
    t_kernel_file* file = get_file_from_filesystem(stp->filesystem, file_name);
    if (file != NULL) {
        modo_apertura mode;
        if (process_has_open_file(pcb, file_name, &mode)) {
            t_queue* proximos_procesos = queue_create();
            process_remove_open_file(pcb, file_name);
            if (mode == W) {
                file_close_write_mode(file, proximos_procesos);
            } else if (mode == R) {
                file_close_read_mode(file, proximos_procesos);
            }

            if (queue_size(proximos_procesos) == 0 && queue_size_mutex(file->read_write_queue) == 0) {
                remove_from_open_files(stp->filesystem, file->nombre_archivo);
            }

            while (queue_size(proximos_procesos) > 0) {
                int next_pid = (int)queue_pop(proximos_procesos); //no  hacer free
                t_pcb* process = find_process(next_pid);
                t_file* pending_file = process->pending_file;
                if (pending_file != NULL) {
                    process_assign_open_file(process, pending_file->nombre_archivo, pending_file->modo_apertura);
                    ready_process(stp->short_term_planner, process);
                }
            }

        }
    }
}

void onFileSeek(t_stp* stp, t_pcb* pcb, char* file_name, int puntero) {
    t_file* archivo = _process_find_open_file(pcb, file_name);
    if (archivo == NULL) {
        log_error(logger, "No se encontro el archivo en la tabla del proceso: %s", file_name);

    } else {
        archivo->puntero_archivo = puntero;
        log_info(logger, "PID: %i - Actualizar Puntero Archivo: %s - Puntero: %i", pcb->pid, file_name, archivo->puntero_archivo);
        ready_process(stp->short_term_planner, pcb);
    }
}

void onFileTruncate(t_stp* stp, t_pcb* pcb, char* file_name, int size) {
    t_file* archivo = _process_find_open_file(pcb, file_name);
    if (archivo == NULL) {
        log_error(logger, "No se encontro el archivo en la tabla del proceso: %s", file_name);
    } else {
        process_block(pcb);
        t_kernel_file* kernel_file = get_file_from_filesystem(stp->filesystem, file_name);

        if (kernel_file == NULL) {
            log_error(logger, "No se encontro el archivo en la tabla global: %s", file_name);
        } else {
            int nuevo_tamanio = _truncate_file(stp->filesystem, file_name, size);
            kernel_file->size = nuevo_tamanio;

            log_info(logger, "PID: %i - Archivo: %s - Tamaño: %i", pcb->pid, kernel_file->nombre_archivo, kernel_file->size);
        }

        ready_process(stp->short_term_planner, pcb);
    }
}

void onFileRead(t_stp* stp, t_pcb* pcb, char* file_name, int dir_fisica) {
    t_file* archivo = _process_find_open_file(pcb, file_name);
    if (archivo == NULL) {
        log_error(logger, "No se encontro el archivo en la tabla del proceso: %s", file_name);
    } else {
        process_block(pcb);

        if (_read_file(stp->filesystem, file_name, archivo->puntero_archivo, dir_fisica, pcb->pid)) {
            t_kernel_file* kernel_file = get_file_from_filesystem(stp->filesystem, file_name);

            if (kernel_file == NULL) {
                log_error(logger, "No se encontro el archivo en la tabla global: %s", file_name);
            } else {
                log_info(logger, "PID: %i - Leer Archivo: %s - Puntero: %i - Dirección Memoria: %i- Tamaño: %i", pcb->pid, kernel_file->nombre_archivo, archivo->puntero_archivo, dir_fisica, kernel_file->size);
            }
        } else {
            log_error(logger, "Error al Leer Archivo: %s", file_name);
        }

        ready_process(stp->short_term_planner, pcb);
    }
}

void onFileWrite(t_stp* stp, t_pcb* pcb, char* file_name, int dir_fisica) {
    t_file* archivo = _process_find_open_file(pcb, file_name);
    if (archivo == NULL) {
        log_error(logger, "No se encontro el archivo en la tabla del proceso: %s", file_name);
    } else {
        if (archivo->modo_apertura == W) {
            process_block(pcb);

            if (_write_file(stp->filesystem, file_name, archivo->puntero_archivo, dir_fisica, pcb->pid)) {
                t_kernel_file* kernel_file = get_file_from_filesystem(stp->filesystem, file_name);

                if (kernel_file == NULL) {
                    log_error(logger, "No se encontro el archivo en la tabla global: %s", file_name);
                } else {
                    log_info(logger, "PID: %i - Escribir Archivo: %s - Puntero: %i - Dirección Memoria: %i- Tamaño: %i", pcb->pid, kernel_file->nombre_archivo, archivo->puntero_archivo, dir_fisica, kernel_file->size);
                }

            } else {
                log_error(logger, "Error al Escribir Archivo: %s", file_name);
            }

            ready_process(stp->short_term_planner, pcb);
        } else {
            log_info(logger, "Modo de apertura invalido para PID %i - Archivo %s", pcb->pid, archivo->nombre_archivo);
            handle_planner_evict_process(stp->short_term_planner, pcb, PROC_EXIT_INVALID_WRITE);
        }
    }
}

void onSleep(t_planner* planner, t_pcb* pcb, int block_time) {
    log_info(logger, "PID: %d - Bloqueado por SLEEP", pcb->pid);
    process_block_sleep(pcb, planner, block_time);
}

void onExit(t_stp* stp, t_pcb* pcb, int exit_code) {
    force_exit_process(pcb, PROC_EXIT_FORCE, stp->memory, stp->short_term_planner);
    stp->short_term_planner->remove_process(stp->short_term_planner, pcb);
    exit_process(pcb, exit_code, stp->memory);
}

void onEvicted(t_planner* planner, t_pcb* pcb) {
    pthread_mutex_lock(&pcb_status_mutex);
    if (pcb != NULL && pcb->current_status != EXIT) {
        pthread_mutex_unlock(&pcb_status_mutex);
        ready_process(planner, pcb);
    } else {
        pthread_mutex_unlock(&pcb_status_mutex);
    }
}

void request_memory_to_load_page(void* arg) {
    t_page_fault* pf = (t_page_fault*)arg;
    int page_to_load = pf->page_fault;
    int result = memory_load_page(pf->pcb, page_to_load, pf->stp->memory);
    if (result >= 0) { //OK
        ready_process(pf->stp->short_term_planner, pf->pcb);
    } else {
        //ON ERROR QUE HAGO ???
    }
    free(pf);
    sem_post(&sem_page_fault);
}

void onPageFault(t_stp* stp, t_pcb* pcb, int page_to_load) {
    sem_wait(&sem_page_fault);
    log_info(logger, "Page Fault PID: %i - Pagina: %i", pcb->pid, page_to_load);
    //distinguir el BLOCK por PAGE FAULT ??
    process_block(pcb);
    t_page_fault* pf = malloc(sizeof(t_page_fault));
    pf->stp = stp;
    pf->page_fault = page_to_load;
    pf->pcb = pcb;
    run_in_detached_thread((void*)request_memory_to_load_page, (void*)pf);
}

//handle_force_process_exit | handle_exit_force_process
void handle_force_process_exit(t_planner* short_term_planner, t_kernel_memory* memoria, int pid) {
    t_pcb* pcb = find_process(pid);
    if (pcb != NULL) {
        pthread_mutex_lock(&pcb_status_mutex);
        if (pcb->current_status == EXEC) {
            pthread_mutex_unlock(&pcb_status_mutex);
            //desalojar de la cpu
            t_interruption* intp = malloc(sizeof(t_interruption));
            intp->pid = pid;
            intp->interruption_reason = PROC_EXIT_FORCE;
            queue_push_mutex(interruptions, intp);
            sem_post(&sem_interruption);
        } else {
            pthread_mutex_unlock(&pcb_status_mutex);
            force_exit_process(pcb, PROC_EXIT_FORCE, memoria, short_term_planner);
            short_term_planner->remove_process(short_term_planner, pcb);
            exit_process(pcb, PROC_EXIT_FORCE, memoria);
        }
    } else {
        log_debug(logger, "No se encontró el ID de proceso a finalizar. PID: %i", pid);
    }
}

void handle_planner_evict_process(t_planner* short_term_planner, t_pcb* pcb, op_code int_reason) {
    if (pcb == NULL) {
        log_error(logger, "PCB no valido");
        return;
    };
    if (pcb->current_status == EXEC) {
        //desalojar de la cpu
        t_interruption* intp = malloc(sizeof(t_interruption));
        intp->pid = pcb->pid;
        intp->interruption_reason = int_reason;
        queue_push_mutex(interruptions, intp);
        log_debug(logger, "PID: %d - evict process - reason: %d", pcb->pid, int_reason);
        sem_post(&sem_interruption);
    }
}

void force_exit_process(t_pcb* pcb, int exit_code, t_kernel_memory* memoria, t_planner* short_term_planner) {
    void free_resource(void* pcb_resource) {
        char* resource_name = (char*)pcb_resource;
        release_resource(short_term_planner, memoria, pcb, resource_name);
        //free(resource_name);
    }

    void* duplicate(void* resource) {
        return string_duplicate((char*)resource);
    }

    // sacar los recursos e ir llamando cada recurso y liberar
    t_list* assigned_resources = list_map(pcb->asigned_resources, &duplicate);
    if (!list_is_empty(assigned_resources)) {
        list_iterate(assigned_resources, &free_resource);
    }

    list_destroy_and_destroy_elements(assigned_resources, &free);
}

void exit_process(t_pcb* pcb, int exit_code, t_kernel_memory* memoria) {
    pcb->exit_code = exit_code;
    list_remove_element_mutex(running_process_contexts, pcb);
    memory_terminate_process(pcb, memoria);

    // release
    if (pcb != NULL) process_release(pcb);
}

void ready_process(t_planner* planner, t_pcb* pcb) {
    process_ready(pcb, planner);
    //TODO : MAXO : Este sem_post quiza deberia hacerse adentro del planificador (planner->to_ready) ???
    sem_post(&ready_process_event);
}

ssize_t evict_process_from_cpu(int cpu_int_fd, int proces_id) {
    //Si hay mas motivos de desalojo de la cpu deberia mandar un codigo de motivo,
    // ademas del PID a desalojar.
    return send(cpu_int_fd, &proces_id, sizeof(proces_id), 0);
}

t_exec_context* cpu_execute(t_stp* stp, t_pcb* pcb, op_code* eviction_reason) {
    t_exec_context* ctx_to_send = context_from_pcb(pcb);
    send_execution_context_via_socket(ctx_to_send, CONTEXTO_EJECUCION, stp->cpu_dispatch_fd);
    destroy_exec_context(ctx_to_send);
    return recv_execution_context_via_socket(stp->cpu_dispatch_fd, eviction_reason);
}

void handle_cpu_eviction(t_stp* stp, op_code eviction_reason, t_pcb* pcb, t_exec_context* ctx) {
    if (pcb == NULL) {
        log_error(logger, "PCB no valido");
        return;
    }
    if (eviction_reason == WAIT) {
        onWait(stp, pcb, ctx->resource_name);
    } else if (eviction_reason == SIGNAL) {
        onSignal(stp, pcb, ctx->resource_name);
    } else if (eviction_reason == SLEEP) {
        onSleep(stp->short_term_planner, pcb, ctx->block_time);
    } else if (is_planner_eviction(eviction_reason)) {
        onEvicted(stp->short_term_planner, pcb);
    } else if (eviction_reason == PAGE_FAULT) {
        onPageFault(stp, pcb, ctx->page_fault_nro);
    } else if (eviction_reason == F_OPEN) {
        onFileOpen(stp, pcb, ctx->nombre_archivo, ctx->modo_apertura);
    } else if (eviction_reason == F_CLOSE) {
        onFileClose(stp, pcb, ctx->nombre_archivo);
    } else if (eviction_reason == F_SEEK) {
        onFileSeek(stp, pcb, ctx->nombre_archivo, ctx->puntero_archivo);
    } else if (eviction_reason == F_READ) {
        onFileRead(stp, pcb, ctx->nombre_archivo, ctx->dir_fisica);
    } else if (eviction_reason == F_WRITE) {
        onFileWrite(stp, pcb, ctx->nombre_archivo, ctx->dir_fisica);
    } else if (eviction_reason == F_TRUNCATE) {
        onFileTruncate(stp, pcb, ctx->nombre_archivo, ctx->tamanio_archivo);
    } else if (is_exit_op_code(eviction_reason)) {
        onExit(stp, pcb, eviction_reason);
    } else {
        log_error(logger, "Unknown CPU eviction reason: %d", eviction_reason);
        //deberia explotar.
    }

    // log_info(logger, "PID: %i - Interrupt motive %d", pcb->pid, ctx->interrupt_motive);
    // if (ctx->interrupt_motive != -1 && is_planner_eviction(ctx->interrupt_motive)) {
    //     // onEvicted(stp->short_term_planner, pcb);
    //     ctx->interrupt_motive = -1;
    // }
}

void short_term_planner(t_stp* stp) {
    t_planner* planner = stp->short_term_planner;
    log_debug(logger, "Short term planner selected: %s", planner->name);

    for (;;) {
        sem_wait(&ready_process_event);
        if (kernel->paused_planning) sem_wait(&sem_stp_paused);

        t_pcb* process = planner->next_process(planner, running_process_contexts);
        if (process != NULL) {
            process_execute(process);

            while (process != NULL && process->current_status == EXEC) {
                op_code eviction_reason;
                if (string_equals_ignore_case(planner->name, "RR")) planner->init_quantum(planner, process);
                t_exec_context* cpu_exec_ctx = cpu_execute(stp, process, &eviction_reason);
                //Ojo aca estoy actualizando el PCB directamente en READYS ??? Sino actualizarlo en la lista de readys !
                update_pcb_from_context(cpu_exec_ctx, process);
                if (kernel->paused_planning) sem_wait(&sem_stp_paused);
                handle_cpu_eviction(stp, eviction_reason, process, cpu_exec_ctx);
                if (process->is_finished) {
                    process_kill(&process);
                }
                free(cpu_exec_ctx);
            }
        } else {
            log_warning(logger, "Ojo planner->next_process(..) devolvio un process NULL !!");
        }
    }
}

void start_short_term_planner(int cpu_dispatch_fd, t_kernel_memory* memory, t_planner* planner, t_kernel_filesystem* filesystem) {
    sem_init(&sem_stp_paused, SHARED, 0);
    sem_init(&ready_process_event, SHARED, 0);
    sem_init(&sem_page_fault, SHARED, 1);
    running_process_contexts = list_init_mutex();
    t_stp* stp = malloc(sizeof(t_stp));
    stp->short_term_planner = planner;
    stp->cpu_dispatch_fd = cpu_dispatch_fd;
    stp->filesystem = filesystem;
    stp->memory = memory;
    run_in_detached_thread((void*)short_term_planner, (void*)stp);
}

void handle_interruptions(void* param) {
    int cpu_interrupt_fd = *((int*)param);
    interruptions = queue_init_mutex();
    sem_init(&sem_interruption, 1, 0);

    while (1) {
        // Send interrupt process to cpu
        sem_wait(&sem_interruption);
        t_interruption* intp = queue_pop_mutex(interruptions);
        t_paquete* paquete = crear_paquete(INTERRUPCION);
        agregar_payload_a_paquete(paquete, &intp->pid, sizeof(int));
        agregar_payload_a_paquete(paquete, &intp->interruption_reason, sizeof(op_code));
        enviar_paquete_serializado_por_socket(cpu_interrupt_fd, paquete);
        paquete_destroy(paquete);
        //sem_post(&ready_process_event);
    }
}

// Create thread for handle interruptions
void start_interruptions_handler(int cpu_interruption_fd) {
    int* interrupt_fd = malloc(sizeof(int));
    *interrupt_fd = cpu_interruption_fd;
    run_in_detached_thread((void*)handle_interruptions, (void*)interrupt_fd);
}

void long_term_planner(t_ltp* ltp) {
    t_planner* short_planner = ltp->short_term_planner;
    for (;;) {
        sem_wait(&new_process_event);
        sem_wait(&max_concurrent_multiprograming);
        if (kernel->paused_planning) sem_wait(&sem_ltp_paused);

        t_pcb* process = queue_pop_mutex(new_process_contexts);
        if (memory_allocate_new_process(process, ltp->memory) == 1) {
            log_error(logger, "Error on allocate memory for process %d", process->pid);
        } else {
            t_list* list_readys_to_print = list_processes_at_ready();
            log_info(logger, "Cola Ready %s: [%s]", short_planner->name, non_mutex_list_toString(list_readys_to_print));
            list_add_mutex(running_process_contexts, process);
            ready_process(short_planner, process);
        }
    }
}

void start_long_term_planner(int max_multiprogramming, t_planner* st_planner, t_kernel_memory* memoria) {
    if (sem_init(&max_concurrent_multiprograming, SHARED, max_multiprogramming) != 0) {
        // rompo con de todo!!!!
    }
    t_ltp* ltp = malloc(sizeof(t_ltp));
    ltp->short_term_planner = st_planner;
    ltp->memory = memoria;

    sem_init(&new_process_event, SHARED, 0);
    sem_init(&sem_ltp_paused, SHARED, 0);
    new_process_contexts = queue_init_mutex();
    run_in_detached_thread((void*)long_term_planner, (void*)ltp);
}

void destroy_proccess_handler() {
    queue_destroy_mutex(new_process_contexts);
    list_destroy_mutex(running_process_contexts);
}

// aux functions
void* pids_concat(void* seed, void* elem) {
    t_pcb* pcb = (t_pcb*)elem;

    return string_is_empty((char*)seed) ?
        string_itoa(pcb->pid) :
        string_from_format("%s, %d", (char*)seed, pcb->pid);
}

char* list_toString(t_lista_mutex* list) {
    return list_fold_mutex(list, "", &pids_concat);
}

void run_in_detached_thread(void* (*func)(void*), void* param) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, func, param);
    pthread_detach(thread_id);
}

void wait_for_thread(void* (*func)(void*), t_pcb* process, op_code* result) {
    pthread_t thread_id;
    pthread_create(&thread_id, NULL, func, (void*)process);
    pthread_join(thread_id, (void*)result);
}

bool is_exit_op_code(op_code opc) {
    return opc == PROC_EXIT_SEG_FAULT ||
        opc == PROC_EXIT_OUT_OF_MEMORY ||
        opc == PROC_EXIT_FORCE ||
        opc == PROC_EXIT_SUCCESS ||
        opc == PROC_EXIT_INVALID_WRITE ||
        opc == PROC_EXIT_RESOURCE_NOT_FOUND;
}

bool is_planner_eviction(op_code opc) {
    return opc == PROC_EXIT_QUANTUM ||
        opc == PROC_EXIT_PRIORITY;
}

char* non_mutex_list_toString(t_list* list) {
    return list_fold(list, "", &pids_concat);
}

t_list* list_processes_at_ready() {
    bool _filter(void* pcb) {
        return ((t_pcb*)pcb)->current_status == READY;
    }

    t_list* result = list_filter_mutex(running_process_contexts, &_filter);
    return result;
}

t_list* list_processes_at_exec() {
    bool _filter(void* pcb) {
        return ((t_pcb*)pcb)->current_status == EXEC;
    }
    return list_filter_mutex(running_process_contexts, &_filter);
}

t_list* list_processes_at_block() {
    bool _filter(void* pcb) {
        return ((t_pcb*)pcb)->current_status == BLOCK;
    }
    return list_filter_mutex(running_process_contexts, &_filter);
}

t_pcb* find_process(int pid) {
    bool _filter(void* pcb) {
        return (_Bool)(((t_pcb*)pcb)->pid == pid);
    }
    t_pcb* process;
    process = list_find_mutex(running_process_contexts, &_filter);
    return process;
}


// FUNCION QUE SE USA EN COLA DE SLEEP
void _simulate_process_block(void* param) {
    //hilo para el tiempo se espera --sleep_thread_params
    t_block_params* params = (t_block_params*)param;
    log_debug(logger, "PID: %d - Bloqueado por %d seg.", params->pcb->pid, params->block_time);
    sleep(params->block_time);
    log_debug(logger, "PID: %d - Ya pasaron los %d seg.", params->pcb->pid, params->block_time);
    process_ready(params->pcb, params->planner);
    sem_post(&ready_process_event);
    free(params);
}

//** METODOS COLA SLEEP **//
void process_block_sleep(t_pcb* process_context, t_planner* planner, int block_time) {
    //log_info(logger, "PID: %d - Bloqueado por: <SLEEP | NOMBRE_RECURSO | NOMBRE_ARCHIVO>", 
    //                  process_context->pid);

    process_block(process_context);
    if (block_time > 0) {
        pthread_t thread_id;
        t_block_params* params = malloc(sizeof(t_block_params));
        params->pcb = process_context;
        params->planner = planner;
        params->block_time = block_time;
        log_info(logger, "PID: %d - Ejecuta SLEEP: %d", process_context->pid, block_time);
        pthread_create(&thread_id, NULL, (void*)&_simulate_process_block, (void*)params);
        pthread_detach(thread_id);
    }
}

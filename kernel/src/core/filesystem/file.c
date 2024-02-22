#include "file.h"

void _initialize_write_lock(t_write_lock* w_lock) {
    pthread_mutex_init(&(w_lock->mutex), NULL);
    w_lock->lock = false;
}

void _initialize_read_lock(t_read_lock* r_lock) {
    pthread_mutex_init(&(r_lock->mutex), NULL);
    r_lock->lock = 0;
}

t_kernel_file* kernel_file_create(char* file_name) {
    t_kernel_file* new_file = malloc(sizeof(t_kernel_file));
    new_file->nombre_archivo = strdup(file_name);
    _initialize_write_lock(&(new_file->active_write_lock));
    _initialize_read_lock(&(new_file->active_read_lock));
    new_file->read_write_queue = queue_init_mutex(new_file->read_write_queue);
    return new_file;
}

bool _is_write_lock_active(t_kernel_file* file) {
    bool lock;
    pthread_mutex_lock(&(file->active_write_lock.mutex));
    lock = file->active_write_lock.lock;
    pthread_mutex_unlock(&(file->active_write_lock.mutex));

    log_info(logger, "Archivo %s - Lock de escritura valor actual: %i", file->nombre_archivo, lock);

    return lock;
    //return queue_size_mutex(file->read_write_queue) > 0;
}

void _queue_lock(t_kernel_file* file, int pid, t_mode mode) {
    t_file_lock_request* lock = malloc(sizeof(t_file_lock_request));
    lock->pid = pid;
    lock->mode = mode;
    log_info(logger, "PID %i - Archivo %s - Se agrega un lock de %s a la cola", pid, file->nombre_archivo, mode == FILE_MODE_READ ? "lectura" : "escritura");
    queue_push_mutex(file->read_write_queue, lock);
}

bool _lock_file_for_writing(t_kernel_file* file) {
    pthread_mutex_lock(&(file->active_write_lock.mutex));
    file->active_write_lock.lock = true;
    pthread_mutex_unlock(&(file->active_write_lock.mutex));

    log_info(logger, "Archivo %s - Se activa lock de escritura del archivo", file->nombre_archivo);
    return true;
}

void _unlock_file_for_writing(t_kernel_file* file) {
    pthread_mutex_lock(&(file->active_write_lock.mutex));
    file->active_write_lock.lock = false;
    pthread_mutex_unlock(&(file->active_write_lock.mutex));

    log_info(logger, "Archivo %s - Se desactiva lock de escritura del archivo", file->nombre_archivo);
}

bool _is_read_lock_active(t_kernel_file* file) {
    bool lock;
    pthread_mutex_lock(&(file->active_read_lock.mutex));
    // quiza deba ser una lista de pids.
    lock = file->active_read_lock.lock > 0;
    pthread_mutex_unlock(&(file->active_read_lock.mutex));

    log_info(logger, "Archivo %s - Lock de lectura valor actual: %i", file->nombre_archivo, lock);
    return lock;
}

void _lock_file_for_reading(t_kernel_file* file) {
    pthread_mutex_lock(&(file->active_read_lock.mutex));
    ++file->active_read_lock.lock;
    pthread_mutex_unlock(&(file->active_read_lock.mutex));

    log_info(logger, "Archivo %s - Se activa lock de lectura del archivo", file->nombre_archivo);
    //return lock;
}

void _add_participant_to_reading_lock(t_kernel_file* file) {
    // hacerlo thread safe !!!
    pthread_mutex_lock(&(file->active_read_lock.mutex));
    ++file->active_read_lock.lock;
    pthread_mutex_unlock(&(file->active_read_lock.mutex));

    log_info(logger, "Archivo %s - Se agrega participante en lock de lectura", file->nombre_archivo);
    //return file->active_read_lock++;
}

void _remove_participant_from_read_lock(t_kernel_file* file) {
    // hacerlo thread safe !!!
    pthread_mutex_lock(&(file->active_read_lock.mutex));
    --file->active_read_lock.lock;
    pthread_mutex_unlock(&(file->active_read_lock.mutex));

    log_info(logger, "Archivo %s - Se saca participante en lock de lectura", file->nombre_archivo);
    //return file->active_read_lock--;
}


bool file_open_in_read_mode(t_kernel_file* file, int pid) {
    bool status;
    if (_is_write_lock_active(file)) {
        _queue_lock(file, pid, FILE_MODE_READ);

        status = false;
    } else if (_is_read_lock_active(file)) {
        _add_participant_to_reading_lock(file);
        status = true;
    } else {
        _lock_file_for_reading(file);
        status = true;
    }
    return status;
}


bool file_open_in_write_mode(t_kernel_file* file, int pid) {
    bool status;
    if (_is_write_lock_active(file)) {
        _queue_lock(file, pid, FILE_MODE_WRITE);
        status = false;
    } else {
        _lock_file_for_writing(file);
        status = true;
    }
    return status;
}


void desencolar_y_dar_lugar_al_siguiente_lock(t_kernel_file* file, t_queue* prox_procesos) {
    t_file_lock_request* next = NULL;

    // Encolo todos los de lectura que tenga pendientes ya que pueden abrirse al mismo tiempo
    // Si encuentro primero un lock de escritura, solo encolo este ya que es exclusivo
    while (queue_size_mutex(file->read_write_queue) > 0) {
        next = queue_peek_mutex(file->read_write_queue);

        if (next != NULL) {
            if (next->mode == FILE_MODE_READ) {
                _lock_file_for_reading(file);
                void* next_pid = (void*)next->pid;
                queue_push(prox_procesos, next_pid);
                queue_pop_mutex(file->read_write_queue);
            } else if (next->mode == FILE_MODE_WRITE) {
                if (queue_size(prox_procesos) == 0) {
                    _lock_file_for_writing(file);
                    void* next_pid = (void*)next->pid;
                    queue_push(prox_procesos, next_pid);
                    queue_pop_mutex(file->read_write_queue);
                }
                break;
            }
        }
    }

    //if (next != NULL) free(next);
}

void file_close_write_mode(t_kernel_file* file, t_queue* prox_procesos) {
    _unlock_file_for_writing(file);
    if (!_is_write_lock_active(file)) {
        desencolar_y_dar_lugar_al_siguiente_lock(file, prox_procesos);
    }
}

void file_close_read_mode(t_kernel_file* file, t_queue* prox_procesos) {
    _remove_participant_from_read_lock(file);
    if (!_is_read_lock_active(file)) {
        desencolar_y_dar_lugar_al_siguiente_lock(file, prox_procesos);
    }
}
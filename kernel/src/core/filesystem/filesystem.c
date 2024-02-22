#include "core/filesystem/filesystem.h"

#define SUCCESS true

t_kernel_file* _get_file(t_kernel_filesystem* filesystem, char* file_name) {
    return diccionario_get(filesystem->open_files, file_name);
}

bool _file_is_already_open(t_kernel_filesystem* filesystem, char* file_name) {
    return diccionario_has_key(filesystem->open_files, file_name);
}

t_kernel_file* _add_to_open_files(t_kernel_filesystem* filesystem, char* file_name) {
    t_kernel_file* new_file = kernel_file_create(file_name);
    diccionario_put(filesystem->open_files, new_file->nombre_archivo, new_file);
    log_info(logger, "Archivo agregado a tabla global de archivos abiertos: %s", file_name);
    return new_file;
}

void remove_from_open_files(t_kernel_filesystem* filesystem, char* file_name) {
    diccionario_remove(filesystem->open_files, file_name);
    log_info(logger, "Archivo removido de la tabla global de archivos abiertos: %s", file_name);
}

t_kernel_filesystem* filesystem_initialize(char* filesystem_host, int filesystem_port) {
    t_kernel_filesystem* kernel_filesystem = malloc(sizeof(t_kernel_filesystem));
    kernel_filesystem->fs_host = filesystem_host;
    kernel_filesystem->fs_port = filesystem_port;
    kernel_filesystem->open_files = diccionario_create();
    return kernel_filesystem;
}

bool _open_file(t_kernel_filesystem* filesystem, char* file_name, int* file_size) {
    int filesystem_fd = _connect_to_filesystem(filesystem, filesystem->fs_host, filesystem->fs_port);
    t_paquete* pkg = crear_paquete(ABRIR_ARCHIVO);
    serialize_string(pkg, file_name);
    enviar_paquete_serializado_por_socket(filesystem_fd, pkg);
    paquete_destroy(pkg);

    int buffer;
    recibir_payload(filesystem_fd, &buffer, sizeof(int));
    log_info(logger, "ABRIR ARCHIVO %s respuesta: %i", file_name, buffer);
    *file_size = buffer;
    liberar_conexion(filesystem_fd);
    return buffer >= 0;
}

int _create_file(t_kernel_filesystem* filesystem, char* file_name) {
    int filesystem_fd = _connect_to_filesystem(filesystem, filesystem->fs_host, filesystem->fs_port);
    t_paquete* pkg = crear_paquete(CREAR_ARCHIVO);
    serialize_string(pkg, file_name);
    enviar_paquete_serializado_por_socket(filesystem_fd, pkg);
    paquete_destroy(pkg);

    int buffer;
    recibir_payload(filesystem_fd, &buffer, sizeof(int));
    log_info(logger, "CREAR ARCHIVO %s respuesta: %i", file_name, buffer);
    liberar_conexion(filesystem_fd);
    return buffer;
}

int _truncate_file(t_kernel_filesystem* filesystem, char* file_name, int new_size) {
    int filesystem_fd = _connect_to_filesystem(filesystem, filesystem->fs_host, filesystem->fs_port);
    t_paquete* pkg = crear_paquete(TRUNCAR_ARCHIVO);
    serialize_string(pkg, file_name);
    agregar_payload_a_paquete(pkg, &new_size, sizeof(int));
    enviar_paquete_serializado_por_socket(filesystem_fd, pkg);
    paquete_destroy(pkg);

    int buffer;
    recibir_payload(filesystem_fd, &buffer, sizeof(int));
    log_info(logger, "TRUNCAR ARCHIVO %s respuesta: %i", file_name, buffer);
    liberar_conexion(filesystem_fd);
    return buffer;
}

bool _write_file(t_kernel_filesystem* filesystem, char* file_name, int puntero, int dir_fisica, int pid) {
    int filesystem_fd = _connect_to_filesystem(filesystem, filesystem->fs_host, filesystem->fs_port);
    t_paquete* pkg = crear_paquete(ESCRIBIR_ARCHIVO);
    serialize_string(pkg, file_name);
    agregar_payload_a_paquete(pkg, &puntero, sizeof(int));
    agregar_payload_a_paquete(pkg, &dir_fisica, sizeof(int));
    agregar_payload_a_paquete(pkg, &pid, sizeof(int));
    enviar_paquete_serializado_por_socket(filesystem_fd, pkg);
    paquete_destroy(pkg);

    int buffer;
    recibir_payload(filesystem_fd, &buffer, sizeof(int));
    log_info(logger, "ESCRIBIR ARCHIVO %s respuesta: %i", file_name, buffer);
    liberar_conexion(filesystem_fd);
    return buffer == ARCHIVO_OK;
}

bool _read_file(t_kernel_filesystem* filesystem, char* file_name, int puntero, int dir_fisica, int pid) {
    int filesystem_fd = _connect_to_filesystem(filesystem, filesystem->fs_host, filesystem->fs_port);
    t_paquete* pkg = crear_paquete(LEER_ARCHIVO);
    serialize_string(pkg, file_name);
    agregar_payload_a_paquete(pkg, &puntero, sizeof(int));
    agregar_payload_a_paquete(pkg, &dir_fisica, sizeof(int));
    agregar_payload_a_paquete(pkg, &pid, sizeof(int));
    enviar_paquete_serializado_por_socket(filesystem_fd, pkg);
    paquete_destroy(pkg);

    int buffer;
    recibir_payload(filesystem_fd, &buffer, sizeof(int));
    log_info(logger, "LEER ARCHIVO %s respuesta: %i", file_name, buffer);
    liberar_conexion(filesystem_fd);
    return buffer == ARCHIVO_OK;
}

int _connect_to_filesystem(t_kernel_filesystem* filesystem, char* host, int port) {
    log_debug(logger, "Trying to create connection with file system...");
    int client_conn = create_conection(host, port);
    if (client_conn == 0) {
        log_error(logger, "Can't create file system connection.");
        return -1;
    }
    log_debug(logger, "Connection created with file system: %d.", client_conn);
    int handshake_result = handshake_client(client_conn, 567);
    if (handshake_result == -1) {
        close(client_conn);
        log_error(logger, "Error en handshake kernel -> filesystem. Connection closed.");
        return -2;
    }
    return client_conn;
}

t_kernel_file* get_file_from_filesystem(t_kernel_filesystem* filesystem, char* file_name) {
    int file_size;
    t_kernel_file* file = NULL;
    if (!_file_is_already_open(filesystem, file_name)) {
        log_info(logger, "Archivo no esta abierto: %s", file_name);
        if (_open_file(filesystem, file_name, &file_size) != SUCCESS) {
            int create_file = _create_file(filesystem, file_name);
            if (create_file == ARCHIVO_OK) {
                int open_file = _open_file(filesystem, file_name, &file_size);
                if (open_file == SUCCESS) {
                    file = _add_to_open_files(filesystem, file_name);
                } else {
                    log_error(logger, "Archivo no se pudo abrir: %s ", file_name);
                    return NULL;
                }
            } else {
                log_error(logger, "Archivo no se pudo crear: %s ", file_name);
                return NULL;
            }
        } else {
            file = _add_to_open_files(filesystem, file_name);
        }
    } else {
        file = _get_file(filesystem, file_name);
        log_info(logger, "Archivo ya esta abierto: %s", file->nombre_archivo);
    }
    return file;
}



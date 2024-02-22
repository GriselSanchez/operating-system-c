#include "memory.h"
#include <pthread.h>

int _connect_to_memory(char* host, int port) {
    log_debug(logger, "Trying to create connection with memory...");
    int client_conn = create_conection(host, port);
    if (client_conn == 0) {
        log_error(logger, "Can't create memory connection.");
        return -1;
    }
    log_debug(logger, "Connection created with memory: %d.", client_conn);
    int handshake_result = handshake_client(client_conn, 567);
    if (handshake_result == -1) {
        close(client_conn);
        log_error(logger, "Error en handshake kernel -> memory. Connection closed.");
        return -2;
    }
    return client_conn;
}


int memory_load_page(t_pcb* pcb, int page_to_load, t_kernel_memory* memory) {
    int memory_fd = _connect_to_memory(memory->memory_host, memory->memory_port);

    t_paquete* paquete = crear_paquete(LOAD_FRAME);
    agregar_payload_a_paquete(paquete, &pcb->pid, sizeof(int));
    agregar_payload_a_paquete(paquete, &page_to_load, sizeof(int));

    enviar_paquete_serializado_por_socket(memory_fd, paquete);
    paquete_destroy(paquete);

    int marco = 0;
    recibir_payload(memory_fd, &marco, sizeof(uint32_t));

    liberar_conexion(memory_fd);

    if (marco >= 0) {
        log_info(logger, "Pagina cargada exitosamente en marco %i.", marco);
    } else {
        log_info(logger, "No se pudo cargar la pagina en memoria.");
    }

    return marco;
}


void memory_terminate_process(t_pcb* pcb, t_kernel_memory* memory) {
    int memory_fd = _connect_to_memory(memory->memory_host, memory->memory_port);
    t_paquete* paquete = crear_paquete(MEMORY_EXIT_PROCESS);

    agregar_payload_a_paquete(paquete, &pcb->pid, sizeof(int));

    enviar_paquete_serializado_por_socket(memory_fd, paquete);
    paquete_destroy(paquete);

    op_code respuesta = 0;
    recibir_payload(memory_fd, &respuesta, sizeof(op_code));
    liberar_conexion(memory_fd);
}



int memory_allocate_new_process(t_pcb* pcb, t_kernel_memory* memory) {
    int memory_fd = _connect_to_memory(memory->memory_host, memory->memory_port);
    t_paquete* paquete = crear_paquete(MEMORY_NEW_PROCESS);

    agregar_payload_a_paquete(paquete, &pcb->pid, sizeof(int));

    int path_len = strlen(pcb->path);
    agregar_payload_a_paquete(paquete, &path_len, sizeof(int));
    agregar_payload_a_paquete(paquete, pcb->path, sizeof(char) * path_len);
    agregar_payload_a_paquete(paquete, &pcb->size, sizeof(int));

    enviar_paquete_serializado_por_socket(memory_fd, paquete);
    paquete_destroy(paquete);

    t_paquete* result = deserealizar_paquete_desde_socket(memory_fd);

    op_code code_result = result->codigo_operacion;
    if (code_result != MEMORY_NEW_PROCESS_OK) {
        paquete_destroy(result);
        liberar_conexion(memory_fd);
        return 1;
    }

    paquete_destroy(result);
    liberar_conexion(memory_fd);
    return 0;
}

t_kernel_memory* memory_initialize(char* memory_host, int memory_port) {
    t_kernel_memory* kernel_memory = malloc(sizeof(t_kernel_memory));
    kernel_memory->memory_host = memory_host;
    kernel_memory->memory_port = memory_port;
    return kernel_memory;
}

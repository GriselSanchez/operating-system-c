#include "sockets.h"

#define SERVER_RUNNING 1
#define BACKLOG 200

int create_server_socket(int listen_port) {
    int status, server_socket;
    struct addrinfo hints;
    struct addrinfo* server_info, * p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(NULL, string_itoa(listen_port), &hints, &server_info);
    if (status != 0) {
        log_error(logger, "Error getting address info: %s", gai_strerror(status));
        return 2; // TODO : MAXO : change to #define SKT_ERR_GAI 2
    }

    // servinfo now points to a linked list of 1 or more struct addrinfos
    for (p = server_info; p != NULL; p = p->ai_next) {
        server_socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (server_socket == -1) {
            log_error(logger, "Error creating socket.");
            continue;
        }
        // bind it to the port
        if (bind(server_socket, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_socket);
            log_error(logger, "Error binding socket port.");
            continue;
        }
        break;
    }

    if (listen(server_socket, BACKLOG) == -1) {
        log_error(logger, "Error listening for incoming connection.");
        return 1; // TODO : MAXO : change to #define SKT_ERR_IC 1
    }
    freeaddrinfo(server_info);
    log_debug(logger, "Success on socket listening for incoming connections at port %d.", listen_port);
    return server_socket;
}

void single_client_server_loop(int server_fd, void (*handle_connection)(int)) {
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_size;

    while (SERVER_RUNNING) {
        /* Aceptar una conexión entrante */
        addr_size = sizeof client_addr;
        if ((client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_size)) == -1) {
            log_error(logger, "Error accepting incomming connection.");
            continue;
        }
        log_debug(logger, "New client connected [%d]", client_fd);
        (*handle_connection)(client_fd);
    }
}

void multi_client_server_loop(int server_fd, void* (*handle_connection)(void*)) {
    int* pclient_fd;
    int client_fd;
    struct sockaddr_in client_addr;
    socklen_t addr_size;

    while (SERVER_RUNNING) {
        /* Aceptar una conexión entrante */
        addr_size = sizeof client_addr;
        pclient_fd = malloc(sizeof(int));
        client_fd = accept(server_fd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_size);
        memcpy(pclient_fd, &client_fd, sizeof(int));
        if (client_fd == -1) {
            log_error(logger, "Error accepting incomming connection.");
            free(pclient_fd);
            continue;
        }
        log_debug(logger, "New client connected [%d]", client_fd);
        /* Crear un nuevo hilo para manejar la conexión */
        pthread_t* thread_id;
        thread_id = malloc(sizeof(pthread_t));
        if (pthread_create(thread_id, NULL, handle_connection, pclient_fd) != 0) {
            log_error(logger, "Error creating thread.");
            free(thread_id);
            free(pclient_fd);
            continue;
        }
        /* Hacer que el hilo sea independiente */
        if (pthread_detach(*thread_id) != 0) {
            log_error(logger, "Error on thread detach.");
            free(thread_id);
            free(pclient_fd);
            continue;
        }
    }
}

int create_conection(char* ip, int puerto) {
    int status, socket_cliente;
    struct addrinfo hints;
    struct addrinfo* server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    status = getaddrinfo(ip, string_itoa(puerto), &hints, &server_info);
    if (status != 0) {
        log_error(logger, "Error getting address info: %s", gai_strerror(status));
        return 2; // TODO : MAXO : change to #define SKT_ERR_GAI 2
    }

    socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);
    if (socket_cliente == -1) {
        log_error(logger, "Error creating socket.");
    }
    // Ahora que tenemos el socket, vamos a conectarlo
    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1) {
        close(socket_cliente);
        log_error(logger, "Error binding socket port.");
    }

    freeaddrinfo(server_info);
    return socket_cliente;
}

void liberar_conexion(int socket_fd) {
    log_debug(logger, "Conexion cerrada [%i]", socket_fd);
    close(socket_fd);
}

int enviar_payload(int socked_fd, void* buffer, size_t tamanio) {
    return send(socked_fd, buffer, tamanio, 0);
}

int recibir_payload(int socked_fd, void* buffer, size_t tamanio) {
    return recv(socked_fd, buffer, tamanio, 0);
}

int handshake_client(int socked_fd, uint32_t handshake) {
    uint32_t result;

    send(socked_fd, &handshake, sizeof(uint32_t), NULL);
    recv(socked_fd, &result, sizeof(uint32_t), MSG_WAITALL);

    return result;
}

int handshake_server(int socked_fd, uint32_t handshake_esperado) {
    uint32_t handshake;
    uint32_t resultOk = 0;
    uint32_t resultError = -1;

    recv(socked_fd, &handshake, sizeof(uint32_t), MSG_WAITALL);
    if (handshake == handshake_esperado) {
        send(socked_fd, &resultOk, sizeof(uint32_t), 0);
        return 0;
    } else {
        send(socked_fd, &resultError, sizeof(uint32_t), 0);
        return -1;
    }
}
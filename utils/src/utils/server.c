#include "server.h"

t_server* server_create(int listen_port) {

  t_server* server = malloc(sizeof(t_server));
  if (server == NULL) {
    log_error(logger, "Error on memory allocation.");
    return NULL;
  }
  server->server_fd = create_server_socket(listen_port);
  return server;
}

t_server* multiclient_server_create(int listen_port,
  void* (*handle_incomming_connection)(void*)) {
  t_server* server = server_create(listen_port);
  server->handle_incomming_connection = handle_incomming_connection;
  server->handle_connection = NULL;
  return server;
}

t_server* singleclient_server_create(int listen_port,
  void(*handle_incomming_connection)(int)) {

  t_server* server = server_create(listen_port);
  server->handle_connection = handle_incomming_connection;
  server->handle_incomming_connection = NULL;
  return server;
}

void server_start(t_server* server) {
  if (server->handle_incomming_connection != NULL) {
    multi_client_server_loop(server->server_fd, server->handle_incomming_connection);
  } else {
    single_client_server_loop(server->server_fd, server->handle_connection);
  }
}

void server_stop(t_server* server) {
  log_info(logger, "Shuting down server...");
  close(server->server_fd);
}

void server_destroy(t_server* server) {
  free(server);
}
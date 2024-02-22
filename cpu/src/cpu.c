#include "cpu.h"

t_log* logger;
t_cpu* cpu;
t_config_cpu* configs;
t_interrupcion* interrupcion;
int tamanio_pagina;

pthread_mutex_t registros_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t interrupcion_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
  logger = log_create("cpu.log", "CPU", 1, LOG_LEVEL_DEBUG);
  configs = cargar_variables_de_archivo();
  cpu = create_cpu();
  interrupcion = malloc(sizeof(t_interrupcion));
  reiniciar_flag_interrupcion(interrupcion);

  cpu_start();

  cpu_config_destroy(configs);
  cpu_destroy(cpu);
  log_destroy(logger);
  free(interrupcion);
}

t_exec_context* actualizar_contexto_de_ejecucion(t_exec_context* ctx) {
  actualizar_registros_ctx(ctx);
  return ctx;
}

void enviar_contexto_de_ejecucion(t_exec_context* ctx, op_code codigo_operacion, int client_fd) {
  log_info(logger, "Enviando contexto actualizado de PID %i a KERNEL con COD_OP %d...", ctx->pid, codigo_operacion);
  send_execution_context_via_socket(ctx, codigo_operacion, client_fd);
}

t_config* init_config() {
  t_config* new_config;
  if ((new_config = config_create("./cfg/cpu.config")) == NULL) {
    log_error(logger, "Unable to get config");
    exit(2);
  }
  return new_config;
}

void finish_cpu(t_server* server, t_log* logger, t_config* config) {
  if (logger != NULL) log_destroy(logger);
  if (config != NULL) config_destroy(config);
  if (server != 0) {
    server_stop(server);
    server_destroy(server);
  }
}

void cpu_start() {
  if (cpu_connect_memory() != 0) {
    log_error(logger, "Cant create connection to memory");
  }

  pthread_t cpu_dispatch_t, cpu_interrupt_t;
  pthread_create(&cpu_interrupt_t, NULL, (void*)start_cpu_interrupt, NULL);
  pthread_detach(cpu_interrupt_t);

  server_start(cpu->server_dispatch);
}

void on_connection_dispatch(int client_fd) {
  int open_connection = 1;

  int result = handshake_server(client_fd, 567);
  if (result == -1) {
    close(client_fd);
    log_error(logger, "Error en handshake");
    return;
  }
  log_info(logger, "Hanshake correcto");

  /* Procesar los datos recibidos del cliente */
  while (open_connection) {
    t_paquete* paquete = deserealizar_paquete_desde_socket(client_fd);
    op_code code_op = paquete->codigo_operacion;

    switch (code_op) {
    case CONTEXTO_EJECUCION:
      t_exec_context* ctx = deserialize_execution_context(paquete->buffer->payload);
      log_info(logger, "Recibo contexto de ejecucion de PID %i.", ctx->pid);
      actualizar_registros_cpu(ctx);

      iniciar_ciclo_instruccion(ctx, client_fd);

      destroy_exec_context(ctx);
      break;
    default:
      log_error(logger, "OP CODE INVALIDO: %i.", code_op);
      open_connection = 0;
      break;
    }

    paquete_destroy(paquete);
  }

  /* Cerrar la conexión */
  close(client_fd);
  return;
}

void on_connection_interrupt(int client_fd) {
  //char buffer[1024];
  //int bytes;
  int open_connection = 1;

  int result = handshake_server(client_fd, 567);
  if (result == -1) {
    close(client_fd);
    log_error(logger, "Error en handshake");
    return;
  }
  log_info(logger, "Hanshake correcto");

  /* Procesar los datos recibidos del cliente */
  while (open_connection) {
    t_paquete* paquete = deserealizar_paquete_desde_socket(client_fd);
    op_code code_op = paquete->codigo_operacion;

    switch (code_op) {
    case INTERRUPCION:
      int offset = 0;
      int pid = 0;
      op_code motivo = 0;

      memcpy(&pid, paquete->buffer->payload, sizeof(int));
      offset += sizeof(int);
      memcpy(&motivo, paquete->buffer->payload + offset, sizeof(op_code));

      pthread_mutex_lock(&interrupcion_mutex);
      interrupcion->pid = pid;
      interrupcion->motivo = motivo;
      pthread_mutex_unlock(&interrupcion_mutex);

      log_info(logger, "Recibo una interrupción con motivo: %s.", OP_CODE_NAME(motivo));

      int resp = 0;
      enviar_payload(client_fd, &resp, sizeof(int));
      break;
    default:
      log_error(logger, "OP CODE INVALIDO: %i.", code_op);
      open_connection = 0;
      break;
    }

    paquete_destroy(paquete);
  }

  /* Cerrar la conexión */
  close(client_fd);
  return;
}


t_cpu* create_cpu() {
  cpu = malloc(sizeof(t_cpu));
  if (cpu == NULL) {
    log_error(logger, "Error on cpu allocation.");
    return NULL;
  }

  cpu->server_dispatch = singleclient_server_create(configs->puerto_escucha_dispatch, &on_connection_dispatch);
  cpu->server_interrupt = singleclient_server_create(configs->puerto_escucha_interrupt, &on_connection_interrupt);

  return cpu;
}

void start_cpu_interrupt() {
  server_start(cpu->server_interrupt);
}

int cpu_connect_memory() {
  log_debug(logger, "Creando conexion a memoria: %i", configs->puerto_memoria);
  cpu->memoria_fd = create_conection(configs->ip_memoria, configs->puerto_memoria);

  op_code code_op = HANDSHAKE_CPU;
  enviar_payload(cpu->memoria_fd, &code_op, sizeof(op_code));
  t_paquete* paquete = deserealizar_paquete_desde_socket(cpu->memoria_fd);

  if (paquete->codigo_operacion == -1) {
    close(cpu->memoria_fd);
    log_error(logger, "Error en handshake");
    return 1;
  } else {
    memcpy(&tamanio_pagina, paquete->buffer->payload, sizeof(int));
    paquete_destroy(paquete);
    log_info(logger, "Hanshake correcto");
  }

  return 0;
}

void reiniciar_flag_interrupcion(t_interrupcion* interrupcion) {
  pthread_mutex_lock(&interrupcion_mutex);
  interrupcion->pid = -1;
  interrupcion->motivo = -1;
  pthread_mutex_unlock(&interrupcion_mutex);
}

void cpu_destroy(t_cpu* cpu) {
  liberar_conexion(cpu->memoria_fd);

  server_stop(cpu->server_dispatch);
  server_destroy(cpu->server_dispatch);
  server_stop(cpu->server_interrupt);
  server_destroy(cpu->server_interrupt);

  free(cpu);
}
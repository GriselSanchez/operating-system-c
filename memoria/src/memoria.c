
#include "memoria.h"

t_log* logger;
t_memory* memory;
t_config_memoria* configs;
t_lista_mutex* procesos_en_memoria;
t_handler_paginacion* handler_paginacion;

pthread_mutex_t operaciones_bloqueantes_mutex = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t pagina_logger = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
  logger = log_create(LOG_PATH, MODULE_NAME, 1, LOG_LEVEL_DEBUG);
  configs = cargar_variables_de_archivo();
  procesos_en_memoria = list_init_mutex();
  handler_paginacion = create_handler_paginacion(configs->tam_memoria, configs->tam_pagina, configs->algoritmo_reemplazo);
  // iniciar servidor
  memory = create_memory(configs);

  // Servidor
  start_memory(memory);

  // termina programa
  list_destroy_mutex(procesos_en_memoria);
  log_destroy(logger);
  handler_paginacion_destroy(handler_paginacion);
  memory_config_destroy(configs);
  memory_destroy(memory);
}

void onConnectionSingle(int client_fd) {
  uint32_t handshake = 0;
  uint32_t resultOk = 0;
  uint32_t resultError = -1;

  recv(client_fd, &handshake, sizeof(uint32_t), MSG_WAITALL);

  if (handshake == 567) {
    enviar_payload(client_fd, &resultOk, sizeof(uint32_t));
    log_info(logger, "Hanshake con Kernel correcto %i", client_fd);
    lanzar_hilo_para_atender_kernel(client_fd);

  } else if (handshake == HANDSHAKE_CPU) {
    enviar_handshake_cpu(client_fd);
    log_info(logger, "Handshake con CPU correcto %i", client_fd);
    lanzar_hilo_para_atender_cpu(client_fd);

  } else if (handshake == HANDSHAKE_FILE_SYSTEM) {
    enviar_payload(client_fd, &resultOk, sizeof(uint32_t));
    log_info(logger, "Handshake con File System correcto %i", client_fd);
    lanzar_hilo_para_atender_filesystem(client_fd);

  } else {
    enviar_payload(client_fd, &resultError, sizeof(uint32_t));
    close(client_fd);
    log_error(logger, "Error en handshake");
    return NULL;
  }
}

void lanzar_hilo_para_atender_kernel(int client_fd) {
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, atender_kernel, client_fd);
  pthread_detach(thread_id);
}

void lanzar_hilo_para_atender_filesystem(int client_fd) {
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, atender_filesystem, client_fd);
  pthread_detach(thread_id);
}

void lanzar_hilo_para_atender_cpu(int client_fd) {
  pthread_t thread_id;
  pthread_create(&thread_id, NULL, atender_cpu, client_fd);
  pthread_detach(thread_id);
}

void* atender_kernel(int client_fd) {
  log_debug(logger, "Se crea hilo para Kernel. Fd: %i", client_fd);

  t_paquete* paquete = deserealizar_paquete_desde_socket(client_fd);
  op_code code_op = paquete->codigo_operacion;

  switch (code_op) {
  case MEMORY_NEW_PROCESS: {
    allocate_new_process(paquete, client_fd);
    break;
  }

  case MEMORY_EXIT_PROCESS: {
    int pid = 0;
    memcpy(&pid, paquete->buffer->payload, sizeof(int));

    remover_proceso(handler_paginacion, pid);

    op_code respuesta = MEMORY_OK;
    enviar_payload(client_fd, &respuesta, sizeof(op_code));

    break;
  }

  case LOAD_FRAME: {
    int offset = 0;
    int pagina = 0;
    int pid = 0;
    tipo_operacion_t operacion;

    memcpy(&pid, paquete->buffer->payload + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&pagina, paquete->buffer->payload + offset, sizeof(int));
    offset += sizeof(int);
    memcpy(&operacion, paquete->buffer->payload + offset, sizeof(tipo_operacion_t));
    offset += sizeof(tipo_operacion_t);

    int marco = cargar_en_memoria(handler_paginacion, pid, pagina);
    enviar_payload(client_fd, &marco, sizeof(int));

    break;
  }

  default:
    log_error(logger, "OP CODE INVALIDO: %i.", code_op);
    break;
  }

  paquete_destroy(paquete);
  liberar_conexion(client_fd);
}

void* atender_filesystem(int client_fd) {
  log_debug(logger, "Se crea hilo para File System. Fd: %i", client_fd);
  int open_connection = 1;

  while (open_connection) {
    t_paquete* paquete = deserealizar_paquete_desde_socket(client_fd);
    op_code code_op = paquete->codigo_operacion;

    switch (code_op) {
    case MEMORY_WRITE: {
      int offset = 0;
      int pid = 0;
      int direccion_fisica = 0;
      int cant_elementos = configs->tam_pagina / sizeof(uint32_t);
      uint32_t* data = malloc(configs->tam_pagina);

      memcpy(&pid, paquete->buffer->payload, sizeof(int));
      offset += sizeof(int);
      memcpy(&direccion_fisica, paquete->buffer->payload + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(data, paquete->buffer->payload + offset, configs->tam_pagina);
      offset += configs->tam_pagina;

      sleep(memory->response_delay);
      escribir_valor_pagina(handler_paginacion, direccion_fisica, cant_elementos, data, pid);

      t_paquete* paquete_response = crear_paquete(MEMORY_OK);
      agregar_payload_a_paquete(paquete_response, &direccion_fisica, sizeof(int));
      enviar_paquete_serializado_por_socket(client_fd, paquete_response);

      break;
    }

    case MEMORY_READ: {
      int offset = 0;
      int pid = 0;
      int direccion_fisica = 0;
      int cant_elementos = configs->tam_pagina / sizeof(uint32_t);

      memcpy(&pid, paquete->buffer->payload, sizeof(int));
      offset += sizeof(int);
      memcpy(&direccion_fisica, paquete->buffer->payload + offset, sizeof(int));

      sleep(memory->response_delay);
      uint32_t* data_leida = leer_valor_pagina(handler_paginacion, direccion_fisica, cant_elementos, pid);

      t_paquete* paquete_response = crear_paquete(MEMORY_OK);
      agregar_payload_a_paquete(paquete_response, data_leida, configs->tam_pagina);
      enviar_paquete_serializado_por_socket(client_fd, paquete_response);
      free(data_leida);

      break;
    }

    default:
      log_error(logger, "OP CODE INVALIDO: %i.", code_op);
      open_connection = 0;
      break;
    }

    paquete_destroy(paquete);
  }

  liberar_conexion(client_fd);
}

void* atender_cpu(int client_fd) {
  log_debug(logger, "Se crea hilo para CPU. Fd: %i", client_fd);
  int open_connection = 1;

  while (open_connection) {
    t_paquete* paquete = deserealizar_paquete_desde_socket(client_fd);
    op_code code_op = paquete->codigo_operacion;

    switch (code_op) {
    case NEXT_INSTRUCTION: {
      send_next_instruction(paquete, client_fd);
      break;
    }

    case MOV_IN: {
      mov_in_instruction(paquete, client_fd);
      break;
    }

    case MOV_OUT: {
      mov_out_instruction(paquete, client_fd);
      break;
    }

    case FRAME_REQUEST: {
      int offset = 0;
      int pagina = 0;
      int pid = 0;
      tipo_operacion_t operacion;

      memcpy(&pid, paquete->buffer->payload + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&pagina, paquete->buffer->payload + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&operacion, paquete->buffer->payload + offset, sizeof(tipo_operacion_t));
      offset += sizeof(tipo_operacion_t);

      pthread_mutex_lock(&operaciones_bloqueantes_mutex);
      int marco = obtener_marco(handler_paginacion, pid, pagina, operacion);
      pthread_mutex_unlock(&operaciones_bloqueantes_mutex);

      enviar_payload(client_fd, &marco, sizeof(int));

      break;
    }

    default:
      log_error(logger, "OP CODE INVALIDO: %i.", code_op);
      open_connection = 0;
      break;
    }

    paquete_destroy(paquete);
  }

  liberar_conexion(client_fd);
}
void enviar_handshake_cpu(int cpu_fd) {
  t_paquete* paquete = crear_paquete(HANDSHAKE_CPU);
  agregar_payload_a_paquete(paquete, &configs->tam_pagina, sizeof(int));
  enviar_paquete_serializado_por_socket(cpu_fd, paquete);
  paquete_destroy(paquete);
}

void mov_in_instruction(t_paquete* paquete, int cpu_fd) {
  int offset = 0;
  int direccion_fisica = 0;
  int pid = 0;

  memcpy(&direccion_fisica, paquete->buffer->payload, sizeof(int));
  offset += sizeof(int);
  memcpy(&pid, paquete->buffer->payload + offset, sizeof(int));
  offset += sizeof(int);

  sleep(memory->response_delay);
  uint32_t valor_real = leer_valor(handler_paginacion, direccion_fisica, pid);
  enviar_payload(cpu_fd, &valor_real, sizeof(uint32_t));
}

void mov_out_instruction(t_paquete* paquete, int cpu_fd) {
  int offset = 0;
  int direccion_fisica = 0;
  int pid = 0;
  uint32_t valor = 0;

  memcpy(&direccion_fisica, paquete->buffer->payload, sizeof(int));
  offset += sizeof(int);
  memcpy(&pid, paquete->buffer->payload + offset, sizeof(int));
  offset += sizeof(int);
  memcpy(&valor, paquete->buffer->payload + offset, sizeof(uint32_t));
  offset += sizeof(uint32_t);

  int result = escribir_valor(handler_paginacion, direccion_fisica, valor, pid);
  sleep(memory->response_delay);
  enviar_payload(cpu_fd, &result, sizeof(int));
}

t_memory* create_memory(t_config_memoria* configs) {
  memory = malloc(sizeof(t_memory));
  if (memory == NULL) {
    log_error(logger, "Error on memory allocation.");
    return NULL;
  }
  log_info(logger, "Puerto:%i", configs->puerto_escucha);
  memory->response_delay = configs->retardo_respuesta;
  memory->server = singleclient_server_create(configs->puerto_escucha, &onConnectionSingle);

  return memory;
}

t_memory* start_memory(t_memory* memory) {
  log_info(logger, "Levantando servidor.");
  server_start(memory->server);
  log_info(logger, "Servidor listo para recibir al cliente");
}

void allocate_new_process(t_paquete* paquete, int fd) {
  int offset = 0;
  int pid = 0;
  int len_path = 0;
  int size = 0;

  memcpy(&pid, paquete->buffer->payload, sizeof(int));
  offset += sizeof(int);

  memcpy(&len_path, (paquete->buffer->payload + offset), sizeof(int));
  offset += sizeof(int);

  char* path = malloc(sizeof(char) * len_path);
  memcpy(path, (paquete->buffer->payload + offset), sizeof(char) * len_path);
  path[len_path] = '\0';
  offset += sizeof(char) * len_path;

  memcpy(&size, paquete->buffer->payload + offset, sizeof(int));
  offset += sizeof(int);

  t_proceso_memoria* new_process = malloc(sizeof(t_proceso_memoria));
  new_process->pid = pid;
  new_process->instrucciones = obtener_instrucciones_proceso(path, configs->path_instrucciones);
  new_process->size = size;

  log_info(logger, "Se obtienen instrucciones de proceso %d - Path: %s - Size: %i", new_process->pid, path, size);
  list_add_mutex(procesos_en_memoria, new_process);

  pthread_mutex_lock(&operaciones_bloqueantes_mutex);
  generar_tabla_paginas(handler_paginacion, new_process->pid, new_process->size);
  pthread_mutex_unlock(&operaciones_bloqueantes_mutex);

  t_paquete* paquete_response = crear_paquete(MEMORY_NEW_PROCESS_OK);
  agregar_payload_a_paquete(paquete_response, &pid, sizeof(int));
  enviar_paquete_serializado_por_socket(fd, paquete_response);
  paquete_destroy(paquete_response);
}

void send_next_instruction(t_paquete* paquete, int client_fd) {
  int offset = 0;
  int pid = 0;
  int program_counter = 0;

  memcpy(&pid, paquete->buffer->payload, sizeof(int));
  offset += sizeof(int);
  memcpy(&program_counter, paquete->buffer->payload + offset, sizeof(int));
  offset += sizeof(int);

  log_info(logger, "PROXIMA INSTRUCCION - PID: %i - PC: %i.", pid, program_counter);

  sleep(memory->response_delay);
  enviar_instruccion_serializada(pid, program_counter, client_fd);
}

int memory_connect_filesystem(t_memory* memory) {
  int filesystem_fd = -1;
  log_debug(logger, "Creando conexion a filesystem: %i", configs->puerto_filesystem);
  filesystem_fd = create_conection(configs->ip_filesystem, configs->puerto_filesystem);
  if (filesystem_fd == -1) {
    log_error(logger, "Error al crear la conexión con el filesystem.");
    return -1;
  }

  int result = handshake_client(filesystem_fd, 567);
  if (result == 0) {

    return filesystem_fd;
  } else {
    log_error(logger, "Error en el handshake con el filesystem.");
    return -1;
  }
}

void memory_destroy(t_memory* memory) {
  server_stop(memory->server);
  server_destroy(memory->server);
  free(memory);
}




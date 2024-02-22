#include "kernel.h"

#define NOT_ACTIVE_CONSOLE 0

t_kernel* kernel;
t_log* logger;

extern t_resources* resources;

pthread_mutex_t pcb_status_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {
  char* config_path = argv[1];
  logger = log_create("kernel.log", "KERNEL", NOT_ACTIVE_CONSOLE, LOG_LEVEL_DEBUG);

  t_kernel_config* config = load_config(config_path);
  if (config == NULL) {
    log_error(logger, "Unable to get/process config");
    exit(2);
  }

  kernel = kernel_create(config);
  if (kernel == NULL) {
    log_error(logger, "Unable to start kernel");
    exit(2);
  }

  kernel_start_connections(kernel, config);
  kernel->resources = malloc(sizeof(t_resources));
  resources_initialize(kernel->resources, config->recursos, config->instancias_recursos);
  // This is very bad -> resources o es global o no lo es, de entrada.
  resources = kernel->resources;
  kernel->filesystem = filesystem_initialize(config->file_system_ip, config->file_system_port);
  kernel->memory = memory_initialize(config->memory_ip, config->memory_port);
  start_planners(kernel, config);

  start_interactive_console(kernel->memory, kernel->short_term_planner);

  finish_kernel(kernel, logger, config);
}

t_planner* create_short_term_planner(t_kernel_config* configs) {
  char* planner_name = configs->planner;
  t_planner* planner = create_planner(planner_name);
  if (string_equals_ignore_case(planner_name, "FIFO")) {
    planner_set_fifo(planner);
  } else if (string_equals_ignore_case(planner_name, "RR")) {
    planner_set_rr(planner, configs->quantum);
  } else if (string_equals_ignore_case(planner_name, "PRIORIDADES")) {
    planner_set_planner_prioridades(planner);
  }
  return planner;
}


t_kernel* kernel_create(t_kernel_config* configs) {
  t_kernel* kernel = malloc(sizeof(t_kernel));
  if (kernel == NULL) {
    log_error(logger, "Error on memory allocation.");
    return NULL;
  }
  kernel->short_term_planner = create_short_term_planner(configs);
  kernel->multiprogramming_degree = configs->multiprogramming_degree;
  kernel->paused_planning = true;
  return kernel;
}

void start_planners(t_kernel* kernel, t_kernel_config* configs) {
  start_short_term_planner(kernel->cpu_dispatch_fd, kernel->memory, kernel->short_term_planner, kernel->filesystem);
  start_long_term_planner(kernel->multiprogramming_degree, kernel->short_term_planner, kernel->memory);
  start_interruptions_handler(kernel->cpu_interrupt_fd);
}

void kernel_start_connections(t_kernel* kernel, t_kernel_config* configs) {
  // Kernel client to cpu dispatch
  log_info(logger, "Try to create connection with cpu dispatch");
  int cpu_dispatch_connection = create_conection(configs->cpu_ip, configs->cpu_dispatch_port);
  if (cpu_dispatch_connection == 0) {
    log_error(logger, "Can't create cpu dispatch connection");
    exit(2);
  }
  log_info(logger, "Connection created with cpu: %d", cpu_dispatch_connection);
  int cpu_dispatch_handshake_result = handshake_client(cpu_dispatch_connection, 567);
  if (cpu_dispatch_handshake_result == -1) {
    close(cpu_dispatch_connection);
    log_error(logger, "Error en handshake kernel -> cpu dispatch; connection closed");
  }

  // Kernel client to cpu interrupt 
  log_info(logger, "Try to create connection with cpu interrupt");
  int cpu_interrupt_connection = create_conection(configs->cpu_ip, configs->cpu_interrupt_port);
  if (cpu_interrupt_connection == 0) {
    log_error(logger, "Can't create cpu interrupt connection");
    exit(2);
  }
  log_info(logger, "Connection created with cpu: %d", cpu_interrupt_connection);
  int cpu_interrupt_handshake_result = handshake_client(cpu_interrupt_connection, 567);
  if (cpu_interrupt_handshake_result == -1) {
    close(cpu_interrupt_connection);
    log_error(logger, "Error en handshake kernel -> cpu interrupt; connection closed");
  }

  kernel->cpu_dispatch_fd = cpu_dispatch_connection;
  kernel->cpu_interrupt_fd = cpu_interrupt_connection;
}

void kernel_destroy(t_kernel* kernel) {

  resources_destroy(kernel->resources);
  planner_destroy(kernel->short_term_planner);
  //TODO: crear destroyer de t_kernel_filesystem
  //filesystem_destroy(kernel->filesystem);
}

void finish_kernel(t_kernel* kernel, t_log* logger, t_kernel_config* config) {
  if (logger != NULL) log_destroy(logger);
  if (config != NULL) kernel_config_destroy(config);
  if (kernel->cpu_dispatch_fd) liberar_conexion(kernel->cpu_dispatch_fd);
  if (kernel->cpu_interrupt_fd) liberar_conexion(kernel->cpu_interrupt_fd);
  kernel_destroy(kernel);
}

#include "config.h"


t_list* char_array_to_list(char** array);
t_list* int_array_to_list(char** array);

t_kernel_config* load_config(char* configs_path) {
	t_kernel_config* config = malloc(sizeof(t_kernel_config));

	t_config* config_file = config_create(configs_path);
	if (config_file == NULL) return NULL;

	bool is_config_valid = validate_config(config_file);
	if (!is_config_valid) return NULL;

	config->memory_ip = strdup(config_get_string_value(config_file, "MEMORY_IP"));
	config->memory_port = config_get_int_value(config_file, "MEMORY_PORT");
	config->cpu_ip = strdup(config_get_string_value(config_file, "CPU_IP"));
	config->cpu_dispatch_port = config_get_int_value(config_file, "CPU_DISPATCH_PORT");
	config->cpu_interrupt_port = config_get_int_value(config_file, "CPU_INTERRUPT_PORT");
	config->file_system_ip = strdup(config_get_string_value(config_file, "FILE_SYSTEM_IP"));
	config->file_system_port = config_get_int_value(config_file, "FILE_SYSTEM_PORT");
	config->multiprogramming_degree = config_get_int_value(config_file, "MULTIPROGRAMMING_DEGREE");
	config->quantum = config_get_int_value(config_file, "QUANTUM");
	config->planner = strdup(config_get_string_value(config_file, "PLANNER"));
	config->recursos = char_array_to_list(config_get_array_value(config_file, "RECURSOS"));
	config->instancias_recursos = int_array_to_list(config_get_array_value(config_file, "INSTANCIAS_RECURSOS"));

	config_destroy(config_file);
	return config;
}



/**
* Validates all required environment variables to start kernel connections
*/
bool validate_config(t_config* config_file) {

  // Cpu environment variables validation
  bool config_has_cpu_ip = config_has_property(config_file, "CPU_IP");
  bool config_has_cpu_dispatch_port = config_has_property(config_file, "CPU_DISPATCH_PORT");
  bool config_has_cpu_interrupt_port = config_has_property(config_file, "CPU_INTERRUPT_PORT");
  if (!config_has_cpu_ip || !config_has_cpu_dispatch_port || !config_has_cpu_interrupt_port) return false;

  // Memory environment variables validation
  bool config_has_memory_ip = config_has_property(config_file, "MEMORY_IP");
  bool config_has_memory_port = config_has_property(config_file, "MEMORY_PORT");
  if (!config_has_memory_ip || !config_has_memory_port) return false;

  // File system environment variables validation
  bool config_has_file_system_ip = config_has_property(config_file, "FILE_SYSTEM_IP");
  bool config_has_file_system_port = config_has_property(config_file, "FILE_SYSTEM_PORT");
  if (!config_has_file_system_ip || !config_has_file_system_port) return false;

	// Planners validation
  bool config_has_multiprogramming_degree= config_has_property(config_file, "MULTIPROGRAMMING_DEGREE");
  bool config_has_quantum = config_has_property(config_file, "QUANTUM");
  bool config_has_planner = config_has_property(config_file, "PLANNER");
  if (!config_has_multiprogramming_degree || !config_has_quantum || !config_has_planner) return false;

  return true;
}


void kernel_config_destroy(t_kernel_config* config){
	free(config->cpu_ip);
	free(config->file_system_ip);
	free(config->planner);
	free(config->memory_ip);
	list_destroy_and_destroy_elements(config->instancias_recursos, free);
	list_destroy_and_destroy_elements(config->recursos, free);
	free(config);
}


// aux func
t_list* char_array_to_list(char** array) {
	t_list* resources = list_create();
	int array_size = string_array_size(array);

	for(int index=0; index<array_size; index++){
		//char* value = string_array_pop(array);
		char* value = array[index];
		list_add(resources, value);
	}
	return resources;
}

t_list* int_array_to_list(char** array) {
	t_list* resources = list_create();
	int array_size = string_array_size(array);

	for(int index=0; index<array_size; index++){
		//char* value = (char*) string_array_pop(array);
		char* value = array[index];
		int int_value = atoi(value);
		int *pint_value = malloc(sizeof(int));
		memcpy(pint_value, &int_value, sizeof(int));
		list_add(resources, pint_value);
	}
	return resources;
}
#include "console.h"

void start_interactive_console(t_kernel_memory* memory, t_planner* short_term_planner) {
	printf("start console \n");

	char* command;
	while (1) {
		printf("Ingrese un comando: \n");
		command = readline(">");

		if (strcmp(command, "\0") == 0) {
			free(command);
			break;
		}

		char** command_parameters = string_split(command, " ");
		char* function = command_parameters[0];

		if (string_equals_ignore_case(function, "INICIAR_PROCESO")) {
			printf("Received INICIAR_PROCESO \n");
			bool isValid = validate_init_process(command_parameters);
			if (isValid) {
				char* path = command_parameters[1];
				int size = atoi(command_parameters[2]);
				char* priority = command_parameters[3];

				handle_new_process(path, size, atoi(priority));
			}
		} else if (string_equals_ignore_case(function, "FINALIZAR_PROCESO")) {
			printf("Received FINALIZAR_PROCESO \n");
			bool isValid = validate_finish_process(command_parameters);
			if (isValid) {
				char* pid = command_parameters[1];
				handle_force_process_exit(short_term_planner, memory, atoi(pid));
			}
		} else if (string_equals_ignore_case(function, "INICIAR_PLANIFICACION")) {
			printf("Received INICIAR_PLANIFICACION \n");
			start_planning();
		} else if (string_equals_ignore_case(function, "DETENER_PLANIFICACION")) {
			printf("Received DETENER_PLANIFICACION \n");
			pause_planning();
		} else if (string_equals_ignore_case(function, "MULTIPROGRAMACION")) {
			printf("Received MULTIPROGRAMACION \n");
			bool isValid = validate_multiprogramming_degree(command_parameters);
			if (isValid) {
				update_max_multi_programming(atoi(command_parameters[1]));
			}
		} else if (string_equals_ignore_case(function, "PROCESO_ESTADO")) {
			printf("Received PROCESO_ESTADO \n");
			listar_procesos_por_estado();
		} else if (string_equals_ignore_case(function, "BASE")) {
			printf("Received BASE \n");
			handle_new_process("PLANI_1", 64, 1);
			handle_new_process("PLANI_2", 64, 3);
			handle_new_process("PLANI_3", 64, 2);
			start_planning();
		} else if (string_equals_ignore_case(function, "DEADLOCK")) {
			printf("Received DEADLOCK \n");
			handle_new_process("DEADLOCK_A", 64, 1);
			handle_new_process("DEADLOCK_B", 64, 3);
			handle_new_process("DEADLOCK_C", 64, 2);
			handle_new_process("DEADLOCK_D", 64, 2);
			handle_new_process("ERROR_1", 64, 1);
			handle_new_process("ERROR_2", 64, 3);
			start_planning();
		} else if (string_equals_ignore_case(function, "MEMO_1")) {
			printf("Received MEMO_1 \n");
			handle_new_process("MEMORIA_1", 128, 1);
			start_planning();
		} else if (string_equals_ignore_case(function, "MEMO_2")) {
			printf("Received MEMO_2 \n");
			handle_new_process("MEMORIA_2", 64, 1);
			handle_new_process("MEMORIA_2", 64, 1);			
			handle_new_process("MEMORIA_2", 64, 1);
			handle_new_process("MEMORIA_2", 64, 1);
			start_planning();
		} else if (string_equals_ignore_case(function, "MEMO_3")) {
			printf("Received MEMO_3 \n");
			handle_force_process_exit(short_term_planner, memory, 2);
			handle_force_process_exit(short_term_planner, memory, 3);
			handle_force_process_exit(short_term_planner, memory, 4);
			handle_force_process_exit(short_term_planner, memory, 5);
			update_max_multi_programming(1);
			handle_new_process("MEMORIA_2", 64, 1);
			handle_new_process("MEMORIA_2", 64, 1);			
			handle_new_process("MEMORIA_2", 64, 1);
			handle_new_process("MEMORIA_2", 64, 1);
			start_planning();
		} else if (string_equals_ignore_case(function, "FS")) {
			printf("Received FS \n");
			handle_new_process("FS_A", 64, 1);
			handle_new_process("FS_B", 64, 1);
			handle_new_process("FS_C", 64, 1);
			handle_new_process("FS_D", 16, 1);
			handle_new_process("FS_E", 64, 1);
			handle_new_process("ERROR_3", 64, 1);
			start_planning();
		} else if (string_equals_ignore_case(function, "ESTRES")) {
			printf("Received ESTRES \n");
			handle_new_process("ESTRES_1", 64, 1);
			handle_new_process("ESTRES_2", 64, 1);
			handle_new_process("ESTRES_3", 64, 1);
			handle_new_process("ESTRES_3", 64, 1);
			handle_new_process("ESTRES_3", 64, 1);
			handle_new_process("ESTRES_3", 64, 1);
			handle_new_process("ESTRES_4", 256, 1);
			start_planning();
		}
	}
}

bool is_valid_alpha(char* alpha) {
	return (alpha != NULL && isalpha(*alpha) != 0);
}

bool is_valid_digit(char* number) {
	return (number != NULL && isdigit(*number) != 0);
}

bool validate_init_process(char** parameters) {
	char* path = parameters[1];
	char* size = parameters[2];
	char* priority = parameters[3];
	char* parameter_errors = string_new();

	int length = string_array_size(parameters);

	if (length <= 1) string_append(&parameter_errors, " needs path,");
	if (length > 1 && !is_valid_alpha(path)) string_append(&parameter_errors, " invalid path type,");
	if (length <= 2) string_append(&parameter_errors, " needs size,");
	if (length > 2 && !is_valid_digit(size)) string_append(&parameter_errors, " invalid size type,");
	if (length <= 3) string_append(&parameter_errors, " needs priority,");
	if (length > 3 && !is_valid_digit(priority)) string_append(&parameter_errors, " invalid priority type,");

	if (!string_is_empty(parameter_errors)) {
		char* error_message = string_new();
		string_append(&error_message, "INICIAR_PROCESO");
		string_append(&error_message, parameter_errors);
		char* final_error_message = string_substring(error_message, 0, string_length(error_message) - 1);
		printf("%s\n", final_error_message);
		return 0;
	}
	return 1;
}

bool validate_finish_process(char** parameters) {
	char* process_pid = parameters[1];
	char* parameter_errors = string_new();

	int length = string_array_size(parameters);

	if (length <= 1) string_append(&parameter_errors, " needs process id,");
	if (length > 1 && !is_valid_digit(process_pid)) string_append(&parameter_errors, " invalid process id type,");

	if (!string_is_empty(parameter_errors)) {
		char* error_message = string_new();
		string_append(&error_message, "FINALIZAR_PROCESO");
		string_append(&error_message, parameter_errors);
		char* final_error_message = string_substring(error_message, 0, string_length(error_message) - 1);
		printf("%s\n", final_error_message);
		return 0;
	}
	return 1;
}

bool validate_multiprogramming_degree(char** parameters) {
	char* programming_degree = parameters[1];
	char* parameter_errors = string_new();

	int length = string_array_size(parameters);

	if (length <= 1) string_append(&parameter_errors, " needs multiprogramming degree,");
	if (length > 1 && !is_valid_digit(programming_degree)) string_append(&parameter_errors, " invalid multiprogramming degree type,");

	if (!string_is_empty(parameter_errors)) {
		char* error_message = string_new();
		string_append(&error_message, "MULTIPROGRAMACION");
		string_append(&error_message, parameter_errors);
		char* final_error_message = string_substring(error_message, 0, string_length(error_message) - 1);
		printf("%s\n", final_error_message);
		return 0;
	}
	return 1;
}
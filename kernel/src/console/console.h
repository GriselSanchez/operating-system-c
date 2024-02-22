#ifndef SRC_KERNEL_CONSOLE_H
#define SRC_KERNEL_CONSOLE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <commons/log.h>
#include <commons/string.h>
#include <readline/readline.h>
#include "core/process/process_handler.h"

void start_interactive_console(t_kernel_memory* memory, t_planner* short_term_planner);
bool is_valid_alpha(char*);
bool is_valid_digit(char*);
bool validate_init_process(char**);
bool validate_finish_process(char**);
bool validate_multiprogramming_degree(char**);

#endif /* SRC_KERNEL_CONSOLE_H */

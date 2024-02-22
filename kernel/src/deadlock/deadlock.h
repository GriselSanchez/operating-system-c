#ifndef DEADLOCK_H
#define DEADLOCK_H


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils/lista_monitor.h"
#include "model/pcb.h"

extern t_log* logger;



// Funcion que chequea si hay deadlock potenciales, pasandole la 
//relacion en entre procesos y recursos
bool checkForDeadlock(t_pcb* pcb, char* required_resource_name, t_lista_mutex* running_process_contexts);

#endif
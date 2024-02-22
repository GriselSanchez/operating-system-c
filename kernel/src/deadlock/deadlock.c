#include "deadlock.h"
#include <string.h>

t_list* get_running_processes_by_asigned_resource(char* required_resource_name, t_lista_mutex* running_process_contexts, bool blockedProcessesOnly) {
    bool _filter_resource(void* asigned_resource) {
        return string_equals_ignore_case(required_resource_name, (char*)asigned_resource);
    }

    bool _filter(void* element) {
        t_pcb* pcb = element;
        return list_any_satisfy(pcb->asigned_resources, &_filter_resource) && ( ! blockedProcessesOnly || pcb->current_status == BLOCK);
    }

    return list_filter_mutex(running_process_contexts, &_filter);
}

/*
    4 recursos, 1 instancia c/u:
    P1 <- REC1
    P2 <- REC2
    P3 <- REC3
    P4 <- REC4
    
    P1 <- REC2 -> BLOCK
    P2 <- REC3 -> BLOCK
    P3 <- REC4 -> BLOCK
    P4 <- REC1 -> BLOCK
------------> aca hay deadlock

    P1 = [REC1, REC2]
    P2 = [REC2, REC3]
    P3 = [REC3, REC4]
    P4 = [REC4, REC1]

    matas P1 -> termina todo ok!

    asi viene:
        - proceso bloqueado 
        - recurso agotado

*/

bool _checkForDeadlock(t_pcb* pcb, char* required_resource_name, t_lista_mutex* running_process_contexts) {
    // Al entrar en esta función asumimos que el recurso deseado NO está disponible. Si lo está, nunca se llama a esta función.
    //   1. Tengo que consultar quien (o quienes) lo tienen asignado
    //     Para cada uno de los procesos que tienen asignado el recurso deseado, consultar si están esperando algún recurso
    //       - Si no están esperando nada, entonces no hay deadlock
    //       - Si están esperando uno o más recursos, entonces:
    //          Por cada recurso que están esperando...
    //            consultar quien (o quienes) lo tienen asignado
    //            si el proceso inicial (el que desea el recurso) está en la lista de procesos anterior, entonces hay deadlock

    void* _get_blocked_by_resource_name(void* element) {
        t_pcb* pcb = element;
        
        return pcb->blocked_by_resource_name;
    }

    void* _get_procesos_que_tienen_recurso_asignado(void* element) {
        char* resource_name = (char*)element;
        
        return get_running_processes_by_asigned_resource(resource_name, running_process_contexts, false);
    }

    bool _contains_original_process(void* elements) {
        t_list* blocking_processes = elements;

        bool _filter_process(void* element) {
            return pcb->pid == ((t_pcb*) element)->pid;
        }
        if( list_is_empty(blocking_processes) ) {
            return false;
        }
        return list_any_satisfy(blocking_processes, &_filter_process);
    }

    bool _filter_not_null(void* element) {
        return NULL != element;
    }

    //voy a obtener los procesos que estan en estado BLOCK & que además tengan en su lista de recursos el que me bloquea a mi.
    t_list* procesos_bloqueados_que_tienen_asignado_el_recurso_que_deseo = get_running_processes_by_asigned_resource(required_resource_name, running_process_contexts, true);

    //obtengo una lista con los recursos que bloquean procesos (pcb->blocked_by_resource_name).
    t_list* recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo = 
        list_map(procesos_bloqueados_que_tienen_asignado_el_recurso_que_deseo, &_get_blocked_by_resource_name);
    t_list* recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo_sin_nulls = 
        list_filter(recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo, &_filter_not_null);
    
        
    //obtengo una lista con los procesos que tienen el recurso asignado de la lista anterior.    
    t_list* procesos_que_deben_terminar_para_poder_obtener_el_recurso_que_deseo = 
        list_map(recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo_sin_nulls, &_get_procesos_que_tienen_recurso_asignado);

    bool hay_deadlock = false;
    if (! list_all_satisfy(procesos_que_deben_terminar_para_poder_obtener_el_recurso_que_deseo, &_contains_original_process)) {
        // It's a me, Deadlock
        if (pcb->asigned_resources != NULL) {
            for (int i=0; i< pcb->asigned_resources->elements_count; i++) {
                log_info(logger, "Deadlock detectado: %d - Recursos en posesion: %s", pcb->pid, list_get(pcb->asigned_resources, i));
            }
        }
        log_info(logger, "Deadlock detectado: %d - Recurso requerido: %s.", pcb->pid, required_resource_name);

        hay_deadlock = true;
    } else {
        log_debug(logger, "No se detecto Deadlock para el proceso %d", pcb->pid);
    }

    list_destroy(procesos_bloqueados_que_tienen_asignado_el_recurso_que_deseo);
    list_destroy(recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo);
    list_destroy(recursos_que_deben_liberarase_para_poder_obtener_el_recurso_que_deseo_sin_nulls);
    list_destroy(procesos_que_deben_terminar_para_poder_obtener_el_recurso_que_deseo);
    return hay_deadlock;
}

bool checkForDeadlock(t_pcb* pcb, char* required_resource_name, t_lista_mutex* running_process_contexts) {
    log_info(logger, "ANALISIS DE DETECCION DE DEADLOCK");
    return _checkForDeadlock(pcb, required_resource_name, running_process_contexts);
}
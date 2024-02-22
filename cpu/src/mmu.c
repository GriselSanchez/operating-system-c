#include "mmu.h"

extern int tamanio_pagina;
extern t_log* logger;
extern t_cpu* cpu;

int obtener_numero_pagina(int dir_logica) {
    return floor(dir_logica / tamanio_pagina);
}

int obtener_desplazamiento(int dir_logica) {
    return dir_logica - obtener_numero_pagina(dir_logica) * tamanio_pagina;
}

int traduccion_logica_a_fisica(int dir_logica, int nro_marco) {
    return nro_marco * tamanio_pagina + obtener_desplazamiento(dir_logica);
}

int obtener_marco(int pagina, int pid, tipo_operacion_t operacion) {
    t_paquete* send_paquete = crear_paquete(FRAME_REQUEST);
    agregar_payload_a_paquete(send_paquete, &pid, sizeof(int));
    agregar_payload_a_paquete(send_paquete, &pagina, sizeof(int));
    agregar_payload_a_paquete(send_paquete, &operacion, sizeof(tipo_operacion_t));
    enviar_paquete_serializado_por_socket(cpu->memoria_fd, send_paquete);
    paquete_destroy(send_paquete);

    int marco = 0;
    recibir_payload(cpu->memoria_fd, &marco, sizeof(uint32_t));

    return marco;
}

uint32_t leer_valor_memoria(int dir_fisica, int pid) {
    t_paquete* send_paquete = crear_paquete(MOV_IN);
    agregar_payload_a_paquete(send_paquete, &dir_fisica, sizeof(int));
    agregar_payload_a_paquete(send_paquete, &pid, sizeof(int));
    enviar_paquete_serializado_por_socket(cpu->memoria_fd, send_paquete);
    paquete_destroy(send_paquete);

    uint32_t valor = 0;
    recibir_payload(cpu->memoria_fd, &valor, sizeof(uint32_t));

    return valor;
}

int escribir_valor_memoria(int dir_fisica, uint32_t valor, int pid) {
    t_paquete* send_paquete = crear_paquete(MOV_OUT);
    agregar_payload_a_paquete(send_paquete, &dir_fisica, sizeof(int));
    agregar_payload_a_paquete(send_paquete, &pid, sizeof(int));
    agregar_payload_a_paquete(send_paquete, &valor, sizeof(uint32_t));
    enviar_paquete_serializado_por_socket(cpu->memoria_fd, send_paquete);
    paquete_destroy(send_paquete);

    int result = 0;
    recibir_payload(cpu->memoria_fd, &result, sizeof(int));

    return result;
}

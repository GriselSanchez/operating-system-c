#include "serializacion.h"
#include <stdint.h>

#define sizeof_pkg(pkg) pkg->buffer->tamanio+2*sizeof(int)
#define STRING_LEN(len) sizeof(char) * (len)

t_paquete* crear_paquete(op_code codigo_operacion) {
  t_paquete* paquete = malloc(sizeof(t_paquete));

  paquete->codigo_operacion = codigo_operacion;
  paquete->buffer = malloc(sizeof(t_buffer));
  paquete->buffer->tamanio = 0;
  paquete->buffer->payload = NULL;

  return paquete;
}

void agregar_payload_a_paquete(t_paquete* paquete, void* nuevo_payload, int nuevo_tamanio) {
  // Realoco memoria con un mayor tamanio
  paquete->buffer->payload = realloc(paquete->buffer->payload, paquete->buffer->tamanio + nuevo_tamanio);

  // Copio el nuevo payload at final del payload anterior, por eso le sumo el tamanio
  memcpy(paquete->buffer->payload + paquete->buffer->tamanio, nuevo_payload, nuevo_tamanio);

  // Actualizo el tamanio anterior del buffer
  paquete->buffer->tamanio += nuevo_tamanio;
}

void* serializar_paquete(t_paquete* paquete) {
  // Buffer nuevo que va a tener la info del paquete serializada
  int sizeof_paquete = sizeof_pkg(paquete);//paquete->buffer->tamanio+2*sizeof(int);
  void* serializado = malloc(sizeof_paquete);
  // Desplazamiento inicial
  int offset = 0;

  memcpy(serializado + offset, &(paquete->codigo_operacion), sizeof((paquete->codigo_operacion)));
  offset += sizeof(paquete->codigo_operacion);
  memcpy(serializado + offset, &(paquete->buffer->tamanio), sizeof((paquete->buffer->tamanio)));
  offset += sizeof(paquete->buffer->tamanio);
  memcpy(serializado + offset, paquete->buffer->payload, paquete->buffer->tamanio);
  //offset += paquete->buffer->tamanio;

  return serializado;
}

int enviar_paquete_serializado_por_socket(int socket, t_paquete* pkg) {
  int sizeof_pkg = sizeof_pkg(pkg);//pkg->buffer->tamanio+2*sizeof(int);
  void* serialized_pkg = serializar_paquete(pkg);
  int status = send(socket, serialized_pkg, sizeof_pkg, 0);
  free(serialized_pkg);
  return status;
}

t_paquete* deserealizar_paquete_desde_socket(int socket) {
  t_paquete* paquete = malloc(sizeof(t_paquete));
  paquete->buffer = malloc(sizeof(t_buffer));

  // Primero recibimos el codigo de operacion
  recv(socket, &(paquete->codigo_operacion), sizeof(int), 0);

  // Después ya podemos recibir el buffer. 
  // Primero su tamaño seguido del contenido
  recv(socket, &(paquete->buffer->tamanio), sizeof(int), 0);
  paquete->buffer->payload = malloc(paquete->buffer->tamanio);
  recv(socket, paquete->buffer->payload, paquete->buffer->tamanio, 0);

  return paquete;
}

void paquete_destroy(t_paquete* paquete) {
  free(paquete->buffer->payload);
  free(paquete->buffer);
  free(paquete);
}

char* OP_CODE_NAME(op_code code) {
  if (code == SLEEP) {
    return "SLEEP\0";
  } else if (code == PROC_EXIT_SEG_FAULT) {
    return "SEG_FAULT\0";
  } else if (code == PROC_EXIT_OUT_OF_MEMORY) {
    return "OUT_OF_MEMORY\0";
  } else if (code == PROC_EXIT_RESOURCE_NOT_FOUND) {
    return "RESOURCE_NOT_FOUND\0";
  } else if (code == PROC_EXIT_FORCE) {
    return "EXIT_FORCED\0";
  } else if (code == PROC_EXIT_INVALID_WRITE) {
    return "INVALID_WRITE\0";
  } else if (code == PROC_EXIT_QUANTUM) {
    return "EVICTION_QUANTUM\0";
  } else if (code == PROC_EXIT_PRIORITY) {
    return "EVICTION_PRIORITY\0";
  } else if (code == PROC_EXIT_SUCCESS) {
    return "SUCCESS\0";
  } else {
    return "OP_CODE OR OP_CODE_NAME UNDEFINED\0";
  }
}

char* deserializar_string(void* payload, int* real_offset) {
  int offset = 0;
  int length;

  memcpy(&length, (payload + offset), sizeof(int));
  offset += sizeof(int);

  char* operacion = malloc(STRING_LEN(length) + 1);
  memcpy(operacion, (payload + offset), STRING_LEN(length));
  operacion[length] = '\0';
  offset += STRING_LEN(length);

  if (real_offset != NULL) *real_offset = (*real_offset) + offset;
  return operacion;
}

void* serialize_string(t_paquete* pkg, char* str) {
  char* aux_str = strdup(str);
  int str_len = strlen(aux_str);
  agregar_payload_a_paquete(pkg, &str_len, sizeof(int));
  agregar_payload_a_paquete(pkg, aux_str, STRING_LEN(str_len));
  free(aux_str);
  return pkg;
}
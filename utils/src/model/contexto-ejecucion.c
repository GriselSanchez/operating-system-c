#include "contexto-ejecucion.h"

#define STRING_LEN(len) sizeof(char) * (len)

void update_pcb_from_context(t_exec_context* ctx, t_pcb* pcb) {
  //TODO : MAXO : copiar esta data y dsp. liberarla!!
  if (pcb->pid != ctx->pid) false; // ROMPER ??
  pcb->program_counter = ctx->program_counter;
  pcb->cpu_registries = ctx->cpu_registries;
  pcb->page_fault_nro = ctx->page_fault_nro;
}

t_exec_context* context_from_pcb(t_pcb* pcb) {
  //TODO : MAXO : copiar esta data y dsp. liberarla!!
  t_exec_context* ctx = malloc(sizeof(t_exec_context));
  ctx->pid = pcb->pid;
  ctx->program_counter = pcb->program_counter;
  ctx->cpu_registries = pcb->cpu_registries;
  ctx->page_fault_nro = pcb->page_fault_nro;
  ctx->resource_name = pcb->resource_name == NULL ? NULL : strdup(pcb->resource_name);

  //Necesario para que corra
  ctx->nombre_archivo = pcb->nombre_archivo == NULL ? NULL : strdup(pcb->nombre_archivo);
  ctx->modo_apertura = pcb->modo_apertura;
  ctx->puntero_archivo = pcb->puntero_archivo;
  ctx->dir_fisica = pcb->dir_fisica;
  ctx->tamanio_archivo = pcb->tamanio_archivo;

  return ctx;
}

t_exec_context* recv_execution_context_via_socket(int fd_socket, op_code* operation_code) {
  t_paquete* paquete_deserializado = deserealizar_paquete_desde_socket(fd_socket);
  t_exec_context* contexto_recibido = deserialize_execution_context(paquete_deserializado->buffer->payload);
  if (operation_code != NULL) *operation_code = paquete_deserializado->codigo_operacion;

  paquete_destroy(paquete_deserializado);
  return contexto_recibido;
}

void send_execution_context_via_socket(t_exec_context* ctx, op_code exec_code, int fd_socket) {
  t_paquete* paquete = crear_paquete(exec_code);
  void* data_serializada = serialize_execution_context(paquete, ctx);

  enviar_paquete_serializado_por_socket(fd_socket, paquete);

  paquete_destroy(paquete);
  free(data_serializada);
}

void* serialize_execution_context(t_paquete* packet, t_exec_context* ctx) {

  agregar_payload_a_paquete(packet, &ctx->pid, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->program_counter, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->block_time, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->page_fault_nro, sizeof(int));

  if (ctx->resource_name != NULL) {
    int resource_name_len = strlen(ctx->resource_name);
    agregar_payload_a_paquete(packet, &resource_name_len, sizeof(int));
    agregar_payload_a_paquete(packet, ctx->resource_name, sizeof(char) * resource_name_len);
  } else {
    int empty_resource_name_len = 0;
    agregar_payload_a_paquete(packet, &empty_resource_name_len, sizeof(int));
  }

  serializar_registros(packet, ctx->cpu_registries);

  if (ctx->nombre_archivo != NULL) {
    int nombre_archivo_len = strlen(ctx->nombre_archivo);
    agregar_payload_a_paquete(packet, &nombre_archivo_len, sizeof(int));
    agregar_payload_a_paquete(packet, ctx->nombre_archivo, sizeof(char) * nombre_archivo_len);
  } else {
    int empty_resource_name_len = 0;
    agregar_payload_a_paquete(packet, &empty_resource_name_len, sizeof(int));
  }

  agregar_payload_a_paquete(packet, &ctx->modo_apertura, sizeof(modo_apertura));
  agregar_payload_a_paquete(packet, &ctx->puntero_archivo, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->dir_fisica, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->tamanio_archivo, sizeof(int));
  agregar_payload_a_paquete(packet, &ctx->interrupt_motive, sizeof(int));

  return serializar_paquete(packet);
}

t_registros* deserializar_registros(void* payload, int* real_offset) {
  t_registros* reg = malloc(sizeof(t_registros));

  int offset = 0;
  for (int i = 0; i < 4; i++) {
    memcpy(&reg->registros_4bytes[i], payload + offset, sizeof(uint32_t));
    offset += sizeof(uint32_t);
  }
  if (real_offset != NULL) *real_offset = (*real_offset) + offset;

  return reg;
}

void serializar_registros(t_paquete* package, t_registros* reg) {
  for (int i = 0; i < 4; i++) {
    agregar_payload_a_paquete(package, &reg->registros_4bytes[i], sizeof(uint32_t));
  }
}

t_exec_context* deserialize_execution_context(void* payload) {
  t_exec_context* new_context = malloc(sizeof(t_exec_context));
  int resource_name_len = 0, nombre_archivo_len = 0, offset = 0;

  memcpy(&new_context->pid, payload, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->program_counter, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->block_time, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->page_fault_nro, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&resource_name_len, payload + offset, sizeof(int));
  offset += sizeof(int);
  if (resource_name_len > 0) {
    new_context->resource_name = malloc(STRING_LEN(resource_name_len) + 1);
    memcpy(new_context->resource_name, (payload + offset), STRING_LEN(resource_name_len));
    new_context->resource_name[resource_name_len] = '\0';
    offset += STRING_LEN(resource_name_len);
  } else {
    new_context->resource_name = NULL;
  }

  new_context->cpu_registries = deserializar_registros(payload + offset, &offset);

  memcpy(&nombre_archivo_len, payload + offset, sizeof(int));
  offset += sizeof(int);
  if (nombre_archivo_len > 0) {
    new_context->nombre_archivo = malloc(STRING_LEN(nombre_archivo_len) + 1);
    memcpy(new_context->nombre_archivo, (payload + offset), STRING_LEN(nombre_archivo_len));
    new_context->nombre_archivo[nombre_archivo_len] = '\0';
    offset += STRING_LEN(nombre_archivo_len);
  } else {
    new_context->nombre_archivo = NULL;
  }

  memcpy(&new_context->modo_apertura, payload + offset, sizeof(modo_apertura));
  offset += sizeof(modo_apertura);

  memcpy(&new_context->puntero_archivo, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->dir_fisica, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->tamanio_archivo, payload + offset, sizeof(int));
  offset += sizeof(int);

  memcpy(&new_context->interrupt_motive, payload + offset, sizeof(int));
  offset += sizeof(int);

  return new_context;
}

void destroy_exec_context(t_exec_context* ctx) {
  if (ctx->resource_name != NULL) {
    free(ctx->resource_name);
  }

  if (ctx->nombre_archivo != NULL) {
    free(ctx->nombre_archivo);
  }

  if (ctx->cpu_registries != NULL) {
    free(ctx->cpu_registries);
  }

  free(ctx);
}
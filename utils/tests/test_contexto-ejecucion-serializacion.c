#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <commons/string.h>
#include <cspecs/cspec.h>
#include <commons/log.h>
#include <commons/collections/list.h>
#include "utils/serializacion.h"
#include "model/instrucciones.h"
#include "model/contexto-ejecucion.h"

t_log* logger;

void populate_registries(t_registros* reg) {
  for (int i = 0; i < 4; i++) {
    reg->registros_4bytes[i] = 1024;
  }
}

context(contexto_ejecucion) {

  describe("Serializacion Contexto Ejecucion") {
    before{
      logger = log_create("tests.log", "Test", 0, LOG_LEVEL_DEBUG);
    } end

      after{
        log_destroy(logger);
    } end

      it("Dados los registros de la CPU los serealizo.") {
      t_registros* reg = initialize_registries();
      for (int i = 0; i < 4; i += 2) {
        reg->registros_4bytes[i] = 1;
      }

      t_paquete* pkg = crear_paquete(CONTEXTO_EJECUCION);
      serializar_registros(pkg, reg);

      uint32_t regs[4];
      int regs_len = pkg->buffer->tamanio;
      memcpy(regs, pkg->buffer->payload, sizeof(uint32_t) * 4);

      should_int(regs[0]) be equal to(1);
      should_int(regs[1]) be equal to(0);
      should_int(regs[2]) be equal to(1);
      should_int(regs[3]) be equal to(0);
      should_int(regs_len) be equal to(16);
    } end

    it("Dado un contexto de ejecucion lo serealizo.") {

      t_pcb* pcb = create_new_pcb("un_path.x", 16, 0);
      pcb->pid = 2;
      pcb->cpu_registries = initialize_registries();
      pcb->program_counter = 1;
      populate_registries(pcb->cpu_registries);
      t_exec_context* ctx = context_from_pcb(pcb);
      ctx->block_time = 0;
      ctx->page_fault_nro = 2;
      ctx->resource_name = malloc(sizeof(char) * 6);
      memcpy(ctx->resource_name, "DISCO\0", sizeof(char) * 6);

      /*
      * Lo serealizo
      */

      void* serialized_pkg = serialize_execution_context(crear_paquete(CONTEXTO_EJECUCION), ctx);

      op_code code;
      int pkg_len, offset, pid, prog_counter;
      int block_time, page_fault_nro, res_name_len;
      char* res_name;
      uint32_t regs[4];
      memcpy(&code, serialized_pkg, sizeof(int));
      offset = sizeof(int);
      memcpy(&pkg_len, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&pid, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&prog_counter, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&block_time, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);
      memcpy(&page_fault_nro, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);

      memcpy(&res_name_len, serialized_pkg + offset, sizeof(int));
      offset += sizeof(int);
      res_name = malloc(sizeof(char) * res_name_len + 1);
      memcpy(res_name, serialized_pkg + offset, sizeof(char) * res_name_len);
      offset += sizeof(char) * res_name_len;
      res_name[res_name_len] = '\0';

      memcpy(regs, serialized_pkg + offset, sizeof(uint32_t) * 4);

      should_int(code) be equal to(CONTEXTO_EJECUCION);
      //pkg_len =   4*sizeof(int) <pkg_len + pid + prog_counter + block_time + page_fault_nro> 
      //          + 5*sizeof(char) <res_name = DISCO>
      //          + 4*sizeof(uint32_t) <registros>;
      should_int(pkg_len) be equal to(41);
      should_int(pid) be equal to(2);
      should_int(prog_counter) be equal to(1);
      should_int(block_time) be equal to(0);
      should_int(page_fault_nro) be equal to(2);
      should_string(res_name) be equal to("DISCO");
      should_int(regs[0]) be equal to(1024);
      should_int(regs[1]) be equal to(1024);
      should_int(regs[2]) be equal to(1024);
      should_int(regs[3]) be equal to(1024);

      free(serialized_pkg);
      finish_pcb(pcb);
    } end

    it("Dado un mensaje de contexto de ejecucion serializado lo deserealizo.") {
      /*
      * Dado un contexto de ejecucion serializado desde el kernel
      */
      void* msg_serializado = malloc(sizeof(int) * 2 + sizeof(int) * 10 + sizeof(char) * 19 + sizeof(char) * 87);
      int offset = 0;
      int block_time, page_fault_nro, resource_name_len;
      //PID
      int value = 1; memcpy(msg_serializado + offset, &value, sizeof(int));
      offset += sizeof(int);
      //PROG COUNTER
      value = 0; memcpy(msg_serializado + offset, &value, sizeof(int));
      offset += sizeof(int);
      //BLOCK TIME
      block_time = 3; memcpy(msg_serializado + offset, &block_time, sizeof(int));
      offset += sizeof(int);
      page_fault_nro = 5; memcpy(msg_serializado + offset, &page_fault_nro, sizeof(int));
      offset += sizeof(int);
      //LARGO NOMBRE RECURSO
      resource_name_len = 5; memcpy(msg_serializado + offset, &resource_name_len, sizeof(int));
      offset += sizeof(int);
      //NOMBRE RECURSO
      memcpy(msg_serializado + offset, "DISCO", sizeof(char) * 5);
      offset += sizeof(char) * 5;
      //REGISTROS 4bytes
      uint32_t reg_value[4] = { 1024, 1024, 1024, 1024 };
      memcpy(msg_serializado + offset, &reg_value, sizeof(uint32_t) * 4);
      offset += sizeof(uint32_t) * 4;

      /*
      * Lo deserealizo
      */
      t_exec_context* context = deserialize_execution_context(msg_serializado);

      /*
      * Cheque que sea correcto el contenido y la estructura
      * luego de su deserealzacion.
      */
      should_int(context->pid) be equal to(1);
      should_int(context->program_counter) be equal to(0);
      should_int(context->block_time) be equal to(3);
      should_int(context->page_fault_nro) be equal to(5);
      should_int(context->cpu_registries->registros_4bytes[0]) be equal to(1024);
      should_int(context->cpu_registries->registros_4bytes[1]) be equal to(1024);
      should_int(context->cpu_registries->registros_4bytes[2]) be equal to(1024);
      should_int(context->cpu_registries->registros_4bytes[3]) be equal to(1024);

    } end

  } end

}


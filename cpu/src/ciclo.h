#ifndef SRC_CICLO_H_
#define SRC_CICLO_H_

#include <string.h>
#include "cpu.h"
#include "registros.h"
#include "mmu.h"
#include "model/instrucciones.h"

#define INSTRUCTION_SET "SET"
#define INSTRUCTION_SUM "SUM"
#define INSTRUCTION_SUB "SUB"
#define INSTRUCTION_JNZ "JNZ"
#define INSTRUCTION_SLEEP "SLEEP"
#define INSTRUCTION_WAIT "WAIT"
#define INSTRUCTION_SIGNAL "SIGNAL"
#define INSTRUCTION_MOV_IN "MOV_IN"
#define INSTRUCTION_MOV_OUT "MOV_OUT"
#define INSTRUCTION_F_OPEN "F_OPEN"
#define INSTRUCTION_F_CLOSE "F_CLOSE"
#define INSTRUCTION_F_SEEK "F_SEEK"
#define INSTRUCTION_F_READ "F_READ"
#define INSTRUCTION_F_WRITE "F_WRITE"
#define INSTRUCTION_F_TRUNCATE "F_TRUNCATE"
#define INSTRUCTION_EXIT "EXIT"

void iniciar_ciclo_instruccion(t_exec_context* pcb, int client_fd);

// FETCH
void fetch(t_exec_context* pcb, t_instruccion* instruccion);
t_instruccion* obtener_siguiente_instruccion_de_memoria(t_exec_context* ctx, t_instruccion* instruccion_deserializada);

// DECODE
int decode(t_instruccion* instruccion, t_exec_context* pcb);
int comparar_operacion(t_instruccion* instruccion, char* nombre);

// EXECUTE
void execute_SET(t_exec_context* ctx, char* registro, char* nuevo_valor);
void execute_SUM(t_exec_context* ctx, char* registro_destino, char* registro_origen);
void execute_SUB(t_exec_context* ctx, char* registro_destino, char* registro_origen);
void execute_JNZ(t_exec_context* ctx, char* registro, char* instruccion);
void execute_SLEEP(t_exec_context* ctx, char* segundos);
void execute_WAIT(t_exec_context* ctx, char* recurso);
void execute_SIGNAL(t_exec_context* ctx, char* recurso);
int execute_MOV_IN(t_exec_context* ctx, char* registro, char* dir_logica);
int execute_MOV_OUT(t_exec_context* ctx, char* dir_logica, char* registro);
void execute_F_OPEN(t_exec_context* ctx, char* nombre_archivo, char* modo_apertura);
void execute_F_CLOSE(t_exec_context* ctx, char* nombre_archivo);
void execute_F_SEEK(t_exec_context* ctx, char* nombre_archivo, char* posicion);
int execute_F_READ(t_exec_context* ctx, char* nombre_archivo, char* dir_logica);
int execute_F_WRITE(t_exec_context* ctx, char* nombre_archivo, char* dir_logica);
void execute_F_TRUNCATE(t_exec_context* ctx, char* nombre_archivo, char* tamanio);
void execute_EXIT(t_exec_context* ctx);

// CHECK INTERRUPT
int check_interrupt(t_exec_context* pcb);

// LOGS
void log_ejecucion_sin_param(t_exec_context* pcb, char* instruction);
void log_ejecucion_1_param(t_exec_context* pcb, char* instruccion, char* param1);
void log_ejecucion_2_param(t_exec_context* pcb, char* instruccion, char* param1, char* param2);

#endif /* SRC_CICLO_H_ */

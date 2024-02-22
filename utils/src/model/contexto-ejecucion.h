#ifndef SRC_MODEL_CONTEXTO_EJECUCION_H_
#define SRC_MODEL_CONTEXTO_EJECUCION_H_


#include <stdint.h>
#include <string.h>
#include "model/pcb.h"
#include "utils/serializacion.h"
#include "model/instrucciones.h"

typedef struct {
	int pid;
	t_list* instructions;
	int program_counter;
	t_registros* cpu_registries;
	int block_time;
	char* resource_name;
	int page_fault_nro;
	op_code interrupt_motive;
	//t_segment_table *segments_table

	// FILE SYSTEM
	char* nombre_archivo;
	modo_apertura modo_apertura;
	int puntero_archivo;
	int dir_fisica;
	int tamanio_archivo;

}t_exec_context;

/**
 * @NAME: recv_execution_context_via_socket
 * @DESC: Recibe un contexto de ejecucion y lo deserealiza a partir del socket ingresado
 *        como parametro. Si `operation_code` no es NULL, devuelve el `op_code` correspondiente
 *        al paquete recibido.
 */
t_exec_context* recv_execution_context_via_socket(int fd_socket, op_code* operation_code);

/**
 * @NAME: send_execution_context_via_socket
 * @DESC: Envia el contexto de ejecucion pasado por paramtros, a traves del socket solicitado
 *        en `fd_socket`. Se debe enviar el codigo de operación en el parametro `exec_code`.
 */
void send_execution_context_via_socket(t_exec_context* ctx, op_code exec_code, int fd_socket);

/**
 * @NAME: context_from_pcb
 * @DESC: Crea un contexto de ejecucion para enviar a la CPU a partir de un PCB.
 */
t_exec_context* context_from_pcb(t_pcb* pcb);

/**
 * @NAME: pcb_from_context
 * @DESC: Actualiza un PCB a partir de un contexto de ejecucion (enviado desde la CPU).
 */
void update_pcb_from_context(t_exec_context* ctx, t_pcb* pcb);

/**
 * @NAME: serialize_execution_context
 * @DESC: Serializa el contexto de ejecucion agregandolo en el payload del `paquete`.
 *        El Paquete ya debe estar creado y con su respectivo codigo de operacion.
 */
void* serialize_execution_context(t_paquete* paquete, t_exec_context* ctx);

/**
 * @NAME: deserialize_execution_context
 * @DESC: Deserializa el `payload` que recibe, interpretandolo como un
 * 			  contexto de ejecucion. Crea y devuelve las estructuras necesarias.
 */
t_exec_context* deserialize_execution_context(void* payload);

/**
 *    !!! La expongo unicamente para los tests, ya que esto se
 *        resuelve dentro del serializado del contexto de ejecucion !!!
 */
void serializar_registros(t_paquete* package, t_registros* registros);

void destroy_exec_context(t_exec_context* ctx);

#endif /* SRC_MODEL_CONTEXTO_EJECUCION_H_ */

/*
 * MSE_OS_Core.c
 *
 *  Created on: mar. 2020
 *      Author:
 */

#include "MSE_OS_Core.h"


/************************************************************************************
 * 			Definición variables Globales
 ***********************************************************************************/

static osCrt crt_OS;


/**********************************************************************************/

/*************************************************************************************************
	 *  @brief Inicializa las tareas que correran en el OS.
     *
     *  @details
     *   Inicializa una tarea para que pueda correr en el OS implementado.
     *   El usuario debe llamar a esta funcion para cada tarea antes que inicie
     *   el OS.
     *
	 *  @param *entryPoint			Puntero a la función asociada a la tarea que se desea inicializar.
	 *  @param *task_init			Puntero a la estructura de la tarea que se desea inicializar.
	 *  @return     None.
***************************************************************************************************/
void os_InitTask(void *entryPoint, task *task_init)
{
	static uint8_t id = 0;				/*el id es una variable local pero statica que va
										 *incrementando a medida que se ingresa una nueva tarea*/

	/*
	 *Se efectua un chequeo del número de tareas que se encuentran en el OS y se verifica que no es mayor
	 *al máximo soportado por el OS. En el caso que sea igual o mayor no se inicializa la tarea y se
	 *carga en la variable de error del OS el código que se excedió en número de tareas
	 */

	if(crt_OS.quantity_task < MAX_TASK_NUMBER)  {

		task_init->stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;				/*Se configura el bit thumb en uno para
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 *indicar que solo se trabaja con
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 *instrucciones thumb
		 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 */
		task_init->stack[STACK_SIZE/4 - PC_REG] = (uint32_t)entryPoint;	 /*Se inicializa el registro PC del stack de
																		  *la tarea con la dirección de la función
																		  *asociada a la tarea, asignandole el
																		  *parámetro (ENTRY_POINT)
																		 */

		/*
		 *Se guarda en el stack el valor previo del LR ya que se necesita
		 *porque el valor del LR en la interrupción de PendSV_Handler
		 *cambia al llamar la funcion de cambio de contexto getContextoSiguiente
		 */
		task_init->stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;

		task_init->stack_pointer = (uint32_t) (task_init->stack + STACK_SIZE/4 - FULL_REG_STACKING_SIZE);

		/*
		 * En esta parte se asigna a las variables de la estructura de la tarea inicializada;
		 * el entryPoint (dirección de la función asociada a la tarea),
		 * el id que se assigna con la variable estatica que incrementa a cada inicialización de tarea,
		 * y el estado de la tarea que se inicializa con READY, todas se inicializan con ese estado.
		 */
		task_init->entry_point = entryPoint;
		task_init->id = id;
		task_init->state = READY;

		/*
		 *Se guarda en el vector de tareas de la estructura de control del sistema operativo la tarea
		 *inicializada en la posición id que es correlativa al ingreso de tareas y se incrementa
		 *la variable que contiene la información de la cantidad de tareas del Sistema Operativo
		 */
		crt_OS.taskList[id] = task_init;
		crt_OS.quantity_task++;

		id++;
	}

	else {
		crt_OS.err = ERR_OS_QUANTITY_TASK; /*Se anexa en la variable err de la estructura de
											*control del os el error definido que se excedió el
											*número máximo de tareas permitidas por el os
											*/
	}
}

/*************************************************************************************************
	 *  @brief Inicializa el OS.
     *
     *  @details
     *   Inicializa el Sistema operativo configurando la prioridad de la
     *   interrupción PendSV como la más baja. Se debe llamar esta función antes de
     *   iniciar el sistema y es importante y mandatorio llamarla después de inicializar las tareas
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
void os_Init(void) {
	/*
	 * Todas las interrupciones tienen prioridad 0 (la maxima) al iniciar la ejecucion. Para que
	 * no se de la condicion de fault mencionada en la teoria, debemos bajar su prioridad en el
	 * NVIC. La cuenta matematica que se observa da la probabilidad mas baja posible.
	 */
	NVIC_SetPriority(PendSV_IRQn, (1 << __NVIC_PRIO_BITS)-1); // @suppress("Symbol is not resolved")

	/*
	 * Al iniciar el OS se especifica que se encuentra en la primer ejecucion desde un reset.
	 * Este estado es util para cuando se debe ejecutar el primer cambio de contexto. Los
	 * punteros de tarea_actual y tarea_siguiente solo pueden ser determinados por el scheduler
	 */
	crt_OS.state = FROM_RESET;
	crt_OS.current_task = NULL;
	crt_OS.next_task = NULL;


	/*
	 * El vector de tareas termina de inicializarse asignando NULL a las posiciones que estan
	 * luego de la ultima tarea. Esta situacion se da cuando se definen menos de 8 tareas.
	 * Estrictamente no existe necesidad de esto, solo es por seguridad.
	 */

	for (uint8_t i = 0; i < MAX_TASK_NUMBER; i++)  {
		if(i>=crt_OS.quantity_task)
			crt_OS.taskList[i] = NULL;
	}
}

/*************************************************************************************************
	 *  @brief Funcion que efectua las decisiones de scheduling.
     *
     *  @details
     *
     *   La función contiene el scheduler que determinar la siguiente tarea que se va a ejecutar
     *   verificando el estado del OS.
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
static void scheduler(void){

	/*
	 * Se verifica si el estado del Sistema Operativo es después de un Reset, en ese caso
	 * la tarea actual se carga con la primera tarea de la lista de la estructura del control de OS
	 */
	if(crt_OS.state == FROM_RESET){
		crt_OS.current_task = (task*) crt_OS.taskList[0];
	}
	else {
		/*
		 * En este apartado se verifica que el id de la tarea actual más uno se encuentre dentro
		 * del rango de cantidad de tareas en el OS, comparando que sea menor a la variable cantidad
		 * de tareas en el OS.
		 * Si está en el rango, a la tarea siguiente se le asigna la tarea de indice siguiente a la actual
		 * si no está en el rango se le asigna la primera tarea de la lista
		 */
		if((crt_OS.current_task->id + 1) < crt_OS.quantity_task){
			crt_OS.next_task = crt_OS.taskList[crt_OS.current_task->id + 1];
		}
		else{
			crt_OS.current_task = (task*) crt_OS.taskList[0];
		}
	}

}

/*************************************************************************************************
	 *  @brief SysTick Handler.
     *
     *  @details
     *   El handler del Systick no debe estar a la vista del usuario. En este handler se llama al
     *   scheduler y luego de determinarse cual es la tarea siguiente a ejecutar, se setea como
     *   pendiente la excepcion PendSV.
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
void SysTick_Handler(void)  {

	/*
	 * Dentro del SysTick handler se llama al scheduler. Separar el scheduler de
	 * getContextoSiguiente da libertad para cambiar la politica de scheduling en cualquier
	 * estadio de desarrollo del OS. Recordar que scheduler() debe ser lo mas corto posible
	 */

	scheduler();

	/**
	 * Se setea el bit correspondiente a la excepcion PendSV
	 */
	SCB->ICSR = SCB_ICSR_PENDSVSET_Msk;

	/**
	 * Instruction Synchronization Barrier; flushes the pipeline and ensures that
	 * all previous instructions are completed before executing new instructions
	 */
	__ISB();

	/**
	 * Data Synchronization Barrier; ensures that all memory accesses are
	 * completed before next instruction is executed
	 */
	__DSB();
}


/*************************************************************************************************
	 *  @brief Funcion para determinar el proximo contexto.
     *
     *  @details
     *   Esta funcion obtiene el siguiente contexto a ser cargado. El cambio de contexto se
     *   ejecuta en el handler de PendSV, dentro del cual se llama a esta funcion
     *
	 *  @param 		sp_actual	Este valor es una copia del contenido de MSP al momento en
	 *  			que la funcion es invocada.
	 *  @return     El valor a cargar en MSP para apuntar al contexto de la tarea siguiente.
***************************************************************************************************/
uint32_t getNextContext(uint32_t sp_current)  {
	uint32_t sp_next;


	/*
	 * Cuando el estado del OS es desde Reset se asigna a la siguiente tarea la tarea actua del control
	 * del OS y se cambia su estado por el de RUNNING ya que las demás tareas tienen el estado de READY.
	 * El estado del Sistema Operativo se cambia a NORMAL_RUN, actualizando de Reset
	 */

	if (crt_OS.state == FROM_RESET)  {
		sp_next = crt_OS.current_task->stack_pointer;
		crt_OS.current_task->state = RUNNING;
		crt_OS.state = NORMAL_RUN;
	}

	/*
	 * Cuando el estado del sistema es diferente a FROM_RESET se ejecuta esta parte de la funcion en la
	 * cual el SP actual se guarda en la estructura de la tarea que va a dejar de ejecutarse y se actualiza su estado
	 * por READY. Luego la variable SP que retorna se le asigna el SP de la tarea siguiente del
	 * control del OS. Se actualiza la variable de la tarea actual del control con la tarea siguiente y se
	 * cambia de estado a estado RUNNING.
	 */
	else {
		crt_OS.current_task->stack_pointer = sp_current;
		crt_OS.current_task->state = READY;

		sp_next = crt_OS.next_task->stack_pointer;

		crt_OS.current_task = crt_OS.next_task;
		crt_OS.current_task->state = RUNNING;
	}

	return sp_next;
}

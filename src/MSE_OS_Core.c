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
static task g_idleTask;

/**********************************************************************************/

/************************************************************************************
 * 			Definición de funciones estaticas, para no ser vistas y editadas por usuario
 ***********************************************************************************/

static void initIdleTask(void);

/*==================[definicion de hooks debiles]=================================*/

/*
 * Esta seccion contiene los hooks de sistema, los cuales el usuario del OS puede
 * redefinir dentro de su codigo y poblarlos segun necesidad
 */


/*************************************************************************************************
	 *  @brief Hook de retorno de tareas
     *
     *  @details
     *   Esta funcion no deberia accederse bajo ningun concepto, porque ninguna tarea del OS
     *   debe retornar. Si lo hace, es un comportamiento anormal y debe ser tratado.
     *
	 *  @param none
	 *
	 *  @return none.
***************************************************************************************************/
void __attribute__((weak)) returnHook(void)  {
	while(1);
}



/*************************************************************************************************
	 *  @brief Hook de tick de sistema
     *
     *  @details
     *   Se ejecuta cada vez que se produce un tick de sistema. Es llamada desde el handler de
     *   SysTick.
     *
	 *  @param none
	 *
	 *  @return none.
	 *
	 *  @warning	Esta funcion debe ser lo mas corta posible porque se ejecuta dentro del handler
     *   			mencionado, por lo que tiene prioridad sobre el cambio de contexto y otras IRQ.
	 *
	 *  @warning 	Esta funcion no debe bajo ninguna circunstancia utilizar APIs del OS dado
	 *  			que podria dar lugar a un nuevo scheduling.
***************************************************************************************************/
void __attribute__((weak)) tickHook(void)  {
	__asm volatile( "nop" );
}



/*************************************************************************************************
	 *  @brief Hook de error de sistema
     *
     *  @details
     *   Esta funcion es llamada en caso de error del sistema, y puede ser utilizada a fin de hacer
     *   debug. El puntero de la funcion que llama a errorHook es pasado como parametro para tener
     *   informacion de quien la esta llamando, y dentro de ella puede verse el codigo de error
     *   en la estructura de control de sistema. Si ha de implementarse por el usuario para manejo
     *   de errores, es importante tener en cuenta que la estructura de control solo esta disponible
     *   dentro de este archivo.
     *
	 *  @param caller		Puntero a la funcion donde fue llamado errorHook. Implementado solo a
	 *  					fines de trazabilidad de errores
	 *
	 *  @return none.
***************************************************************************************************/
void __attribute__((weak)) errorHook(void *caller)  {
	/*
	 * Revisar el contenido de control_OS.error para obtener informacion. Utilizar os_getError()
	 */
	while(1);
}


/*************************************************************************************************
	 *  @brief Tarea Idle
     *
     *  @details
     *   Esta función tiene como atributo dèbil para que pueda ser sobre escrita por el usuario.
     *	En el caso de no ser sobre escrita llama a la función WFI para que el procesador
     *	entre en estado de baja potencia
     *
	 *  @param none
	 *  @return none.
***************************************************************************************************/
void __attribute__((weak)) idleTask(void)  {
	while(1){
		__WFI();
	}
}

/*************************************************************************************************
	 *  @brief Función que configura el estado de determinada tarea
     *
     *  @details
	 *  Función que asigna a una tarea con id determinado estado
     *
	 *  @param id			Id de la tarea que a la que se desea cambiar el estado
	 *  @param state		estado que de la tarea a cambiar
	 *  @see errorHook
***************************************************************************************************/

void setStateTask(uint8_t id,taskState state)
{
	int i = 0;
	for(i = 0; i < crt_OS.quantity_task; i++)
	{
		if(crt_OS.taskList[i]->id == id)
			crt_OS.taskList[i]->state = state;
	}
}

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
		task_init->stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;		/*Se configura el registro Linker return al hook de retorno
																		* En el caso de que alguna tarea retorne,
																		* no deberia pasar nunca, si pasa hay un error.
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
		errorHook(os_InitTask); /*Se llama al hook de error con parametro de la función donde
		 	 	 	 	 	 	  *ha pasado el error
		 	 	 	 	 	 	  **/
	}
}

/*************************************************************************************************
	 *  @brief Inicializa la tarea idle del OS
     *
     *  @details
     *   Inicializa una tarea estatica idle que se debe llamar en el sistema y no debe ser vista ni
     *   editada por el usuario
     *
	 *  @return     None.
	 *  @return     None.
***************************************************************************************************/
static void initIdleTask(void)
{
	g_idleTask.stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;				/*Se configura el bit thumb en uno para
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 *indicar que solo se trabaja con
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 *instrucciones thumb
	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 	 */
	g_idleTask.stack[STACK_SIZE/4 - PC_REG] = (uint32_t)idleTask;	 /*Se inicializa el registro PC del stack de
																	  *la tarea con la dirección de la función
																	  *asociada a la tarea, asignandole el
																	  *parámetro (ENTRY_POINT)
																	 */
	g_idleTask.stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;		/*Se configura el registro Linker return al hook de retorno
																	* En el caso de que alguna tarea retorne,
																	* no deberia pasar nunca, si pasa hay un error.
																	*/
	/*
	 *Se guarda en el stack el valor previo del LR ya que se necesita
	 *porque el valor del LR en la interrupción de PendSV_Handler
	 *cambia al llamar la funcion de cambio de contexto getContextoSiguiente
	 */
	g_idleTask.stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;

	g_idleTask.stack_pointer = (uint32_t) (g_idleTask.stack + STACK_SIZE/4 - FULL_REG_STACKING_SIZE);

	/*
	 * En esta parte se asigna a las variables de la estructura de la tarea inicializada;
	 * el entryPoint (dirección de la función asociada a la tarea),
	 * el id que se assigna con la variable estatica que incrementa a cada inicialización de tarea,
	 * y el estado de la tarea que se inicializa con READY, todas se inicializan con ese estado.
	 */
	g_idleTask.entry_point = idleTask;
	g_idleTask.id = 0xFF;
	g_idleTask.state = READY;
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
	 * Inicialización de la tarea Idle
	 */

	initIdleTask();
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
	 *  @brief Extrae el codigo de error de la estructura de control del OS.
     *
     *  @details
     *   La estructura de control del OS no es visible al usuario, por lo que se facilita una API
     *   para extraer el ultimo codigo de error ocurrido, para su posterior tratamiento. Esta
     *   funcion puede ser utilizada dentro de errorHook
     *
	 *  @param 		None.
	 *  @return     Ultimo error ocurrido dentro del OS.
	 *  @see errorHook
***************************************************************************************************/
int32_t os_getError(void)  {
	return crt_OS.err;
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
	uint8_t index = 0;
	bool flag = true;
	uint8_t blockedTasks = 0;
	/*
	 * Se verifica si el estado del Sistema Operativo es después de un Reset, en ese caso
	 * la tarea actual se carga con la primera tarea de la lista de la estructura del control de OS
	 */
	if(crt_OS.state == FROM_RESET){
		crt_OS.current_task = (task*) crt_OS.taskList[0];
		crt_OS.contexSwitch = true;
	}
	else {
		/*
		 * En este apartado se verifica que el id de la tarea actual más uno se encuentre en estado
		 * running, si es asi, a la tarea de siguiente del sistema se le asigna la tarea que se encuentra en el
		 * siguiente indice del vector. Sin embargo, si la tarea se encuentra en estado blocked pasa a la siguiente
		 * tarea recorriendo el vector hasta que una tarea este en estado Running. si todas las tareas estan
		 * bloqueadas entonces se pasa a la tarea idle.
		 * En el caso de que la tarea actual sigue en Running y las demas estan bloqueadas el sistema no realiza
		 * cambio de contexto
		 *
		 */
		index = crt_OS.current_task->id + 1;

		while(flag){

            if(index >= crt_OS.quantity_task)
                index = 0;

			switch (((task*)crt_OS.taskList[index])->state) {

				case READY:
					crt_OS.next_task = (task*) crt_OS.taskList[index];
					crt_OS.contexSwitch = true;
					flag = false;
					break;
				case BLOCKED:
					blockedTasks++;
					if (blockedTasks == crt_OS.quantity_task)  {
						crt_OS.next_task = &g_idleTask;
						crt_OS.contexSwitch = true;
						flag = false;
					}
					break;
				case RUNNING:
					crt_OS.contexSwitch = false;
					flag = false;
					break;
				default:
					crt_OS.err = ERR_OS_SCHEDULER;
					errorHook(scheduler);
			}
			index++;
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

	/*
	 * Luego de determinar cual es la tarea siguiente segun el scheduler, se ejecuta la funcion
	 * tickhook.
	 */

	tickHook();

	if(crt_OS.contexSwitch){
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

		if (crt_OS.current_task->state == RUNNING)
			crt_OS.current_task->state = READY;

		sp_next = crt_OS.next_task->stack_pointer;

		crt_OS.current_task = crt_OS.next_task;
		crt_OS.current_task->state = RUNNING;
	}

	crt_OS.contexSwitch = false;

	return sp_next;
}

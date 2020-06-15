/*
 * MSE_OS_Core.c
 *
 *  Created on: mayo. 2020
 *      Author: jamm
 */

#include "../inc/JAMMOS.h"


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
static void initPriority(void);
static void scheduler(void);

/*==================[definicion de hooks debiles]=================================*/

/*
 * Esta sección contiene los hooks de sistema, los cuales el usuario del OS puede
 * redefinir dentro de su código y poblarlos según necesidad
 */

/*************************************************************************************************
	 *  @brief Hook de retorno de tareas
     *
     *  @details
     *   Esta función no deberia accederse bajo ningun concepto, porque ninguna tarea del OS
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
	 *  @warning	Esta función debe ser lo mas corta posible porque se ejecuta dentro del handler
     *   			mencionado, por lo que tiene prioridad sobre el cambio de contexto y otras IRQ.
	 *
	 *  @warning 	Esta función no debe bajo ninguna circunstancia utilizar APIs del OS dado
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
     *  Esta función tiene como atributo dèbil para que pueda ser sobre escrita por el usuario.
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
	 *  @brief Inicio de una sección crìtica.
     *
     *  @details
     *  Función que desabilita las interrupciones en alguna sección del código que sea atómica
     *  quiere decir que no se relice llamado al scheduler y  un cambio de contexto porque posiblemente
     *  hay cambios sobre esa sección
     *
	 *  @param 		None
	 *  @return     None
***************************************************************************************************/
inline void osEnterCritical(void)  {
	__disable_irq();
	crt_OS.countCritical++;
}


/*************************************************************************************************
	 *  @brief Final de una sección crìtica.
     *
     *  @details
     *  Función que habilita las interrupciones en alguna sección del código que sea atómica
     *  Verifica que el contador sea mayor o igual que cero.
     *
	 *  @param 		None
	 *  @return     None
***************************************************************************************************/
inline void osExitCritical(void)  {
	if (--crt_OS.countCritical <= 0)  {
		crt_OS.countCritical = 0;
		__enable_irq();
	}
}

/*************************************************************************************************
	 *  @brief Función que obtiene el estado del control del OS
     *
	 *  @param 		None
	 *  @return     bool Estado de la del control del OS.
***************************************************************************************************/
osState osGetSytemState(void)
{
	return crt_OS.state;
}

/*************************************************************************************************
	 *  @brief Función que configura el estado del control del OS
     *
	 *  @param 		state parametro con el estado que se desea configurar el control del OS
	 *  @return     none
***************************************************************************************************/
void osSetSytemState(osState state)
{
	crt_OS.state = state;
}

/*************************************************************************************************
	 *  @brief Función que configura la bandera que determina la ejecución de un scheduler después del
	 *  llamado a una interrupción
     *
	 *  @param 		bool value true o false
	 *  @return     none
***************************************************************************************************/
void osSetScheduleFromISR(bool value)
{
	crt_OS.schedulingFromIRQ = value;
}

/*************************************************************************************************
	 *  @brief Función que obtiene la bandera que determina la ejecución de un scheduler después del
	 *  llamado a una interrupción
     *
	 *  @param 		none
	 *  @return     bool true o false
***************************************************************************************************/
bool osGetScheduleFromISR(void)
{
	return crt_OS.schedulingFromIRQ;
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
void osInitTask(void *entryPoint, task *task_init, uint8_t priority)
{
	static uint8_t id = 0;				/*el id es una variable local pero statica que va
										 *incrementando a medida que se ingresa una nueva tarea*/

	/*
	 * Se efectua un chequeo del número de tareas que se encuentran en el OS y se verifica que no es mayor
	 * al máximo soportado por el OS. En el caso que sea igual o mayor no se inicializa la tarea y se
	 * carga en la variable de error del OS el código que se excedió en número de tareas
	 */

	if(crt_OS.quantity_task < MAX_TASK_NUMBER)  {

		/*
		 * Se configura el bit thumb en uno para indicar que solo se trabaja con instrucciones thumb
		 */
		task_init->stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;

		/* Se inicializa el registro PC del stack de la tarea con la dirección de la función asociada
		 * a la tarea, asignandole el parámetro (ENTRY_POINT)
		 */
		task_init->stack[STACK_SIZE/4 - PC_REG] = (uint32_t)entryPoint;

		/* Se configura el registro Linker return al hook de retorno, En el caso de que alguna tarea
		 *  retorne, no deberia pasar nunca, si pasa hay un error.
		 */
		task_init->stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;

		/*
		 * Se guarda en el stack el valor previo del LR ya que se necesita
		 * porque el valor del LR en la interrupción de PendSV_Handler
		 * cambia al llamar la función de cambio de contexto getContextoSiguiente
		 */
		task_init->stack[STACK_SIZE/4 - LR_PREV_VALUE] = EXEC_RETURN;

		task_init->stack_pointer = (uint32_t) (task_init->stack + STACK_SIZE/4 - FULL_REG_STACKING_SIZE);

		task_init->ticksWaiting = 0; /*
									* Se inicializa la variable dee conteo de la función osDelay a 0
		 	 	 	 	 	 	 	*/

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
		 * Se inicializa la prioridad de la tarea asignando la prioridad de la tarea con el parametro
		 * ingresado por el usuario, si el parametro se encuentra fuera del rango de prioridades del OS,
		 * entonces a la prioridad de la tarea se le asigna la prioridad mas baja
		 */

		if(priority >= PRIORITY_MAX && priority <= PRIORITY_MIN){
			task_init->priority = priority;
		}
		else{
			task_init->priority = PRIORITY_MIN;
		}

		/*
		 * Se guarda en el vector de tareas de la estructura de control del sistema operativo la tarea
		 * inicializada en la posición id que es correlativa al ingreso de tareas y se incrementa
		 * la variable que contiene la información de la cantidad de tareas del Sistema Operativo
		 */
		crt_OS.taskList[id] = task_init;
		crt_OS.quantity_task++;

		id++;
	}

	else {

		/*
		 * Se anexa en la variable err de la estructura de control del os el error definido que se
		 * excedió el número máximo de tareas permitidas por el os
		 */
		crt_OS.err = ERR_OS_QUANTITY_TASK;

		/* Se llama al hook de error con parámetro de la función donde ha pasado el error
		 */
		errorHook(osInitTask);
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
	/* Se configura el bit thumb en uno para indicar que solo se trabaja con instrucciones thumb
	 */
	g_idleTask.stack[STACK_SIZE/4 - XPSR] = INIT_XPSR;

	/* Se inicializa el registro PC del stack de la tarea con la dirección de la función
	 * asociada a la tarea, asignandole el parámetro (ENTRY_POINT)
	 */
	g_idleTask.stack[STACK_SIZE/4 - PC_REG] = (uint32_t)idleTask;

	/* Se configura el registro Linker return al hook de retorno. En el caso de que alguna tarea
	 * retorne, no deberia pasar nunca, si pasa hay un error.
	 */

	g_idleTask.stack[STACK_SIZE/4 - LR] = (uint32_t)returnHook;
	/*
	 * Se guarda en el stack el valor previo del LR ya que se necesita
	 * porque el valor del LR en la interrupción de PendSV_Handler
	 * cambia al llamar la funcion de cambio de contexto getContextoSiguiente
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
void osInit(void) {
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
	crt_OS.countCritical = 0;
	crt_OS.next_task = NULL;

	/*
	 * Función que realiza la inicialización de la tarea Idle, esta tarea es de naturaleza estática
	 * para que el usuario no pueda manipular, ni ver.
	 */
	initIdleTask();

	/*
	 * Se realiza la Inicialización de las variables que controlan la prioridad en las tareas
	 * se tiene un vector bidimencional y un vector con los índices
	 */
	initPriority();

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
	 *  @brief Inicializa las variables de control de las prioridades de las tareas
     *
     *  @details
     *  Función que realiza el llenado del vector bidimencional que contiene la información de las
     *  prioridades de las tareas.
     *  Inicializa los índices que tiene la información de la cantidad de tareas
     *
	 *  @param 		None.
	 *  @return     None.
***************************************************************************************************/
static void initPriority(void)
{
	uint8_t ind = 0;
	uint8_t priority = 0;
	uint8_t totalSize = 0;

	/*
	 * Se clarea todos los contadores de los contadores de las prioridades
	 */
	for(ind = 0; ind<PRIORITY_SIZE; ind++)
		crt_OS.countPriority[ind] = 0;

	/*
	 * Código donde se asigna a cada campo del vector bidimencional de las prioridades
	 * las tareas correspondientes dependiendo de su prioridad y se incrementan los
	 * contadores de prioridades correspondiente una vez se ingrese una tarea a determinado
	 * campo del vertor de prioridades
	 */

	for(ind = 0; ind<crt_OS.quantity_task; ind++){
		priority = crt_OS.taskList[ind]->priority;
		crt_OS.taskPriority[priority][crt_OS.countPriority[priority]] = crt_OS.taskList[ind];
		crt_OS.countPriority[priority]++;
	}

	/*
	 * Se verifica que todos los contadores de las prioridades sea igual al número de
	 * tareas inicializadas en el sistema, si es diferente genera un error y llama al hook de error
	 */

	for(ind = 0; ind<PRIORITY_SIZE; ind++)
		totalSize = totalSize + crt_OS.countPriority[ind];

	if(totalSize != crt_OS.quantity_task)
	{
		crt_OS.err = ERR_OS_PRIORITY_TOTAL_COUNT;
		errorHook(initPriority);
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
	 *  @brief Función que getCurrentTaskOS
     *
     *  @details
	 *  Función que devuelve como parametro la tarea actual del sistema
     *
	 *  @param none
	 *  @return task Tarea actual en el sistema de control del OS
***************************************************************************************************/
task* getCurrentTask(void)
{
	return crt_OS.current_task;
}

/*************************************************************************************************
	 *  @brief Forzado de Scheduling (Llama al scheduler y a cambio de contexto si es necesario)
     *
     *  @details
     *  Función que llama al scheuler y al cambio de contexto si es necesario
     *  llama a la funcion scheuler y reviza la bandera de cambio de contexto y llama a la
     *  interupcion PENDSVSET para realizar cambio de contexto si es necesario.
     *
	 *  @param 		None
	 *  @return     None.
***************************************************************************************************/
void osForceSchCC(void){
	scheduler();
	if(crt_OS.contexSwitch)
	{
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
static void scheduler(void) {

	static uint8_t priorityIndex[PRIORITY_SIZE];
	uint8_t blockedTasks[PRIORITY_SIZE];

	uint8_t i = 0;
	uint8_t priority = PRIORITY_MAX;
	uint8_t priorityAux = PRIORITY_MAX;

	uint8_t indexTask = 0;

	bool flag = true;

	bool priorityChange = false;

	/*
	 * Se cuenta con un vector (priorityIndex) que contiene los indices para recorrer cada uno de los
	 * vectores de las prioridades alojados en el vector bidimencional (taskPriority)
	 *
	 * Se cuenta también con un vector (blockedTasks) se clarea cada vez que entra al scheduler
	 * y que contiene el número de tareas bloqueadas en cada uno de los vectores de prioridad.
	 *
	 * Se tiene una variable priority que sirve para recorrer el primer campo (campo de prioridades) del vector
	 * bidimencional (taskPriority), este incrementa cada vez que el algoritmo verifica que todas las tareas
	 * de una prioridad mayor estan bloqueadas
	 *
	 * La variable indexTask se utiliza como índice temporal para recorrer las tareas en los
	 * vectores de las prioridades
	 *
	 * La bandera (flag) sirve para verificar si se debe salir o no del blucle de verificación de prioridades.
	 *
	 * La bandera (priorityChange) sirve para verificar si hubo un cambio de prioridad realizar las comparaciones
	 * con la prioridad anterior
	 *
	 * Primero se verifica si el estado del Sistema Operativo es después de un Reset, si viene de un reset
	 * la tarea actual se carga con la tarea Idle del OS.
	 *
	 * Se clarea el tambien el vector (priorityIndex).
	 *
	 */

	if(crt_OS.state == FROM_RESET){
		crt_OS.current_task = &g_idleTask;
		crt_OS.contexSwitch = true;

		for(i=0; i < PRIORITY_SIZE; i++)
			priorityIndex[i] = 0;
	}
	else {

		/**
		 * Se verifica el estado del control del OS, si se encuentra en estado SCHEDULING no se
		 * ejecuta el algoritmo y la función retorna.
		 * Se realiza en este parte de la función porque siempre entra a este apartado
		 * deferente de cuando viene de RESET.
		 */
		if(crt_OS.state == SCHEDULING)
			return;

		/**
		 * El estado del control del OS se cambia a SCHEDULING evitando de que se realice ejecute
		 * el algoritmo cuando se realizó el llamado en otro lado diferente al sistick
		 */
		crt_OS.state = SCHEDULING;
		/*
		 * Cuando el estado del sistema es diferente al proveniente de un Reset se clarean los contadores
		 * de las tareas bloqueadas en todos los niveles.
		 *
		 * priority se asigna inicialmente con la prioridad más alta para que el algoritmo empiece a recorrer
		 * y verificar el estado de las tareas desde el vector de la prioridad de nivel 0 (la Más alta) y con
		 * el índice que tenga priorityIndex en la posición que se encontraba anteriormente.
		 *
		 * La bandera flag se encuentra en true hasta que el algoritmo asigne la tarea siguiente
		 */
		for(i=0; i < PRIORITY_SIZE; i++)
			blockedTasks[i] = 0;

		while(flag)
		{
			/*
			 * Una vez adentro del bucle el algoritmo verifica el estado de todas las tareas en la matriz
			 * (taskPriority) y recorre desde la máxima prioridad hasta la mínima verificando y resuelve
			 * a que tarea asignar la tarea siguiente.
			 */

			indexTask = priorityIndex[priority];

			if(indexTask >= crt_OS.countPriority[priority])
				indexTask = 0;

			switch (((task*)crt_OS.taskPriority[priority][indexTask])->state){

				case READY:

					/*
					 * En este parte del código se asigna a la tarea siguiente del control
					 * la tarea que tiene el estado en READY asociada al indice de cada prioridad
					 */
					crt_OS.next_task = (task*) crt_OS.taskPriority[priority][indexTask];
					crt_OS.contexSwitch = true;
					flag = false;
					break;

				case BLOCKED:

					/*
					 * En este parte del código verifica si la tarea en el recorrido de la matriz se
					 * encuentra en estado bloqueado incrementa el valor de las tareas bloqueadas
					 * asociadas a cada prioridad en el vertor blockedTasks
					 */
					blockedTasks[priority]++;
					if (blockedTasks[priority] >= crt_OS.countPriority[priority]){
						priority++;
						priorityChange = true;
						if(priority > PRIORITY_MIN){
							crt_OS.next_task = &g_idleTask;
							crt_OS.contexSwitch = true;
							flag = false;
						}
					}
					break;

				case RUNNING:

					/*
					 * En el caso de que la unica tarea que no se encuentra en estado bloqueado es la tarea
					 * actual entonces no activa el cambio de contexto
					 */
					crt_OS.contexSwitch = false;
					flag = false;
					break;

				default:

					/*
					 * Si detecta un estado que no se encuentra definido entonces llama al hook de error
					 * con argumento de la función scheduler
					 */
					crt_OS.err = ERR_OS_SCHEDULER;
					errorHook(scheduler);
					break;
			}

			/*
			 * En este apartado del código se verifica e incrementa el indice que está asociado
			 * a cada una de las prioridades. si hubo un cambio de prioridad le realiza los cambios a la
			 * prioridad anterior.
			 * La variable priorityAux nunca va a tener el valor de -1 porque siempre se llama a la bandera
			 * después de que se incrementa la variable prioridad en el caso que cambie de prioridad 0 a 1.
			 */

			if(priorityChange)
			{
                priorityAux = priority - 1;
                priorityChange = false;
			}
			else
                priorityAux = priority;

            priorityIndex[priorityAux]++;
            if(priorityIndex[priorityAux] >= crt_OS.countPriority[priorityAux])
				priorityIndex[priorityAux] = 0;
		}
	}
	crt_OS.state = NORMAL_RUN;
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

	int i = 0;
	/*
	 * Se realiza un recorrido por todas las tareas decrementando la variable de conteo
	 * del timer para la función delay
	 * Verificación de que las tareas con determinado indice que están bloqueadas y han llegado a
	 * su contadores de ticksTimer a cero se cambia a estado READY
	 */
	for(i = 0; i < crt_OS.quantity_task; i++)
	{
		if(crt_OS.taskList[i]->ticksWaiting > 0)
			crt_OS.taskList[i]->ticksWaiting--;

		if(crt_OS.taskList[i]->ticksWaiting == 0 && crt_OS.taskList[i]->state == BLOCKED)
			crt_OS.taskList[i]->state = READY;
	}

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

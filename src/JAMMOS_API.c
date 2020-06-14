/*
 * JAMMOS_API.c
 *
 *  Created on: 14 jun. 2020
 *      Author: jamm
 */

#include "JAMMOS_API.h"

/*************************************************************************************************
	 *  @brief función de retraso
     *
     *  @details
     *   Esta función funciona a referencia de los ticks del sistema
     *
	 *  @param ticks de retraso del sistema, 1 milisegundo
	 *
	 *  @return none.
***************************************************************************************************/
void osDelay(uint32_t ticks)
{
	task* currentTask;
	/*
	 * Verifica que el numero de ticks ingresados es mayor que cero
	 * para realizar el delay en caso contrario no realiza nada
	 * */
	if(ticks > 0)
	{
		/*
		 * Obtiene la tarea actual que tiene el control del sistema
		 * se utiliza una función getCurrentTask
		 * */
		currentTask = getCurrentTask();
		/*
		 * Se verifica que el estado de la tarea actual esté conrriendo para
		 * poder ser bloqueda, en caso distinto no se realiza nada
		 * */
		if(currentTask->state == RUNNING)
		{
			/*
			 * linea que configura los tickstimer de la tarea actual con el
			 * valor ingresado por el parametro proveniente desde la tarea
			 * */
			currentTask->ticksWaiting = ticks;
			/*
			 * Si por algún motivo el contro del cpu vuelve a la tarea se verifica que
			 * los ticks del timer aun no han espirado y vuelve a bloaquear la tarea y
			 * a forzar un scheuler y cambio de contexto si es necesario
			 * */
			while(currentTask->ticksWaiting > 0)
			{
				currentTask->state = BLOCKED;
				osForceSchCC();
			}
		}
	}
}

/*************************************************************************************************
	 *  @brief función inicialicación de un semáforo
     *
     *  @details
     *   Esta función inicializa el semáforo con la tarea null e inicializa el semáforo tomado
     *
	 *  @param semáforo que se va a inicializar
	 *  @return none.
***************************************************************************************************/
void osInitSemaphore(semaphore *sem)
{
	sem->semaphoreTask = NULL;
	sem->state = TAKEN;
}

/*************************************************************************************************
	 *  @brief función de liberación de un semáforo
     *
     *  @details
     *   Esta libera el semáforo de entrada de parametro verificando que tenga tarea asociada y
     *   que se encuentre tomado, también se verifica que la tarea actual esté en estado RUNNING
     *
	 *  @param semáforo que se libera
	 *  @return none.
***************************************************************************************************/
void osGiveSemaphore(semaphore *sem)
{
	task* currentTask;
	if(sem->semaphoreTask != NULL && sem->state == TAKEN)
	{
		currentTask = getCurrentTask();
		if(currentTask->state == RUNNING)
		{
			sem->state = RELEASED;
			sem->semaphoreTask = READY;
		}
	}
}

/*************************************************************************************************
	 *  @brief función tomar un semáforo
     *
     *  @details
     *   Esta función verifica si el semáforo está tomado, si es así,
     *   asigna la tarea asociada del semaforo con la tarea actual y la bloquea
     *   llamando luego a la función que realiza el scheduler y el llamado a
     *   contexto si se necesita.
     *
	 *  @param sem semáforo que se toma
	 *  @return none.
***************************************************************************************************/
void osTakeSemaphore(semaphore *sem)
{
	bool isTaken = true;

	/*
	 * Este while evita que la tarea se ejecute a no se de que se haga un llamado
	 * a la función Give con el semáforo correspondiente
	 */
	while(isTaken)
	{
		if(sem->state == TAKEN)
		{
			sem->semaphoreTask = getCurrentTask();
			sem->semaphoreTask->state = BLOCKED;
			osForceSchCC();
		}
		else
		{
			sem->state = TAKEN;
			isTaken = false;
		}
	}
}

/*************************************************************************************************
	 *  @brief función de inicialicación de una cola
     *
     *  @details
     *   Esta función inicializa el cola con la tarea asociada igual a null y el tamaño del
     *   elemento que se va a transmitir
     *
     *   Inicializa los indices a cero y determina el tamaño del dato que va a enviar
     *
	 *  @param que, cola que se va a inicializar
	 *  @param size, Tamaño del elemento o estructura que se enviaran por la cola
	 *  @return none.
***************************************************************************************************/
void osInitQueue(queue *que, uint16_t size)
{
	que->size = size;
	que->head = 0;
	que->tail = 0;
	que->queueTask = NULL;
}

/*************************************************************************************************
	 *  @brief función de que pone datos sobre una cola
     *
     *  @details
     *   Esta función realiza una copia en el vector de la cola los datos del puntero data
     *   que entra como parametro
     *
     *
	 *  @param cola donde se va a escribir
	 *  @param data, puntero de los datos a enviar
	 *  @return none.
***************************************************************************************************/
void osPutQueue(queue *que, void* data)
{
	task* currentTask;
	uint16_t elements;
	uint16_t indexHead;
	/*
	 * Verifica que la cola no este vacia
	 * */
	if(que->head == que->tail)
	{
		/*
		 * Verifica que la cola tenga una tarea asociada
		 * */
		if(que->queueTask != NULL)
		{
			/*
			 * Verifica que la tarea esté bloaqueda, en el caso de que
			 * se bloqueo por algùn motivo se cambia a estado ready, porque
			 * se escribira un dato en esa tarea
			 * */
			if(que->queueTask->state == BLOCKED)
				que->queueTask->state = READY;
		}
	}
	/*
	 * Obtengo el puntero de la tarea actual
	 * */
	currentTask = getCurrentTask();
	/*
	 * Verifico que la tarea actual se esta ejecutando para realizar la copia
	 * de los datos ingresados en la cola
	 * */
	if(currentTask->state == RUNNING)
	{
		elements = QUEUE_SIZE/que->size;
		indexHead = que->head * que->size;
		/*
		 * Se verifica que la cola tenga espacio para incluir los datos
		 * si no tiene el while bloquea la tarea actual y se forza a un
		 * scheuler y cambio de contexto hasta que cumpla con la
		 * condicion de tener lugar disponible
		 * */
		while((que->head + 1) % elements == que->tail)
		{
			currentTask->state = BLOCKED;
			que->queueTask = currentTask;
			osForceSchCC();
		}
		/*
		 * Se realiza una copia de los datos en el vector de la pila en la posición del índice head
		 * */
		memcpy(que->data + indexHead,data,que->size);
		/*
		 * Se desasocia la tarea de la cola
		 * */
		que->head = (que->head + 1) % elements;
		que->queueTask = NULL;
	}
}

/*************************************************************************************************
	 *  @brief función obtiene los datos de la cola
     *
     *  @details
     *   Esta función realiza una copia en el  puntero data
     *   que entra como parametro del vector de la cola
     *
     *
	 *  @param cola de donde se va a leer
	 *  @param data, puntero de los datos a escribir
	 *  @return none.
***************************************************************************************************/
void osGetQueue(queue *que, void* data)
{
	task* currentTask;
	uint16_t elements;
	uint16_t indexTail;

	elements = QUEUE_SIZE / que->size;
	/*
	 * Se verifica que la cola esté llena
	 * */
	if((que->head + 1) % elements == que->tail)
	{
		/*
		 * Verifica que la cola tenga una tarea asociada
		 * */
		if(que->queueTask != NULL)
		{
			/*
			 * Verifica que la tarea esté bloaqueda, en el caso de que
			 * se bloqueo por algùn motivo se cambia a estado ready, porque
			 * se leerá un dato en esa tarea
			 * */
			if(que->queueTask->state == BLOCKED)
				que->queueTask->state = READY;
		}
	}
	/*
	 * Obtengo el puntero de la tarea actual
	 * */
	currentTask = getCurrentTask();
	/*
	 * Verifico que la tarea actual se esta ejecutando para realizar la copia
	 * de los datos ingresados en la cola
	 * */
	if(currentTask->state == RUNNING)
	{
		indexTail = que->tail * que->size;
		/*
		 * Se verifica que la cola tenga datos por lees
		 * si no tiene datos por leer el while bloquea la tarea actual y
		 * se forza a un scheuler y cambio de contexto hasta que cumpla
		 * con la condicion de tener datos por leer
		 * */
		while(que->head == que->tail)
		{
			currentTask->state = BLOCKED;
			que->queueTask = currentTask;
			osForceSchCC();
		}
		/*
		 * Se realiza una copia vector de la pila al elemento que entra
		 * como parametro de los datos en el en la posición del índice Tail
		 * */
		memcpy(data,que->data + indexTail,que->size);
		/*
		 * Se actualiza el índice Head
		 * */
		que->tail = (que->tail + 1) % elements;
		/*
		 * Se desasocia la tarea de la cola
		 * */
		que->queueTask = NULL;
	}
}


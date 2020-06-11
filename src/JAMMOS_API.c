/*
 * JAMMOS_API.c
 *
 *  Created on: 10 jun. 2020
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
	 *  @param semáforo que se toma
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


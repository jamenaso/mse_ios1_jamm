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
			currentTask->ticksTimer = ticks;
			/*
			 * Si por algún motivo el contro del cpu vuelve a la tarea se verifica que
			 * los ticks del timer aun no han espirado y vuelve a bloaquear la tarea y
			 * a forzar un scheuler y cambio de contexto si es necesario
			 * */
			while(currentTask->ticksTimer > 0)
			{
				currentTask->state = BLOCKED;
				osForceSchCC();
			}
		}
	}
}

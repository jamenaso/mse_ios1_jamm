/*==================[inclusions]=============================================*/

#include "main.h"
#include "board.h"
#include "JAMMOS.h"
#include "JAMMOS_API.h"
#include "sapi.h"

/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Declaracion de prioridades]==============================*/

#define PRIORITY_0		0
#define PRIORITY_1		1
#define PRIORITY_2		2
#define PRIORITY_3		3

/*==================[Global data declaration]==============================*/

#define nBLINK	10

task g_task1,g_task2,g_task3,g_task4; //Se declaran 8 tareas
task g_task5,g_task6,g_task7,g_task8;

semaphore sem1, sem2, sem3;

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms
}


/*==================[Definicion de tareas para el OS]==========================*/

//id = 0
void task1(void)  {
	int i = 0;
	while (1) {
		osTakeSemaphore(&sem1);
		for(i = 0; i < nBLINK; i++)
		{
			gpioWrite(LED1,true); // @suppress("Symbol is not resolved")
			osDelay(200);
			gpioWrite(LED1,false); // @suppress("Symbol is not resolved")
			osDelay(200);
		}
	}
}

//id = 1
void task2(void)  {
	int j = 0;
	while (1) {
		osTakeSemaphore(&sem2);
		for(j = 0; j < nBLINK; j++)
		{
			gpioWrite(LED2,true); // @suppress("Symbol is not resolved")
			osDelay(200);
			gpioWrite(LED2,false); // @suppress("Symbol is not resolved")
			osDelay(200);
		}
	}
}

//id = 2
void task3(void)  {
	int k = 0;
	while (1) {
		for(k = 0; k < nBLINK; k++)
		{
			osTakeSemaphore(&sem3);
			gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
			osDelay(200);
			gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
			osDelay(200);
		}
	}
}

//id = 3
void task4(void)  {
	while (1) {
		if(!gpioRead(TEC1)) // @suppress("Symbol is not resolved")
			osGiveSemaphore(&sem1);
		osDelay(50);
	}
}

//id = 4
void task5(void)  {
	while (1) {
		if(!gpioRead(TEC2)) // @suppress("Symbol is not resolved")
			osGiveSemaphore(&sem2);
		osDelay(50);
	}
}

//id = 5
void task6(void)  {
	while (1) {
		if(!gpioRead(TEC3)) // @suppress("Symbol is not resolved")
			osGiveSemaphore(&sem3);
		osDelay(50);
	}
}

//id = 6
void task7(void)  {
	int o = 0;
	while (1) {
		osDelay(500);
		o++;
	}
}

//id = 7
void task8(void)  {
	int p = 0;
	while (1) {
		osDelay(500);
		p++;
	}
}
/*============================================================================*/
/*
 * Descripción de la prueba de semáforos
 *
 * Se crea 8 tareas en total, las tareas 1,2,3 son de la mayor prioridad (prioridad 0) y manejan
 * el parpadeo de los led de la eduCIAA correspondiente al número de los LED
 *
 * Las tareas 4,5,6 de prioridad 1 manejan los pulsadores de usuario TEC1, TEC2 y TEC3 respectivamente
 *
 * Las tareas 7 y 8 de prioridad 2 nunca deberían entrar porque las tareas de prioridad 1 nunca se bloquean
 *
 *
 * */

int main(void)  {

	initHardware();

	osInitTask(task1, &g_task1, PRIORITY_0);
	osInitTask(task2, &g_task2, PRIORITY_0);
	osInitTask(task3, &g_task3, PRIORITY_0);
	osInitTask(task4, &g_task4, PRIORITY_1);
	osInitTask(task5, &g_task5, PRIORITY_1);
	osInitTask(task6, &g_task6, PRIORITY_1);
	osInitTask(task7, &g_task7, PRIORITY_2);
	osInitTask(task8, &g_task8, PRIORITY_2);

	osInitSemaphore(&sem1);
	osInitSemaphore(&sem2);
	osInitSemaphore(&sem3);

	osInit();

	while (1) {}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

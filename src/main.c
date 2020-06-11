/*==================[inclusions]=============================================*/

#include "main.h"
#include "board.h"
#include "JAMMOS.h"
#include "JAMMOS_API.h"

/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Declaracion de prioridades]==============================*/

#define PRIORITY_0		0
#define PRIORITY_1		1
#define PRIORITY_2		2
#define PRIORITY_3		3

/*==================[Global data declaration]==============================*/

task g_task1,g_task2,g_task3,g_task4; //Se declaran 8 tareas
task g_task5,g_task6,g_task7,g_task8;

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
		osDelay(500);
		i++;
	}
}

//id = 1
void task2(void)  {
	int j = 0;
	while (1) {
		j++;
	}
}

//id = 2
void task3(void)  {
	int k = 0;
	while (1) {
		k++;
	}
}

//id = 3
void task4(void)  {
	int l = 0;
	while (1) {
		l++;
	}
}

//id = 4
void task5(void)  {
	int m = 0;
	while (1) {
		m++;
	}
}

//id = 5
void task6(void)  {
	int n = 0;
	while (1) {
		n++;
	}
}

//id = 6
void task7(void)  {
	int o = 0;
	while (1) {
		o++;
	}
}

//id = 7
void task8(void)  {
	int p = 0;
	while (1) {
		osDelay(200);
		p++;
	}
}
/*============================================================================*/

int main(void)  {

	initHardware();

	osInitTask(task1, &g_task1, PRIORITY_0);
	osInitTask(task2, &g_task2, PRIORITY_2);
	osInitTask(task3, &g_task3, PRIORITY_0);
	osInitTask(task4, &g_task4, PRIORITY_3);
	osInitTask(task5, &g_task5, PRIORITY_2);
	osInitTask(task6, &g_task6, PRIORITY_1);
	osInitTask(task7, &g_task7, PRIORITY_1);
	osInitTask(task8, &g_task8, PRIORITY_0);

	osInit();

	while (1) {}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

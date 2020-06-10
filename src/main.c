/*==================[inclusions]=============================================*/

#include "main.h"

#include "board.h"

#include "MSE_OS_Core.h"

/*==================[macros and definitions]=================================*/

#define MILISEC		1000

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
void task1(void)  {
	int i;
	while (1) {
		i++;
	}
}

void task2(void)  {
	int j;
	while (1) {
		j++;
	}
}

void task3(void)  {
	int k;
	while (1) {
		k++;
	}
}

void task4(void)  {
	int l;
	while (1) {
		l++;
	}
}

void task5(void)  {
	int m;
	while (1) {
		m++;
	}
}

void task6(void)  {
	int n;
	while (1) {
		n++;
	}
}

void task7(void)  {
	int o;
	while (1) {
		o++;
	}
}

void task8(void)  {
	int p;
	while (1) {
		p++;
	}
}
/*============================================================================*/

int main(void)  {

	initHardware();

	os_InitTask(task1, &g_task1);
	os_InitTask(task2, &g_task2);
	os_InitTask(task3, &g_task3);
	os_InitTask(task4, &g_task4);
	os_InitTask(task5, &g_task5);
	os_InitTask(task6, &g_task6);
	os_InitTask(task7, &g_task7);
	os_InitTask(task8, &g_task8);

	os_Init();

	while (1) {
		__WFI();
	}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

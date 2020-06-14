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

struct _userData{
	uint16_t nInt;
	float nFloat;
	uint8_t nVector[4];
};
typedef struct _userData userData;

task g_task1,g_task2,g_task3,g_task4; //Se declaran 8 tareas
task g_task5,g_task6,g_task7,g_task8;

queue queue1, queue2, queue3;

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
	userData data;
	while (1) {
		osGetQueue(&queue1,&data);
		gpioWrite(LED1,true); // @suppress("Symbol is not resolved")
		osDelay(200);
		gpioWrite(LED1,false); // @suppress("Symbol is not resolved")
		osDelay(200);
		memset(&data,0x00,sizeof(userData));
	}
}

//id = 1
void task2(void)  {
	userData data;
	while (1) {
		osGetQueue(&queue2,&data);
		gpioWrite(LED2,true); // @suppress("Symbol is not resolved")
		osDelay(200);
		gpioWrite(LED2,false); // @suppress("Symbol is not resolved")
		osDelay(200);
		memset(&data,0x00,sizeof(userData));
	}
}

//id = 2
void task3(void)  {
	userData data;
	while (1) {
		osGetQueue(&queue2,&data);
		gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
		osDelay(200);
		gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
		osDelay(200);
		memset(&data,0x00,sizeof(userData));
	}
}

//id = 3
void task4(void)  {
	userData data;
	while (1) {
		if(!gpioRead(TEC1)) // @suppress("Symbol is not resolved")
		{
			data.nFloat = 0.4444;
			data.nInt = 4;
			data.nVector[0] = 4;
			data.nVector[0] = 5;
			data.nVector[0] = 6;
			data.nVector[0] = 7;
			osPutQueue(&queue1, &data);
		}
		osDelay(100);
	}
}

//id = 4
void task5(void)  {
	userData data;
	while (1) {
		if(!gpioRead(TEC2)) // @suppress("Symbol is not resolved")
		{
			data.nFloat = 0.5555;
			data.nInt = 5;
			data.nVector[0] = 5;
			data.nVector[0] = 6;
			data.nVector[0] = 7;
			data.nVector[0] = 8;
			osPutQueue(&queue2, &data);
		}
		osDelay(50);
	}
}

//id = 5
void task6(void)  {
	userData data;
	while (1) {
		if(!gpioRead(TEC3)) // @suppress("Symbol is not resolved")
		{
			data.nFloat = 0.6666;
			data.nInt = 6;
			data.nVector[0] = 6;
			data.nVector[0] = 7;
			data.nVector[0] = 8;
			data.nVector[0] = 9;
			osPutQueue(&queue3, &data);
		}
		osDelay(50);
	}
}

/*============================================================================*/
/*
 * Descripción de la prueba de Colas
 *
 * Se crea 6 tareas en total, las tareas 1,2,3 son de la mayor prioridad (prioridad 0), realizan la lectura de las colas y manejan
 * el parpadeo de los led de la eduCIAA correspondiente al número de los LED
 *
 * Las tareas 4,5,6 de prioridad 1 manejan los pulsadores de usuario TEC1, TEC2 y TEC3 respectivamente y realizan la escritura de
 * datos sobre las colas respectivamante
 *
 * la tarea 4 hace escritura sobre la cola1 (queue1) y la tarea 0 realiza la lectura de la cola1 (queue1)
 * la tarea 5 hace escritura sobre la cola2 (queue2) y la tarea 1 realiza la lectura de la cola2 (queue2)
 * la tarea 6 hace escritura sobre la cola3 (queue3) y la tarea 2 realiza la lectura de la cola3 (queue3)
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

	osInitQueue(&queue1,sizeof(userData));
	osInitQueue(&queue2,sizeof(userData));
	osInitQueue(&queue3,sizeof(userData));

	osInit();

	while (1) {}
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

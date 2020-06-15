/*==================[inclusions]=============================================*/

#include "main.h"
#include "board.h"
#include "JAMMOS.h"
#include "JAMMOS_API.h"
#include "JAMMOS_IRQ.h"
#include "sapi.h"
#include <string.h>

/*==================[macros and definitions]=================================*/

#define MILISEC		1000

/*==================[Declaracion de prioridades]==============================*/

#define PRIORITY_0		0
#define PRIORITY_1		1
#define PRIORITY_2		2
#define PRIORITY_3		3

#define TEC1_PORT   0
#define TEC1_BIT    4

#define TEC2_PORT   0
#define TEC2_BIT    8

/*==================[Global data declaration]==============================*/

#define nBLINK	10

task g_task1,g_task2,g_task3,g_task4; //Se declaran 8 tareas
task g_task5,g_task6,g_task7,g_task8;

semaphore sem1, sem2;
queue queueUart, timeBlinkTask4, timeBlinkTask5, timeBlinkTask6, timeBlinkTask7;

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

void t1_ISR(void);
void t2_ISR(void);


/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms

	/*
	 * Se congifura la tecla 1 con flanco de desendente para la interrupcion 0
	 * */
	Chip_SCU_GPIOIntPinSel( 0, TEC1_PORT, TEC1_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) ); // INT0
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
	Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 0 ) );

	/*
	 * Se congifura la tecla 2 con flanco de desendente para la interrupcion 1
	 * */
	Chip_SCU_GPIOIntPinSel( 1, TEC2_PORT, TEC2_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) ); // INT1
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
	Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 1 ) );

	uartConfig( UART_USB, 115200 );
}


/*==================[Definicion de tareas para el OS]==========================*/

//id = 0
void task1(void)  {
	char msg[20];
	uint8_t i = 0;

	strcpy(msg,"Se presionó T1\n\r");

	while (1) {
		osTakeSemaphore(&sem1);
		for(i = 0; i < nBLINK; i++)
		{
			gpioWrite(LED1,true); // @suppress("Symbol is not resolved")
			osDelay(100);
			gpioWrite(LED1,false); // @suppress("Symbol is not resolved")
			osDelay(100);
		}

		i = 0;
		while(msg[i] != 0)  {
			osPutQueue(&queueUart,(msg + i));
			i++;
		}
	}
}

//id = 1
void task2(void)  {
	char msg[20];
	uint8_t j = 0;

	strcpy(msg,"Se presionó T2\n\r");

	while (1) {
		osTakeSemaphore(&sem2);
		for(j = 0; j < nBLINK; j++)
		{
			gpioWrite(LED2,true); // @suppress("Symbol is not resolved")
			osDelay(100);
			gpioWrite(LED2,false); // @suppress("Symbol is not resolved")
			osDelay(100);
		}

		j = 0;
		while(msg[j] != 0)  {
			osPutQueue(&queueUart,(msg + j));
			j++;
		}
	}
}

//id = 2
void task3(void)  {
	char ch;
	while(1)  {
		osGetQueue(&queueUart,&ch);
		uartWriteByte(UART_USB,ch);
	}
}

void task4(void)  {
	uint32_t tBlink = 0;
	uint8_t k = 0;
	while(1)  {
		osGetQueue(&timeBlinkTask4,&tBlink);
		for(k = 0; k < nBLINK; k++)
		{
			gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
			gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
		}
	}
}

void task5(void)  {
	uint32_t tBlink = 0;
	uint8_t l = 0;
	while(1)  {
		osGetQueue(&timeBlinkTask5,&tBlink);
		for(l = 0; l < nBLINK; l++)
		{
			gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
			gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
		}
	}
}

void task6(void)  {
	uint32_t tBlink = 0;
	uint8_t m = 0;
	while(1)  {
		osGetQueue(&timeBlinkTask6,&tBlink);
		for(m = 0; m < nBLINK; m++)
		{
			gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
			gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
		}
	}
}

void task7(void)  {
	uint32_t tBlink = 0;
	uint8_t n = 0;
	while(1)  {
		osGetQueue(&timeBlinkTask7,&tBlink);
		for(n = 0; n < nBLINK; n++)
		{
			gpioWrite(LED3,true); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
			gpioWrite(LED3,false); // @suppress("Symbol is not resolved")
			osDelay(tBlink);
		}
	}
}

void task8(void)  {
	uint16_t countTask = 0;
	uint32_t timeBlink = 1;
	while(1)  {
		if(!gpioRead( TEC3 ))  {

			switch (countTask){
				case 0:
					osPutQueue(&timeBlinkTask4,100 * timeBlink);
				break;
				case 1:
					osPutQueue(&timeBlinkTask5,100 * timeBlink);
				break;
				case 2:
					osPutQueue(&timeBlinkTask6,100 * timeBlink);
				break;
				case 3:
					osPutQueue(&timeBlinkTask7,100 * timeBlink);
				break;
				default:
				break;
			}
			countTask++;
			if(countTask >= 4)
				countTask = 0;

			timeBlink++;
			if(timeBlink >= 10)
				timeBlink = 0;
		}
		osDelay(200);
	}
}

/*============================================================================*/
/*
 * Descripción de la del sistema operativo JAMMOS en general
 *
 * Se configura dos interrupciones asociadas a la INT0 e INT1 de cambio de estado de pines en flanco descendente
 * El pulsador T1 se lo asocia con la interrupción INT0 y el pulsador T2 se lo asocia con la INT1
 *
 * Se crea 8 tareas en total, las tareas 1,2 son de la mayor prioridad (prioridad 0), manejan
 * el parpadeo de los led de la eduCIAA correspondiente al número de los LED1 y LED2 respectivamente, luego escriben un mensaje a la
 * cola de usart dependiendo de que tarea se ejecute.
 *
 * Las tarea 3 de prioridad 1 maneja la escritura sobre el periférico del uart esperando datos de la cola
 * queueUart que se escriben en la tarea 1 y 2.
 *
 * Las tareas 4, 5, 6 y 7 son tareas de prioridad 2 estan esperando datos de las colas correspondientes a la timeBlinkTask4,
 * timeBlinkTask5, timeBlinkTask6 y timeBlinkTask7.
 *
 * la tarea 8 de más baja prioridad (prioridad 3) simplemente realiza la lectura del estado del T3 y verifica el contador countTask.
 * Dependiendo del valor del contador envia datos a determinada cola para que se ejecuten las tareas de mas alta prioridad 4, 5, 6 y 7
 *
 * El valor que se envian por las colas representan el tiempo de parpadeo del LED3 en las tareas 4,5,6 y 7. Mientras tanto no se estén
 * ejecutando las tareas de mayor prioridad.
 *
 * La variable timeBlink en la tarea8 incrementa cada vez que se envía una dato por la cola, de esta forma se determina en que ciclo
 * puede estar el algoritmo. Si la variable llega a 10 se resetea.
 *
 * */

int main(void)  {

	initHardware();

	osInitTask(task1, &g_task1, PRIORITY_0);
	osInitTask(task2, &g_task2, PRIORITY_0);
	osInitTask(task3, &g_task3, PRIORITY_1);

	osInitTask(task4, &g_task4, PRIORITY_2);
	osInitTask(task5, &g_task5, PRIORITY_2);
	osInitTask(task6, &g_task6, PRIORITY_2);
	osInitTask(task7, &g_task7, PRIORITY_2);

	osInitTask(task8, &g_task8, PRIORITY_3);

	osInitSemaphore(&sem1);
	osInitSemaphore(&sem2);

	osInitQueue(&queueUart,sizeof(char));

	osInitQueue(&timeBlinkTask4,sizeof(uint32_t));
	osInitQueue(&timeBlinkTask5,sizeof(uint32_t));
	osInitQueue(&timeBlinkTask6,sizeof(uint32_t));
	osInitQueue(&timeBlinkTask7,sizeof(uint32_t));

	osInstallIRQ(PIN_INT0_IRQn,t1_ISR);
	osInstallIRQ(PIN_INT1_IRQn,t2_ISR);

	osInit();

	while (1) {}
}

void t1_ISR(void) {
	osGiveSemaphore(&sem1);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
}

void t2_ISR(void)  {
	osGiveSemaphore(&sem2);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

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

#define TEC1_PORT   0
#define TEC1_BIT    4

#define TEC2_PORT   0
#define TEC2_BIT    8

/*==================[Global data declaration]==============================*/

#define nBLINK	10

task g_task1,g_task2,g_task3,g_task4; //Se declaran 8 tareas
task g_task5,g_task6,g_task7,g_task8;

semaphore sem1, sem2;
queue queueUart;

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
	int i = 0;

	strcpy(msg,"Se presionó T1\n\r");

	while (1) {
		osTakeSemaphore(&sem1);
		for(int i = 0; i < nBLINK; i++)
		{
			gpioWrite(LED1,true); // @suppress("Symbol is not resolved")
			osDelay(100);
			gpioWrite(LED1,false); // @suppress("Symbol is not resolved")
			osDelay(100);
		}

		i = 0;
		while(msg[i] != NULL)  {
			osPutQueue(&queueUart,(msg + i));
			i++;
		}
	}
}

//id = 1
void task2(void)  {
	char msg[20];
	int j = 0;

	strcpy(msg,"Se presionó T2\n\r");

	while (1) {
		osTakeSemaphore(&sem2);
		for(int j = 0; j < nBLINK; j++)
		{
			gpioWrite(LED2,true); // @suppress("Symbol is not resolved")
			osDelay(100);
			gpioWrite(LED2,false); // @suppress("Symbol is not resolved")
			osDelay(100);
		}

		j = 0;
		while(msg[j] != NULL)  {
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
		//uartWriteByte(UART_USB,ch);
	}
}

/*============================================================================*/
/*
 * Descripción de la prueba de Interrupciones
 *
 * Se configura dos interupciones asociadas a la INT0 e INT1 de cambio de estado de pines en flanco desendente
 * El pulsador T1 se lo asocia con la interrupcion INT0 y el pulsador T2 se lo asocia con la INT1
 *
 * Se crea 3 tareas en total, las tareas 1,2 son de la mayor prioridad (prioridad 0), manejan
 * el parpadeo de los led de la eduCIAA correspondiente al número de los LED y escriben un mensaje a la
 * cola de usart dependiendo de que tarea se ejecute.
 *
 * Las tarea 3 de prioridad 1 maneja la escritura sobre el periférico del uart esperando datos de la cola
 * queueUart que se escriben en la tarea 1 y 2
 *
 * */

int main(void)  {

	initHardware();

	osInitTask(task1, &g_task1, PRIORITY_0);
	osInitTask(task2, &g_task2, PRIORITY_0);
	osInitTask(task3, &g_task3, PRIORITY_1);

	osInitSemaphore(&sem1);
	osInitSemaphore(&sem2);

	osInitQueue(&queueUart,sizeof(char));

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

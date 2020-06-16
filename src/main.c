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

#define MAX_MSG_LENGTH 250

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

task g_taskFallingEdge,g_taskRisingEdge, g_taskEvent,g_taskSendUart; //Se declaran tareas

queue queueButtonFallingEdge, queueButtonRisingEdge, queueEvent, queueUart; //Se declaran Colas

uint32_t timeMseconds; //Variable de tiempo que se incrementa en cada tickHook renombrada por el usuario

/*
 * Tipo de dato de id del botón que tiene la información del botón que tuvo evento
 * */

enum _buttonId {
	B1,
	B2
};
typedef enum _buttonId buttonId;

/*
 * Modo de flanco, tienen dos modos flanco ascendente y flaco descendente
 * */

enum _modeEdge {
	RISING_EDGE,
	FALLING_EDGE
};
typedef enum _modeEdge modeEdge;

/*
 * Definición del tipo de dato button que es una estructura que tiene el Id de identificación del
 * botón el tiempo en que ha tenido el último evento y guarda el modo de flanco del último evento
 */

struct _button{
	buttonId id;
	uint32_t time;
	modeEdge mEdge;
};
typedef struct _button button;

/*
 * Definición de dato que guarda la información del modo de evento que consiste en orden de ejecución de los botónes
 * Ejemplo: B1_B2 ocurrió primero botón 1 y después el botón 2
 */

enum _modeEvent {
	B1_B2,
	B2_B1,
	INVALID
};
typedef enum _modeEvent modeEvent;

/*
 * Definición del tipo de dato envento que es una estructura que tiene el modo de evento, el modo de flanco y el
 * tiempo en que trancurrieron los eventos de los botones
 */

struct _event{
	modeEvent mEvent;
	modeEdge mEdge;
	uint32_t time;
};
typedef struct _event event;

/*==================[internal functions declaration]=========================*/

/*==================[internal data definition]===============================*/

/*==================[external data definition]===============================*/

/*==================[internal functions definition]==========================*/

/*
 * Se declaran cuatro funciones de interrupción botón 1 flanco descendente, botón 1 flanco ascendente
 * botón 2 flanco descendente y botón 2 flanco ascendente.
 * */

void b1_low_ISR(void);
void b1_high_ISR(void);
void b2_low_ISR(void);
void b2_high_ISR(void);

/** @brief hardware initialization function
 *	@return none
 */
static void initHardware(void)  {
	Board_Init();
	SystemCoreClockUpdate();
	SysTick_Config(SystemCoreClock / MILISEC);		//systick 1ms

	/*
	 * Se configura la interrupcion 0 para el flanco descendente en la tecla 1
	 */
	Chip_SCU_GPIOIntPinSel( 0, TEC1_PORT, TEC1_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) ); // INT0 flanco descendente
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
	Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 0 ) );

	/*
	 * Se configura la interrupcion 1 para el flanco ascendente en la tecla 1
	 */
	Chip_SCU_GPIOIntPinSel( 1, TEC1_PORT, TEC1_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) ); // INT1 flanco ascendente
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
	Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT, PININTCH( 1 ) );

	/*
	 * Se configura la interrupcion 0 para el flanco descendente en la tecla 1
	 */
	Chip_SCU_GPIOIntPinSel( 2, TEC2_PORT, TEC2_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 2 ) ); // INT0 flanco descendente
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 2 ) );
	Chip_PININT_EnableIntLow( LPC_GPIO_PIN_INT, PININTCH( 2 ) );

	/*
	 * Se configura la interrupcion 1 para el flanco ascendente en la tecla 1
	 */
	Chip_SCU_GPIOIntPinSel( 3, TEC2_PORT, TEC2_BIT );
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 3 ) ); // INT1 flanco ascendente
	Chip_PININT_SetPinModeEdge( LPC_GPIO_PIN_INT, PININTCH( 3 ) );
	Chip_PININT_EnableIntHigh( LPC_GPIO_PIN_INT, PININTCH( 3 ) );

	uartConfig( UART_USB, 115200 );
}


/*==================[Definicion de tareas para el OS]==========================*/

/*
 * En esta tarea espera un dato de la cola queueButtonFallingEdge proveniente de las interrupciones
 * de flanco descendiente b1_low_ISR y b2_low_ISR.
 *
 * Cuando se ha realizado dos pulsaciones de flanco descendiente de los botones verifica si los dos flancos
 * son descendientes, verifica el orden de pulsación para configurar la variable modo del Evento (ev.mEvent)
 * y asigna el tiempo entre pulsaciónes de los botones. Si el evento es válido envía la información del evento a
 * la tarea taskEvent por medio de la cola queueEvent
 * */
void taskFallingEdge(void)  {
	button btn, btnPrevious;
	uint8_t nBtn = 0;
	event ev;

	while (1) {
		osGetQueue(&queueButtonFallingEdge,&btn);
		if(nBtn == 0)
		{
			btnPrevious.id = btn.id;
			btnPrevious.mEdge = btn.mEdge;
			btnPrevious.time = btn.time;
			nBtn++;
		}
		else
		{
			if(btnPrevious.mEdge == FALLING_EDGE && btn.mEdge == FALLING_EDGE)
			{
				ev.mEdge = FALLING_EDGE;
				if(btnPrevious.id == B1 && btn.id == B2)
				{
					ev.mEvent = B1_B2;
					ev.time = btn.time - btnPrevious.time;
				}
				else if(btnPrevious.id == B2 && btn.id == B1)
				{
					ev.mEvent = B2_B1;
					ev.time = btn.time - btnPrevious.time;
				}
				else
				{
					ev.mEvent = INVALID;
					ev.time = 0;
				}
				if(ev.mEvent != INVALID)
					osPutQueue(&queueEvent,&ev);
			}
			nBtn = 0;
		}
	}
}

/*
 * En esta tarea espera un dato de la cola queueButtonRisingEdge proveniente de las interrupciones
 * de flanco ascendente b1_high_ISR y b2_high_ISR.
 *
 * Cuando se ha realizado dos pulsaciones de flanco ascendente de los botones verifica si los dos flancos
 * son ascendentes, verifica el orden de pulsación para configurar la variable modo del Evento (ev.mEvent)
 * y asigna el tiempo entre pulsaciónes de los botones. Si el evento es válido envía la información del evento a
 * la tarea taskEvent por medio de la cola queueEvent
 * */
void taskRisingEdge(void)  {
	button btn, btnPrevious;
	uint8_t nBtn = 0;
	event ev;

	while (1) {
		osGetQueue(&queueButtonRisingEdge,&btn);
		if(nBtn == 0)
		{
			btnPrevious.id = btn.id;
			btnPrevious.mEdge = btn.mEdge;
			btnPrevious.time = btn.time;
			nBtn++;
		}
		else
		{
			if(btnPrevious.mEdge == RISING_EDGE && btn.mEdge == RISING_EDGE)
			{
				ev.mEdge = RISING_EDGE;
				if(btnPrevious.id == B1 && btn.id == B2)
				{
					ev.mEvent = B1_B2;
					ev.time = btn.time - btnPrevious.time;
				}
				else if(btnPrevious.id == B2 && btn.id == B1)
				{
					ev.mEvent = B2_B1;
					ev.time = btn.time - btnPrevious.time;
				}
				else
				{
					ev.mEvent = INVALID;
					ev.time = 0;
				}
				if(ev.mEvent != INVALID)
					osPutQueue(&queueEvent,&ev);
			}
			nBtn = 0;
		}
	}
}
/*
 * Esta tarea recibe de las tareas taskFallingEdge y taskRisingEdge eventos producidos por los botones y su órden de ejecución.
 * Cuando recibe dos eventos verifica que el orden del evento anterior y el evento actual estén en la correcta secuencia.
 * primero de flanco Descendente y después de flanco Ascendente.
 *
 * Verifica el modo de los eventos para determinar el orden de ejecución de los botones en cada uno de los flancos
 * y dependiendo de eso prende el respectivo color del LED RGB y asigna al tiempo de encendido la suma de los tiempo los
 * eventos anterior y actual.
 *
 * Guarda en una cadena de caracteres dependiendo de los ordenes expuestos anteriormente y arma el string del mensaje
 * para después ser enviados a la tarea taskSendUart por medio de la cola queueUart que escribe por el usart cada caracter
 * que detecta en la lectura de la cola.
 *
 * */
void taskEvent(void)  {
	event ev, evPrevious;
	uint8_t nEv = 0;
	uint32_t tTotal = 0;
	char message[MAX_MSG_LENGTH];
	char msgColor[10];
	uint16_t msgIndex = 0;


	while(1)  {
		osGetQueue(&queueEvent,&ev);
		if(nEv == 0)
		{
			evPrevious.mEdge = ev.mEdge;
			evPrevious.mEvent = ev.mEvent;
			evPrevious.time = ev.time;
			nEv++;
		}
		else
		{
			if(evPrevious.mEdge == FALLING_EDGE && ev.mEdge == RISING_EDGE)
			{
				tTotal = evPrevious.time + ev.time;

				if(evPrevious.mEvent == B1_B2 && ev.mEvent == B1_B2)
				{
					gpioWrite(LEDG,true);
					osDelay(tTotal);
					gpioWrite(LEDG,false);
					strcpy( msgColor, "Verde" );
				}

				if(evPrevious.mEvent == B1_B2 && ev.mEvent == B2_B1)
				{
					gpioWrite(LEDR,true);
					osDelay(tTotal);
					gpioWrite(LEDR,false);
					strcpy( msgColor, "Rojo" );
				}

				if(evPrevious.mEvent == B2_B1 && ev.mEvent == B1_B2)
				{
					gpioWrite(LEDR,true);
					gpioWrite(LEDG,true);
					osDelay(tTotal);
					gpioWrite(LEDR,false);
					gpioWrite(LEDG,false);
					strcpy( msgColor, "Amarillo" );
				}

				if(evPrevious.mEvent == B2_B1 && ev.mEvent == B2_B1)
				{
					gpioWrite(LEDB,true);
					osDelay(tTotal);
					gpioWrite(LEDB,false);
					strcpy( msgColor, "Azúl" );
				}

				sprintf( message, "Led %s encendido:\n\r\t Tiempo encendido: %lu ms\n\r\t Tiempo entre flancos descendentes: %lu ms \n\r\t Tiempo entre flancos ascendentes: %lu ms \n\r", msgColor, tTotal, evPrevious.time,ev.time );

				msgIndex = 0;
				while(message[msgIndex] != NULL)  {
					osPutQueue(&queueUart,(message + msgIndex));
					msgIndex++;
				}
			}
			nEv = 0;
		}
	}
}

/*
 * Tarea que recibe cada caracter proveniente de la tarea taskEvent para ser escrita al buffer del Usart
 *
 * */
void taskSendUart(void)  {
	char ch;
	while(1)  {
		osGetQueue(&queueUart,&ch);
		uartWriteByte(UART_USB,ch);
	}
}

/**
 * Redefinición de la función hook que se llama en el sistick que lleva la temporizacio de cuantos milisegundo transcurren
 *
 */

void tickHook(void)
{
	timeMseconds++;
}

/*============================================================================*/
/*
 * Descripción de la prueba para examen de la materia ISO1 con el sistema operativo JAMMOS
 *
 * Se implementa un sistema que mide la diferencia de tiempos entre flancos positivos y negativos
 * generados por dos pulsos provenientes de dos pulsadores, cuyas ocurrencias se solapen temporalmente.
 * Cada caso de solapamiento tendrá un led específico asociado, el cual se encenderá
 * inmediatamente luego de que los dos botones dejen de ser presionados. El tiempo en que el led
 * correspondiente estará encendido será la suma de los tiempos entre flancos ascendentes y
 * descendentes respectivamente.
 * */

int main(void)  {

	initHardware();

	osInitTask(taskFallingEdge, &g_taskFallingEdge, PRIORITY_0);
	osInitTask(taskRisingEdge, &g_taskRisingEdge, PRIORITY_0);
	osInitTask(taskEvent, &g_taskEvent, PRIORITY_1);
	osInitTask(taskSendUart, &g_taskSendUart, PRIORITY_3);

	osInitQueue(&queueButtonFallingEdge,sizeof(button));
	osInitQueue(&queueButtonRisingEdge,sizeof(button));

	osInitQueue(&queueEvent,sizeof(event));
	osInitQueue(&queueUart,sizeof(char));

	osInstallIRQ(PIN_INT0_IRQn, b1_low_ISR);
	osInstallIRQ(PIN_INT1_IRQn, b1_high_ISR);
	osInstallIRQ(PIN_INT2_IRQn, b2_low_ISR);
	osInstallIRQ(PIN_INT3_IRQn, b2_high_ISR);

	osInit();

	timeMseconds = 0; //Variable rollover que lleva el trancurso del tiempo

	while (1) {}
}

/*
 * Tareas que que se asocian a la interrupciones de cada uno de los botones, se arma una
 * estructura button con la información del la pulsación de cada botón para ser procesadas
 * en las tareas taskFallingEdge y taskRisingEdge
 *
 * */

void b1_low_ISR(void){
	button btn;
	btn.id = B1;
	btn.mEdge = FALLING_EDGE;
	btn.time = timeMseconds;
	osPutQueue(&queueButtonFallingEdge,&btn);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 0 ) );
}

void b1_high_ISR(void){
	button btn;
	btn.id = B1;
	btn.mEdge = RISING_EDGE;
	btn.time = timeMseconds;
	osPutQueue(&queueButtonRisingEdge,&btn);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 1 ) );
}

void b2_low_ISR(void){
	button btn;
	btn.id = B2;
	btn.mEdge = FALLING_EDGE;
	btn.time = timeMseconds;
	osPutQueue(&queueButtonFallingEdge,&btn);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 2 ) );
}

void b2_high_ISR(void){
	button btn;
	btn.id = B2;
	btn.mEdge = RISING_EDGE;
	btn.time = timeMseconds;
	osPutQueue(&queueButtonRisingEdge,&btn);
	Chip_PININT_ClearIntStatus( LPC_GPIO_PIN_INT, PININTCH( 3 ) );
}

/** @} doxygen end group definition */

/*==================[end of file]============================================*/

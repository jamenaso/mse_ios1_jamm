/*
 * JAMMOS_IRQ.c
 *
 *  Created on: 14 jun. 2020
 *      Author: root
 */

#include "JAMMOS_IRQ.h"

static void* isrUserVector[IRQ_QUANTITY];

/*************************************************************************************************
	 *  @brief Función que configura una interupnción para el sistema operativo
     *
     *  @details
     *  Función que instala y configura una interrupcion del sistema.
     *
	 *  @param 		IRQ parámetro que contiene el valor de la interupción correspondiente a la que se desea activar
	 *  @param 		usrIsr puntero a la función que el usuario quiere que se llame cuando ocurra la interrupción
	 *  @return     bool obtiene el valor de si la instalación de la interrupción fué satisfactoria
***************************************************************************************************/
bool osInstallIRQ(LPC43XX_IRQn_Type irq, void* usrIsr)
{
	bool irqInstallOk = 0;

	if (isrUserVector[irq] == NULL)
	{
		isrUserVector[irq] = usrIsr;
		NVIC_ClearPendingIRQ(irq);
		NVIC_EnableIRQ(irq);
		irqInstallOk = true;
	}

	return irqInstallOk;
}

/*************************************************************************************************
	 *  @brief Función que desinstala y remueve una interupnción para el sistema operativo
     *
     *  @details
     *  Función que desinstala interrupcion del sistema.
     *
	 *  @param 		IRQ parámetro que contiene el valor de la interupción correspondiente a la que se desea activar
	 *  @return     bool obtiene el valor de si la desinstalación de la interrupción fué satisfactoria
***************************************************************************************************/
bool osRemoveIRQ(LPC43XX_IRQn_Type irq)
{
	bool irqRemoveOk = 0;

	if (isrUserVector[irq] != NULL)
	{
		isrUserVector[irq] = NULL;
		NVIC_ClearPendingIRQ(irq);
		NVIC_DisableIRQ(irq);
		irqRemoveOk = true;
	}

	return irqRemoveOk;
}

/********************************************************************************
 * Esta funcion es la que todas las interrupciones llaman. Se encarga de llamar
 * a la funcion de usuario que haya sido cargada. LAS FUNCIONES DE USUARIO
 * LLAMADAS POR ESTA FUNCION SE EJECUTAN EN MODO HANDLER DE IGUAL FORMA. CUIDADO
 * CON LA CARGA DE CODIGO EN ELLAS, MISMAS REGLAS QUE EN BARE METAL
 *******************************************************************************/
/*************************************************************************************************
	 *  @brief Función se llama cuando alguna interrupción es llamada
     *
     *  @details
     *  Función que determina la función de uruario a llamar dependiendo del valor del parámetro IRQn
     *
	 *  @param 		IRQn parámetro que contiene el valor de la interupción correspondiente a la que se
					ha llamado
	 *  @return     none
***************************************************************************************************/
static void osIrqHandler(LPC43XX_IRQn_Type IRQn)  {
	osState osPreviousState;
	void (*userFuntion)(void);

	osPreviousState = osGetSytemState();

	osSetSytemState(RUN_IRQ);

	userFuntion = isrUserVector[IRQn];
	userFuntion();

	osSetSytemState(osPreviousState);

	NVIC_ClearPendingIRQ(IRQn);

	if (osGetScheduleFromISR())  {
		osSetScheduleFromISR(false);
		osForceSchCC();
	}
}

/*==================[interrupt service routines]=============================*/

void DAC_IRQHandler(void){osIrqHandler(         DAC_IRQn         );}
void M0APP_IRQHandler(void){osIrqHandler(       M0APP_IRQn       );}
void DMA_IRQHandler(void){osIrqHandler(         DMA_IRQn         );}
void FLASH_EEPROM_IRQHandler(void){osIrqHandler(RESERVED1_IRQn   );}
void ETH_IRQHandler(void){osIrqHandler(         ETHERNET_IRQn    );}
void SDIO_IRQHandler(void){osIrqHandler(        SDIO_IRQn        );}
void LCD_IRQHandler(void){osIrqHandler(         LCD_IRQn         );}
void USB0_IRQHandler(void){osIrqHandler(        USB0_IRQn        );}
void USB1_IRQHandler(void){osIrqHandler(        USB1_IRQn        );}
void SCT_IRQHandler(void){osIrqHandler(         SCT_IRQn         );}
void RIT_IRQHandler(void){osIrqHandler(         RITIMER_IRQn     );}
void TIMER0_IRQHandler(void){osIrqHandler(      TIMER0_IRQn      );}
void TIMER1_IRQHandler(void){osIrqHandler(      TIMER1_IRQn      );}
void TIMER2_IRQHandler(void){osIrqHandler(      TIMER2_IRQn      );}
void TIMER3_IRQHandler(void){osIrqHandler(      TIMER3_IRQn      );}
void MCPWM_IRQHandler(void){osIrqHandler(       MCPWM_IRQn       );}
void ADC0_IRQHandler(void){osIrqHandler(        ADC0_IRQn        );}
void I2C0_IRQHandler(void){osIrqHandler(        I2C0_IRQn        );}
void SPI_IRQHandler(void){osIrqHandler(         I2C1_IRQn        );}
void I2C1_IRQHandler(void){osIrqHandler(        SPI_INT_IRQn     );}
void ADC1_IRQHandler(void){osIrqHandler(        ADC1_IRQn        );}
void SSP0_IRQHandler(void){osIrqHandler(        SSP0_IRQn        );}
void SSP1_IRQHandler(void){osIrqHandler(        SSP1_IRQn        );}
void UART0_IRQHandler(void){osIrqHandler(       USART0_IRQn      );}
void UART1_IRQHandler(void){osIrqHandler(       UART1_IRQn       );}
void UART2_IRQHandler(void){osIrqHandler(       USART2_IRQn      );}
void UART3_IRQHandler(void){osIrqHandler(       USART3_IRQn      );}
void I2S0_IRQHandler(void){osIrqHandler(        I2S0_IRQn        );}
void I2S1_IRQHandler(void){osIrqHandler(        I2S1_IRQn        );}
void SPIFI_IRQHandler(void){osIrqHandler(       RESERVED4_IRQn   );}
void SGPIO_IRQHandler(void){osIrqHandler(       SGPIO_INT_IRQn   );}
void GPIO0_IRQHandler(void){osIrqHandler(       PIN_INT0_IRQn    );}
void GPIO1_IRQHandler(void){osIrqHandler(       PIN_INT1_IRQn    );}
void GPIO2_IRQHandler(void){osIrqHandler(       PIN_INT2_IRQn    );}
void GPIO3_IRQHandler(void){osIrqHandler(       PIN_INT3_IRQn    );}
void GPIO4_IRQHandler(void){osIrqHandler(       PIN_INT4_IRQn    );}
void GPIO5_IRQHandler(void){osIrqHandler(       PIN_INT5_IRQn    );}
void GPIO6_IRQHandler(void){osIrqHandler(       PIN_INT6_IRQn    );}
void GPIO7_IRQHandler(void){osIrqHandler(       PIN_INT7_IRQn    );}
void GINT0_IRQHandler(void){osIrqHandler(       GINT0_IRQn       );}
void GINT1_IRQHandler(void){osIrqHandler(       GINT1_IRQn       );}
void EVRT_IRQHandler(void){osIrqHandler(        EVENTROUTER_IRQn );}
void CAN1_IRQHandler(void){osIrqHandler(        C_CAN1_IRQn      );}
void ADCHS_IRQHandler(void){osIrqHandler(       ADCHS_IRQn       );}
void ATIMER_IRQHandler(void){osIrqHandler(      ATIMER_IRQn      );}
void RTC_IRQHandler(void){osIrqHandler(         RTC_IRQn         );}
void WDT_IRQHandler(void){osIrqHandler(         WWDT_IRQn        );}
void M0SUB_IRQHandler(void){osIrqHandler(       M0SUB_IRQn       );}
void CAN0_IRQHandler(void){osIrqHandler(        C_CAN0_IRQn      );}
void QEI_IRQHandler(void){osIrqHandler(         QEI_IRQn         );}

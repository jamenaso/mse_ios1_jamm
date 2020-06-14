/*
 * JAMMOS_IRQ.h
 *
 *  Created on: 14 jun. 2020
 *      Author: root
 */

#ifndef PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_IRQ_H_
#define PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_IRQ_H_

#include "JAMMOS.h"
#include "JAMMOS_API.h"
#include "board.h"
#include "cmsis_43xx.h"

#define IRQ_QUANTITY	53

extern osCrt crt_OS;

bool osInstallIRQ(LPC43XX_IRQn_Type irq, void* usrIsr);
bool osRemoveIRQ(LPC43XX_IRQn_Type irq);

#endif /* PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_IRQ_H_ */

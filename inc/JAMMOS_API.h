/*
 * JAMMOS_API.h
 *
 *  Created on: 10 jun. 2020
 *      Author: JAMM
 */

#ifndef PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_API_H_
#define PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_API_H_

#include <stdint.h>
#include <stdbool.h>
#include "JAMMOS.h"

extern osCrt crt_OS;

/**
 *Enumeración de los estados del semáforo
 */

enum _semState {
	RELEASED,
	TAKEN
};

typedef enum _semState semState;

/**
 *Definición de la estructura del semáforo
 */

struct _semaphore{
	task* semaphoreTask;
	semState state;
};

typedef struct _semaphore semaphore;

void osDelay(uint32_t ticks);
void osInitSemaphore(semaphore *sem);
void osGiveSemaphore(semaphore *sem);
void osTakeSemaphore(semaphore *sem);

#endif /* PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_API_H_ */

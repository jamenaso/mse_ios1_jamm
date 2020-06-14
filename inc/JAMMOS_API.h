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

#define QUEUE_SIZE		64 //tamaño reservado de los datos de la cola

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


/********************************************************************************
 * Definicion de la estructura para las colas
 *******************************************************************************/
/**
 *Definición de la estructura de la cola
 */

struct _queue {

	uint8_t data[QUEUE_SIZE];   /*Datos de la cola*/
	uint16_t size;				/*tamaño de los datos*/
	uint16_t head;				/*índice del último elemento de la cola*/
	uint16_t tail;				/*índice del primer elemento de la cola*/
	task* queueTask;			/*tarea asociada a la cola*/
};

typedef struct _queue queue;

void osDelay(uint32_t ticks);

void osInitSemaphore(semaphore *sem);
void osGiveSemaphore(semaphore *sem);
void osTakeSemaphore(semaphore *sem);

void osInitQueue(queue *que, uint16_t size);
void osPutQueue(queue *que, void* data);
void osGetQueue(queue *que, void* data);

#endif /* PROJECTS_MSE_IOS1_JAMM_INC_JAMMOS_API_H_ */

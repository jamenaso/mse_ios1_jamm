/*
 * MSE_OS_Core.h
 *
 *  Created on: 26 mar. 2020
 *      Author:
 */

#ifndef ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_
#define ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_

#include <stdint.h>
#include <stdbool.h>
#include "board.h"

/************************************************************************************
 * 			Tamaño del stack predefinido para cada tarea expresado en bytes
 ***********************************************************************************/

#define STACK_SIZE 256

//----------------------------------------------------------------------------------

/************************************************************************************
 * 	Posiciones dentro del stack frame de los registros que conforman el stack frame
 ***********************************************************************************/

#define XPSR			1
#define PC_REG			2
#define LR				3
#define R12				4
#define R3				5
#define R2				6
#define R1				7
#define R0				8
#define LR_PREV_VALUE	9
#define R4				10
#define R5				11
#define R6				12
#define R7				13
#define R8				14
#define R9				15
#define R10 			16
#define R11 			17

//----------------------------------------------------------------------------------


/************************************************************************************
 * 			Valores necesarios para registros del stack frame inicial
 ***********************************************************************************/

#define INIT_XPSR 	1 << 24				//xPSR.T = 1
#define EXEC_RETURN	0xFFFFFFF9			//retornar a modo thread con MSP, FPU no utilizada

/************************************************************************************
 * 						Definiciones varias
 ***********************************************************************************/
#define STACK_FRAME_SIZE	8
#define FULL_REG_STACKING_SIZE 		17	//16 core registers + el valor del registro de Lr Previo link register

#define TASK_NAME_SIZE			10 //Tamaño máximo del vector del nombre de las tareas
#define MAX_TASK_NUMBER			8  //número máximo de tareas en el OS

/*==================[definicion codigos de error del sistema operativo]=================================*/

#define ERR_OS_QUANTITY_TASK	-1

/*==================[definicion de datos del sistema operativo]=================================*/

/************************************************************************************
 * 			Definición de los estados del sistema operativo
 ***********************************************************************************/

enum _osState {
	NORMAL_RUN,
	FROM_RESET
};

typedef enum _osState osState;

/************************************************************************************
 * 			Definición de los estados de las tareas
 ***********************************************************************************/

enum _taskState {
	READY,
	RUNNING
};

typedef enum _taskState taskState;

/************************************************************************************
 * 			Definición de la estructura Tarea
 ***********************************************************************************/

struct _task{
	uint32_t stack[STACK_SIZE/4];
	uint32_t stack_pointer;
	void *entry_point;
	uint8_t id;
	taskState state;
};

typedef struct _task task;

/************************************************************************************
 * 			Definición de la estructura del sistema operativo
 ***********************************************************************************/

struct _osCrt{
	void *taskList[MAX_TASK_NUMBER]; //vector que almacena los punteros a estructuras (task)
									 //de las tareas ingresadas al sistema operativo
	int32_t err;					 //Contiene el ultimo error generado en el OS
	uint8_t quantity_task;			 //cantidad de tareas programadas por el usuario
	osState state;					 //variable que contiene el estado del sistema operativo

	task *current_task;				//variable que almacena el puntero de la tarea actual
	task *next_task;				//variable que almacena el puntero de la tarea siguiente
};

typedef struct _osCrt osCrt;

/*==================[definicion de prototipos]=================================*/

void os_InitTask(void *entryPoint, task *task_init);
void os_Init(void);
int32_t os_getError(void);

#endif /* ISO_I_2020_MSE_OS_INC_MSE_OS_CORE_H_ */

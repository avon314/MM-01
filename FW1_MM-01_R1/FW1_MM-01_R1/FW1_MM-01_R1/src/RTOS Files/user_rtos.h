/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_rtos.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 23/08/2023.
* Module
* Description	: Header file for user_rtos.c
				  Defines constants and macros for user_rtos.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 23/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef USER_RTOS_H_
#define USER_RTOS_H_

/* Definition of macros of RTOS */
#define TASK_GENERAL_STACK_SIZE				(2048/sizeof(portSTACK_TYPE))
#define TASK_GENERAL_STACK_PRIORITY			(tskIDLE_PRIORITY + 2)
#define TASK_MODBUS_TX_STACK_SIZE			(2048/sizeof(portSTACK_TYPE))
#define TASK_MODBUS_TX_STACK_PRIORITY       (tskIDLE_PRIORITY + 3)

extern xSemaphoreHandle xSemaphore;

void Create_Tasks(void);		// Create RTOS tasks in this function.
xSemaphoreHandle Create_Mutex(void);	// Create Mutex.
void Create_Binary_Semaphore(void);		// Create Binary Semaphore.

/***** Function Prototypes *****/
void General_Task(void* pvParameters);
void Modbus_Functionality_Task(void *pvParameters);

/* Extern / Global function declarations related to RTOS */
//extern void vApplicationStackOverflowHook(xTaskHandle *pxTask, signed char *pcTaskName);
extern void vApplicationIdleHook(void);
extern void vApplicationTickHook(void);
extern void vApplicationMallocFailedHook(void);
extern void xPortSysTickHandler(void);

#endif /* USER_RTOS_H_ */
/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_rtos.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 23/08/2023.
* Module
* Description	: All the RTOS related functions, creating tasks, creating
*				  semaphores, mutexes and setting priority for tasks are
*				  set in this file.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 23/08/2023.
* Changes		: NA.
*****************************************************************************/
/* System Includes */
#include "asf.h"

/* User Defined Includes */
#include "tcp_server.h"
#include "user_uart.h"
#include "user_rtos.h"
#include "config_mode.h"
//#include "task_defs.h"

xSemaphoreHandle xMutex;
xSemaphoreHandle xSemaphore;

void vApplicationMallocFailedHook(void);
void vApplicationIdleHook(void);
void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName);
void vApplicationTickHook(void);

/***** Extern / Global Functions *****/
extern void General_Task(void*);
extern void Access_Control_Task(void*);

void vApplicationMallocFailedHook(void)
{
	/**
	 * vApplicationMallocFailedHook() will only be called if
	 * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.
	 * It is a hook function that will get called if a call to
	 * pvPortMalloc() fails. pvPortMalloc() is called internally by
	 * the kernel whenever a task, queue, timer or semaphore is created.
	 * It is also called by various parts of the demo application.
	 * If heap_1.c or heap_2.c are used, then the size of the heap
	 * available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
	 * FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can
	 * be used to query the size of free heap space that remains
	 * (although it does not provide information on how the remaining heap
	 * might be fragmented).
	 */
	taskDISABLE_INTERRUPTS();
	for (;;) {
	}
}

void vApplicationIdleHook(void)
{
	/**
	 * vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK
	 * is set to 1 in FreeRTOSConfig.h.  It will be called on each iteration
	 * of the idle task.  It is essential that code added to this hook
	 * function never attempts to block in any way (for example, call
	 * xQueueReceive() with a block time specified, or call vTaskDelay()).
	 * If the application makes use of the vTaskDelete() API function
	 * (as this demo application does) then it is also important that
	 * vApplicationIdleHook() is permitted to return to its calling
	 * function, because it is the responsibility of the idle task to
	 * clean up memory allocated by the kernel to any task that has
	 * since been deleted.
	 */
}

void vApplicationStackOverflowHook(xTaskHandle pxTask, signed char *pcTaskName)
{
    // Ensure task name is not NULL
    if (pcTaskName != NULL)
    {
        Print_Message("Stack overflow detected in task: ");
        Print_Message((const char *)pcTaskName);  // Convert to `const char *`
        Print_Message("\n");
    }
    else
    {
        Print_Message("Stack overflow detected in unknown task.\n");
    }

    // Add a small delay to allow UART to finish transmission
    vTaskDelay(20);

	/**
	 * Run time stack overflow checking is performed if
	 * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.
	 * This hook function is called if a stack overflow is
	 * detected.
	 */
    // Avoid disabling interrupts before printing
    taskDISABLE_INTERRUPTS();

    // Halt the system
    for (;;);
}


void vApplicationTickHook(void)
{
	/**
	 * This function will be called by each tick interrupt if
	 * configUSE_TICK_HOOK is set to 1 in FreeRTOSConfig.h.
	 * User code can be added here, but the tick hook is called from
	 * an interrupt context, so code must not attempt to block,
	 * and only the interrupt safe FreeRTOS API
	 * functions can be used (those that end in FromISR()).
	 */
}

/************************************************************************************/
/************************************************************************************/

/*****************************************************************************
* Function name	: void Create_Tasks(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	:	Create tasks.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Create_Tasks(void)
{
	if (gb_config_mode_f != 1)
	{
		/* Create task to monitor Ethernet activity */
		if (xTaskCreate(General_Task, "General_Task", /*TASK_GENERAL_STACK_SIZE*//*384*/512, NULL,
		TASK_GENERAL_STACK_PRIORITY, NULL) != pdPASS) {
			Print_Message("\nFailed to create General task.");
		}
		else
		__NOP();
		
		/* Create task to transmit modbus frames. */
		if (xTaskCreate(Modbus_Functionality_Task, "MODBUS_Transmit", TASK_MODBUS_TX_STACK_SIZE, NULL,
		TASK_MODBUS_TX_STACK_PRIORITY, NULL) != pdPASS) {
			Print_Message("\nFailed to create Modbus_Transmit_Task.");
		}
		else
		__NOP();
				
	}
	else
	__NOP();
}

/*****************************************************************************
* Function name	: xSemaphoreHandle Create_Mutex(void)
* Returns		: xSemaphoreHandle ---> Return handle to the mutex.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	:	Create Mutexes.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
xSemaphoreHandle Create_Mutex(void)
{
	xMutex = xSemaphoreCreateMutex();

	if( xMutex != NULL )
	{
		// The semaphore was created successfully.
		// The semaphore can now be used.
		// 		Print_Message("\nMutex is created successfully.");
		// 		delay_s(1);
	}
	
	return xMutex;
}

/*****************************************************************************
* Function name	: xSemaphoreHandle Create_Mutex(void)
* Returns		: xSemaphoreHandle ---> Return handle to the mutex.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	:	Create Mutexes.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Create_Binary_Semaphore(void)
{
	vSemaphoreCreateBinary(xSemaphore);

	if( xSemaphore != NULL )
	{
		// The semaphore was created successfully.
		// The semaphore can now be used.
		
		/* Release the semaphore when created, to access by tasks. */
		xSemaphoreGive(xSemaphore);
	}
}

/*****************************************************************************
* Function name	: configure_timer_for_run_time_stats(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: Compiler generated Function. Used in FreeRTOSConfig.h File.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void configure_timer_for_run_time_stats(void)
{
	pmc_enable_periph_clk(ID_TC0);
	tc_init(TC0, 1,						// Init timer counter 0 channel 0.
	TC_CMR_WAVE |				// Waveform Mode is enabled.
	TC_CMR_TCCLKS_TIMER_CLOCK5	// Use slow clock to avoid overflow.
	);

	tc_write_rc(TC0, 1, 0xffffffff);	// Load the highest possible value into TC.

	tc_start(TC0, 1);					// Start Timer counter 0 channel 0.
}

/*****************************************************************************
* Function name	: get_run_time_counter_value(void)
* Returns		: uint32_t ---> Return count value.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: Compiler generated Function. Used in FreeRTOSConfig.h File.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
uint32_t get_run_time_counter_value(void)
{
#if SAM4E
	return TC0->TC_CHANNEL[1].TC_CV;
#else
	static uint32_t count = 0;
	uint32_t val = TC0->TC_CHANNEL[1].TC_CV;

	val >>= 4;
	if ((count & 0x00000FFF) < val)
	{
		count += 0x1000;
	}
	count = (count & 0xFFFFF000) | val;
	return count;
#endif
}
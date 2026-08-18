/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: main.c
* Created By	: Shirisha Ankam.
* Created Date	: 1/12/2025.
* Module
* Description	:	Main file which holds the driver initialization functions,
					KNX and application related functions.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NA.
*****************************************************************************/
/* System Includes */
#include "asf.h"

/* User Includes */
#include "application.h"
#include "ext_eeprom.h"
#include "app_eeprom.h"



/*****************************************************************************
* Function name	: int main (void)
* Returns		: int.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
*
*
* Description	:	Main function which holds the system clock initialization.
*					driver functions, KNX initialization functions and
*					application related functions.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void main(void)
{
	sysclk_init();	/* ASF function to setup clocking. */
	NVIC_SetPriorityGrouping(0);	/* Ensure all priority bits are assigned as preemption priority bits. */
	board_init();	/* Atmel library function to setup for the evaluation kit being used. */
	
	Driver_Initialization();	// Initialize Drivers.
	
	Run_RTOS();		// Create tasks and start scheduler.
	

	/* If all is well, the scheduler will now be running, and the following line
	will never be reached.  If the following line does execute, then there was
	insufficient FreeRTOS heap memory available for the idle and/or timer tasks
	to be created.  See the memory management section on the FreeRTOS web site
	for more details.. */
	for (;;)
	{
		
	}
	
	return;
}

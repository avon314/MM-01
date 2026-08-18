/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: application.c
* Created By	: Shirisha Ankam.
* Created Date	: 1/12/2025.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NA.
*****************************************************************************/

/***** User defined Includes *****/
#include "application.h"
#include "user_uart.h"
#include "rs485_uart.h"
#include "rs485_driver.h"
#include "user_rtos.h"
#include "digital_input.h"
#include "relay_output.h"
#include "user_timer.h"
#include "onboard_key.h"
#include "config_mode.h"
#include "ext_eeprom.h"
#include "led_operation.h"
#include "Modbus.h"
#include "mm_app.h"
#include "group_config.h"
/**************************************/
#include "ethernet.h"
#include "tcp_server.h"

#include <user_rtc.h>
#include "mem.h"
#include "string.h"
#include "memory_storage.h"
#include "core_sntp.h"
#include "modbus_app.h"

/***** External RAM Section *****/
#include "user_spi.h"
#include "ext_eeprom_1.h"
#include "CRC.h"

xSemaphoreHandle xMutex1;

U8 gb_tm_cnt_f = 0;
U16 gb_srh_count = 0;

/*****************************************************************************
* Function name	: void Driver_Initialization(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
*
* Description	:	All the drivers all initialized here.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Driver_Initialization(void)
{
	//wdt_disable(WDT);					/* Disable WDT at power on */
	UART_Debug_Init();					/* Initialization of UART0 for debug communication. */
	Print_Message("\nDevice Power-Up.");
	Configure_Onboard_Key();			/* Configure port pin as an input and interrupt edge for inputs. */
	Configure_LED_PortPins();			/* Configure port pins for leds. */
	Timer_Init();						/* Configure timer to generate an interrupt at every 1 ms. */
	Digital_Input_Driver_Init();		/* Configure port pins as an input and interrupt edge for inputs. */
	Relay_Output_Driver_Init();			/* Configure port pins as an output & enable pull ups. */
	RS485_Driver_Init();				/* Initialization of RS485 Driver. */
	configure_twi();					/* Configure twi(i2c) to write data to eeprom. */
	eeprom_pin_config();				/* Configure pin used for eeprom write protect. */
	Read_MacID_And_Update_InSystem();	/* Read MAC ID and update the default one. */
	
	configure_spi_master();             // Initialize SPI master interface
	RTC_Interrupt_Pin_Configure();      // Setup RTC interrupt pin configuration
	EXT_EEPROM_Init();                  // Initialize external EEPROM
	enter_config_mode();                // Enter configuration mode for device setup
	
	Read_Memory_For_PowerON_State();    // Restore saved state from memory after power-on
	init_ethernet();					/* Configure the Ethernet driver */
	Configure_WDT();					/* Configure 3 sec's of WDT timer to prevent the software stuck. */
	Modbus_init();                      // Initialize Modbus communication protocol
	gb_sntp_powerOn_f = 1;              // Set SNTP power-on flag to indicate fresh boot
	
	U8 arr[512] = {0};                  
	memset(arr, 0, 512);               
	
	Print_Message("\nFirmware Version : ");
	memcpy(arr, MfgInfo.fw_vs, 6);     
	arr[6] = '\0';                      
	Print_Message(arr);                 
	
	Print_Message("\n\nThis is Monitor Module 01\n\n");

}




/*****************************************************************************
* Function name	: void Run_RTOS(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	:	Create tasks, semaphores, mutexes and set priority
*					and run scheduler.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Run_RTOS(void)
{
	Create_TCP_Server_Task();
	Create_Tasks();					// Create tasks and set priority.
	Create_Binary_Semaphore();		// Create Binary Semaphore to access shared resource or synchronization.

	/* Start the scheduler. */
	vTaskStartScheduler();
}

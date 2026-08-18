/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: config_mode.c
* Created By	: Harshit Agnihotri.
* Created Date	: 20/03/2024.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 20/03/2024.
* Changes		: NA.
*****************************************************************************/

/* System Includes */
#include "asf.h"

/* User Includes */
#include "config_mode.h"
#include "user_uart.h"
#include "app_utility.h"
#include "app_eeprom.h"
#include "definitions.h"
#include "onboard_key.h"
#include "bootloader.h"	
#include "led_operation.h"
U8 gb_device_power_on = 0;

volatile U8 gb_config_mode_f = 0;             // Configuration mode flag.
volatile U8 gb_cfg_mode_keyPressed = 0;            // To check entered in config mode or not.
volatile U8 gb_exit_from_cfg_f = 0;	// flag to check after exit from config mode, key released or not?

volatile int gb_config_timer = 0;                  // Configuration timer.
volatile U8 gb_config_timer_running_f = 0;    // Configuration timer flag.

volatile U16 gb_config_led_timer = 0;             // Configuration LED timer.
volatile U8 gb_config_led_on_f = 0;           // Configuration LED flag.

volatile U8 gb_config_rst_key_f = 0;              // Configuration mode reset key flag.
volatile int gb_config_rst_key_timer = 0;     // Configuration mode reset key timer.



/*****************************************************************************
* Function name: void config_mode_init(void).
* Returns		: nothing.
* Arguments    : None.
* Created by	: Harshit Agnihotri.
* Date created	: 20/03/2024.
*
* Description	: Function to initialize the key used to enter configuration mode.
*              :
* Notes		: NA.
* Global Variables Affected : .
*****************************************************************************/
 void config_mode_init(void)
 {
     REG_CCFG_SYSIO = CCFG_SYSIO_SYSIO10;         // Instead of system pin, configure PB10 as I/O.
     pmc_enable_periph_clk(ID_PIOB);              /* Enable Peripheral clock */
     pio_set_input(PIOB, PIO_PB10, PIO_PULLUP);   // Set as input pin.
 }

 /*****************************************************************************
 * Function name: void config_mode_key_detect(void).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Function to enter configuration mode by detecting whether configuration mode key is pressed at power on and set config mode flag.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f.
 *****************************************************************************/
 void enter_config_mode(void)
 {	
	 gb_config_mode_f = 0;
	 gb_device_power_on = 1;
	 Read_Config_From_EEP_1_Memory();
 }

  /*****************************************************************************
 * Function name: void exit_config_mode(void).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Function to exit configuration mode by reseting the configuration mode flag and connect with device flag.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f, gb_conn_dvc_f.
 *****************************************************************************/
 void exit_config_mode(void)
 {
     gb_config_mode_f = 0;                         // Exit configuration mode by reseting configuration mode flag and connect with device flag.
     gb_conn_dvc_f = 0;

     #if DEBUG_ALL || DEBUG_CONFIG_MODE
     Print_Message("\nOut of configuration mode.");
     #endif
                           // Write all configuration data to EEPROM.
	Write_Config_To_EEP_1_Memory();

	gb_device_power_on = 1;
	ON_RED_LED;
	
	// Trigger a software reset
	RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
 }

 /*****************************************************************************
 * Function name: void config_mode(void *pvParameters).
 * Returns	: nothing.
 * Arguments   : None.
 * Created by	: Harshit Agnihotri.
 * Date created: 20/03/2024.
 *
 * Description	: Main logic of configuration mode.
 *             :
 * Notes		: NA.
 * Global Variables Affected : gb_config_mode_f.
 *****************************************************************************/
 void config_mode(void *pvParameters)
 {
	 /* Just to avoid compiler warnings. */
	 UNUSED(pvParameters);

     while (1)
     {
          if (gb_config_mode_f == FLAG_SET)                                                  // Check if configuration mode flag is set.
          {
               if(gb_config_timer_running_f == 0)                           // Check if configuration mode timer is running.
               {
                    gb_config_timer = 0;                                        // Start the configuration mode timer and configuration LED timer and set the configuration mode timer running flag.
                    gb_config_led_timer = 0;
                    gb_config_timer_running_f = 1;
               }
               else if(gb_config_timer_running_f == 1 && gb_utility_rcv_f == 0 && gb_config_timer >= CONFIG_FRAME_RCV_TIME_OUT)      // Check if configuration mode timer running flag is set and utility receive flag is not set and utility receive frame timeout value.
               {
				   /*****
				   Exit from configuration due to no frame
				   *****/
				   gb_cfg_mode_keyPressed = 0;
                   exit_config_mode();                                         // Exit configuration mode.
               }
			   else
			   __NOP();

               if((gb_config_led_on_f == 0) && (gb_config_led_timer >= CONFIG_LED_TOGGLE_TIME))      // Toggle configuration LED for every CONFIG_LED_TOGGLE_TIME ms.
               {
                    gb_config_led_on_f = 1;
                    gb_config_led_timer = 0;
                    ON_RED_LED;
               }
               else if((gb_config_led_on_f == 1) && (gb_config_led_timer >= CONFIG_LED_TOGGLE_TIME))
               {
                    gb_config_led_on_f = 0;
                    gb_config_led_timer = 0;
                    OFF_RED_LED;
               }
			   else
			   __NOP();

          }
		  else
		  {
			  vTaskDelete(NULL);
		  }
		  
		  /* Boot loader Function */
		  Check_Boot_Command() ;      // Jump into Boot loader 
		  
          vTaskDelay(1);
     }
 }
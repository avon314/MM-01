/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: onboard_key.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: File handles the configuration of an on board key as an input
*				  and configure its interrupt routine to read the state. And also,
*				  function written to set an default IP address to the system.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NA.
*****************************************************************************/
/* System Includes */
#include "asf.h"
#include "onboard_key.h"

/* User Includes */
#include "user_uart.h"
#include "definitions.h"
#include "ethernet.h"
#include "tcp_server.h"
#include "app_eeprom.h"
#include "comn_config.h"
#include "ext_eeprom.h"
#include "dflt_config.h"
 
#include "memory_storage.h"
#include "ext_eeprom_1.h"
#include "CRC.h"


/***** Macros / Definitions *****/
#define ONB_KEY_PIN			(PIO_PB10)
#define ON_BOARD_KEY_PORT	(PIOB)
#define ON_BOARD_K_PRT_ID	(ID_PIOB)

#define ONB_KEY_PRESSED_HTL	(!pio_get(PIOB, PIO_TYPE_PIO_INPUT, PIO_PB10))

ON_BOARD_KEY onb_key_flag;

/***** Function Prototypes *****/
static void OnBoard_Key_Handler(const uint32_t id, const uint32_t index);

/*****************************************************************************
* Function name	: void Configure_Onboard_Key(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to configure an on board key as an input.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Configure_Onboard_Key(void)
{
	REG_CCFG_SYSIO = CCFG_SYSIO_SYSIO10;			// Instead of system pin, configure PB10 as I/O.
	pmc_enable_periph_clk(ON_BOARD_K_PRT_ID);		/* Enable Peripheral clock */
	pio_set_input(ON_BOARD_KEY_PORT, ONB_KEY_PIN, PIO_PULLUP);		/* Enable PIN as Input */
	
 	pio_handler_set(ON_BOARD_KEY_PORT, ON_BOARD_K_PRT_ID, ONB_KEY_PIN, PIO_IT_EDGE, OnBoard_Key_Handler);
 	pio_enable_interrupt(ON_BOARD_KEY_PORT, ONB_KEY_PIN);
 	pio_set_debounce_filter(ON_BOARD_KEY_PORT, ONB_KEY_PIN, 10);    /* de-bounce period - 1/10 i.e. 100ms */
	NVIC_EnableIRQ(PIOB_IRQn);
}

/*****************************************************************************
* Function name	: static void OnBoard_Key_Handler(const uint32_t id, const uint32_t index)
* Returns		: Nothing.
* Arguments    	: const uint32_t id ---> pass port ID.
*				  const uint32_t index ---> pass port index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: It is a key detect handler, reads the state of pin
*				  and set/reset "onb_key_flag.key_detect_f" flag based on high to low transition.
*               :
* Notes			: NA.
* Global Variables Affected : onb_key_flag.key_detect_f.
*****************************************************************************/
static void OnBoard_Key_Handler(const uint32_t id, const uint32_t index)
{
	if ((id == ON_BOARD_K_PRT_ID) && (index == ONB_KEY_PIN))
	{
		if (ONB_KEY_PRESSED_HTL)
		{
			#if DEBUG_ALL || DEBUG_ONBOARD_KEY
			Print_Message("\nOn board key pressed.");
			#endif
			
			onb_key_flag.key_detect_f = FLAG_SET;
			
			// Trigger a software reset
			//RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
		}
		else
		{
			#if DEBUG_ALL || DEBUG_ONBOARD_KEY
			Print_Message("\nOn board key released.");
			#endif
			
			onb_key_flag.key_detect_f = FLAG_RST;
		}
	}
}

/*****************************************************************************
* Function name	: void Monitor_Key_For_Default_IPAddress(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 02/12/2023.
* Description	: Function is written to start the timer after key detection.
*				  Set an led indication for default ip setting mode.
*				  Set required flags to set a default ip address.
*				  And triggers the software reset, after setting default ip address.
*               :
* Notes			: NA.
* Global Variables Affected : onb_key_flag.key_control_f, onb_key_flag.key_set_for_default_ip.
							  onb_key_flag.set_default_ip_f, onb_key_flag.start_timer_f
							  
*****************************************************************************/
void Monitor_Key_For_Default_IPAddress(void)
{
	// Check if the onboard key is detected and control flag is not set
	if ((onb_key_flag.key_detect_f == FLAG_SET) && (onb_key_flag.key_control_f == FLAG_RST))
	{
		Print_Message("\nkey detected");
		onb_key_flag.key_control_f = FLAG_SET; // Set control flag
		
		// Check if the key is set for default IP
		if (onb_key_flag.key_set_for_default_ip == FLAG_SET)
		{
			Print_Message("\nSet to default");
			onb_key_flag.key_set_for_default_ip = FLAG_RST; // Reset key flag
			
			// Perform actions for setting default IP
			write_dflt_comn_config(); // Write default communication configuration
			U8 lcl_rd_config_data[COMN_CONFIG_BYTES] = {0};
			read_comn_settings(lcl_rd_config_data); // Read communication configuration and write to EEPROM and flash
			CRC_Calculate_Check(lcl_rd_config_data, (COMN_CONFIG_BYTES - 2), CRC_CALC);
			lcl_rd_config_data[(COMN_CONFIG_BYTES - 2)] = gb_Temp_CRC[0];	// Higer Byte.
			lcl_rd_config_data[(COMN_CONFIG_BYTES - 1)] = gb_Temp_CRC[1];	// Lower Byte.
			//write_to_eeprom(EEPROM_COMN_CONFIG_ADDR, lcl_rd_config_data, COMN_CONFIG_BYTES); // Write to EEPROM
			EEPROM_Page_Write(UT_EEP_START_PAGE, UT_EEP_PAGE_OFFSET, lcl_rd_config_data, COMN_CONFIG_BYTES);  // write to flash
			
			onb_key_flag.set_default_ip_f = FLAG_SET; // Set flag for default IP set
			ON_RED_LED; // Turn on red LED
		}
		else
		{
			onb_key_flag.start_timer_f = FLAG_SET; // Start timer
			onb_key_flag.timer_value = 0;	// Reset timer value
		}
	}
	// If key is not detected and control flag is set, reset flags and timer
	else if ((onb_key_flag.key_detect_f == FLAG_RST) && (onb_key_flag.key_control_f == FLAG_SET))
	{
		onb_key_flag.key_control_f = FLAG_RST;
		onb_key_flag.start_timer_f = FLAG_RST;
		onb_key_flag.timer_value = 0;
	}
	
	// If key is pressed for defined time, toggle red LED
	if (onb_key_flag.key_set_for_default_ip == FLAG_SET)
	{
		if (onb_key_flag.led_on_f == FLAG_SET)
		ON_RED_LED;
		else if (onb_key_flag.led_off_f == FLAG_SET)
		OFF_RED_LED;
	}
	
	// If default IP is set and key is not detected, trigger a software reset
	if ((onb_key_flag.set_default_ip_f == FLAG_SET) && (onb_key_flag.key_detect_f == FLAG_RST))
	{
		onb_key_flag.set_default_ip_f = FLAG_RST;
		
		// Trigger a software reset
		RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
	}
}

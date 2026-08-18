/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: rs485_driver.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 22/08/2023.
* Module
* Description	:	
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 22/08/2023.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User defined Includes *****/
#include "rs485_driver.h"
#include "definitions.h"
#include "rs485_uart.h"
#include "led_operation.h"

/* User defined macros */
/* Define a macro of intercharacter delay to check error while receiving bytes. */
#define ALLOWED_INTERCHAR_TIME_RS485	20	/*20 ms delay*/

/* Configure enable pin of rs485 converter. */
#define RS485_EN_PIN			(PIO_PA20)
#define PORT_RS485_EN_PIN		(PIOA)
#define PORT_ID_RS485_EN_PIN	(ID_PIOA)
#define Enable_RS485_TX()		(pio_set(PORT_RS485_EN_PIN, RS485_EN_PIN))
#define Enable_RS485_RX()		(pio_clear(PORT_RS485_EN_PIN, RS485_EN_PIN))

/***** Global variables *****/
U8 gb_rs485_iChar_error_f = 0;	// Flag sets when an inter char error occurred.
U8 gb_interchar_time_ms = 0;	// Fill the variable with interchar delay.

/*****************************************************************************
* Function name	: void RS485_Driver_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: RS485 driver/converter enable pin is configured.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void RS485_Driver_Init(void)
{
	/* Initialization of PORT pin for Enabling the TX/RX of the converter. */
	pmc_enable_periph_clk(PORT_ID_RS485_EN_PIN);	// Enable peripheral clock.
	pio_set_output(PORT_RS485_EN_PIN, RS485_EN_PIN, LOW, DISABLE, ENABLE);	// Configure PA20 as output, pull up enabled and logic low at initial.
}

/*****************************************************************************
* Function name	: void Send_data_On_RS485(U8 *bufdata, U8 buflen)
* Returns		: Nothing.
* Arguments    	: U8 *bufdata ---> Holds the address of an array/buffer of data.
*				  U8 buflen ---> Pass the length of the buffer.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/08/2023.
*
* Description	: Puts data on the RS485 line.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Send_data_On_RS485(U8 *bufdata, U8 buflen)
{
	Enable_RS485_TX();	// Enable RS485 TX line.
	ON_GREEN_LED;
	delay_ms(1);	// Delay need to be implemented bcz to stable the Enable line.
	Put_Data_On_RS485_UART(bufdata, buflen);
	while (!gb_usart_ready_f);	// Wait till complete data transmission.
	OFF_GREEN_LED;
	Enable_RS485_RX();	// Enable RS485 RX line.
}

/*****************************************************************************
* Function name	: void RS485IntercharDelayLoad(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 29/12/2022.
* Description	: Function to fill intercharacter delay time.
*               :
* Notes			: Call this function after a byte received.
* Global Variables Affected : gb_interchar_time_ms.
*****************************************************************************/
void RS485IntercharDelayLoad(void)
{
	gb_interchar_time_ms = ALLOWED_INTERCHAR_TIME_RS485;
}

/*****************************************************************************
* Function name	: void RS485IntercharDelayError(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 29/12/2022.
* Description	: used to check for error in inter-character delay.
                  Allowed delay between two consecutive receiving characters in RS485 is 20 ms.
*               :
* Notes			: Call at 1ms timer ISR.
* Global Variables Affected : gb_interchar_time_ms, gb_receiving_f
*****************************************************************************/
void RS485IntercharDelayError(void)
{
	if (gb_interchar_time_ms > 0)
	{
		gb_interchar_time_ms--;
		if (gb_interchar_time_ms == 0)
		{
			gb_rs485_iChar_error_f = uSET;	// Set this flag to say that inter character error occured.
		}
	}
}


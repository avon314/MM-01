/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: led_operation.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 09/11/2023.
* Module
* Description	: The on board LED functionalities are written here.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 09/11/2023.
* Changes		: NA.
*****************************************************************************/
/***** Systems Includes *****/
#include "asf.h"
#include "pio.h"

/***** User Includes *****/
#include "led_operation.h"

/***** Function Prototypes / Declarations *****/
static void Configure_Power_LED(void);
static void Configure_RS485_LEDs(void);

/*****************************************************************************
* Function name	: void Configure_LED_PortPins(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 09/11/2023.
*
* Description	: Configure port pins as digital output.
*				  1. Configure port pin as power led.
*				  2. Configure port pins for rs485 LEDs.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Configure_LED_PortPins(void)
{
	Configure_Power_LED();	/* Configure red led */
	Configure_RS485_LEDs();	/* Configure green and yellow leds */
}

/*****************************************************************************
* Function name	: static void Configure_Power_LED(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 03/10/2023.
*
* Description	: Configure port pin as digital output, now the pin is connected
*				  to led, led is using as power led. Also an user can use for
*				  general purpose.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
static void Configure_Power_LED(void)
{
	pmc_enable_periph_clk(ID_PIOA);	/* Enable Peripheral clock */
	pio_set_output(PIOA, POWER_LED, LOW, DISABLE, ENABLE);	/* Configure red led as an output and default state to low. */
}

/*****************************************************************************
* Function name	: static void Configure_RS485_LEDs(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 03/10/2023.
*
* Description	: Configure port pins as digital output, and those pins are
*				  used for RS485 tx/rx led operations.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
static void Configure_RS485_LEDs(void)
{
	pmc_enable_periph_clk(ID_PIOA);			/* Enable Peripheral clock */
	pio_set_output(PIOA, YLW_LED_RX, HIGH, DISABLE, ENABLE);	/* Configure yellow led as an output and default state to high. */
	
	REG_CCFG_SYSIO = CCFG_SYSIO_SYSIO11;	// Instead of system pin configure PB11 as I/O.
	pmc_enable_periph_clk(ID_PIOB);			/* Enable Peripheral clock */
	pio_set_output(PIOB, GRN_LED_TX, HIGH, DISABLE, ENABLE);	/* Configure green led as an output and default state to high. */
}

/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_input.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 05/10/2023.
* Module
* Description	: Driver to configure PORT pins as relay output.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 05/10/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "relay_output.h"

/***** Definitions / Macros *****/
/***** Connected On Port A *****/
#define RELAY_OUT_PIN_1			(PIO_PA22)
#define RELAY_OUT_PIN_2			(PIO_PA8)
#define RELAY_OUT_PIN_3			(PIO_PA7)
/****************************************/
#define RELAY_PORT_A			(PIOA)
#define ID_RLY_PORTA			(ID_PIOA)
#define RELAYA_PIN_MASK			((RELAY_OUT_PIN_1) | (RELAY_OUT_PIN_2) | (RELAY_OUT_PIN_3))

/***** Connected On Port D *****/
#define RELAY_OUT_PIN_4			(PIO_PD30)
/****************************************/
#define RELAY_PORT_D			(PIOD)
#define ID_RLY_PORTD			(ID_PIOD)

/*****************************************************************************
* Function name	: void Relay_Output_Driver_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 06/10/2023.
*
* Description	: Configure port pins as an output and enable pull up set the
*				  default state to HIGH (Initially).
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Relay_Output_Driver_Init(void)
{
	/*****
	* Configure Port pins as an Output and set an initial state to HIGH.
	* And PULL UP enabled for outputs 1,2,3
	*****/
	pmc_enable_periph_clk(ID_RLY_PORTA);	/* Enable peripheral clock */
	pio_set_output(RELAY_PORT_A, RELAYA_PIN_MASK, HIGH, DISABLE, ENABLE);	/* Set defined port pins as an output */
	
	/*****
	* Configure Port pins as an Output and set an initial state to HIGH.
	* And PULL UP enabled for output 4.
	*****/
	pmc_enable_periph_clk(ID_RLY_PORTD);	/* Enable peripheral clock */
	pio_set_output(RELAY_PORT_D, RELAY_OUT_PIN_4, HIGH, DISABLE, ENABLE);	/* Set defined port pins as an output */
}

/*****************************************************************************
* Function name	: void Operate_Relay(U32 RELAY_NUMBER, U32 operation)
* Returns		: Nothing.
* Arguments    	: U32 RELAY_NUMBER	---> Pass the relay number to operate.
*				  U32 operation	---> make on or off, pass 1 to on and 0 to off.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 06/10/2023.
*
* Description	: Function written to operate output relay.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Operate_Relay(U32 RELAY_NUMBER, U32 operation)
{
	switch (RELAY_NUMBER)
	{
		case RELAY1 : 
			if (operation == ON)
			{
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_1);	/* ON the Relay. */
			}
			else
			{
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_1);		/* Off the Relay. */
			}
		break;
		case RELAY2 :
			if (operation == ON)
			{
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_2);	/* ON the Relay. */
			}
			else
			{
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_2);		/* Off the Relay. */
			}
		break;
		case RELAY3 :
			if (operation == ON)
			{
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_3);	/* ON the Relay. */
			}
			else
			{
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_3);		/* Off the Relay. */
			}
		break;
		case RELAY4 :
			if (operation == ON)
			{
				pio_clear(RELAY_PORT_D, RELAY_OUT_PIN_4);	/* ON the Relay. */
			}
			else
			{
				pio_set(RELAY_PORT_D, RELAY_OUT_PIN_4);		/* Off the Relay. */
			}
		break;
		case ALL_RELAY :
			if (operation == ON)
			{
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_1);		/* Off the Relay. */
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_2);		/* Off the Relay. */
				pio_clear(RELAY_PORT_A, RELAY_OUT_PIN_3);		/* Off the Relay. */
				pio_clear(RELAY_PORT_D, RELAY_OUT_PIN_4);		/* Off the Relay. */
			}
			else
			{
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_1);		/* Off the Relay. */
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_2);		/* Off the Relay. */
				pio_set(RELAY_PORT_A, RELAY_OUT_PIN_3);		/* Off the Relay. */
				pio_set(RELAY_PORT_D, RELAY_OUT_PIN_4);		/* Off the Relay. */
			}
		break;
		default:
			pio_set(RELAY_PORT_A, RELAY_OUT_PIN_1);		/* Off the Relay. */
			pio_set(RELAY_PORT_A, RELAY_OUT_PIN_2);		/* Off the Relay. */
			pio_set(RELAY_PORT_A, RELAY_OUT_PIN_3);		/* Off the Relay. */
			pio_set(RELAY_PORT_D, RELAY_OUT_PIN_4);		/* Off the Relay. */
		break;
	}
}
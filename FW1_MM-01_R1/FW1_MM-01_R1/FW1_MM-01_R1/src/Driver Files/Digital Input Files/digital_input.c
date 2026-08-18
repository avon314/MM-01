/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_input.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 05/10/2023.
* Module
* Description	: Driver to configure PORT pins as digital inputs and Implemented
*				  logic to detect a signal based on rising edge and falling edge.
*				  And setting appropriate flags.
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
#include "digital_input.h"
#include "user_uart.h"
#include "group_config.h"


/***** Structure Variable *****/

input_state_t ip_state[MAX_INPUTS];
U8 gb_input_detected_f = 0;
U8 gb_input_cotrol_f[MAX_INPUTS] = {0};

/*****************************************************************************
* Function name	: void Digital_Input_Driver_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/10/2023.
*
* Description	: Initialization of Port pins as an Input, enable interrupt on
*				  edge detection. Enabled clock for the peripheral
*               :
* Notes			: Pins are bit masked please check the defined macros.
* Global Variables Affected : NA.
*****************************************************************************/						 
void Digital_Input_Driver_Init(void)
{
	/*****
	* Configure Port pins as an Input and set an interrupt on edge detection.
	* for inputs 1,2,3,5,7,8
	*****/
	pmc_enable_periph_clk(ID_PORT_PIN_123_5_78);	/* Enable peripheral clock for defined PORT */
	pio_set_input(PORT_PIN_123_5_78, PORTD_PIN_MASK, PIO_DEFAULT);	/* Set defined Port pins as an input */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_1, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_2, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_3, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_5, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_7, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_123_5_78, ID_PORT_PIN_123_5_78, DIG_INPUT_PIN_8, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	
	pio_get_interrupt_status(PORT_PIN_123_5_78);                   // Clear any pending interrupt status BEFORE enabling interrupt	
		
	pio_enable_interrupt(PORT_PIN_123_5_78, PORTD_PIN_MASK);	/* Enable interrupt for defined port pins */
	pio_set_debounce_filter(PORT_PIN_123_5_78, PORTD_PIN_MASK, 10);    /* de-bounce period - 1/10 i.e. 100ms */
	NVIC_EnableIRQ(PORTD_IRQn); /* Set an interrupt source */
	
	/*****
	* Configure Port pins as an Input and set an interrupt on edge detection.
	* for inputs 4,6, 12, 13, 14
	*****/
	pmc_enable_periph_clk(ID_PORT_PIN_4_6);	/* Enable peripheral clock for defined PORT */
	pio_set_input(PORT_PIN_4_6, PORTA_PIN_MASK, PIO_DEFAULT);	/* Set defined Port pins as an input */
	pio_handler_set(PORT_PIN_4_6, ID_PORT_PIN_4_6, DIG_INPUT_PIN_4, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	pio_handler_set(PORT_PIN_4_6, ID_PORT_PIN_4_6, DIG_INPUT_PIN_6, PIO_IT_EDGE, Pin_Edge_Handler);	/* Set interrupt on edge detection for defined port pins. */
	
	pio_get_interrupt_status(PORT_PIN_4_6);
		
	pio_enable_interrupt(PORT_PIN_4_6, PORTA_PIN_MASK);	/* Enable interrupt for defined port pins */
	pio_set_debounce_filter(PORT_PIN_4_6, PORTA_PIN_MASK, 10);    /* de-bounce period - 1/10 i.e. 100ms */
	NVIC_EnableIRQ(PORTA_IRQn); /* Set an interrupt source */
	
}

/*****************************************************************************
* Function name	: void Pin_Edge_Handler(const uint32_t id, const uint32_t index)
* Returns		: Nothing.
* Arguments    	: const uint32_t id ---> PORT ID.
*				  const uint32_t index ---> Pin Index or Port pin number.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/10/2023.
*
* Description	: Pin edge handler is written to get the value of the pin state
*				  when pin edge changes from high to low or high to low.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Pin_Edge_Handler(const uint32_t id, const uint32_t index)
{	
	gb_input_detected_f = 1;
	if (id == ID_PORT_PIN_123_5_78)
	{
		switch (index)
		{
			case DIG_INPUT_PIN_1 : gb_input_cotrol_f[0] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_1))
				{
					#if DIG_INPUT
					Print_Message("\nIP 1 High");
					#endif
					ip_state[0].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 1 Low");
					#endif
					ip_state[0].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_2 : gb_input_cotrol_f[1] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_2))
				{
					#if DIG_INPUT
					Print_Message("\nIP 2 High");
					#endif
					ip_state[1].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 2 Low");
					#endif
					ip_state[1].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_3 : gb_input_cotrol_f[2] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_3))
				{
					#if DIG_INPUT
					Print_Message("\nIP 3 High");
					#endif
					ip_state[2].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 3 Low");
					#endif
					ip_state[2].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_5 : gb_input_cotrol_f[4] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_5))
				{
					#if DIG_INPUT
					Print_Message("\nIP 5 High");
					#endif
					ip_state[4].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 5 Low");
					#endif
					ip_state[4].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_7 : gb_input_cotrol_f[6] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_7))
				{
					#if DIG_INPUT
					Print_Message("\nIP 7 High");
					#endif
					ip_state[6].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 7 Low");
					#endif
					ip_state[6].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_8 : gb_input_cotrol_f[7] = 1;
				if (pio_get(PORT_PIN_123_5_78, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_8))
				{
					#if DIG_INPUT
					Print_Message("\nIP 8 High");
					#endif
					ip_state[7].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 8 Low");
					#endif
					ip_state[7].htl_flag = 1;
				}
			break;
			default:
				__NOP();
			break;
		}
	}
	else if (id == ID_PORT_PIN_4_6)
	{
		switch (index)
		{
			case DIG_INPUT_PIN_4 : gb_input_cotrol_f[3] = 1;
				if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_4))
				{
					#if DIG_INPUT
					Print_Message("\nIP 4 High");
					#endif
					ip_state[3].lth_flag = 1;
				}
				else				
				{
					#if DIG_INPUT
					Print_Message("\nIP 4 Low");
					#endif
					ip_state[3].htl_flag = 1;
				}
			break;
			case DIG_INPUT_PIN_6 : gb_input_cotrol_f[5] = 1;
				if (pio_get(PORT_PIN_4_6, PIO_TYPE_PIO_INPUT, DIG_INPUT_PIN_6))
				{
					#if DIG_INPUT
					Print_Message("\nIP 6 High");
					#endif
					ip_state[5].lth_flag = 1;
				}
				else
				{
					#if DIG_INPUT
					Print_Message("\nIP 6 Low");
					#endif
					ip_state[5].htl_flag = 1;
	
				}
			break;
			default:
				__NOP();
			break;
		}
	}
	else
	{
		__NOP();
	}
}


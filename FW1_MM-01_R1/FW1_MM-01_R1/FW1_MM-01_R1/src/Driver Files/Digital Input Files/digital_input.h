/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: digital_input.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 05/10/2023.
* Module
* Description	: Header file for digital_input.c
				  Defines constants and macros for digital_input.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 05/10/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef DIGITAL_INPUT_H_
#define DIGITAL_INPUT_H_

#include "group_config.h"

#define DIG_INPUT                (0)

/***** Definitions / Macros *****/
/***** Connected On Port D *****/
#define DIG_INPUT_PIN_1					(PIO_PD22)
#define DIG_INPUT_PIN_2					(PIO_PD21)
#define DIG_INPUT_PIN_3					(PIO_PD23)
#define DIG_INPUT_PIN_5					(PIO_PD24)
#define DIG_INPUT_PIN_7					(PIO_PD25)
#define DIG_INPUT_PIN_8					(PIO_PD26)

/****************************************/
#define PORT_PIN_123_5_78				(PIOD)
#define ID_PORT_PIN_123_5_78			(ID_PIOD)
#define PORTD_PIN_MASK					((DIG_INPUT_PIN_1) | (DIG_INPUT_PIN_2) | (DIG_INPUT_PIN_3) |\
										(DIG_INPUT_PIN_5) | (DIG_INPUT_PIN_7) | (DIG_INPUT_PIN_8))
#define PORTD_IRQn						(PIOD_IRQn)
/***** End of section Connected On Port D *****/

/***** Connected On Port A *****/
#define DIG_INPUT_PIN_4					(PIO_PA25)
#define DIG_INPUT_PIN_6					(PIO_PA24)
/****************************************************/
#define PORT_PIN_4_6					(PIOA)
#define ID_PORT_PIN_4_6					(ID_PIOA)
#define PORTA_PIN_MASK					((DIG_INPUT_PIN_4) | (DIG_INPUT_PIN_6) )

#define PORTA_IRQn						(PIOA_IRQn)


typedef struct {
	U8 lth_flag;
	U8 htl_flag;
	U8 current_state;
	U8 previous_state;
} input_state_t;

extern input_state_t ip_state[MAX_INPUTS];
extern U8 gb_input_detected_f;
extern U8 gb_input_cotrol_f[MAX_INPUTS];

/***** Function Declarations / Prototypes *****/
void Digital_Input_Driver_Init(void);
void Pin_Edge_Handler(const uint32_t id, const uint32_t index);

#endif /* DIGITAL_INPUT_H_ */

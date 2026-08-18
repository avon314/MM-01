/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: led_operation.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 09/11/2023.
* Module
* Description	: Header file for led_operation.c
				  Defines constants and macros for led_operation.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 09/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef LED_OPERATION_H_
#define LED_OPERATION_H_

/***** Systems Includes *****/
/***** User Includes *****/
/***** Macros / Definitions *****/
#define POWER_LED	(PIO_PA18)
#define GRN_LED_TX	(PIO_PB11)
#define YLW_LED_RX	(PIO_PA19)

#define ON_RED_LED		(pio_clear(PIOA, POWER_LED))
#define OFF_RED_LED		(pio_set(PIOA, POWER_LED))
#define ON_YELLOW_LED	(pio_clear(PIOA, PIO_PA19))
#define OFF_YELLOW_LED	(pio_set(PIOA, PIO_PA19))
#define ON_GREEN_LED	(pio_clear(PIOB, PIO_PB11))
#define OFF_GREEN_LED	(pio_set(PIOB, PIO_PB11))

/***** Function Prototypes / Declarations *****/
void Configure_LED_PortPins(void);

#endif /* LED_OPERATION_H_ */
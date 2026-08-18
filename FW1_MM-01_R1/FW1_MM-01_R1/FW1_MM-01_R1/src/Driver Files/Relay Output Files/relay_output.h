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
#ifndef RELAY_OUTPUT_H_
#define RELAY_OUTPUT_H_

/***** General Definitions *****/
#define ON			(1)
#define OFF			(0)
#define RELAY1		(1)
#define RELAY2		(2)
#define RELAY3		(3)
#define RELAY4		(4)
#define ALL_RELAY	(0x0F)
/***** End of General Definitions *****/

/***** Function Declarations / Prototypes *****/
void Relay_Output_Driver_Init(void);
void Operate_Relay(U32 RELAY_NUMBER, U32 operation);

#endif /* RELAY_OUTPUT_H_ */

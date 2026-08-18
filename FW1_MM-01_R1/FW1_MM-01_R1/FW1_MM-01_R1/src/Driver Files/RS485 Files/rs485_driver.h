/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: rs485_driver.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 22/08/2023.
* Module
* Description	: Header file for rs485_driver.c
				  Defines constants and macros for rs485_driver.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 22/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef RS485_DRIVER_H_
#define RS485_DRIVER_H_

/***** Global Variables *****/
extern U8 gb_rs485_iChar_error_f;	// Flag sets when an inter char error occurred.

/***** Function Prototypes *****/
void RS485_Driver_Init(void);
void Send_data_On_RS485(U8 *bufdata, U8 buflen);
void RS485IntercharDelayError(void);
void RS485IntercharDelayLoad(void);
#endif /* RS485_DRIVER_H_ */
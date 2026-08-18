/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_uart.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 18/08/2023.
* Module
* Description	: Header file for user_uart.c
				  Defines constants and macros for user_uart.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef USER_UART_H_
#define USER_UART_H_

#include "asf.h"

#ifndef uSET
#define uSET	(1)
#endif

#ifndef uRESET
#define uRESET	(0)
#endif

#define	UART_POLLING_EN	(uSET)

extern volatile U8 gb_uart_ready_f;
extern volatile U8 gb_char_sent_f;
extern volatile U8 gb_uart_byte_rec_f;	// Flag used to to set when a single byte received.
extern volatile U8 gb_uart_rec_byte;	// variable used to get a byte data from uart.

/***** Declare prototypes for functions *****/
void UART_Debug_Init(void);
void UART_Debug_PutChar(uint8_t ch);
void Print_Message(const char *str);
void Print_Number(int data);
void Print_Number_Float(float data);
void Print_ASCII_HEX(int aValue);
void Display_HEX(U8 hValue);
void Send_Frame_On_UART(U8 *tFrame_data, U16 tFrame_len);
void Print_Data_On_Uart(U8 *buf, int len);
#endif /* USER_UART_H_ */
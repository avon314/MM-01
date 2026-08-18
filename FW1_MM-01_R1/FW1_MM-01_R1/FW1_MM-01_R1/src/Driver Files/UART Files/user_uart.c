/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_uart.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 18/08/2023.
* Module
* Description	:	Initialization of Debug UART, Definitions of respective functions
*					and setting up baud rate, parity, length of bits and interrupts.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"
#include "tgmath.h"
/***** User defined Includes *****/
#include "user_uart.h"

/***** Definations for Debug uart initializations *****/
#define	DEBUG_BAUDRATE				(115200)
#define	DEBUG_PARITY				(UART_MR_PAR_NO)
#define UTX_BUF_LEN					(100)
/***** Definations for debug uart initializations *****/

volatile U8 gb_uart_ready_f = uSET;	// Flag used to check UART is ready or not for transmission.
volatile U8 gb_char_sent_f = 0;		// Flag used to set and reset after a char is sent.
volatile U8 uart_transmit_buff[UTX_BUF_LEN] = {0};	// Right now 100 characters are loaded in a buffer/array.
volatile U8 uart_trans_buff_len = 0;	// Variable used to fill buffer length.
volatile U8 gb_uart_byte_rec_f = 0;	// Flag used to to set when a single byte received.
volatile U8 gb_uart_rec_byte = 0;	// variable used to get a byte data from uart.

extern void Get_TimeDate_By_UART(U8 rcvByte);

/*****************************************************************************
* Function name	: void UART_Debug_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/05/2022.
*
*
* Description	:	Initialization of UART for debug communication.
* 					Baud rate is set at 9600 speed. length is 8bit, 1 stop bit.
* 					parity is none.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void UART_Debug_Init(void)
{
	unsigned int lcl_error=0;
	pio_set_peripheral(PIOA, PIO_PERIPH_A, PIO_PA9A_URXD0 | PIO_PA10A_UTXD0);				//Allow UART to control PA9 and PA10
	sysclk_enable_peripheral_clock(ID_UART0);													//Enable UART0 Clock
	const sam_uart_opt_t uart0Settings = {sysclk_get_cpu_hz(), DEBUG_BAUDRATE , DEBUG_PARITY};	//UART0 Settings (configures desired baudrate and sets no parity)
	lcl_error = uart_init(UART0, &uart0Settings);												//Setup UART0 with configuration
	if (lcl_error)																				// If any error go inside
	{
		while(1);																				// error
	}
	else
	__NOP();
	
	uart_enable_tx(UART0);	// Enable UART0 for tx
	//uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	uart_enable_rx(UART0);	// Enable UART0 for rx
	uart_enable_interrupt(UART0, UART_IER_RXRDY);	// enable interrupt for Reception on one byte
	NVIC_EnableIRQ(UART0_IRQn);	// Enable interrupt for UART.
	NVIC_SetPriority(UART0_IRQn, 1);	// Set priority level, lower the value highest in the priority.
}

/*****************************************************************************
* Function name	: void UART_Debug_PutChar(uint8_t ch)
* Returns		: Nothing.
* Arguments    	: uint8_t ch--> receives a byte and send over UART.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/05/2022.
*
*
* Description	:	Send a byte of data over debug UART.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void UART_Debug_PutChar(uint8_t ch)
{
	#if UART_POLLING_EN
	/***** Polling Method Transmit *****/
	while (!(UART0->UART_SR & UART_SR_TXRDY));					// Wait until TX buffer is not empty
	uart_write(UART0, ch);
	#else
	/***** Interrupt Method Transmit *****/
	uart_write(UART0, ch);
	uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	gb_uart_ready_f = uRESET;
	#endif
}

/*****************************************************************************
* Function name	: void Print_Message(unsigned char *str)
* Returns		: Nothing.
* Arguments    	: unsigned char *str---> string pointer.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/05/2022.
*
* Description	:	Send strings over debug UART.
*               :
* Notes			: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Print_Message(const char *str)
{
	#if UART_POLLING_EN
	/***** Polling Method Transmit *****/
	while(*str)
	{
		while (!(UART0->UART_SR & UART_SR_TXRDY));
		UART_Debug_PutChar(*str);
		str++;
	}
	#else
	/***** Interrupt Method Transmit *****/
	U8 uidx = 0;
	while(*str)
	{
		uart_transmit_buff[uidx] = *str;
		str++;
		uidx++;
	}
	uart_trans_buff_len = uidx;
	uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	gb_uart_ready_f = uRESET;
	#endif
}

/*****************************************************************************
* Function name	: void Print_Number(int data1, char float_f)
* Returns		: Nothing.
* Arguments    	: int data ---> Put an integer data.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/05/2022.
*
*
* Description	:	Send ASCII Characters to UART.
*
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Print_Number(int data)
{
	char len = 0;
	int len_data = 0;
	int quotient = 0;

	len_data = data;

	if (data >= 10)
	{
		do
		{
			quotient = len_data/10;
			len++;
			len_data = quotient;
		} while (quotient >= 10);
	}
	else
	__NOP();
	len++;

	#if UART_POLLING_EN
	/***** Polling Method Transmit *****/
	while (len > 0)
	{
		double divisr = pow(10, --len);
		UART_Debug_PutChar((data/(int)divisr)%10+0x30);
	}
	#else
	/***** Interrupt Method Transmit *****/
	U8 uidx = 0;
	while (len > 0)
	{
		double divisr = pow(10, --len);
		uart_transmit_buff[uidx] = ((data/(int)divisr)%10+0x30);
		uidx++;
	}
	uart_trans_buff_len = uidx;
	uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	gb_uart_ready_f = uRESET;
	#endif
}
/*****************************************************************************
* Function name	: void Print_Number_Float(float data)
* Returns		: Nothing.
* Arguments    	: float data ---> Put float data to UART.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/05/2022.
*
*
* Description	:	Send Float data to UART.
*
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Print_Number_Float(float data)
{
	char len = 0;
	int data1 = 0;
	int len_data = 0;
	int quotient = 0;
	
	data1 = (int)(data * 100.0);
	len_data = data1;

	if (data1 >= 10)
	{
		do
		{
			quotient = len_data/10;
			len++;
			len_data = quotient;
		} while (quotient >= 10);
	}
	else
	__NOP();
	len++;

	#if UART_POLLING_EN
	/***** Polling Method Transmit *****/
	while (len > 0)
	{
		double divisr = pow(10, --len);
		UART_Debug_PutChar((data1/(int)divisr)%10+0x30);
		if (len == 2)
		UART_Debug_PutChar('.');
		else
		__NOP();
	}
	#else
	/***** Interrupt Method Transmit *****/
	U8 uidx = 0;
	while (len > 0)
	{
		double divisr = pow(10, --len);
		uart_transmit_buff[uidx] = ((data1/(int)divisr)%10+0x30);
		if (len == 2)
		uart_transmit_buff[++uidx] = 0x2E;	// Put a dot in debug window.
		else
		__NOP();
		uidx++;
	}
	uart_trans_buff_len = uidx;
	uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	gb_uart_ready_f = uRESET;
	#endif
}

/*****************************************************************************
* Function name	: void Print_ASCII_HEX(int aValue)
* Returns		: Nothing.
* Arguments    	: int aValue ---> Pass a Byte value to display in HEX.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
*
* Description	: Display 1 Byte HEX value on serial port when selected
*				  ASCII to display.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Print_ASCII_HEX(int aValue)
{
	U16 lcl_alpVal = 0;
	U16 lcl_numVal = 0;
	
	lcl_alpVal = (aValue / 16);
	lcl_numVal = (aValue % 16);
	
	if (aValue > 15)
	{
		Display_HEX(lcl_alpVal);
		Display_HEX(lcl_numVal);
	}
	else
	{
		Display_HEX(0);
		Display_HEX(lcl_numVal);
	}
}

/*****************************************************************************
* Function name	: void Display_HEX(U8 hValue)
* Returns		: Nothing.
* Arguments    	: U8 hValue ---> Pass a single digit to display in HEX.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
*
* Description	: Display HEX value on serial port when selected ASCII to display.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Display_HEX(U8 hValue)
{
	if (hValue == 10)
	Print_Message("A");
	else if (hValue == 11)
	Print_Message("B");
	else if (hValue == 12)
	Print_Message("C");
	else if (hValue == 13)
	Print_Message("D");
	else if (hValue == 14)
	Print_Message("E");
	else if (hValue == 15)
	Print_Message("F");
	else if (hValue < 10)
	Print_Number(hValue);
	else
	Print_Number(0);
}

/*****************************************************************************
* Function name	: void Send_Frame_On_UART(U8 *tFrame_data, U8 tFrame_len)
* Returns		: Nothing.
* Arguments    	: U8 *tFrame_data ---> Holds address of buffer.
				  U8 tFrame_len ---> pass buffer length to transmit data.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
*
* Description	:	Function to pass data over UART.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Send_Frame_On_UART(U8 *tFrame_data, U16 tFrame_len)
{
	#if UART_POLLING_EN
	/***** Polling Method Transmit *****/
	while(tFrame_len--)
	{
		while (!(UART0->UART_SR & UART_SR_TXRDY));
		UART_Debug_PutChar(*tFrame_data);
		tFrame_data++;
	}
	#else
	/***** Interrupt Method Transmit *****/
	U8 uidx = 0;
	while (tFrame_len--)
	{
		uart_transmit_buff[uidx] = *tFrame_data;
		tFrame_data ++;
		uidx ++;
	}
	uart_trans_buff_len = uidx;
	uart_enable_interrupt(UART0, UART_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
	gb_uart_ready_f = uRESET;
	#endif
}

/*****************************************************************************
* Function name	: void UART0_Handler(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
*
* Description	:	Debug UART Handler when data receive on UART,
*					it reads a byte of data.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void UART0_Handler(void)
{
	U8 Local_data = 0;
	static U8 tidx = 0;
	
	if ((uart_get_status(UART0) & UART_SR_TXRDY) == UART_SR_TXRDY)	// If transmitter is ready?
	{
		if (uart_trans_buff_len > 0)
		{
			uart_write(UART0, uart_transmit_buff[tidx]);
			uart_trans_buff_len --;
			tidx ++;
		}
		else
		{
			__NOP();
		}
	}
	else
	{
		__NOP();
	}
	
	if ((uart_get_status(UART0) & UART_SR_TXEMPTY) == UART_SR_TXEMPTY)
	{
		tidx = 0;
		gb_uart_ready_f = uSET;
		uart_disable_interrupt(UART0, UART_IDR_TXRDY);
	}
	else
	{
		__NOP();
	}
	
	if ((uart_get_status(UART0) & UART_SR_RXRDY) == UART_SR_RXRDY)	// If data received.
	{
		uart_read(UART0, &Local_data);	// Read a byte.
		gb_uart_byte_rec_f = uSET;	// Set this flag once received a single byte.
		gb_uart_rec_byte = Local_data;
		Get_TimeDate_By_UART(Local_data);
	}
	else
	{
		__NOP();
	}
}


/*****************************************************************************
* Function name	: void Print_Data_On_Uart(U8 *buf, int len)
* Returns		: Nothing.
* Arguments    	: U8 * ->buffer, int->  length.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
*
* Description	:	Debug UART Handler when data receive on UART,
*					it reads a byte of data.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Print_Data_On_Uart(U8 *buf, int len)
{
	int pIdx = 0;
	while (pIdx < len)
	{
		Print_Number(buf[pIdx ++]);
		if (pIdx != len)
		Print_Message(",");
		else
		__NOP();
	}
}
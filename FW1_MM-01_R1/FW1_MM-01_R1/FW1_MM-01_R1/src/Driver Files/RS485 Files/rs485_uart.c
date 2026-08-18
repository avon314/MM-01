/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: rs485_uart.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 19/08/2023.
* Updated by    : Shirisha Ankam
* Updated Date  : 25/12/25
* Description	:	Initialization of RS485 UART, Definitions of respective functions
*					and setting up baud rate, parity, length of bits and interrupts.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 19/08/2023.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"

/***** User defined Includes *****/
#include "rs485_uart.h"
#include "definitions.h"
#include "Modbus.h"
#include "modbus_app.h"
#include "user_uart.h"
#include <string.h>

uint32_t RS485_PARITY_VALUE;
uint32_t RS485_STOPBIT_VALUE;

/***** Definations for RS485 uart initializations *****/
#define	RS485_BAUDRATE				(ms_info.boudrate)//9600//
#define	RS485_CHAR_LEN				(US_MR_CHRL_8_BIT)
#define	RS485_PARITY				(RS485_PARITY_VALUE)//US_MR_PAR_NO//
#define	RS485_STOP_BIT				(RS485_STOPBIT_VALUE)//US_MR_NBSTOP_1_BIT//
/***** Definations for RS485 uart initializations *****/
#define USTX_BUF_LEN				(100)	/*Bytes*/


volatile U8 usart_trans_buff_len = 0;	// Variable used to fill buffer length.
volatile U8 usart_transmit_buff[USTX_BUF_LEN] = {0};	// Right now 100 characters are loaded in a buffer/array.
	
volatile U8 gb_usart_ready_f = uSET;	// Flag used to check USART is ready or not for transmission.
volatile U8 gb_usart_byte_rec_f = 0;	// Flag used to to set when a single byte received.
volatile U8 gb_usart_rec_byte = 0;		// variable used to get a byte data from usart.

/***** Extern / Global Function declaration *****/
extern void RS485IntercharDelayLoad(void);


/*****************************************************************************
* Function name	: void RS485_UART_Init(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
* Updated by    : Shirisha Ankam
* Updated Date  : 25/12/25

* Description	:	Initialization of UART for RS485 communication.
* 					Baud rate is set at 9600 speed. length is 8bit, 1 stop bit.
* 					parity is none.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void RS485_UART_Init(void)
{	
	Set_RS485_Parity_Stop(ms_info.parity_bit, ms_info.stop_bit);
	
	unsigned int lcl_error=0;
	pio_set_peripheral(PIOB, PIO_PERIPH_C, PIO_PB0C_RXD0 | PIO_PB1C_TXD0);						//Allow USART0 to control PB0 and PB1
	sysclk_enable_peripheral_clock(ID_USART0);													//Enable USART0 Clock
	const sam_usart_opt_t usart1Settings =
	{
		RS485_BAUDRATE,
		RS485_CHAR_LEN,
		RS485_PARITY,
		RS485_STOP_BIT
	};	//USART1 Settings (configures desired baudrate and sets no parity)
	lcl_error = usart_init_rs232(USART0, &usart1Settings, sysclk_get_peripheral_hz());			// Setup USART0 with configuration
	if (lcl_error)																				// If any error go inside
	{
		while(1);																				// error
	}
	else
	__NOP();
		
 	usart_enable_tx(USART0);																	// Enable USART0 for tx
 	//usart_enable_interrupt(USART0, US_IER_TXRDY);	// enable interrupt when 1 byte is filled in TX buffer
 	usart_enable_rx(USART0);	// Enable USART0 for rx
 	usart_enable_interrupt(USART0, US_IER_RXRDY);												// enable interrupt for Reception on one byte
	NVIC_SetPriority(USART0_IRQn, 2);															// Set priority level, lower the value highest in the priority.
	NVIC_EnableIRQ(USART0_IRQn);																// Enable interrupt for USART0.
}

/*****************************************************************************
* Function name	: void Put_Data_On_RS485_UART(U8 *tdata, U8 tbuflen)
* Returns		: Nothing.
* Arguments    	: U8 *tdata ---> Holds address of buffer.
				  U8 tbuflen ---> pass buffer length to transmit data.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 19/08/2023.
* Updated by    : Shirisha Ankam
* Updated Date  : 25/12/25
* Description	:	Function to pass data over UART.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Put_Data_On_RS485_UART(U8 *tdata, U8 tbuflen)
{
	/***** Interrupt Method Transmit *****/
	U8 uidx = 0;
	while (tbuflen--)
	{
		usart_transmit_buff[uidx] = *tdata;
		tdata ++;
		uidx ++;
	}
	usart_trans_buff_len = uidx;
	
	gb_usart_ready_f = uRESET;
	
	usart_enable_interrupt(USART0, US_IER_TXRDY);	// Enable interrupt when 1 byte is filled in TX buffer
	
}

/*****************************************************************************
* Function name	: void USART0_Handler(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 18/08/2023.
* Updated by    : Shirisha Ankam
* Updated Date  : 25/12/25
* Description	: RS485 USART Handler when data receive on USART,
*				  it reads a byte of data.
*
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void USART0_Handler(void)
{
	U32 Local_data = 0;                                                     // Local variable to store received data
	static U8 tidx = 0;                                                     // Static transmit buffer index, persists across interrupts
	
	if ((usart_get_status(USART0) & US_CSR_TXRDY) == US_CSR_TXRDY)	        // If transmitter is ready?
	{
		if (usart_trans_buff_len > 0)                                       // Check if there is data remaining to transmit
		{
			usart_write(USART0, usart_transmit_buff[tidx]);                 // Write next byte from transmit buffer
			usart_trans_buff_len --;                                        // Decrement remaining bytes count
			tidx ++;                                                        // Move to next byte in buffer
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
	
	if ((usart_get_status(USART0) & US_CSR_TXEMPTY) == US_CSR_TXEMPTY)     // Check if transmit buffer is completely empty
	{
		tidx = 0;                                                           // Reset transmit index for next transmission
		gb_usart_ready_f = uSET;                                            // Set flag indicating USART is ready for next operation
		usart_disable_interrupt(USART0, US_IDR_TXRDY /*| US_IDR_TXEMPTY*/); // Disable TX interrupts as transmission is complete
	}
	else
	{
		__NOP();
	}
	
	if (usart_get_status(USART0) & ( US_CSR_PARE | US_CSR_FRAME | US_CSR_OVRE ))	// If data received.
	{
		usart_read(USART0, &Local_data);	                                // Read a byte to clear error flags
		Modbus_Rx_Reset();                                                  // Reset Modbus receive state machine due to error
		return;                                                             // Exit handler early to discard corrupted data
	}
	else
	{
		__NOP();
	}
	
	if ((usart_get_status(USART0) & US_CSR_RXRDY) == US_CSR_RXRDY)	        // If data received.
	{
		usart_read(USART0, &Local_data);									/* Read a byte. */
		gb_usart_byte_rec_f = uSET;											/* Set this flag once received a single byte. */
		gb_usart_rec_byte = Local_data;										/* Copy a byte to a global variable. */
		//RS485IntercharDelayLoad();										/* Fill inter character delay, after a byte received. */
		Get_Modbus_UART_Data(Local_data);                                   // Process received byte for Modbus protocol
	}
	else
	{
		__NOP();
	}
}

/*****************************************************************************
* Function name	: Modbus_Rx_Reset
* Returns		: Nothing.
* Arguments    	: None.
* Created by	: Shirisha Ankam.
* Date created	: 2/01/2026.
*
* Description	: Resets Modbus receive buffers and flags when communication
*                 error occurs.
*
* Notes			: Called on parity, frame, or overrun errors.
* Global Variables Affected : gb_rcvd_flag, gb_flag_inter_char_error, idex,
*                              gb_rcv_str, gb_par_frame_err
*****************************************************************************/
void Modbus_Rx_Reset(void)
{
	gb_rcvd_flag = 0;                           // Clear message received flag
	gb_flag_inter_char_error = 0;               // Clear inter-character timeout error flag
	idex = 0;                                   // Reset receive buffer index to start
	memset(gb_rcv_str, 0, sizeof(gb_rcv_str));  // Clear entire receive buffer
	gb_par_frame_err = 1;                       // Set parity/frame error flag to indicate error state
}



/*****************************************************************************
* Function name	: Set_RS485_Parity_Stop
* Returns		: Nothing.
* Arguments    	: recv_parity - Parity setting (0=None, 1=Odd, 2=Even)
*                 recv_stop - Stop bit setting (0=1 bit, 1=2 bits)
* Created by	: Shirisha Ankam.
* Date created	: 2/01/2026.
*
* Description	: Configures RS485 parity and stop bit settings based on
*                 received parameters.
*
* Notes			: Default settings are No Parity and 1 Stop Bit.
* Global Variables Affected : RS485_PARITY_VALUE, RS485_STOPBIT_VALUE
*****************************************************************************/
void Set_RS485_Parity_Stop(uint8_t recv_parity, uint8_t recv_stop)
{
	/* ----- Parity ----- */
	switch(recv_parity)
	{
		case 0:                                         // No parity
		RS485_PARITY_VALUE = US_MR_PAR_NO;
		break;
		case 1:                                         // Odd parity
		RS485_PARITY_VALUE = US_MR_PAR_ODD;
		break;
		case 2:                                         // Even parity
		RS485_PARITY_VALUE = US_MR_PAR_EVEN;
		break;
		default:                                        // Default to no parity for invalid values
		RS485_PARITY_VALUE = US_MR_PAR_NO;
		break;
	}
	/* ----- Stop Bits ----- */
	switch(recv_stop)
	{
		case 0:                                         // 1 stop bit
		RS485_STOPBIT_VALUE = US_MR_NBSTOP_1_BIT;
		break;
		case 1:                                         // 2 stop bits
		RS485_STOPBIT_VALUE = US_MR_NBSTOP_2_BIT;
		break;
		default:                                        // Default to 1 stop bit for invalid values
		RS485_STOPBIT_VALUE = US_MR_NBSTOP_1_BIT;
		break;
	}
}

/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: Modbus.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 25/05/2022.
* IDE			: STM32CubeIDE (Version- 1.8.0).
* Compiler		: GCC C,C++(Advanced)
*
* Module Description :	Send frame, receive frame and modbus frame are defined.
* 						baud rate , parity  and receive frame delay are defined here.
*
*
* Controller	: 	ATSAM4S2B
*					512KB		Flash.
*					128KB		RAM.
*
* REVISION HISTORY
* Version : 1.0
* Revision Date: 25/05/2022.
* Changes : NA.
*
*****************************************************************************/
#include "asf.h"
#include "Modbus.h"
#include "user_uart.h"
#include "rs485_driver.h"
#include "rs485_uart.h"
#include "CRC.h"
#include "modbus_app.h"
#include "mm_app.h"
#include "led_operation.h"
#include <string.h>

#define DEBUG_UART		0
#define NO_OF_RETRIES	3
#define PIN_EN			1
#define PIN_DIS			0
#define DEBUG_TEST      1

volatile uint8_t gb_correct_frame_rcvd = 0;          //Var used for correct response of the frame used in application for further processing
volatile uint8_t gb_baud_rate_sel = 4;

volatile uint32_t gb_baud_value = 0;					//Var used to fill value as per ETS configuration and write to uart peripheral in main file
volatile uint32_t gb_parity_value = 0;					//Var used to fill value as per ETS configuration and write to uart peripheral in main file

volatile uint8_t gb_inter_char_timeout1 = 0;
volatile uint16_t gb_frame_response_timeout1 = 0;
volatile uint8_t gb_send_frame;						//Var used in Modbus sending frame as a flag

unsigned char gb_tx_frame[50];                      	//Var used for sending frame on UART
volatile uint8_t gb_rcv_str[MAX_BUF_SIZE];		// Declare data receive array.

volatile uint8_t count;									//Var used to maintain count for CRC function
volatile uint8_t gb_retry_count=0;                      //Var used as retry counter
volatile uint32_t gb_response_time;						//Var used as response time
volatile uint16_t gb_retry_timeout = 0;                 //Var used for timeout for sending retrial frame
volatile uint32_t gb_word_length = 0;					//Word Length for parity(If even or odd parity selected then 9 word length or for none 8 word length)

/* 1200, 2400, 4800, 9600, 19200, 38400, 5600, 115200 */
volatile uint8_t gb_inter_char_timer[8] = {250, 125, 63, 32, 16, 8, 6, 3};
volatile uint16_t gb_timer_frame[8] = {584, 300, 146, 73, 36, 18, 13, 6};
volatile uint16_t frame_error_f=0;

uint8_t gb_readReg_val[MAX_BUF_SIZE]={0};
uint8_t gb_exit_loop_f=0;
extern uint8_t rcv_val;


volatile uint8_t gb_rcvd_flag=0;  					//flag set if data not received in 3.5char time i.e EOF received
volatile uint8_t gb_flag_inter_char_error =0;   	//Var used for error
uint16_t blink=0;
volatile uint32_t gb_timer_msec;					//Var used for delay
volatile uint8_t gb_par_frame_err = 0;

unsigned char data_rec[2];

uint8_t rcv_val=0;
volatile uint8_t flag = 0;
volatile uint8_t data_rcvd = 0;
volatile uint8_t gb_inter_char = 0;                 //Var used for inter frame timeout
volatile uint8_t idex = 0;                               //var used for array index

volatile uint8_t ridx=0, rcv_f=0, rcv_arr[]={0};

volatile uint16_t gb_frame_response_timeout = 0;    //Var used for frame response timeout
volatile unsigned int gb_store_val=0;				//Var used to receive character in uart interrupt
volatile int gb_byte_count = 0;                      //For number of bytes received

volatile uint8_t gb_parity_sel = 1;                  //Var used for frame not received correctly
volatile uint16_t gb_timer_cyclic=0;

typedef union
{
	unsigned char quantity_register[2];					//union for quantity of register
	uint16_t register_quantity;
}data_quantity;

typedef union
{                                         //union for register address
	unsigned char Reg_Address[2];
	uint16_t register_addr;
}data1;

data1 reg_addr;
data_quantity number_of_register;

/*****************************************************************************
* Function name	:  modbus_init()
* Returns	    :  Nothing
* Arguments    	:  Nothing
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to initialize variables set baud rate,select parity and load value for inter char and frame response according to baud rate
*               :
* Notes		: Additional details
* Global Variables Affected:gb_baud_rate_sel,gb_inter_char_timeout1,gb_frame_response_timeout1
*****************************************************************************/
void Modbus_init()
{
	Print_Message("\nModbus Driver Initialization");
	/***************** Standard MODBUS(Driver) File Initialization. **********************/
	//for (count = 0; count < 50; count++)
	//{
		//gb_tx_frame[count] = 0x00;	//	Clear transmission buffer
		//gb_rcv_str[count] = 0x00;	// Clear reception buffer
	//}
//
	//Modbus_Baud_Rate(gb_baud_rate_sel);			// Function to select baud rate, baud rate selected to 9600
	//Set_InterChar_Timeout(gb_baud_rate_sel);	// Function where 1.5char and 3.5 char timeout
	//Parity_Selection(PARITY_NONE);				// Select no parity rt now.
	//gb_retry_timeout = 2000;                    // read parameter from KNX as of now value is set to 1 sec
//
    ///* clear variables used for sending frame, retry count and response time */
    //gb_send_frame = 0;
    //gb_retry_count = 0;
    //gb_response_time = 0;
/***************** End of Standard MODBUS(Driver) File Initialization. **********************/

/***************** Application based MODBUS Initialization. **********************/
    for (count = 0; count < 50; count++)
    {

	    gb_tx_frame[count] = 0x00;		// Clear transmission buffer
	    gb_rcv_str[count] = 0x00;		// Clear reception buffer
    }
	
	gb_baud_rate_sel = Get_Baud_Index(ms_info.boudrate);
	gb_baud_value = ms_info.boudrate;
    // Modbus_Baud_Rate(gb_baud_rate_sel);		// Pass baudrate selected in ETS.
    Set_InterChar_Timeout(gb_baud_rate_sel);	// Function where 1.5char and 3.5 char timeout, based on baudrate.
	
	//Parity_Selection(PARITY_NONE);				// Select no parity rt now.
	//gb_parity_value = ms_info.parity_bit;
	
    #if DEBUG_TEST
    Print_Message("\n\nModbus baudrate is\t\t\t: ");
    Print_Number(gb_baud_value);
    Print_Message("\ngb_inter_char_timeout1 is\t\t: ");
    Print_Number(gb_inter_char_timeout1);
    Print_Message("\ngb_frame_response_timeout1 is\t: ");
    Print_Number(gb_frame_response_timeout1);
    Print_Message("\ngb_parity_sel is\t\t\t: ");
    Print_Number(gb_parity_sel);
    #endif

    gb_retry_timeout =  100;	// read parameter from KNX.

    /* clear variables used for sending frame,retry count and response time */
    gb_send_frame = 0;
    gb_retry_count = 0;
    gb_response_time = 0;
	gb_byte_count = 0;

    RS485_UART_Init();	// Enable UART0 for RS485 communication.
	/***************** End of Application based MODBUS Initialization. ********************/
}


/*****************************************************************************
* Function name	:  void Set_Timeout(void)
* Returns	    :  Nothing
* Arguments    	:  Nothing
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to load 1.5char and 3.5char timeout in variables
*               :
* Notes		: This variables are further copied when character is received(In timer ISR) and decremented in timer of 50us
* Global Variables Affected:gb_inter_char_timeout1,gb_frame_response_timeout1
*****************************************************************************/
void Set_InterChar_Timeout(uint8_t gb_baud_rate_sel)
{
	#if DEBUG_UART
	//HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
	//HAL_Delay(500);
	#endif
	
	if (gb_baud_rate_sel == 1)	// Selecting baudrate as per the configuration read from ETS
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 1200");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[0];
		gb_frame_response_timeout1 = gb_timer_frame[0];
	}
	else if (gb_baud_rate_sel == 2)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 2400");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[1];
		gb_frame_response_timeout1 = gb_timer_frame[1];
	}
	else if (gb_baud_rate_sel == 3)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 4800");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[2];
		gb_frame_response_timeout1 = gb_timer_frame[2];
	}
	else if (gb_baud_rate_sel == 4)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 9600");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[3];
		gb_frame_response_timeout1 = gb_timer_frame[3];
	}
	else if (gb_baud_rate_sel == 5)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 19200");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[4];
		gb_frame_response_timeout1 = gb_timer_frame[4];
	}
	else if (gb_baud_rate_sel == 6)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 38400");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[5];
		gb_frame_response_timeout1 = gb_timer_frame[5];
	}
	else if (gb_baud_rate_sel == 7)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 56000");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[6];
		gb_frame_response_timeout1 = gb_timer_frame[6];
	}
	else if (gb_baud_rate_sel == 8)
	{
		#if DEBUG_UART
		Print_Message("\nModbus Baud rate 115200");
		#endif
		gb_inter_char_timeout1 = gb_inter_char_timer[7];
		gb_frame_response_timeout1 = gb_timer_frame[7];
	}
}


/*****************************************************************************
* Function name	:  void Parity_Selection(int selec)
* Returns	    :  Nothing
* Arguments    	:  int as selec
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to select parity for communication
*               :
* Notes		: Additional details
* Global Variables Affected:gb_parity_value
*****************************************************************************/
void Parity_Selection(int selec)
{
	switch (selec)
	{
		case 1:
			gb_parity_value = PARITY_NONE;				//Value of none parity filled into variable
			break;
		case 2:
			gb_parity_value = PARITY_EVEN;				//Value of even parity filled into variable
			break;
		case 3:
			gb_parity_value = PARITY_ODD;				//Value of odd parity filled into variable
			break;
		default:
			break;
	}
}

/*****************************************************************************
* Function name	:  void Modbus_Baud_Rate(uint8_t baud_val
* Returns	    :  Nothing
* Arguments    	:  unsigned int as baud rate
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to select baud rate for communication
*               :
* Notes		: 	Additional details
* Global Variables Affected:gb_baud_value
*****************************************************************************/
void Modbus_Baud_Rate(uint8_t baud_val)
{
	switch (baud_val)
	{
		case 1:
			gb_baud_value = 1200;
			break;
		case 2:
			gb_baud_value = 2400;
			break;
		case 3:
			gb_baud_value = 4800;
			break;
		case 4:
			gb_baud_value = 9600;
			break;
		case 5:
			gb_baud_value = 19200;
			break;
		case 6:
			gb_baud_value = 38400;
			break;
		case 7:
			gb_baud_value = 56000;
			break;
		case 8:
			gb_baud_value = 115200;
			break;
		default:
			gb_baud_value = 9600;
			break;
	}
}

const uint32_t modbus_baud_list[8] =
{
	1200, 2400, 4800, 9600, 19200, 38400, 56000, 115200
};

uint8_t Get_Baud_Index(uint32_t baud)
{
	for (uint8_t i = 0; i < 8; i++)
	{
		if (modbus_baud_list[i] == baud)
		return i + 1;
	}
	return 4;   // default to 9600 baud if not matched
}


/*****************************************************************************
* Function name	:  modbus_frame(uint8_t slave_address,uint8_t function_code,uint16_t starting_address,uint16_t number_of_reg,uint8_t byte_count,uint8_t *data_buff)
* Returns	    :  Nothing
* Arguments    	:  uint8_t as slave address
                   uint8_t as function code
                   uint16_t as starting address of register/coil
                   uint16_t as number of register/coil
                   uint8_t as byte count (this argument is taken into consideration for write multiple coils and write multiple registers
                   uint8_t as pointer for data to be sent for multiple write
* Created by	: Gaurav Kulkarni
* Date created	: 23/01/2020
* Description	: Function to buid frame according to function code and other arguments,calculate CRC and set flag for sending data on UART.
*               :
* Notes		: Additional details
* Global Variables Affected:gb_tx_frame[]
*****************************************************************************/
/* in case of multiple write register byte count should be in num of reg * 2 format */
/* in case of write multiple coil byte count is calculated in form of number of coils to be write and number of coils/8 */
void modbus_frame(uint8_t slave_address, uint8_t function_code, uint16_t starting_address,
					uint16_t number_of_reg, uint8_t byte_count, uint8_t *data_buff)
{
    switch (function_code)
	{
        case 0x01:
                /* Read coil status */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;                               //copy slave address
                gb_tx_frame[count++] = function_code;                               //copy function code
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];                     //copy starting address higher byte
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];                     //copy starting address lower byte
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];     //copy quantity of register higher byte
                gb_tx_frame[count++] = number_of_register.quantity_register[0];     //copy quantity of register lower byte
                break;
        case 0x02:
                /* Read input status */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];
                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                break;
        case 0x03:
                /* Read holding register */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];
                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                break;
        case 0x04:
                /* Read input register */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];
                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                break;
        case 0x05:
		        /*Print_Message("\nWrite single coils");*/
                /* Write single coil */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
//                number_of_register.register_quantity = number_of_reg;
//                gb_tx_frame[count++] = number_of_register.quantity_register[1];
//                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                for(int i = 0; i < 2; i++){                                //According to byte count, data passed to buffer is copied to tx buffer and send on UART
                    gb_tx_frame[count++] = data_buff[i];
                }
                break;
        case 0x06:
				/*Print_Message("\nWrite single register ");*/
                /* Write single register  */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
//                number_of_register.register_quantity = number_of_reg;
//                gb_tx_frame[count++] = number_of_register.quantity_register[1];
//                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                for(int i = 0; i < 2; i++){                                //According to byte count, data passed to buffer is copied to tx buffer and send on UART
                    gb_tx_frame[count++] = data_buff[i];
                }
                break;
        case 0x10:
                /* write multiple register */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];
                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                gb_tx_frame[count++] = number_of_register.register_quantity * 2;
                for(int i = 0; i < byte_count; i++){                                //According to byte count, data passed to buffer is copied to tx buffer and send on UART
                    gb_tx_frame[count++] = data_buff[i];
                }
                break;
        case 0x0F:
	        	/*Print_Message("\nWrite multiple coils");*/
                /* write multiple coils */
                count = 0;                                                          //clear count var used for index
                gb_tx_frame[count++] = slave_address;
                gb_tx_frame[count++] = function_code;
                reg_addr.register_addr = starting_address;
                gb_tx_frame[count++] = reg_addr.Reg_Address[1];
                gb_tx_frame[count++] = reg_addr.Reg_Address[0];
                number_of_register.register_quantity = number_of_reg;
                gb_tx_frame[count++] = number_of_register.quantity_register[1];
                gb_tx_frame[count++] = number_of_register.quantity_register[0];
                if(number_of_register.register_quantity % 8 != 0){
                    gb_tx_frame[count++] = (number_of_register.register_quantity / 8 + 1);
                    byte_count = (number_of_register.register_quantity / 8 + 1);                        //byte count var modified as for multiple coil depencing upon quantity of coils data should be filled
                }else{
                    gb_tx_frame[count++] = (number_of_register.register_quantity / 8);
                    byte_count = (number_of_register.register_quantity / 8);     //byte count var modified as for multiple coil depencing upon quantity of coils data should be filled
                }

                for(int i = 0; i < byte_count; i++){                                //According to byte count, data passed to buffer is copied to tx buffer and send on UART
                    gb_tx_frame[count++] = data_buff[i];
                }
                break;

    }

    CRC_Calculate_Check(gb_tx_frame,count,CRC_CALC);                        //calculate CRC

    gb_tx_frame[count++] = gb_Temp_CRC[0];                                  //copy CRC lower byte
    gb_tx_frame[count++] = gb_Temp_CRC[1];                                  //copy CRC higher byte
	

    gb_send_frame = 1;														/* set flag to send frame */
    gb_retry_count = 0;	// count made 0 in this function as new frame is prepared only once after successful data reception or data to transmit is ready
}


/*****************************************************************************
* Function name	:  void send_frame(void)
* Returns	    :  Nothing
* Arguments    	:  Nothing
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to send frame on uart and load response timer,increment retry count
*               :
* Notes		: Additional details
* Global Variables Affected:gb_send_frame,gb_retry_count,gb_response_time
*****************************************************************************/
void send_frame(void)
{	
    if (gb_send_frame == 1 && gb_retry_count <= NO_OF_RETRIES && gb_usart_byte_rec_f == 0)//&& gb_response_time == 0)
    {			
		Send_data_On_RS485(gb_tx_frame, count);
		
		gb_send_frame = 0;                      			//clear var used for sending frame
		
		#if 0
		if (gb_retry_count == 0)
		{
			Print_Message("\nSending frame to : ");
			Print_Number(gb_tx_frame[0]);
		}
		else
		{
			Print_Message("\nRetrials ");
			Print_Number(gb_retry_count);
		}
		
		
		Print_Message("\n\rSend modbus frame to RS485 : ");
		for(U8 i = 0; i < count; i++)
		{
			Print_Number(gb_tx_frame[i]);
			Print_Message(",");
		}
		#endif	
		
		gb_response_time = gb_retry_timeout;    			//Value of response time loaded from the configuration performed in ETS
		gb_retry_count++;									//Retry count incremented
		
		// *** STOP POLLING UNTIL retry ends ***
		//gb_send_mb_data = 0;
    }
   else if ((gb_send_frame == 1 ) && (gb_retry_count > NO_OF_RETRIES))	// Check if retry count greater then equal to 4
   {
	    //Print_Message("\nAll retries failed.");
		
	   	gb_retry_count = 0;									//Reset count and timer variable
	   	gb_response_time = 0;
	   	gb_send_frame = 0;
	   	// *** RESUME POLLING ***	
		if (modbus_op_state == MB_OP_WRITE)
		{
			// WRITE failed ? resume polling safely
			Print_Message("\nWrite failed, resuming DPT poll");

			mb_poll_dpt = 1;
			gb_mb_write_processing_f = 0;            
			modbus_op_state = MB_OP_DPT_READ; 
			gb_send_mb_data = 1;
			return;
		}
		   			   
		if (gb_start_sending_dpt == 1)
		{
			if(gb_par_frame_err == 0)
			{
				//Print_Message("\nSlave not responding after configuration");
				
				if(active_slaves[gb_tx_frame[0] - 1] == 1)
				{
					active_slaves[gb_tx_frame[0] - 1] = 0;
					gb_send_cs_to_server = 1;
				}
				else
				{
					__NOP();
				}
			}
			else
			{
				Print_Message("\nParity Frame Error");
				gb_par_frame_err = 0;
				active_slaves[gb_tx_frame[0] - 1] = 1;
			}

		}
		else
		{
			go_to_next_slave();
			gb_exit_loop_f = 1;
		}
	}
	else
	{
		__NOP();
	}
}


/*****************************************************************************
* Function name	:  void frame_rcvd(void)
* Returns	    :  Nothing
* Arguments    	:  Nothing
* Created by	: Gaurav Kulkarni
* Date created	: 10/07/2021
* Description	: Function to calculate CRC and check flag set after EOF reception and further frame is available for furthe processing
*               :
* Notes		: Additional details
* Global Variables Affected:gb_rcvd_flag,gb_response_time,gb_retry_count
*****************************************************************************/
int frame_rcvd(void)
{
	//UART_Debug_PutString("\ngb_rcvd_flag is : ");
	//UART_Debug_ASCII(gb_rcvd_flag);
	//UART_Debug_PutString("\ngb_flag_inter_char_error is : ");
	//UART_Debug_ASCII(gb_flag_inter_char_error);
    if (gb_rcvd_flag == 1 && gb_flag_inter_char_error == 1)	// check receive flag is set when EOF is received and 1.5 char is achieved
    {
        int ret;
        uint8_t lc1_rcv_data[50];
// 		#if DEBUG_UART 
//         UART_Debug_PutChar(gb_rcv_str[2]);
//         UART_Debug_PutChar(gb_rcv_str[3]);
//         UART_Debug_PutChar(gb_byte_count/100%10+0x30);
//         UART_Debug_PutChar(gb_byte_count/10%10+0x30);
//         UART_Debug_PutChar(gb_byte_count/1%10+0x30);
//         Print_Message("\n\rIn this function.");
//         #endif

		#if INTERFACE_485
		//HAL_GPIO_WritePin(GPIOE, LED2_Pin, RESET);
		#endif
		
		Print_Message("\n\rResponse received from slave: ");
		for (int count1 =0; count1 < gb_byte_count; count1++)
		{
			//UART_Debug_PutChar(gb_rcv_str[count1]);
			Print_ASCII_HEX(gb_rcv_str[count1]);
			UART_Debug_PutChar(' ');
		}
		
        ret = CRC_Calculate_Check((uint8_t *)gb_rcv_str, gb_byte_count-2, CRC_CHECK);     //Calculate CRC and check if CRC is matched with the received

        if (ret == 1)
        {
            /* if CRC is correct then check function code and if function code is of error
                then take appropriate action */
            if ((gb_rcv_str[0] & (1 << 7)))
            {
                #if DEBUG_UART || 1
            	Print_Message("\n\rError Function code");
                #endif

                /* Depending upon which type of error ,return can be done here */
                gb_byte_count = 0;                                                          			//Var cleared used to count received byte for crc calculation
                /*** Error is occurred and so retrial is not to be performed again ,Need to check*/
//                gb_response_time = 0;                                                       			//clear timer value loaded
//                gb_send_frame = 0;                                                          			//clear flag to send frame
//                gb_retry_count = 0;                                                         			//Clear retry count as if EOF is received retry count is reseted to 0
                if (gb_rcv_str[1] == 0x01)
                {
                    /***Function not supported ****/
                    gb_rcvd_flag = 0;                                                                   //clear receive flag
                    gb_flag_inter_char_error = 0;                                                        //clear flag set in 1.
                    return -1;
                }
                else if (gb_rcv_str[1] == 0x02)
                {
                    /**** Starting address not OK and starting address + quantity of outputs not OK ****/
                    gb_rcvd_flag = 0;                                                                   //clear receive flag
                    gb_flag_inter_char_error = 0;                                                        //clear flag set in 1.
                    return -2;
                }
                else if (gb_rcv_str[1] == 0x03)
                {
                    /***Quantity of output is not in limit****/
                    gb_rcvd_flag = 0;                                                                   //clear receive flag
                    gb_flag_inter_char_error = 0;
                    return -3;
                }
                else if (gb_rcv_str[1] == 0x04)
                {
                    /*** Read Discrete output not OK ***/
                    gb_rcvd_flag = 0;                                                                   //clear receive flag
                    gb_flag_inter_char_error = 0;
                    return -4;
                }
            }
            else if ((gb_rcv_str[0] & (1 << 7)) == 0)
            {
                #if DEBUG_UART
            	Print_Message("\n\rCorrect response received:\n");
                for (int count1 =0; count1 < gb_byte_count; count1++)
                {
                	UART_Debug_PutChar(gb_rcv_str[count1]/100%10+0x30);
                	UART_Debug_PutChar(gb_rcv_str[count1]/10%10+0x30);
                	UART_Debug_PutChar(gb_rcv_str[count1]/1%10+0x30);
                	UART_Debug_PutChar(',');
                }               
                #endif
				 gb_byte_count = 0;                                                          //var cleared used to count received byte for crc calculation
                /* flag to be set for further processing in application*/
                gb_correct_frame_rcvd = 1;
                /* --Included for testing
                 *
                 */
				memset(gb_modbus_data,0,4);
				memcpy(gb_modbus_data,&gb_rcv_str[3],4);
				
                if (gb_rcv_str[1] == 0x04)	// Check if read input register as function code
                {
                	for (int count = 3; count < gb_rcv_str[2]+3; count++)	// Loop for copying data to local variable
                	{
                		lc1_rcv_data[count-3] = gb_rcv_str[count];
                	}
                }
                gb_response_time = 0;                                                       //clear timer value loaded
                gb_send_frame = 0;                                                          //clear flag to send frame
                gb_retry_count = 0;
				if((active_slaves[gb_rcv_str[0] - 1] == 0) && (gb_start_sending_dpt == 1))
				{
					active_slaves[gb_rcv_str[0] - 1] = 1;
					gb_send_cs_to_server = 1;
				}
            }
        }
        else
        {
			#if DEBUG_UART 
        	Print_Message("\n");
			for (int count1 =0; count1 < gb_byte_count; count1++)
			{
				//UART_Debug_PutChar(gb_rcv_str[count1]);
				Print_ASCII_HEX(gb_rcv_str[count1]);
				UART_Debug_PutChar(' ');
			}
			Print_Message("\n\rCorrect response not received");                  //In this case no need to set flag for transmitting frame as send frame flag is not cleared
			#endif
			
			mb_poll_dpt = 1;            //  resume polling if correct response is not received
			gb_mb_write_processing_f = 0;
			modbus_op_state = MB_OP_DPT_READ;

            /* clear all array index where data is fill into array */
            for (int count1 =0; count1 < gb_byte_count; count1++)
            {
                gb_rcv_str[count1] = 0;
            }
            /* clear var incremented in uart ISR*/
            gb_byte_count = 0;                                                              //To be cleared when flag is set in RX ISR
        }
        gb_rcvd_flag = 0;                                                                   //clear receive flag
        gb_flag_inter_char_error = 0;                                                        //clear flag set in 1.5char
        frame_error_f=0;
    }
	#if INTERFACE_485
    //HAL_GPIO_WritePin(GPIOE, LED2_Pin, SET);
	#endif

    return 0;
}

/*****************************************************************************
* Function name	: void Modbus_Flags_Set_In_Timer(void)
* Returns		: Nothing.
* Arguments    	: void.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 28/05/2022.
*
*
* Description	:	gb_flag_inter_char_error, gb_rcvd_flag, gb_send_frame these
					flags are sets based on baudrate, parity, response time and
					inter character receive time. this function should be called
					in timer interrupt service routine.
*               :
* Notes			: This function must be called in Timer ISR.
* Global Variables Affected :	gb_inter_char---> decrement in timer routine.
								gb_flag_inter_char_error ---> flag set.
								gb_frame_response_timeout---> decrement in timer routine.
								gb_rcvd_flag ---> flag set.
								gb_response_time---> decrement in timer routine.
								gb_send_frame ---> flag set.
*****************************************************************************/
void Modbus_Flags_Set_In_Timer(void)
{
	if (gb_inter_char > 0)		// check if inter char count is greater then 0
	{              		
		gb_inter_char--;                		//decrement inter char count
		if (gb_inter_char == 0)
		{
			gb_flag_inter_char_error = 1;
		}
	}

	if (gb_frame_response_timeout > 0)		// Check if frame response timer greater then 0
	{          
		gb_frame_response_timeout--;	// Decrement frame response count
		if (gb_frame_response_timeout == 0)	// Check if frame response timeout equal to 0
		{  
			gb_usart_byte_rec_f = 0;			/* clear this flag once received all byte. */   
			gb_rcvd_flag = 1;                   //Set flag to indicate wait and processing state(EOF detected)
			idex = 0;
			OFF_YELLOW_LED;
		}
	}

	if (gb_response_time > 0)
	{
		gb_response_time--;
		if (gb_response_time == 0)
		{
			gb_send_frame = 1;		//Variable decremented for function to send retrial frame if response not received.
		}
	}
}

/*****************************************************************************
* Function name	: void Get_Modbus_UART_Data(uint32_t rcv_data)
* Returns		: Nothing.
* Arguments    	: uint32_t rcv_data---> 1 byte data receive from the RS485 based
					UART.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 28/05/2022.
*
*
* Description	:	receives a byte of data and copy into an array and response
					time, inter char receive timeout and frame_response_timeout
					variables are updated, based on RS485 UART interrupt service
					routine.
*               :
* Notes			: This function must be called in UART ISR.
* Global Variables Affected :	gb_byte_count ---> Received byte counts updates.
								gb_response_time, gb_inter_char, 
								gb_frame_response_timeout are updates.
								gb_rcv_str[] ---> array fills with received data.
*****************************************************************************/
void Get_Modbus_UART_Data(uint32_t rcv_data)
{
	if (gb_flag_inter_char_error == 0)				//check if char error flag is not set for waiting state
	{
		gb_byte_count++;                                                    //Increment count for number of bytes received used in Modbus driver .c file
		/* Load response time */
		gb_response_time = gb_retry_timeout;
		gb_inter_char = gb_inter_char_timeout1;                             //Load inter character counter for baud rate selected
		gb_frame_response_timeout = gb_frame_response_timeout1;             //Load frame response counter for baud rate selected
		gb_rcv_str[idex++] = rcv_data;                                  //store received data into array
		ON_YELLOW_LED;
	}
}
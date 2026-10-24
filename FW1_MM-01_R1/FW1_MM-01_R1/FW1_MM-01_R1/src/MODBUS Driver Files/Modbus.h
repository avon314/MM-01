/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: Modbus.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 25/05/2022.
*
* Module Description :	Defines constants and macros.
*
*
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 25/05/2022.
* Changes		: NO.
*
*****************************************************************************/
#define INTERFACE_485 1

/***** Baud rates *****/
#define BAUD_RATE_1200 	1
#define BAUD_RATE_2400 	2
#define BAUD_RATE_4800 	3
#define BAUD_RATE_9600 	4
#define BAUD_RATE_19200 5
#define BAUD_RATE_38400 6
#define BAUD_RATE_56000 7
#define BAUD_RATE_115200 8

/***** Parity *****/
#define PARITY_NONE 1
#define PARITY_EVEN 2
#define PARITY_ODD 3

#define MAX_BUF_SIZE 200	//50


/***** Extern declarations of global variables *****/
extern volatile uint8_t gb_flag_inter_char_error;		// Var used for error
extern volatile uint32_t gb_response_time;				// Var used as response time
extern volatile uint16_t gb_retry_timeout;				// Var used for timeout for sending retrial frame
extern volatile int gb_byte_count;						// For number of bytes received
extern volatile uint8_t gb_inter_char_timeout1;
extern volatile uint16_t gb_frame_response_timeout;		// Var used for frame response timeout
extern volatile uint16_t gb_frame_response_timeout1;
extern volatile uint8_t gb_rcv_str[MAX_BUF_SIZE];		// Declare data receive array.
extern volatile uint8_t idex;							// var used for array index
extern volatile uint8_t gb_inter_char;					// Var used for inter frame timeout
extern volatile uint8_t gb_rcvd_flag;					// flag set if data not received in 3.5char time i.e EOF received
extern volatile uint8_t gb_send_frame;					// Var used in Modbus sending frame as a flag

extern volatile uint8_t gb_retry_count;					// Var used as retry counter
extern volatile uint8_t gb_correct_frame_rcvd;			// Var used for correct response of the frame used in application for further processing
extern volatile uint8_t gb_mb_exc_frame_rcvd;			// Set when a valid exception response (FC | 0x80) is received from slave
extern volatile uint8_t gb_mb_exc_slave;				// Slave address of the last exception response
extern volatile uint8_t gb_mb_exc_code;					// Exception code of the last exception response (01, 02, 03 ...)
extern volatile uint8_t gb_parity_sel;					// Var used for frame not received correctly
extern volatile uint8_t gb_baud_rate_sel;
extern volatile uint32_t gb_baud_value;					// Var used to fill value as per ETS configuration and write to uart peripheral in main file

extern uint8_t gb_modbus_data[4];

extern volatile uint8_t gb_par_frame_err;
/***** Function prototypes *****/
void Parity_Selection(int selec);
void Modbus_Baud_Rate(uint8_t baud_val);
void Modbus_init(void);
void modbus_frame(uint8_t slave_address,uint8_t function_code,uint16_t starting_address,uint16_t number_of_reg,uint8_t byte_count,uint8_t *data_buff);
void send_frame(void);
int frame_rcvd(void);
void Set_InterChar_Timeout(uint8_t gb_baud_rate_sel);
void Get_Modbus_UART_Data(uint32_t);
void Modbus_Flags_Set_In_Timer(void);
uint8_t Get_Baud_Index(uint32_t baud);

/***** End of File *****/

/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: modbus_app.c
* Created By	: Shirisha  Ankam.
* Created Date	: 3/12/2025.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*                   128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 3/12/2025.
* Changes		: NA.
*****************************************************************************/

#ifndef MODBUS_APP_H_
#define MODBUS_APP_H_

#include "asf.h"

#define TOTAL_SLAVES            (16)                                        // Maximum number of Modbus slaves supported
#define MAX_RETRY_COUNT		    (3)		                                    // Maximum retries for failed frame transmission
#define MAX_DATA_PACKETS        (100)                                       // Maximum number of data packets configurable
#define DEBUG_MODBUS_DATA       (0)	                                        // Debug flag for Modbus data operations
#define DEBUG_MODBUS_STORAGE    (0)                                         // Debug flag for Modbus storage operations
#define DEBUG_MD_SERVER         (0)                                         // Debug flag for Modbus server communication
#define DEBUG_WR_DATA           (0)                                         // Debug flag for Modbus write operations

extern uint8_t active_slaves[TOTAL_SLAVES];                                 // Array to track active/responding slaves (1=active, 0=inactive)
extern U8 gb_send_cs_to_server;                                             // Flag to send connection status to server
extern U8 gb_mb_write_processing_f;                                         // Flag indicating Modbus write operation in progress
extern U8 gb_write_to_req_f;                                                // Flag indicating write request pending

typedef struct
{
	U8 dp_num;                                                              // Data packet number (1-100)
	U8 dp_data_len;                                                         // Data length in bytes (0=1byte, 1=1byte, 2=2bytes, 3=4bytes)
	U8 dp_slave_addr;                                                       // Modbus slave address (1-247)
	U32 dp_reg_addr;                                                        // Modbus register address
	U8 dp_mb_fun;                                                           // Modbus function code (1-6, 15)
	U8 dp_event;                                                            // Storage event type (0=COV, 1=Specific Time, 2=Periodic)
	U8 dp_hours;                                                            // Hours for scheduled/periodic storage
	U8 dp_min;                                                              // Minutes for scheduled/periodic storage
}modbus_configuration;

extern modbus_configuration mp_config;                                     // Current data packet configuration

typedef struct
{
	U16 boudrate;                                                           // Modbus baudrate (e.g., 9600, 19200)
	U8 parity_bit;                                                          // Parity setting (0=None, 1=Odd, 2=Even)
	U8 stop_bit;                                                            // Stop bit setting (0=1bit, 1=2bits)
	U16 poll_interval;                                                      // Polling interval in milliseconds
}MS;

extern MS ms_info;                                                          // Modbus serial configuration

typedef struct
{
	U8  dpt_num;                                                            // Data packet number to write
	U8  data[2];                                                            // Data buffer for write operation
} mb_write_req_t;

volatile extern mb_write_req_t gb_mb_write_req;                            // Modbus write request structure

typedef enum
{
	MB_OP_NONE = 0,                                                         // No operation
	MB_OP_SCAN,                                                             // Scanning for active slaves
	MB_OP_DPT_READ,                                                         // Reading data packets from slaves
	MB_OP_WRITE                                                             // Writing data to slave registers
} MB_OP_STATE;

extern volatile MB_OP_STATE modbus_op_state;                               // Current Modbus operation state

void test_mudbus(void);
void Segregate_DPT_Data_To_Buffer(unsigned char dp_number);
void Update_Modbus_Poll_Interval(uint16_t dp_val);
void Detect_Slaves_On_PowerOn(void);
void go_to_next_slave(void);
void Process_All_DPT_Packets(void);
void Validate_DPT_Packet(void);
U8* Send_Modbus_Data_Packets_ToServer(U8 md_logs,U8 *entryCount);
void Write_Last_Sent_MD_Location(int rcv_last_sent_loc);
void Write_Last_Stored_Data_Packet_Location(int rcv_last_store_loc);
void Store_Modbus_Data_Packets( U8 *day_date, U8 *time_arr, U8 dp_number, U8 *dp_value);
void Update_Last_Sent_Bytes_MD(void);
void Write_to_Register_Functionality(U8 dpt_num,U8 *data_buf);
void Get_Modbus_Data(uint8_t *dst, uint8_t *src, uint8_t order);
void Store_DPT_Into_Memory(uint8_t store_type, uint8_t *dpt_value);
#endif /* MODBUS_APP_H_ */
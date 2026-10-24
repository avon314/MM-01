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

/*****  User defined Includes *****/
#include "modbus_app.h"
#include "user_uart.h"
#include "ext_eeprom_1.h"
#include "definitions.h"
#include "memory_storage.h"
#include "json_extraction.h"
#include "Modbus.h"
#include "user_rtc.h"
#include "mm_app.h"
#include "CRC.h"
#include "string.h"
/***** End for User defined Includes *****/

/**** Declaration for global variables ****/
uint8_t active_slaves[TOTAL_SLAVES] = {0};   // 1 if slave responds, 0 if not
uint8_t num_active_slaves = 0;
uint8_t slave_address = 0;       // start from slave 1
uint8_t current_dp_number = 1;      // 1 to 100
uint8_t gb_modbus_data[4] = {0};	
modbus_configuration mp_config = {0};
MS ms_info = {0};
volatile mb_write_req_t gb_mb_write_req;
U8 gb_send_cs_to_server = 0;
U8 gb_mb_write_processing_f = 0;
U8 gb_write_to_req_f = 0;
U8 gb_dpt_cycle_reset[TOTAL_SLAVES] = {1};	
extern xSemaphoreHandle xSemaphore;	
volatile MB_OP_STATE modbus_op_state = MB_OP_NONE;	

U8 gb_mb_exc_status[TOTAL_SLAVES] = {0};                                    // Exception code per slave reported to server ("ME"), 0 = no error
static U8 gb_mb_exc_cycle[TOTAL_SLAVES] = {0};                              // Exception codes collected during the current DPT poll cycle
U8 gb_mb_exc_clr[TOTAL_SLAVES] = {0};                                      // 1 = exception cleared, report [SA,0] until server ack
U8 gb_send_me_to_server = 0;                                                // Flag to send "ME" to server, cleared on server ack
volatile U8 gb_me_check_state = ME_CHECK_DISABLED;                         // ME checking state, enabled after CS frame is acknowledged
U8 gb_me_change_cnt = 0;                                                    // Incremented on every change of gb_mb_exc_status
U8 gb_me_sent_cnt = 0;                                                      // gb_me_change_cnt value carried in the last frame sent

/**** End for Declaration for global variables ****/

/*****************************************************************************
* Function name : Segregate_DPT_Data_To_Bufferr.
* Returns		: nothing.
* Arguments     : unsigned char --> dp_number.
* Created by	: Shirisha Ankam.
* Date created	: 18/12/2025.
*
* Description	: This function is to segregate modbus data.

* Notes	        : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Segregate_DPT_Data_To_Buffer(unsigned char dp_number)
{
	int read_loc = (MP_EEP_START_LOC + ((dp_number - 1) * MP_SIZE_PER_DATA));
	
	U8 mp_buffer[MP_SIZE_PER_DATA] = {0};
	memset(mp_buffer, 0, MP_SIZE_PER_DATA);
	EEPROM_Byte_Read(read_loc, mp_buffer, MP_SIZE_PER_DATA);
	
	#if 0
	Print_Message("\nRead data is : \n");
	for (U16 idx = 0; idx < MP_SIZE_PER_DATA; idx ++)
	{
		Print_Number(mp_buffer[idx]);
		Print_Message(",");
	}
	Print_Message("\n");
	#endif
	
	//fill variable with actual configured values.
	mp_config.dp_data_len = mp_buffer[0];
	mp_config.dp_slave_addr = mp_buffer[1];
	mp_config.dp_reg_addr = ((mp_buffer[2] << 8) | mp_buffer[3]);
	mp_config.dp_mb_fun = mp_buffer[4];
	mp_config.dp_event = mp_buffer[5];
	mp_config.dp_hours = mp_buffer[6];
	mp_config.dp_min = mp_buffer[7];
	
}


/*****************************************************************************
* Function name : test_mudbus.
* Returns		: nothing.
* Arguments     : void.
* Created by	: Shirisha Ankam.
* Date created	: 18/12/2025.
*
* Description	: This function is to test modbus data transmit and receiving

* Notes	        : function not to be included in application.
* Global Variables Affected : NA.
*****************************************************************************/
void test_mudbus()
{		
	if(gb_rtc_1sec_delay_f == 1)
	{
		gb_rtc_1sec_delay_f = 0;
		
		U8  slaveAddr = 2;
		U8  fun_code  = 0x04;   // Read Input Registers
		U16 regAddr   = 0;      // 30001
		U16 regNum    = 8;      // read 8 registers
		U8 byteCnt = 2 * regNum;
		U8 buff[2] = {0,0};
				
		modbus_frame(slaveAddr,fun_code,regAddr,regNum,byteCnt,buff);
		send_frame();
	}

}
	
/*****************************************************************************
* Function name : Validate_DPT_Packet.
* Returns		: void.
* Arguments     : nothing.
* Created by	: Shirisha Ankam.
* Date created	: 18/12/2025.
*
* Description	: Validates received Modbus data packet and extracts values
*                 based on configured byte count (1, 2, or 4 bytes).
*
* Notes	        : Clears received value if function code doesn't match.
* Global Variables Affected : gb_modbus_data
*****************************************************************************/
void Validate_DPT_Packet(void)
{
	U8 received_value[4] = {0};                                             // Buffer to store extracted Modbus data
	memset(received_value, 0, sizeof(received_value));
	
	if(gb_rcv_str[1] == mp_config.dp_mb_fun)								// Check if received function code matches configured value
	{
		if (gb_rcv_str[2] == 1)                                             // 1 byte received
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\n Read 1 byte");
			#endif
			received_value[3] = gb_modbus_data[0];                          // Copy to LSB position
		}
		
		else if (gb_rcv_str[2] == 2)                                        // 2 bytes received
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nRead register value 2 bytes");
			#endif
			
			received_value[2] = gb_modbus_data[0];                          // Copy high byte
			received_value[3] = gb_modbus_data[1];                          // Copy low byte
		}
		else if (gb_rcv_str[2] == 4)                                        // 4 bytes received
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nRead register value 4 bytes");
			#endif
			
			memcpy(received_value,gb_modbus_data, 4);                       // Copy all 4 bytes
		}
		else
		{
			__NOP();
		}
	}
	else
	{
		memset(received_value, 0, sizeof(received_value));                  // Clear buffer if function code doesn't match
	}
	
	#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
	Print_Message("\nStored Modbus Value : ");
	for(U8 i = 0; i < 4; i++)
	{
		Print_ASCII_HEX(received_value[i]);
		Print_Message(",");
	}
	#endif
	
	Store_DPT_Into_Memory(mp_config.dp_event,received_value);              // Store validated data to memory
}



/*****************************************************************************
 * Function name : Process_All_DPT_Packets.
 * Returns		 : void.
 * Arguments     : nothing.
 * Created by	 : Shirisha Ankam.
 * Date created	 : 18/12/2025.
 *
 * Description	 : Processes data packets sequentially and polls only active
 *                 Modbus slaves based on configuration.
 *
 * Notes	     : Wraps back to packet 1 after processing all packets.
 * Global Variables Affected : current_dp_number, gb_send_mb_data,
 *                              gb_dpt_cycle_reset
 *****************************************************************************/  
void Process_All_DPT_Packets(void)
{
	#if 0
	Print_Message("\nProcessing Data Packets : ");
	Print_Number(current_dp_number);
	#endif
	
	Segregate_DPT_Data_To_Buffer(current_dp_number);                        // Extract configuration for current data packet
						
	U8 num_of_reg = 0;                                                      // Number of Modbus registers to read
	U8 byte_len = 0;                                                        // Total byte length to read
		
	if((mp_config.dp_data_len == 0) || 	(mp_config.dp_data_len == 1) || (mp_config.dp_data_len == 2)) // 1 or 2 byte data
	{
		num_of_reg = 1;                                                     // Read 1 register (2 bytes)
	}
	else if(mp_config.dp_data_len == 3)                                     // 4 byte data
	{
		num_of_reg = 2;                                                     // Read 2 registers (4 bytes)
	}
	else
	{
		__NOP();
	}
	byte_len = num_of_reg * 2;                                              // Calculate byte length (each register is 2 bytes)
		
	uint8_t dummyBuf[2] = {0};                                              // Placeholder buffer for write operations (if needed)
	// Check if slave is active
	if (((mp_config.dp_slave_addr > 0) && (mp_config.dp_slave_addr <= TOTAL_SLAVES)) // Validate slave address range
	&& (mp_config.dp_mb_fun != 0)                                           // Function code must be non-zero
	&&  (mp_config.dp_mb_fun <= 4))                                         // Function code must be valid (1-4)
	{
		#if (DEBUG_ALL || DEBUG_MODBUS_DATA)
		Print_Message("\nProcessing Data Packet for Slave ID: ");
		Print_Number(current_dp_number);
		Print_Message(" | ");
		Print_Number(mp_config.dp_slave_addr);
		#endif
		
		if ((active_slaves[mp_config.dp_slave_addr - 1] == 1) || (gb_dpt_cycle_reset[mp_config.dp_slave_addr - 1] == 1)) // Check if slave is active or cycle reset triggered
		{
			gb_dpt_cycle_reset[mp_config.dp_slave_addr - 1] = 0;           // Clear cycle reset flag for this slave
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA)
			Print_Message("\nSlave is Active");
			#endif
			
			modbus_frame(mp_config.dp_slave_addr, mp_config.dp_mb_fun, mp_config.dp_reg_addr, num_of_reg, byte_len, dummyBuf); // Build Modbus request frame
 			send_frame();                                                   // Transmit Modbus frame to slave
			gb_send_mb_data = FLAG_RST;                                     // Reset flag to wait for response
		}
		else
		{
			#if DEBUG_MODBUS_DATA
			Print_Message("\nSlave Not Active");
			#endif
			// Slave not active, skip
			gb_send_mb_data = FLAG_SET;                                     // Set flag to move to next packet immediately
		}
	}
	else
	{
		#if DEBUG_MODBUS_DATA && 0
		Print_Message("\nInvalid Slave or function code is not for Read");
		#endif
		gb_send_mb_data = FLAG_SET;                                         // Skip invalid configuration
	}
	// Move to next DP
	current_dp_number++;                                                    // Increment to next data packet
	if (current_dp_number > MAX_DATA_PACKETS)                               // Check if all packets processed
	{
		current_dp_number = 1;                                              // Wrap back to first packet
		memset(gb_dpt_cycle_reset, 1, MAX_INPUTS);                          // Set cycle reset flags for all slaves
		Evaluate_Modbus_Exception_COV();                                    // Report "ME" to server if exception status changed
	}
}




/*****************************************************************************
 * Function name : go_to_next_slave.
 * Returns		 : void.
 * Arguments     : nothing.
 * Created by	 : Shirisha Ankam.
 * Date created	 : 18/12/2025.
 *
 * Description	 : Moves to next slave for checking active response status.
 *                 Starts data packet transmission after all slaves checked.
 *
 * Notes	     : NA.
 * Global Variables Affected : slave_address, gb_start_sending_dpt,
 *                              gb_send_mb_data, gb_send_cs_to_server
 *****************************************************************************/  
void go_to_next_slave(void)
{
	slave_address++;                                                        // Move to next slave address
	if (slave_address > TOTAL_SLAVES)                                       // Check if all slaves have been scanned
	{
		slave_address = 0;                                                  // Reset slave address counter
		gb_start_sending_dpt = 1;                                           // Enable data packet transmission
		gb_send_mb_data = FLAG_SET;                                         // Set flag to begin sending Modbus data
		gb_send_cs_to_server = FLAG_SET;                                    // Set flag to send connection status to server
		#if 1
		Print_Message("\nNumber of Slave Connected: ");
		Print_Number(num_active_slaves);
		Print_Message("\nSlave No: ");
		for (U8 i = 0; i < TOTAL_SLAVES; i++)                               // Loop through all slaves
		{
			if (active_slaves[i] == 1)                                      // Check if slave is active
			{
				Print_Number(i + 1);                                        // Print active slave number (1-indexed)
				Print_Message(",");
			}
		}
		Print_Message("\n");
		#endif
		return;                                                             // Exit function after completing scan
	}
}



/*****************************************************************************
 * Function name : Modbus_Functionality_Task.
 * Returns		 : void.
 * Arguments     : void --> *pvParameters.
 * Created by	 : Shirisha Ankam.
 * Date created	 : 10/12/2025.
 *
 * Description	 : RTOS task for managing Modbus operations including slave
 *                 scanning, data packet polling, and write requests.
 *
 * Notes	     : Task runs continuously with semaphore protection.
 * Global Variables Affected : gb_device_power_on, gb_send_mb_data,
 *                              gb_start_sending_dpt, modbus_op_state
 *****************************************************************************/
void Modbus_Functionality_Task(void *pvParameters)
{
	UNUSED(pvParameters);                                                   // Suppress unused parameter warning
	
	gb_device_power_on = 1;                                                 // Set power-on flag to trigger initial slave scan
	while (1)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		/* Attempt to take the semaphore (wait for it to be available) */
		if (xSemaphoreTake(xSemaphore, portMAX_DELAY))                      // Wait indefinitely for semaphore
		{
			//Print_Message("\nxSemaphoreTake.");
			
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
			//send_frame();            //check for send frame flag 
			frame_rcvd();                                                   // Check for received Modbus frames
			
			if (gb_reinit_uart == 1)                                        // Check if UART reinitialization requested
			{
				gb_reinit_uart = 0;                                         // Clear reinitialization flag
				// Stop Modbus TX/RX safely
				uart_disable_interrupt(USART0, UART_IER_RXRDY | UART_IER_TXRDY); // Disable UART interrupts
				
				// Disable transmitter and receiver
				uart_disable(USART0);                                       // Disable UART peripheral
					
				// Reinitialize Modbus variables
				Modbus_init();                                              // Reset Modbus state machine
				gb_device_power_on = 1;                                     // Trigger slave scan after reinit
				gb_start_sending_dpt = 0;                                   // Disable data packet transmission
				
				//continue; // restart loop
			}
			if (gb_device_power_on == FLAG_SET)                             // Check if device just powered on or reset
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA)
				Print_Message("\n1---------------------->");
				#endif
				gb_device_power_on = FLAG_RST;                              // Reset the device power-on flag
				
				num_active_slaves = 0;                                      // Reset active slave counter
				gb_me_check_state = ME_CHECK_DISABLED;                      // No ME checking until CS frame is sent after scan
				memset(active_slaves, 0, sizeof(active_slaves));            // Clear active slaves array
				slave_address = 1;                                          // Start scanning from slave address 1
				gb_send_mb_data = FLAG_SET;                                 // Trigger first Modbus frame transmission
			}
			
			if ((gb_send_mb_data == FLAG_SET) && (gb_start_sending_dpt == 0)) // Check if ready to send and not in DPT mode
			{
				uint8_t dummy_data[2] = {0};                                // Placeholder data for read operation
				gb_send_mb_data = FLAG_RST;                                 // Clear send flag
				modbus_op_state = MB_OP_SCAN;                               // Set operation state to slave scanning
				modbus_frame(slave_address, 0x04, 0x0000, 1, 2, dummy_data); // Build frame to read input register (FC 0x04)
				
			}
			send_frame();                                                   // Transmit Modbus frame if ready
					
			if ((gb_correct_frame_rcvd || gb_mb_exc_frame_rcvd) && (gb_start_sending_dpt == 0) && (modbus_op_state == MB_OP_SCAN)) // Valid response received during scan
			{
				gb_correct_frame_rcvd = 0;                                  // Clear frame received flag
				gb_mb_exc_frame_rcvd = 0;                                   // Exception reply also means slave is present
				active_slaves[slave_address - 1] = 1;                       // Mark current slave as active
				num_active_slaves++;                                        // Increment active slave count
				go_to_next_slave();                                         // Move to next slave address
			}
			
			if ((gb_write_to_req_f == 1) && (gb_mb_write_processing_f == 0) && (gb_correct_frame_rcvd == 0) && (dm_dev_info.dev_sts == APPROVED)) // Write request pending and system approved
			{
				gb_write_to_req_f = 0;                                      // Clear write request flag
				Write_to_Register_Functionality(gb_mb_write_req.dpt_num,gb_mb_write_req.data); // Execute write operation
			}
			
			if ((gb_start_sending_dpt == 1) && (mb_poll_dpt == 1) && (dm_dev_info.dev_sts == APPROVED)) // DPT polling enabled and device approved
			{
				if(gb_send_mb_data == FLAG_SET)                             // Check if ready to send next packet
				{
					modbus_op_state = MB_OP_DPT_READ;                       // Set operation state to data packet reading
					Process_All_DPT_Packets();                              // Process and send next data packet request
				}
				
			}
									
			if (gb_correct_frame_rcvd == 1)                                 // Valid Modbus response received
			{
				gb_correct_frame_rcvd = 0;                                  // Clear frame received flag
				switch (modbus_op_state)                                    // Handle response based on current operation
				{
					case MB_OP_DPT_READ:                                    // Response for data packet read
					if (mb_poll_dpt == 1)                                   // Polling enabled
					{
						Validate_DPT_Packet();                              // Validate and store received data
					}
					break;
					case MB_OP_WRITE:                                       // Response for write operation
					// WRITE ACK received — DO NOT parse as DPT
					mb_poll_dpt = 1;                                        // Resume polling after write
					gb_mb_write_processing_f = 0;                           // Clear write processing flag
					modbus_op_state = MB_OP_DPT_READ;                       // Return to DPT reading mode
					break;
					default:
					break;
				}
			}
			
			if (gb_mb_exc_frame_rcvd == 1)                                  // Exception response received from slave
			{
				gb_mb_exc_frame_rcvd = 0;                                   // Clear exception received flag
				switch (modbus_op_state)
				{
					case MB_OP_DPT_READ:                                    // Exception for data packet read
					Record_Modbus_Exception(gb_mb_exc_slave, gb_mb_exc_code); // Collect for "ME" frame, do not store as DPT value
					break;
					case MB_OP_WRITE:                                       // Exception for write request, not reported in "ME"
					mb_poll_dpt = 1;                                        // Resume polling after write
					gb_mb_write_processing_f = 0;                           // Clear write processing flag
					modbus_op_state = MB_OP_DPT_READ;                       // Return to DPT reading mode
					break;
					default:
					break;
				}
			}

			//Print_Message("\nxSemaphoreRelease.");				
			/* Release the semaphore when done with the critical section */
			xSemaphoreGive(xSemaphore);                                     // Release semaphore for other tasks
			
		}
		/* Delay must be used to suspend the task */
		vTaskDelay(5);                                                      // Delay 5ms to yield CPU to other tasks
	}
}



/*****************************************************************************
* Function Name  : Store_Modbus_Data_Packets
* Returns        : void.
* Arguments      : U8 * -->current date, U8 * -->current time, 
*                  U8 --> dpt number,  U8 * --> dpt value
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Stores Modbus data packet with timestamp to EEPROM for
*                  later transmission to server.
*
* Notes          : Handles circular buffer wrap-around at boundary.
* Global Variables Affected : gb_md_reached_boundary_f
****************************************************************************/
void Store_Modbus_Data_Packets( U8 *day_date, U8 *time_arr, U8 dp_number,  U8 *dp_value)
{
	#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
	Print_Message("\nInside Store modbus data packets fun");
	#endif
	if (dp_number <= 0 || dp_number > MAX_DATA_PACKETS)                     // Validate data packet number range
	return;	                                                                // Exit if invalid packet number
	
	U8 dp_array[MD_BUFFER_LEN] = {0};                                       // Buffer to hold complete data packet
	U8 lcl_idx = 0;                                                         // Index for building packet array
	U8 csnIdx = 0;
	int lcl_last_loc = 0;                                                   // Last EEPROM storage location
	
	dp_array[lcl_idx ++] = day_date[0];                                     // Store day
	dp_array[lcl_idx ++] = day_date[1];                                     // Store month
	dp_array[lcl_idx ++] = day_date[2];                                     // Store year
	dp_array[lcl_idx ++] = day_date[3];                                     // Store day of week
	dp_array[lcl_idx ++] = time_arr[0];                                     // Store hour
	dp_array[lcl_idx ++] = time_arr[1];                                     // Store minute
	dp_array[lcl_idx ++] = time_arr[2];                                     // Store second
	dp_array[lcl_idx ++] = dp_number;                                       // Store data packet number
	
	// ---- Fixed 4-byte value field ----
	dp_array[lcl_idx++] =  dp_value[0];                                     // Store MSB of data value
	dp_array[lcl_idx++] =  dp_value[1];                                     // Store byte 2 of data value
	dp_array[lcl_idx++] =  dp_value[2];                                     // Store byte 3 of data value
	dp_array[lcl_idx++] =  dp_value[3];                                     // Store LSB of data value
	
	#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
	Print_Message("\nData packet Log Data: ");
	for (int i = 0; i < MD_BUFFER_LEN; i++)
	{
		Print_Number(dp_array[i]);
		Print_Message(",");
	}
	Print_Message("\n");
	#endif
	
	U8 lcl_eep_buf[5];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, 5);
	/* Read last location stored in the flash. */
	lcl_eep_status = EEPROM_Byte_Read(MD_STORED_LAST_ADDS, lcl_eep_buf, 5); // Read last storage location with CRC
	if (lcl_eep_status == 0)
	{
		/* Verify the CRC for last location value. */
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)            // Validate CRC of stored location
		{
			/* If CRC validated, update last loc with the flash value. */
			lcl_last_loc = lcl_eep_buf[0];							        // Get higher byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[1];	        // Get middle byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[2];	        // Get lower byte
			
//   			Print_Message("\nCRC validated. \nLast loc value is : ");
//   			Print_Number(lcl_last_loc);
		}
		else
		{
			/* If CRC fails, update last loc with default value. */
			lcl_last_loc = MD_EEP_START_LOC;                                // Reset to start location on CRC failure
			
			/****************************************************************/
			/*****
			* If CRC fails, last sent location should be updated, it should be
			* as equal as or less than last stored location 
			*****/
			int lcl_last_sent_loc = MD_EEP_START_LOC;                       // Reset sent location to start
			Write_Last_Sent_MD_Location(lcl_last_sent_loc);                // Update sent location in EEPROM
			/****************************************************************/
			
//  			Print_Message("\nCRC failed. \nLast loc value is : ");
//  			Print_Number(lcl_last_loc);
		}
		
		if (lcl_last_loc >= MD_EEP_END_LOC)                                 // Check if reached end of allocated EEPROM space
		{
			/* If reserved locations filled for TL, update last loc with default value / start value. */
			lcl_last_loc = MD_EEP_START_LOC;                                // Wrap around to start location
			gb_md_reached_boundary_f = 1;                                   // Set boundary flag
			EEPROM_Byte_Write(MD_BOUNDARY_FLAG, &gb_md_reached_boundary_f, 1); // Store boundary flag in EEPROM
		}
		
		/* Store TL at flash mem */
		EEPROM_Byte_Write(lcl_last_loc, dp_array, MD_BUFFER_LEN);           // Write data packet to EEPROM
		lcl_last_loc += MD_BUFFER_LEN;                                      // Increment location by packet size
		Write_Last_Stored_Data_Packet_Location(lcl_last_loc);               // Update last stored location in EEPROM
	}
	else
	{
		__NOP();
	}
}



/*****************************************************************************
* Function name	: Update_Last_Sent_Bytes_MD
* Returns		: Nothing.
* Arguments    	: void.
* Created by	: Shirisha Ankam.
* Date created	: 23/12/2025.
* Description	: Update last sent location in the memory..
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Update_Last_Sent_Bytes_MD(void)
{
	int lcl_last_sent_loc = 0;
	
	U8 lcl_eep_buf[5];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, 5);
	lcl_eep_status = EEPROM_Byte_Read(MD_SENT_LAST_ADDS, lcl_eep_buf, 5);
	if (lcl_eep_status == 0)
	{
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)
		{
			/* If CRC validated, update last sent loc with the flash value. */
			lcl_last_sent_loc = lcl_eep_buf[0];								// Get higher byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[1];	 // Get middle byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[2];	 // Get lower byte
			
// 			Print_Message("\nCRC Validated. \nLast sent loc value of MD is : ");
// 			Print_Number(lcl_last_sent_loc);
		}
		else
		{
			/* If CRC fails, update last sent loc with default value. */
			lcl_last_sent_loc = MD_EEP_START_LOC;
			
// 			Print_Message("\nCRC failed. \nLast sent loc value of MD is : ");
// 			Print_Number(lcl_last_sent_loc);
		}
		
		lcl_last_sent_loc += gb_last_sent_md_bytes;
		Write_Last_Sent_MD_Location(lcl_last_sent_loc);
	}
	else
	__NOP();
}



/*****************************************************************************
* Function name	: Write_Last_Stored_Data_Packet_Location
* Returns		: Nothing.
* Arguments    	: int -> last stored location
* Created by	: Shirisha Ankam.
* Date created	: 23/12/2025.
* Description	: To store last stored location into memory.
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Write_Last_Stored_Data_Packet_Location(int rcv_last_store_loc)
{
// 	Print_Message("\nInside write last stored data packets loc fun");
// 	Print_Message("\nValue of last location received : ");
// 	Print_Number(rcv_last_store_loc);
	
	U8 lcl_lst_storeArr[5] = {0};
	/* Updated last location value is added in the buffer to write to the flash mem */
	lcl_lst_storeArr[0] = (U8) ((rcv_last_store_loc & 0x00FF0000) >> 16);	// Add Higher Byte.
	lcl_lst_storeArr[1] = (U8) ((rcv_last_store_loc & 0x0000FF00) >> 8);		// Add Middle Byte.
	lcl_lst_storeArr[2] = (U8) ((rcv_last_store_loc & 0x000000FF));			// Add Lower Byte.
	/* Calculate CRC for last loc value. */
	CRC_Calculate_Check(lcl_lst_storeArr, 3, CRC_CALC);
	lcl_lst_storeArr[3] = gb_Temp_CRC[0];
	lcl_lst_storeArr[4] = gb_Temp_CRC[1];
	/* Then store last loc value to the flash mem */
	EEPROM_Byte_Write(MD_STORED_LAST_ADDS, lcl_lst_storeArr, 5);
}


/*****************************************************************************
* Function name	: Write_Last_Sent_MD_Location
* Returns		: Nothing.
* Arguments    	: int -> last sent location
* Created by	: Shirisha Ankam.
* Date created	: 23/12/2025.
* Description	: To store last sent location into memory.
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Write_Last_Sent_MD_Location(int rcv_last_sent_loc)
{
/*	Print_Message("\nInside last send data packet loc fun");*/
	U8 lcl_lst_sentArr[5] = {0};
	/* Updated last sent location value is added in the buffer to write to the flash mem */
	lcl_lst_sentArr[0] = (U8) ((rcv_last_sent_loc & 0x00FF0000) >> 16);		// Add Higher Byte.
	lcl_lst_sentArr[1] = (U8) ((rcv_last_sent_loc & 0x0000FF00) >> 8);		// Add Middle Byte.
	lcl_lst_sentArr[2] = (U8) ((rcv_last_sent_loc & 0x000000FF));			// Add Lower Byte.
	/* Calculate CRC for last loc value. */
	CRC_Calculate_Check(lcl_lst_sentArr, 3, CRC_CALC);
	lcl_lst_sentArr[3] = gb_Temp_CRC[0];
	lcl_lst_sentArr[4] = gb_Temp_CRC[1];
	/* Then store last loc value to the flash mem */
	EEPROM_Byte_Write(MD_SENT_LAST_ADDS, lcl_lst_sentArr, 5);
}



/*****************************************************************************
* Function Name  : Send_Modbus_Data_Packets_ToServer
* Returns        : U8* - Pointer to buffer containing Modbus data logs
* Arguments      : U8 md_logs - Number of logs to read,
*                  U8 *entryCount - Output parameter for actual entries read
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Reads stored Modbus data packets from EEPROM and prepares
*                  them for transmission to server. Handles circular buffer
*                  wrap-around.
*
* Notes          : Returns NULL if no data available.
* Global Variables Affected : gb_update_md_last_sent_loc_f,
*                              gb_send_modbus_data_to_serverf,
*                              gb_last_sent_md_bytes, gb_md_reached_boundary_f,
*                              gb_md_empty_location_f
*****************************************************************************/
U8* Send_Modbus_Data_Packets_ToServer(U8 md_logs, U8 *entryCount)
{
	int lcl_last_sent_loc = 0;                                              // Last location sent to server
	int lcl_last_loc = 0;                                                   // Last location where data was stored
	*entryCount = 0;                                                        // Initialize entry count to zero
	
	U8 lcl_eep_buf[5];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, 5);
	lcl_eep_status = EEPROM_Byte_Read(MD_STORED_LAST_ADDS, lcl_eep_buf, 5); // Read last stored location from EEPROM
	if (lcl_eep_status == 0)
	{
		/* Verify the CRC for last location value. */
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)            // Validate CRC of stored location
		{
			/* If CRC validated, update last loc with the flash value. */
			lcl_last_loc = lcl_eep_buf[0];							        // Get higher byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[1];	        // Get middle byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[2];	        // Get lower byte
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
			Print_Message("\nCRC validated. \nLast loc value is for modbus : ");
			Print_Number(lcl_last_loc);
			#endif
		}
		else
		{
			/* If CRC fails, update last loc with default value. */
			lcl_last_loc = MD_EEP_START_LOC;                                // Reset to start location on CRC failure
			Write_Last_Stored_Data_Packet_Location(lcl_last_loc);
			
			/* If CRC fails, update last sent location from start location */
			lcl_last_sent_loc = MD_EEP_START_LOC;                           // Reset sent location to start
			Write_Last_Sent_MD_Location(lcl_last_sent_loc);
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
			Print_Message("\nCRC failed. \nLast loc value is for modbus : ");
			Print_Number(lcl_last_loc);
			#endif
		}
	}
	else
	{
		__NOP();
	}
	
	memset(lcl_eep_buf, 0, 5);
	
	lcl_eep_status = EEPROM_Byte_Read(MD_SENT_LAST_ADDS, lcl_eep_buf, 5);   // Read last sent location from EEPROM
	if (lcl_eep_status == 0)
	{
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)            // Validate CRC of sent location
		{
			/* If CRC validated, update last sent loc with the flash value. */
			lcl_last_sent_loc = lcl_eep_buf[0];								// Get higher byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[1];	// Get middle byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[2];	// Get lower byte
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
			Print_Message("\nCRC Validated. \nLast sent modbus loc value is : ");
			Print_Number(lcl_last_sent_loc);
			#endif
		}
		else
		{
			/* If CRC fails, update last sent loc with default value. */
			lcl_last_sent_loc = MD_EEP_START_LOC;                           // Reset to start location on CRC failure
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
			Print_Message("\nCRC failed. \nLast sent modbus loc value is : ");
			Print_Number(lcl_last_sent_loc);
			#endif
		}
		
		U16 read_num_of_logs = 0;                                           // Number of log bytes to read from EEPROM

		
		if ((lcl_last_sent_loc < lcl_last_loc) && (gb_update_md_last_sent_loc_f == 0)) // Normal case: sent location is behind stored location
		{
			U16 lcl_diff_loc = 0;
			
			// Calculate number of pending MD bytes
			lcl_diff_loc = (lcl_last_loc - lcl_last_sent_loc);             // Calculate pending data bytes
			
			// Limit read size to configured MD log count
			if (lcl_diff_loc <= (md_logs * MD_BUFFER_LEN))                 // Check if pending data fits in requested count
			{
				read_num_of_logs = lcl_diff_loc;                           // Read all pending data
			}
			else
			{
				read_num_of_logs = (md_logs * MD_BUFFER_LEN);              // Read requested number of logs
			}
			
			static U8 lcl_log_array[EEPROM_PAGE_SIZE];                      // Static buffer to hold log data
			U8 lcl_eep_status = 0;
			memset(lcl_log_array, 0, EEPROM_PAGE_SIZE);
			lcl_eep_status = EEPROM_Byte_Read(lcl_last_sent_loc, lcl_log_array, read_num_of_logs); // Read log data from EEPROM
			if (lcl_eep_status == 0)
			{
				// Allow sending only if device is active
				if ((dm_dev_info.dev_sts == 1) && (dm_dev_info.pause_rsm == RESUME)) // Check if device is active and not paused
				{
					gb_update_md_last_sent_loc_f = 1;                       // Set flag to update last sent location after server response
					gb_send_modbus_data_to_serverf = 1;                     // Set flag to trigger data transmission
				}
				else
				__NOP();
				
				// Store number of bytes read for transmission
				gb_last_sent_md_bytes = read_num_of_logs;                  // Store number of bytes being sent for tracking
				
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
				Print_Message("\nSend MD data");
				#endif
				
				*entryCount = (read_num_of_logs / MD_BUFFER_LEN);           // Calculate number of complete log entries
				return lcl_log_array;                                       // Return pointer to log data buffer
			}
			else
			__NOP();
		}
		else if ((lcl_last_sent_loc > lcl_last_loc) && (gb_md_reached_boundary_f == 1) // Wrap-around case: reading from end of circular buffer
		&& (gb_update_md_last_sent_loc_f == 0))
		{
			U16 lcl_diff_loc = 0;
			lcl_diff_loc = (MD_EEP_END_LOC - lcl_last_sent_loc);           // Calculate remaining data till end of EEPROM
			
			if (lcl_diff_loc == 0)                                          // Reached exact end of EEPROM boundary
			{
				gb_md_reached_boundary_f = 0;                               // Clear boundary flag
				
				EEPROM_Byte_Write(MD_BOUNDARY_FLAG, &gb_md_reached_boundary_f, 1); // Update boundary flag in EEPROM
				lcl_last_sent_loc = MD_EEP_START_LOC;                       // Wrap around to start location
				Write_Last_Sent_MD_Location(lcl_last_sent_loc);
				gb_md_empty_location_f = 1;                                 // Set empty location flag
			}
			else if (lcl_diff_loc <= (md_logs * MD_BUFFER_LEN))            // Remaining data fits in requested count
			{
				read_num_of_logs = lcl_diff_loc;                           // Read all remaining data
			}
			else
			{
				read_num_of_logs = (md_logs * MD_BUFFER_LEN);              // Read requested number of logs
			}
			
			static U8 lcl_log_array[EEPROM_PAGE_SIZE];                      // Static buffer to hold log data
			U8 lcl_eep_status = 0;
			memset(lcl_log_array, 0, EEPROM_PAGE_SIZE);
			lcl_eep_status = EEPROM_Byte_Read(lcl_last_sent_loc, lcl_log_array, read_num_of_logs); // Read log data from EEPROM
			if (lcl_eep_status == 0)
			{
				if ((dm_dev_info.dev_sts == 1) && (dm_dev_info.pause_rsm == RESUME)) // Check if device is active and not paused
				{
					gb_update_md_last_sent_loc_f = 1;                       // Set flag to update last sent location after server response
					gb_send_modbus_data_to_serverf = 1;                     // Set flag to trigger data transmission
				}
				else
				__NOP();
				gb_last_sent_md_bytes = read_num_of_logs;                  // Store number of bytes being sent for tracking
				
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MD_SERVER)
				Print_Message("\nSend MD data");
				#endif
				
				// Calculate number of MD entries read
				*entryCount = (read_num_of_logs / MD_BUFFER_LEN);           // Calculate number of complete log entries
				return lcl_log_array;                                       // Return pointer to log data buffer
			}
			else
			__NOP();
		}
		else                                                                // No pending data to send
		{
			gb_md_empty_location_f = 1;                                     // Set flag indicating no data available
		}
	}
	else
	{
		__NOP();
	}
	return 0;                                                               // Return NULL if no data available
}


/*****************************************************************************
* Function Name  : Write_to_Register_Functionality
* Returns        : Nothing
* Arguments      : U8 --> dpt number, U8* --> data buffer to write
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Writes data to Modbus register using configured data
*                  packet parameters. Validates slave status before writing.
*
* Notes          : Supports function codes 5, 6, and 15 (write operations).
* Global Variables Affected : gb_mb_write_processing_f, mb_poll_dpt,
*                              modbus_op_state
*****************************************************************************/
void Write_to_Register_Functionality(U8 dpt_num,U8 *data_buf)
{
	#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_WR_DATA)
	Print_Message("\nWriting to Register");
	#endif
	
	U8 num_of_reg = 0;                                                      // Number of registers to write
	U8 byte_len = 0;                                                        // Data byte length
	if(gb_mb_write_processing_f == 1)                                       // Check if write operation already in progress
	{
		//Write modbus is in process
		return;                                                             // Exit to prevent concurrent write operations
	}
	/* Load DP configuration from EEPROM */
	Segregate_DPT_Data_To_Buffer(dpt_num);                                  // Extract configuration for specified data packet
	
	#if 1
	Print_Message("\ndata length : ");
	Print_Number(mp_config.dp_data_len);
	Print_Message("\nSlave address : ");
	Print_Number(mp_config.dp_slave_addr);
	Print_Message("\nFunction code : ");
	Print_Number(mp_config.dp_mb_fun);
	Print_Message("\nRegister address : ");
	Print_Number(mp_config.dp_reg_addr);
	#endif
	
	if(mp_config.dp_mb_fun == 15)                                           // Function code 15: Write Multiple Coils
	{
		num_of_reg = mp_config.dp_data_len;                                 // Number of coils to write
		byte_len = 0;                                                       // Byte length set to 0 for coil writes
	}
	else                                                                    // Function code 5 or 6: Write Single Coil/Register
	{
		num_of_reg = 1;                                                     // Write single register/coil
		byte_len   = 2;                                                     // Standard 2-byte length for single write
	}
	/* Validate slave and function */
	if ((mp_config.dp_slave_addr <= 0) || (mp_config.dp_slave_addr > TOTAL_SLAVES) || (mp_config.dp_mb_fun <= 4)) // Check valid slave address and write function code
	{
		#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_WR_DATA)
		Print_Message("\nInvalid Slave or Function for write");
		#endif
		return;                                                             // Exit if invalid configuration
	}
	if (active_slaves[mp_config.dp_slave_addr - 1] == 0)                   // Check if target slave is active
	{
		#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_WR_DATA)
		Print_Message("\nSlave Not Active");
		#endif
		return;                                                             // Exit if slave is not responding
	}
	#if DEBUG_MODBUS_DATA || DEBUG_WR_DATA
	Print_Message("\nSlave is Active");
	#endif
	
	if((mp_config.dp_mb_fun == 5) || (mp_config.dp_mb_fun == 6) || (mp_config.dp_mb_fun == 15)) // Validate function code is for write operations
	{
		gb_mb_write_processing_f = 1;                                       // Set flag to indicate write in progress
		mb_poll_dpt = 0;                                                    // Pause DPT polling during write operation
		modbus_op_state = MB_OP_WRITE;                                      // Set operation state to write mode
		/* Build and send Modbus frame */
		modbus_frame(mp_config.dp_slave_addr,mp_config.dp_mb_fun,mp_config.dp_reg_addr,num_of_reg,byte_len,data_buf); // Build Modbus write request
		send_frame();                                                       // Transmit write request to slave
	}
	
}



/*
order
 0 : MSB/MSW First
 1 : MSB/LSW First
 2 : LSB/LSW First
 3 : LSB/MSW First
*/
/*****************************************************************************
* Function Name  : Get_Modbus_Data
* Returns        : Nothing
* Arguments      : uint8_t * -->destination array, uint8_t * -->source array,
                   uint8_t -- > byte order.
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Function is to get data as ber byte order.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void Get_Modbus_Data(uint8_t *dst, uint8_t *src, uint8_t order)
{
    switch (order)
    {
        case 0: // MSB first, MSW first (AB CD EF GH)
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
            break;
		
		case 1: // MSB first, LSW first (EF GH AB CD)
			dst[0] = src[2];
			dst[1] = src[3];
			dst[2] = src[0];
			dst[3] = src[1];
			break;
		
		case 2: // LSB first, LSW first (GH EF CD AB)
			dst[0] = src[3];
			dst[1] = src[2];
			dst[2] = src[1];
			dst[3] = src[0];
			break;
		
        case 3: // LSB first, MSW first (CD AB GH EF)
            dst[0] = src[1];
            dst[1] = src[0];
            dst[2] = src[3];
            dst[3] = src[2];
            break;

        default:
            /* Safe fallback */
            dst[0] = src[0];
            dst[1] = src[1];
            dst[2] = src[2];
            dst[3] = src[3];
            break;
    }
}



/*****************************************************************************
 * Function name : Store_DPT_Into_Memory.
 * Returns		 : nothing.
 * Arguments     : uint8_t --> storage type, uint8_t* --> dpt value.
 * Created by	 : Shirisha Ankam.
 * Date created	 : 18/11/2025.
 *
 * Description	 : Stores data packet to memory based on configured event type:
 *                 COV (Change on Value), Specific Time, or Periodic.
 *
 * Notes	     : Uses EEPROM to track last stored values and timestamps.
 * Global Variables Affected : None directly (uses current_dp_number)
 *****************************************************************************/    
void Store_DPT_Into_Memory(uint8_t store_type, uint8_t *dpt_value)
{
	uint8_t cur_hr  = 0;
	uint8_t cur_min = 0;
	uint8_t cur_date = 0;                                                   // dd/mm/yy/day
	U8 value_changed = 0;                                                   // Flag to indicate value changed from last stored
	uint8_t dpt_number = current_dp_number - 1;                             // Get current data packet number (0-indexed)
	switch (store_type)
	{
		case 0:   // COV = Change On Value
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nStorage Based on change in value for DPT :");
			Print_Number(dpt_number);
			#endif
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nCurrent DPT value : ");
			for(U8 dp = 0; dp < 4; dp++)
			{
				Print_ASCII_HEX(dpt_value[dp]);
				Print_Message(",");
			}
			#endif
			
			U8 lcl_eep_buf[MP_LAST_VAL_UPDATE_SIZE];
			U8 lcl_eep_status = 0;
			memset(lcl_eep_buf, 0, MP_LAST_VAL_UPDATE_SIZE);
			int read_loc = (MP_LAST_VAL_UPDATE_LOC + ((dpt_number - 1) * MP_LAST_VAL_UPDATE_SIZE)); // Calculate EEPROM location for this DPT's last value
			lcl_eep_status = EEPROM_Byte_Read(read_loc, lcl_eep_buf, MP_LAST_VAL_UPDATE_SIZE); // Read last stored value from EEPROM
			
			U8 last_dpt[MP_LAST_VAL_UPDATE_SIZE];
			memset(last_dpt,0,MP_LAST_VAL_UPDATE_SIZE);
			memcpy(last_dpt,lcl_eep_buf,MP_LAST_VAL_UPDATE_SIZE);          // Copy last value to local buffer
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nLast value read loc for this dpt : ");
			Print_Number(read_loc);
			Print_Message("\nLast DPT value is : ");
			for(U8 dp = 0; dp < 4; dp++)
			{
				Print_ASCII_HEX(last_dpt[dp]);
				Print_Message(",");
			}
			#endif

			for (U8 i = 0; i < MP_LAST_VAL_UPDATE_SIZE; i++)               // Compare current value with last stored value
			{
				if (dpt_value[i] != last_dpt[i])                            // Check if any byte differs
				{
					value_changed = 1;                                      // Mark value as changed
					break;
				}
				
			}
			
			if (value_changed == 1)                                         // Only store if value has changed
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nValue Changed - Stored");
				#endif
				
				EEPROM_Byte_Write(read_loc,dpt_value, MP_LAST_VAL_UPDATE_SIZE); // Update last value in EEPROM
				
				Store_Modbus_Data_Packets(gb_rtcDateArr,gb_rtcTimeArr,dpt_number,dpt_value); // Store to memory with timestamp
			}
			else
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nNo Change in Value (COV)");
				#endif
			}
			break;
		}
		
		case 1:   // Specific Time (store once per day at HH:MM)
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nStorage Based on specific time");
			#endif

			uint8_t cur_hr   = gb_rtcTimeArr[0];                            // Get current hour
			uint8_t cur_min  = gb_rtcTimeArr[1];                            // Get current minute
			uint8_t cur_date = gb_rtcDateArr[0];                            // Get current date (1–31)

			uint8_t sch_hr  = mp_config.dp_hours;                           // Get scheduled hour from config
			uint8_t sch_min = mp_config.dp_min;                             // Get scheduled minute from config

			int read_loc = (MP_SPECIFIC_TIME_LOC + ((dpt_number - 1) * MP_SPECIFIC_TIME_SIZE)); // Calculate EEPROM location for this DPT's last stored date
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nLast stored specific time loc for this dpt : ");
			Print_Number(read_loc);
			#endif
			
			U8 lcl_eep_buf[1] = {0};
			/* Read last stored date from EEPROM */
			EEPROM_Byte_Read(read_loc, lcl_eep_buf, 1);                     // Read last date when data was stored
			uint8_t last_date = lcl_eep_buf[0];

			if (last_date == 0xFF)                                          // Check if EEPROM is erased (fresh state)
			last_date = 0;
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\n| Last Date is:    |");
			Print_Number(last_date);
			Print_Message("\n| Current Date is: |");
			Print_Number(cur_date);
			#endif
			/* Already stored today? */
			if (last_date == cur_date)                                      // Check if already stored for today
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nAlready stored today");
				#endif
				break;                                                      // Skip storage if already done today
			}

			/* Has scheduled time been crossed? */
			U8 time_crossed = 0;                                            // Flag to check if scheduled time passed
			if (cur_hr > sch_hr)                                            // Current hour is past scheduled hour
			time_crossed = 1;
			else if ((cur_hr == sch_hr) && (cur_min >= sch_min))            // Current time is at or past scheduled time
			time_crossed = 1;

			/* Store FIRST time after scheduled time */
			if (time_crossed)                                               // Only store if scheduled time has been reached
			{
				Store_Modbus_Data_Packets(gb_rtcDateArr,gb_rtcTimeArr,dpt_number,dpt_value); // Store to memory with timestamp

				/* Mark stored for today */
				EEPROM_Byte_Write(read_loc, &cur_date, MP_SPECIFIC_TIME_SIZE); // Update last stored date
				
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nStored (Specific Time)");
				#endif
			}
			break;
		}

		case 2:   // PERIODIC (every N minutes)
		{
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nStorage Based on periodic time");
			#endif

			uint8_t cur_hr  = gb_rtcTimeArr[0];                             // Get current hour
			uint8_t cur_min = gb_rtcTimeArr[1];                             // Get current minute

			/* Convert to minute-of-day (0–1439) */
			uint16_t cur_min_of_day = (cur_hr * 60) + cur_min;             // Convert current time to minutes since midnight
			
			uint8_t buf[2];
			buf[0] = (uint8_t)(cur_min_of_day >> 8);                        // Store high byte of minute-of-day
			buf[1] = (uint8_t)(cur_min_of_day);                             // Store low byte of minute-of-day
			
			/* Period in minutes */
			uint16_t period = (mp_config.dp_hours * 60) + mp_config.dp_min; // Calculate storage period in minutes

			if (period == 0 && period >= 1440)                              // Validate period (must be 1-1439 minutes)
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nInvalid Period");
				#endif
				
				break;                                                      // Skip if invalid period
			}

			/* EEPROM location per DP */
			int read_loc = MP_PERIODIC_TIME_LOC + ((dpt_number - 1) * MP_PERIODIC_TIME_SIZE); // Calculate EEPROM location for this DPT's last stored time
			
			#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
			Print_Message("\nLast stored periodic time loc for this dpt : ");
			Print_Number(read_loc);
			#endif
				
			/* Read last stored minute */
			U8 lcl_eep_buf[MP_PERIODIC_TIME_SIZE];
			memset(lcl_eep_buf, 0, MP_PERIODIC_TIME_SIZE);
			EEPROM_Byte_Read(read_loc, lcl_eep_buf, MP_PERIODIC_TIME_SIZE); // Read last stored minute-of-day
			uint16_t last_min_of_day = ((uint16_t)lcl_eep_buf[0] << 8) |   // Reconstruct 16-bit minute-of-day value
			(uint16_t)lcl_eep_buf[1];
			
			/* First time (EEPROM empty or invalid) */
			if (last_min_of_day == 0)                                       // Check if first time or EEPROM erased
			{
				Store_Modbus_Data_Packets(gb_rtcDateArr,gb_rtcTimeArr,dpt_number,dpt_value); // Store initial value
	  
				EEPROM_Byte_Write(read_loc, buf, MP_PERIODIC_TIME_SIZE);    // Save current time as last stored time
				
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nStored (First Periodic)");
				#endif
				
				break;
			}

			/* Calculate elapsed minutes safely */
			uint16_t elapsed;                                               // Minutes elapsed since last storage

			if (cur_min_of_day >= last_min_of_day)                          // No midnight rollover
			{
				elapsed = cur_min_of_day - last_min_of_day;                 // Simple subtraction
			}
			else
			{
				/* Midnight rollover */
				elapsed = (1440 - last_min_of_day) + cur_min_of_day;        // Handle day boundary crossing
			}

			/* Store if period reached */
			if (elapsed >= period)                                          // Check if enough time has passed
			{
				Store_Modbus_Data_Packets(gb_rtcDateArr,gb_rtcTimeArr,dpt_number,dpt_value); // Store value

			    EEPROM_Byte_Write(read_loc, buf, MP_PERIODIC_TIME_SIZE);    // Update last stored time
				
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nStored (Periodic)");
				#endif
			}
			else
			{
				#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
				Print_Message("\nelapsed time pending : ");
				Print_Number(elapsed);
				#endif
				__NOP();
			}

			break;
		}
		default:
		#if (DEBUG_ALL || DEBUG_MODBUS_DATA || DEBUG_MODBUS_STORAGE)
		Print_Message("\nInvalid Event Type");
		#endif
		break;
	}
}



/*****************************************************************************
 * Function name : Record_Modbus_Exception.
 * Returns		 : void.
 * Arguments     : U8 --> slave address, U8 --> exception code.
 * Created by	 :
 * Date created	 : 23/10/2026.
 *
 * Description	 : Collects the exception code returned by a slave during the
 *                 current DPT poll cycle. Last exception of the cycle is kept.
 *
 * Notes	     : Reported to server by Evaluate_Modbus_Exception_COV().
 * Global Variables Affected : gb_mb_exc_cycle
 *****************************************************************************/
void Record_Modbus_Exception(U8 slave_addr, U8 exc_code)
{
	if (gb_me_check_state != ME_CHECK_ACTIVE)                               // Not yet enabled after power on (CS frame not sent)
	{
		return;
	}
	
	if ((slave_addr == 0) || (slave_addr > TOTAL_SLAVES) || (exc_code == 0)) // Validate slave address range and code
	{
		return;
	}

	#if (DEBUG_ALL || DEBUG_MB_EXC)
	Print_Message("\nModbus exception, Slave : ");
	Print_Number(slave_addr);
	Print_Message(" Code : ");
	Print_Number(exc_code);
	#endif

	gb_mb_exc_cycle[slave_addr - 1] = exc_code;                             // Store exception code for this slave
}



/*****************************************************************************
 * Function name : Evaluate_Modbus_Exception_COV.
 * Returns		 : void.
 * Arguments     : nothing.
 * Created by	 :
 * Date created	 : 23/10/2026.
 *
 * Description	 : Called after every complete DPT poll cycle. Compares the
 *                 exceptions collected in this cycle with the last reported
 *                 status and requests an "ME" frame only on change of value.
 *                 Slave whose exception is cleared is reported as [SA,0]
 *                 until the server acknowledges it.
 *
 * Notes	     : After power on, checking starts with the first complete cycle
 *                 after the CS frame is acknowledged by the server.
 * Global Variables Affected : gb_mb_exc_status, gb_mb_exc_cycle, gb_mb_exc_clr,
 *                             gb_send_me_to_server, gb_me_change_cnt
 *****************************************************************************/
void Evaluate_Modbus_Exception_COV(void)
{
	if (gb_me_check_state != ME_CHECK_ACTIVE)                               // Checking not active yet
	{
		if (gb_me_check_state == ME_CHECK_ARMED)                            // CS sent, start with next complete cycle
		{
			gb_me_check_state = ME_CHECK_ACTIVE;
		}
		else
		{
			__NOP();
		}
		memset(gb_mb_exc_cycle, 0, TOTAL_SLAVES);
		return;
	}
	
	if (memcmp(gb_mb_exc_cycle, gb_mb_exc_status, TOTAL_SLAVES) != 0)       // Change of value
	{
		taskENTER_CRITICAL();                                               // Frame is built from TCP task
		for (U8 idx = 0; idx < TOTAL_SLAVES; idx++)
		{
			if (gb_mb_exc_cycle[idx] != 0)                                  // Slave in exception, reported with its code
			{
				gb_mb_exc_clr[idx] = 0;
			}
			else if (gb_mb_exc_status[idx] != 0)                             // Exception cleared
			{
				gb_mb_exc_clr[idx] = 1;                                     // Report as [SA,0]
			}
			else
			{
				__NOP();
			}
		}
		memcpy(gb_mb_exc_status, gb_mb_exc_cycle, TOTAL_SLAVES);            // Update reported status
		gb_me_change_cnt++;                                                 // Mark new status to be acknowledged
		gb_send_me_to_server = FLAG_SET;                                    // Send "ME" in next frame
		taskEXIT_CRITICAL();

		#if (DEBUG_ALL || DEBUG_MB_EXC)
		Print_Message("\nModbus exception status changed, send ME");
		#endif
	}
	else
	{
		__NOP();
	}

	memset(gb_mb_exc_cycle, 0, TOTAL_SLAVES);                               // Start collecting for next cycle
}

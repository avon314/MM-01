/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mm_app.c
* Created By	: Shirisha  Ankam.
* Created Date	: 30/07/2025.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*                   128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 30/07/2025.
* Changes		: NA.
*****************************************************************************/

/***** User defined Includes *****/
#include "asf.h"
#include "mm_app.h"
#include "user_uart.h"
#include "ext_eeprom_1.h"
#include "CRC.h"
#include <string.h>
#include "memory_storage.h"
#include "definitions.h"
#include "json_extraction.h"
#include "ext_eeprom.h"
#include "onboard_key.h"
#include "user_rtc.h"
#include "config_mode.h"
#include "core_sntp.h"
#include "memory_storage.h"
#include "input_output.h"
#include "digital_input.h"
#include "Modbus.h"
#include "modbus_app.h"
/***** End for User defined Includes *****/

/**** Declaration for global variables ****/
DI dm_dev_info;
U8 gb_server_poll_freq = 0;
U8 gb_start_sending_dpt = 0;
U8 gb_send_mb_data = 1;
U8 gb_reinit_uart = 0;
U8 gb_requesting_cancel = 0;
U8 gb_ac_power_f = 1;
int gb_last_sent_is_bytes = 0;
int gb_last_sent_md_bytes = 0;
U8 gb_dev_connect_f = 0;
U8 gb_last_dt_tm[6] = {0};
U8 gb_rtc_send_to_server_f = 1;
U8 gb_is_reached_boundary_f = 0;
U8 gb_update_is_last_sent_loc_f = 0;
U8 gb_update_md_last_sent_loc_f = 0;
U8 gb_frame_logs_remaining = 0;

U8 gb_send_input_data_to_serverf = 0;
U8 gb_send_modbus_data_to_serverf = 0;	
U8 gb_md_empty_location_f = 0;
U8 gb_md_reached_boundary_f = 0;

int gb_data_packets = 1;
extern U8 gb_network_up_f;
int csnMS = 0, csnMS_f = 0;
int mb_poll_timer = 0;
U8 mb_poll_dpt = 1;

/** input output variables ***/
U8 input_data[MAX_INPUTS] = {0};
U8 ipTemp_data[MAX_INPUTS] = {0};
U8 gb_output_state[MAX_OUTPUTS] = {0};
U8 gb_op_send_server_f = 0;
U8 gb_op_detected_f = 0;
U8 gb_op_at_pwr_on = 0;
U8 gb_current_op_st[MAX_OUTPUTS] = {0};
U8 gb_previous_op_st[MAX_OUTPUTS] = {0};
/** End of input output variables ***/
	 
char jsonBuffer[SERVER_DATA_SIZE];  // Adjust the size as needed

/***** SNTP Variables *****/
U8 ntp_sync_at_time[2] = {0, 0};	// Set Hour to 0 and Minute to 0 so that, time can sync in the midnight (12:00 Am).
U8 sntp_create_task_f = 0;
U8 clr_pwr_sntp_task_f = 0;
/***** End of SNTP Variables *****/

U8 gb_req_erp_time_f = 0;	// It should be zero @ power on

/**** End for Declaration for global variables ****/

extern void Task_Get_SNTP_Data(void *pvParameters);
extern xSemaphoreHandle xSemaphore;



/*****************************************************************************
* Function name	: Cancel_The_Device_From_Server
* Returns		: Nothing.
* Arguments    	: void.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/01/2025.
* Description	: Cancel the device from the server if requested..
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Cancel_The_Device_From_Server(void)
{
	/* Read boot flag value from the mem before erasing chip */
	U8 boot_flag[1];
	U8 lcl_eep_buf[1] = {0};
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(BL_APP_STATUS_LOC, lcl_eep_buf, 1);
	if (lcl_eep_status == 0)
	{
		boot_flag[0] = lcl_eep_buf[0];
	}
	
	/* Read Current DI information */
	U8 di_buffer[DI_DATA_SIZE];
	memset(di_buffer, 0, DI_DATA_SIZE);
	Read_DI_Data_From_EEPROM(di_buffer);	// Call this function to update RAM variables.
	
	// Completely erase the chip.
	EEPROM_Chip_Erase();
	
	/* Re write the boot flag value to the mem after erasing the chip */
	EEPROM_Byte_Write(BL_APP_STATUS_LOC, boot_flag, 1);
	
	/* Re write the DI information after an erase. */
	EEPROM_Byte_Write(DI_EEP_START_LOC, di_buffer, DI_DATA_SIZE);

	// Call the software reset function once canceled the device state.
	RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
}



/*****************************************************************************
* Function name	: Update_Last_Sent_Bytes_IS
* Returns		: Nothing.
* Arguments    	: void.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/01/2025.
* Description	: Update last sent location in the memory..
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Update_Last_Sent_Bytes_IS(void)
{
	int lcl_last_sent_loc = 0;
	
	U8 lcl_eep_buf[5];
	memset(lcl_eep_buf, 0, 5);
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(IS_SENT_LAST_ADDS, lcl_eep_buf, 5);
	if (lcl_eep_status == 0)
	{
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)
		{
			/* If CRC validated, update last sent loc with the flash value. */
			lcl_last_sent_loc = lcl_eep_buf[0];								// Get higher byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[1];	 // Get middle byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[2];	 // Get lower byte
			
			// 			Print_Message("\nCRC Validated. \nLast sent loc value is : ");
			// 			Print_Number(lcl_last_sent_loc);
		}
		else
		{
			/* If CRC fails, update last sent loc with default value. */
			lcl_last_sent_loc = IS_EEP_START_LOC;
			
			// 			Print_Message("\nCRC failed. \nLast sent loc value is : ");
			// 			Print_Number(lcl_last_sent_loc);
		}
		lcl_last_sent_loc += gb_last_sent_is_bytes;
		Write_Last_Sent_IS_Location(lcl_last_sent_loc);
	}
	else
	__NOP();
}




/*****************************************************************************
* Function name	: Write_Last_Sent_IS_Location
* Returns		: Nothing.
* Arguments    	: int -> last stored location
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/01/2025.
* Description	: To store last stored location into memory.
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Write_Last_Stored_Location_To_Flash(int rcv_last_store_loc)
{
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
	EEPROM_Byte_Write(IS_STORED_LAST_ADDS, lcl_lst_storeArr, 5);
}


/*****************************************************************************
* Function name	: Write_Last_Sent_IS_Location
* Returns		: Nothing.
* Arguments    	: int -> last sent location
* Created by	: Ranjitkumar Ainapure.
* Date created	: 23/01/2025.
* Description	: To store last sent location into memory.
* Notes			: NA
* Global Variables Affected : NA.
******************************************************************************/
void Write_Last_Sent_IS_Location(int rcv_last_sent_loc)
{
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
	EEPROM_Byte_Write(IS_SENT_LAST_ADDS, lcl_lst_sentArr, 5);
}



/*****************************************************************************
* Function name	: void Update_Poll_Frequency(uint16_t dp_val)
* Returns		: Nothing.
* Arguments    	: NA
* Created by	: Anup Silvan Mascarenhas.
* Date created	: 10/11/2025.
* Updated By	: Ranjitkumar Ainapure.
* Date Updated	: 23/01/2025.
* Description	: To store and get Dynamic Poll value from Flash Memory
* Notes			: NA
* Global Variables Affected : gb_server_poll_freq  ---> Get poll value from Flash data
******************************************************************************/
void Update_Poll_Frequency(uint16_t dp_val)
{
	if (dp_val > 101)
	{
		gb_server_poll_freq = 2;	// Poll with 2 sec.
	}
	else
	{
		if (dm_dev_info.poll_freq == 0)
		gb_server_poll_freq = 30;	// 30 Secs.
		else if (dm_dev_info.poll_freq == 1)
		gb_server_poll_freq = 60;	// 60 Secs.
		else if (dm_dev_info.poll_freq == 2)
		gb_server_poll_freq = 2;	// 1 Sec. (If received 1 sec request, adjusted to 2 secs, to neglect the issues.)
		else if (dm_dev_info.poll_freq == 3)
		gb_server_poll_freq = 5;	// 5 Secs.
		else if (dm_dev_info.poll_freq == 4)
		gb_server_poll_freq = 10;	// 10 Secs.
		else
		__NOP();
		
		//gb_server_poll_freq = ((dm_dev_info.poll_freq * 30) + 30);
	}
}


/*****************************************************************************
* Function Name  : Flash_Store_InputState_Log
* Returns        : Nothing
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Function is responsible to send change in input state to the server.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void Flash_Store_InputState_Log( U8 *day_date, U8 *time_arr, U8 input_num, U8 input_state)
{
	if (input_num < 0) 
	return;	// Check valid input number is received or not?
	
	U8 tl_array[IS_BUFFER_LEN] = {0};
	U8 lcl_idx = 0;
	U8 csnIdx = 0;
	int lcl_last_loc = 0;
	
	tl_array[lcl_idx ++] = day_date[0];
	tl_array[lcl_idx ++] = day_date[1];
	tl_array[lcl_idx ++] = day_date[2];
	tl_array[lcl_idx ++] = day_date[3];
	tl_array[lcl_idx ++] = time_arr[0];
	tl_array[lcl_idx ++] = time_arr[1];
	tl_array[lcl_idx ++] = time_arr[2];
	tl_array[lcl_idx ++] = input_num;
	tl_array[lcl_idx ++] = input_state;
	
	// Print packed array
	#if (DEBUG_ALL || DEBUG_IP_EXT)
	Print_Message("\nInput Log Data: ");
	for (int i = 0; i < IS_BUFFER_LEN; i++)
	{
		Print_Number(tl_array[i]);
		Print_Message(",");
	}
	Print_Message("\n");
	#endif
	
	/* Read last location stored in the flash. */
	U8 lcl_eep_buf[5];
	memset(lcl_eep_buf, 0, 5);
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(IS_STORED_LAST_ADDS, lcl_eep_buf, 5);
	if (lcl_eep_status == 0)
	{
		/* Verify the CRC for last location value. */
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)
		{
			/* If CRC validated, update last loc with the flash value. */
			lcl_last_loc = lcl_eep_buf[0];							 // Get higher byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[1];	 // Get middle byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[2];	 // Get lower byte
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
 			Print_Message("\nCRC validated. \nLast loc value is for input : ");
 			Print_Number(lcl_last_loc);
		    #endif
		}
		else
		{
			/* If CRC fails, update last loc with default value. */
			lcl_last_loc = IS_EEP_START_LOC;
			
			/****************************************************************/
			/*****
			* If CRC fails, last sent location should be updated, it should be
			* as equal as or less than last stored location 
			*****/
			int lcl_last_sent_loc = IS_EEP_START_LOC;
			Write_Last_Sent_IS_Location(lcl_last_sent_loc);
			/****************************************************************/
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
 			Print_Message("\nCRC failed. \nLast loc value is for input : ");
 			Print_Number(lcl_last_loc);
			#endif
		}
		
		if (lcl_last_loc >= IS_EEP_END_LOC)
		{
			/* If reserved locations filled for TL, update last loc with default value / start value. */
			lcl_last_loc = IS_EEP_START_LOC;
			gb_is_reached_boundary_f = 1;
			EEPROM_Byte_Write(IS_BOUNDARY_FLAG, &gb_is_reached_boundary_f, 1);
		}
		
		/* Store TL at flash mem */
		EEPROM_Byte_Write(lcl_last_loc, tl_array, IS_BUFFER_LEN);
		lcl_last_loc += IS_BUFFER_LEN;
		Write_Last_Stored_Location_To_Flash(lcl_last_loc);
	}
	else
	{
		__NOP();
	}
}


/*****************************************************************************
* Function Name  : Send_Input_Log_ToServer
* Returns        : Nothing
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Reads stored input state logs from EEPROM and prepares
*                  JSON data packets to send to server. Handles circular
*                  buffer wrap-around and tracks last sent location.
*
* Notes          : Sends empty JSON if no logs available.
* Global Variables Affected : gb_update_is_last_sent_loc_f,
*                              gb_send_input_data_to_serverf,
*                              gb_last_sent_is_bytes, gb_is_reached_boundary_f,
*                              gb_md_empty_location_f, jsonBuffer
*****************************************************************************/
void Send_Input_Log_ToServer(void)
{
	int lcl_last_sent_loc = 0;                                              // Last location sent to server
	int lcl_last_loc = 0;                                                   // Last location where data was stored
	U8 lcl_gen_empty_json_f = 0;                                            // Flag to generate empty JSON if no data
	
	U8 lcl_eep_buf[5];
	memset(lcl_eep_buf, 0, 5);
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(IS_STORED_LAST_ADDS, lcl_eep_buf, 5); // Read last stored location from EEPROM
	if (lcl_eep_status == 0)
	{
		/* Verify the CRC for last location value. */
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)
		{
			/* If CRC validated, update last loc with the flash value. */
			lcl_last_loc = lcl_eep_buf[0];							        // Get higher byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[1];	        // Get middle byte
			lcl_last_loc = (lcl_last_loc << 8 ) + lcl_eep_buf[2];	        // Get lower byte
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\nCRC validated. \nLast loc value is for input: ");
			Print_Number(lcl_last_loc);
			#endif
		}
		else
		{
			/* If CRC fails, update last loc with default value. */
			lcl_last_loc = IS_EEP_START_LOC;                                // Reset to start location on CRC failure
			Write_Last_Stored_Location_To_Flash(lcl_last_loc);
			
			/* If CRC fails, update last sent location from start location */
			lcl_last_sent_loc = IS_EEP_START_LOC;                           // Reset sent location to start
			Write_Last_Sent_IS_Location(lcl_last_sent_loc);
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\nCRC failed. \nLast input loc value is : ");
			Print_Number(lcl_last_loc);
			#endif
		}
	}
	else
	{
		__NOP();
	}
	
	memset(lcl_eep_buf, 0, 5);
	lcl_eep_status = EEPROM_Byte_Read(IS_SENT_LAST_ADDS, lcl_eep_buf, 5);   // Read last sent location from EEPROM
	if (lcl_eep_status == 0)
	{
		if (CRC_Calculate_Check(lcl_eep_buf, 3, CRC_CHECK) == 1)
		{
			/* If CRC validated, update last sent loc with the flash value. */
			lcl_last_sent_loc = lcl_eep_buf[0];								// Get higher byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[1];	// Get middle byte
			lcl_last_sent_loc = (lcl_last_sent_loc << 8 ) + lcl_eep_buf[2];	// Get lower byte
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\nCRC Validated. \nLast sent input loc value is : ");
			Print_Number(lcl_last_sent_loc);
			#endif
		}
		else
		{
			/* If CRC fails, update last sent loc with default value. */
			lcl_last_sent_loc = IS_EEP_START_LOC;                           // Reset to start location on CRC failure
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\nCRC failed. \nLast sent input loc value is : ");
			Print_Number(lcl_last_sent_loc);
			#endif
		}
		
		U16 read_num_of_logs = 0;                                           // Number of log bytes to read from EEPROM
		
		
		if ((lcl_last_sent_loc < lcl_last_loc) && (gb_update_is_last_sent_loc_f == 0)) // Normal case: sent location is behind stored location
		{
			U16 lcl_diff_loc = 0;
			lcl_diff_loc = (lcl_last_loc - lcl_last_sent_loc);             // Calculate pending data bytes
			
			if (lcl_diff_loc <= (MAX_NUM_IS_LOGS * IS_BUFFER_LEN))         // Check if pending data fits in single packet
			{
				read_num_of_logs = lcl_diff_loc;                           // Read all pending data
			}
			else
			{
				read_num_of_logs = (MAX_NUM_IS_LOGS * IS_BUFFER_LEN);      // Read maximum allowed logs per packet
			}
			
			U8 lcl_eep_buf[EEPROM_PAGE_SIZE];
			memset(lcl_eep_buf, 0, EEPROM_PAGE_SIZE);
			U8 lcl_eep_status = 0;
			lcl_eep_status = EEPROM_Byte_Read(lcl_last_sent_loc, lcl_eep_buf, read_num_of_logs); // Read log data from EEPROM
			if (lcl_eep_status == 0)
			{
				if ((dm_dev_info.dev_sts == 1) && (dm_dev_info.pause_rsm == RESUME)) // Check if device is active and not paused
				{
					gb_update_is_last_sent_loc_f = 1;                       // Set flag to update last sent location after server response
					gb_send_input_data_to_serverf = 1;                      // Set flag to trigger data transmission
				}
				else
				__NOP();
				
				gb_last_sent_is_bytes = read_num_of_logs;                  // Store number of bytes being sent for tracking
				
				
				/* Create a JSON frame with log data */
				Create_JSON_Frame(gs_uc_mac_address, gb_dev_ipAddress, MODEL_TYPE, DEVICE_TYPE, gb_last_dt_tm, gb_data_packets,
				lcl_eep_buf, (read_num_of_logs / (IS_BUFFER_LEN)), jsonBuffer, sizeof(jsonBuffer));
			}
		}
		else if ((lcl_last_sent_loc > lcl_last_loc) && (gb_is_reached_boundary_f == 1) // Wrap-around case: reading from end of circular buffer
		&& (gb_update_is_last_sent_loc_f == 0))
		{
			U16 lcl_diff_loc = 0;
			lcl_diff_loc = (IS_EEP_END_LOC - lcl_last_sent_loc);           // Calculate remaining data till end of EEPROM
			
			if (lcl_diff_loc == 0)                                          // Reached exact end of EEPROM boundary
			{
				gb_is_reached_boundary_f = 0;                               // Clear boundary flag
				
				EEPROM_Byte_Write(IS_BOUNDARY_FLAG, &gb_is_reached_boundary_f, 1); // Update boundary flag in EEPROM
				lcl_last_sent_loc = IS_EEP_START_LOC;                       // Wrap around to start location
				Write_Last_Sent_IS_Location(lcl_last_sent_loc);
				lcl_gen_empty_json_f = 1;                                   // Generate empty JSON for this cycle
				gb_md_empty_location_f = 1;
			}
			else if (lcl_diff_loc <= (MAX_NUM_IS_LOGS * IS_BUFFER_LEN))    // Remaining data fits in single packet
			{
				read_num_of_logs = lcl_diff_loc;                           // Read all remaining data
			}
			else
			{
				read_num_of_logs = (MAX_NUM_IS_LOGS * IS_BUFFER_LEN);      // Read maximum allowed logs per packet
			}
			
			U8 lcl_eep_buf[EEPROM_PAGE_SIZE];
			memset(lcl_eep_buf, 0, EEPROM_PAGE_SIZE);
			U8 lcl_eep_status = 0;
			lcl_eep_status = EEPROM_Byte_Read(lcl_last_sent_loc, lcl_eep_buf, read_num_of_logs); // Read log data from EEPROM
			if (lcl_eep_status == 0)
			{
				if ((dm_dev_info.dev_sts == 1) && (dm_dev_info.pause_rsm == RESUME)) // Check if device is active and not paused
				{
					gb_update_is_last_sent_loc_f = 1;                       // Set flag to update last sent location after server response
					gb_send_input_data_to_serverf = 1;                      // Set flag to trigger data transmission
				}
				else
				__NOP();
				gb_last_sent_is_bytes = read_num_of_logs;                  // Store number of bytes being sent for tracking
				
				/* Create a JSON frame with log data */
				Create_JSON_Frame(gs_uc_mac_address, gb_dev_ipAddress, MODEL_TYPE, DEVICE_TYPE, gb_last_dt_tm, gb_data_packets,
				lcl_eep_buf, (read_num_of_logs / (IS_BUFFER_LEN)), jsonBuffer, sizeof(jsonBuffer));
			}
		}
		else                                                                // No pending data to send
		{
			lcl_gen_empty_json_f = 1;                                       // Set flag to generate empty JSON
			gb_md_empty_location_f = 1;
			__NOP();
		}
		
		//lcl_gen_empty_json_f = 1;
		if ((lcl_gen_empty_json_f == 1) && (gb_md_empty_location_f == 1))  // Generate empty JSON when no data available
		{
			lcl_gen_empty_json_f = 0;                                       // Clear flag after use
			
			U8 dummy_arr[1];
			/* Create a JSON frame with an empty log data */
			Create_JSON_Frame(gs_uc_mac_address, gb_dev_ipAddress, MODEL_TYPE, DEVICE_TYPE, gb_last_dt_tm, gb_data_packets,
			dummy_arr, 0, jsonBuffer, sizeof(jsonBuffer));                  // Send empty JSON with 0 logs
		}
	}
	else
	{
		__NOP();
	}

}

/*****************************************************************************
* Function Name  : Read_MacID_And_Update_InSystem
* Returns        : Nothing
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 18/09/2024
*
* Description    : Reads the device's MAC address from the EEPROM and updates
*                  it in the system. The MAC address is stored in the global
*                  buffer `gs_uc_mac_address` and printed to the debug console
*                  in ASCII HEX format if debug is enabled.
*
* Notes          : The function reads the 6-byte EUI-48 (MAC address) from the
*                  EEPROM via the TWI interface and prints it to the console
*                  if debugging is enabled.
*
* Global Variables Affected : gs_uc_mac_address
*****************************************************************************/
void Read_MacID_And_Update_InSystem(void)
{
	EEPROM_EUI_READ(gs_uc_mac_address);
	
	//#if DEBUG_ALL || DEBUG_EXT_EEPROM
	Print_Message("\ndevice MAC address is : ");
	for (U8 mclen = 0; mclen < 6; mclen ++)
	{
		Print_ASCII_HEX(gs_uc_mac_address[mclen]);
		Print_Message(" ");
	}
	//#endif
}


/*****************************************************************************
* Function Name  : General_Task
* Returns        : Nothing
* Arguments      : void *pvParameters (Holds the pointer)
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : Monitor general task and RTC time.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void General_Task(void *pvParameters)
{
	//Print_Message("\nHit General Application rtosTask.");
	/* Just to avoid compiler warnings. */
	UNUSED(pvParameters);
	
	U8 lcl_power_on_f = 1;
	
	
	while (1)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		/***** RTC Related Functions *****/
		Get_RTC_Data_At_Every_Second();
		if (gb_rtc_time_update_f == 1)
		{
			gb_rtc_time_update_f = 0;
			Update_Time_And_Date();
		}
		
		/***** End of RTC Related Functions *****/
		if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
		{			
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
			if (gb_config_mode_f == FALSE)
			{
				if ((gb_rtc_1sec_IS_set_f == 1) && (lcl_power_on_f == 1))
				{
					lcl_power_on_f = 0;
					gb_rtc_1sec_IS_set_f = 0;
					Input_PowerUp_Sync();       //read input state with current state at power on
					PowerOn_Output_Activation();
				}
				else
				__NOP();
				
				Monitor_Key_For_Default_IPAddress();	/*Monitors keys for setting the default IP address.*/
				General_1_Sec_RTC_Timer_Routine();      /*Get RTC time.*/
				Input_Functionality();					/*Monitors Input state.*/
				monitor_output();                       /*Monitors Output state.*/       
				Auto_Output_Activation();
				Safety_And_Intruder_Monitor();			// Function to monitor inputs of intruder.
			}
			
			// Observe RTC time here, if RTC time becomes 00:00:00 (HH:MM:SS) Create a task for sync time with SNTP.
			//const U8 SYNC_WITH_NTP = 1;

			if ((dm_dev_info.sync_time == NTP_SERVER) && (sntp_create_task_f == 0) &&
			((gb_sntp_powerOn_f == 1) || ((gb_rtcTimeArr[0] == ntp_sync_at_time[0]) && (gb_rtcTimeArr[1] == ntp_sync_at_time[1]))))
			{
				/* Create the task for SNTP server */
				if (xTaskCreate(Task_Get_SNTP_Data, (const signed char *const) "Task_Get_SNTP_Data",
				256, NULL, mainTCP_TASK_PRIORITY, NULL) != pdPASS)
				{
					Print_Message("\nFailed to Create SNTP Task");
				}
				else
				{
					sntp_create_task_f = 1;
					
					// Reset power-on flag only if it was set
					if (gb_sntp_powerOn_f == 1)
					{
						gb_sntp_powerOn_f = 0;
						clr_pwr_sntp_task_f = 1;	// Set this flag to clear or delete the task @ every on condition.
					}
				}
			}
			else if (((dm_dev_info.sync_time == NTP_SERVER) || (dm_dev_info.sync_time == ERP_SERVER)) && (sntp_create_task_f == 1) &&
			((gb_rtcTimeArr[0] > ntp_sync_at_time[0]) ||
			((gb_rtcTimeArr[0] == ntp_sync_at_time[0]) && (gb_rtcTimeArr[1] > ntp_sync_at_time[1]))))
			{
				// Clear the task flag once time has passed
				sntp_create_task_f = 0;
			}
			else if ((dm_dev_info.sync_time == ERP_SERVER) && (sntp_create_task_f == 0) &&
			((gb_sntp_powerOn_f == 1) || ((gb_rtcTimeArr[0] == ntp_sync_at_time[0]) && (gb_rtcTimeArr[1] == ntp_sync_at_time[1]))))
			{
				// SYNC time with ERP Server. So, request new time from the ERP.
				gb_req_erp_time_f = 1;	// Set flag to send in TL log.
				sntp_create_task_f = 1;	// Set this flag to enter only once in a loop until reset.
				
				// Reset power-on flag only if it was set
				if (gb_sntp_powerOn_f == 1)
				{
					gb_sntp_powerOn_f = 0;
					clr_pwr_sntp_task_f = 1;	// Set this flag to clear or delete the task @ every on condition.
				}
			}
			else
			{
				__NOP();
			}
			
			/* Release the semaphore when done with the critical section */
			xSemaphoreGive(xSemaphore);
		}
		
		vTaskDelay(5);
	}
}


/*****************************************************************************
* Function Name  : General_1_Sec_RTC_Timer_Routine
* Returns        : Nothing.
* Arguments      : void.
* Created by     : Ranjitkumar Ainapure.
* Date Created   : 22/11/2024
*
* Description    : Get RTC time at poll frequency.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void General_1_Sec_RTC_Timer_Routine(void)
{
	if (gb_rtc_one_sec_flag == FLAG_SET)
	{
		gb_rtc_one_sec_flag = FLAG_RST;
		
		if (gb_network_up_f != 1)
		{
			static U8 chk_eth_plug_in_time = 0;
			chk_eth_plug_in_time ++;
			if (chk_eth_plug_in_time >= 5)
			{
				chk_eth_plug_in_time = 0;
				
				U8 network_sts_err = 0;
				/* Auto Negotiate, work in RMII mode. */
				if (ethernet_phy_auto_negotiate(GMAC, BOARD_GMAC_PHY_ADDR) != GMAC_OK) {
					LWIP_DEBUGF(NETIF_DEBUG, ("gmac_low_level_init: auto negotiate ERROR!\n"));
					network_sts_err = 1;
					//return;
				}
				
				if (network_sts_err != 1)
				{
					/* Establish ethernet link. */
					while (ethernet_phy_set_link(GMAC, BOARD_GMAC_PHY_ADDR, 1) != GMAC_OK) {
						LWIP_DEBUGF(NETIF_DEBUG, ("gmac_low_level_init: set link ERROR!\n"));
						//return;
					}
				}
			}
		}
		
		Check_Time_To_Poll_Server();
		
		/* Below code snippet is written for incrementing SNTP response time */
		if ((sntp_waiting_for_response == 1) && (sntp_retry_f == 0))
		{
			sntp_resp_time ++;
			if (sntp_resp_time > SNTP_RESPONSE_TIME)
			{
				sntp_resp_time = 0;				// reset timer.
				sntp_retry_f = 1;				// Set flag to indicate that timer has exceeded & retry once again.
				sntp_waiting_for_response = 0;	// Reset flag to re check.
			}
			else
			__NOP();
		}
		/* End of code snippet is written for incrementing SNTP response time */		
	}
	else
	__NOP();
}



/*****************************************************************************
* Function Name  : Check_Time_To_Poll_Server
* Returns        : Nothing.
* Arguments      : void.
* Created by     : Ranjitkumar Ainapure.
* Date Created   : 22/11/2024
*
* Description    : Check time at poll frequency.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void Check_Time_To_Poll_Server(void)
{
	static U8 poll_secs;
	
	poll_secs ++;
	
	#if 0
	Print_Message("\nSec : ");
	Print_Number(poll_secs);
	#endif
	
	if (poll_secs >= gb_server_poll_freq)
	{
		poll_secs = 0;
		gb_rtc_send_to_server_f = FLAG_SET;  //set flag after poll time reach
		
		if (gb_arm_feedback_f == 1)
		{
			gb_arm_feedback_f = 0;
			gb_arm_send_server_f = 1;
		}
		
		if ((gb_ip_send_to_srver_f == 1) || (gb_op_send_server_f == 1))
		{
			gb_op_send_server_f = 0;
			gb_ip_send_to_srver_f = 0;
		}
	}
	else if ((gb_ip_send_to_srver_f == 1) || (gb_op_send_server_f == 1) || (gb_arm_feedback_f == 1))
	{
		if (gb_ip_send_to_srver_f == 1)
		gb_ip_send_to_srver_f = 0;
		if (gb_op_send_server_f == 1)
		gb_op_send_server_f = 0;
		
		if (gb_arm_feedback_f == 1)
		{
			gb_arm_feedback_f = 0;
			gb_arm_send_server_f = 1;
		}
		
		gb_rtc_send_to_server_f = FLAG_SET;  //set flag after poll time reach
	}
	else
	{
		__NOP();
	}
	
}


/*****************************************************************************
* Function Name  : Get_Last_Dt_Tm_From_Flash
* Returns        : Nothing.
* Arguments      : void.
* Created by     : Ranjitkumar Ainapure.
* Date Created   : 22/11/2024
*
* Description    : store Last date and time at power on.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void Get_Last_Dt_Tm_From_Flash(void)
{
	// 	U8 temp[6] ={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
	// 	EEPROM_Byte_Write(TL_LAST_DT_LOC, temp, 6);
	U8 lcl_eep_buf[6];
	memset(lcl_eep_buf, 0, 6);
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(IS_LAST_DT_LOC, lcl_eep_buf, 6);
	if (lcl_eep_status == 0)
	{
		memcpy(gb_last_dt_tm, lcl_eep_buf, 6);
		
		if ((gb_last_dt_tm[0] < 0) || (gb_last_dt_tm[0] > 99))
		gb_last_dt_tm[0] = 0x00;
		if ((gb_last_dt_tm[1] < 0) || (gb_last_dt_tm[1] > 12))
		gb_last_dt_tm[1] = 0x01;
		if ((gb_last_dt_tm[2] < 0) || (gb_last_dt_tm[2] > 31))
		gb_last_dt_tm[2] = 0x01;
		
		if ((gb_last_dt_tm[3] < 0) || (gb_last_dt_tm[3] > 24))
		gb_last_dt_tm[3] = 0x0C;
		if ((gb_last_dt_tm[4] < 0) || (gb_last_dt_tm[4] > 59))
		gb_last_dt_tm[4] = 0x00;
		if ((gb_last_dt_tm[5] < 0) || (gb_last_dt_tm[5] > 59))
		gb_last_dt_tm[5] = 0x00;
	}
}

/*****************************************************************************
* Function Name  : Read_IS_Boundary_Flag
* Returns        : void
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 02/01/2025
* Description    : Reads the IS boundary flag from flash memory. The boundary flag
*                  indicates if the IS (Transactional log) has reached its specified boundary.
*                  The function reads the flag from flash memory and updates the
*                  global variable `gb_is_reached_boundary_f` accordingly.
* Notes          : This function depends on the `EEPROM_Byte_Read` function for reading
*                  data from flash memory.
* Global Variables Affected :
*                  - gb_is_reached_boundary_f: Stores the value of the IS boundary flag.
*****************************************************************************/
void Read_IS_Boundary_Flag(void)
{
	// Read a single byte from the flash memory location TL_BOUNDARY_FLAG
	U8 boundary_arr[1] = {0};
	U8 lcl_eep_sts = EEPROM_Byte_Read(IS_BOUNDARY_FLAG, boundary_arr, 1);
	// Check if the flash byte read operation has completed
	if (lcl_eep_sts == 0)
	{
		// Update the global boundary flag variable with the value read from flash
		gb_is_reached_boundary_f = boundary_arr[0];
	}
	else
	{
		// Optional: Add error handling or debugging logs if needed
		__NOP(); // No Operation as placeholder
	}
}



/*****************************************************************************
* Function Name  : Read_MD_Boundary_Flag
* Returns        : void
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 02/01/2025
* Description    : Reads the MD boundary flag from flash memory. The boundary flag
*                  indicates if the MD (Transactional log) has reached its specified boundary.
*                  The function reads the flag from flash memory and updates the
*                  global variable `gb_md_reached_boundary_f` accordingly.
* Notes          : This function depends on the `EEPROM_Byte_Read` function for reading
*                  data from flash memory.
* Global Variables Affected :
*                  - gb_md_reached_boundary_f: Stores the value of the MD boundary flag.
*****************************************************************************/
void Read_MD_Boundary_Flag(void)
{
	// Read a single byte from the flash memory location TL_BOUNDARY_FLAG
	U8 boundary_arr[1] = {0};
	U8 lcl_eep_sts = EEPROM_Byte_Read(MD_BOUNDARY_FLAG, boundary_arr, 1);
	// Check if the flash byte read operation has completed
	if (lcl_eep_sts == 0)
	{
		// Update the global boundary flag variable with the value read from flash
		gb_md_reached_boundary_f = boundary_arr[0];
	}
	else
	{
		// Optional: Add error handling or debugging logs if needed
		__NOP(); // No Operation as placeholder
	}
}

/*****************************************************************************
* Function Name  : Read_Memory_For_PowerON_State
* Returns        : Nothing.
* Arguments      : void.
* Created by     : Shirisha Ankam
* Date Created   : 22/11/2025
*
* Description    : function holds all the function whose data to be read at power on.
*
* Notes          : NA.
*
* Global Variables Affected : NA.
*****************************************************************************/
void Read_Memory_For_PowerON_State(void)
{
	Read_IP_Data_From_EEP();				//read input data at power on
	Read_OP_Data_From_EEP();				//read output data at power on
	Update_SI_Variables();
	Arm_DisArm_At_Powerup();
	U8 dummy_data[DI_DATA_SIZE];
	memset(dummy_data, 0, DI_DATA_SIZE);
	Read_DI_Data_From_EEPROM(dummy_data);	//read device info. data at power on
	Read_MS_Data_From_EEPROM();				//read modbus setting data at power on
	Get_Last_Dt_Tm_From_Flash();            //read last store date and time
	Read_IS_Boundary_Flag();                //read input log boundary flag value
	Read_MD_Boundary_Flag();                //read modbus log bundary flag value
}
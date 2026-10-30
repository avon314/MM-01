/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: json_extraction.c
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

#include "asf.h"

#include "json_extraction.h"
#include "frozen.h"
#include "string.h"
#include "user_uart.h"
#include "CRC.h"
#include "ext_eeprom_1.h"
#include "definitions.h"
#include "memory_storage.h"
#include "user_rtc.h"
#include "mm_app.h"
#include "input_output.h"
#include "modbus_app.h"
#include "app_eeprom.h"
#include "comn_config.h"

#define DEBUG_FROZEN   (1)

struct json_token message_token;
GEN_MEM extract;

U8 gb_update_sev_token = 0;

extern void Heap_Memory_Is(void);

U8 gb_more_data_f = 0;

GEN_MEM gen = {0};
// Field mappings
FieldEntry fields[] =
{
    {"IP", Extract_IP_Data_From_JSON},
	{"OP", Extract_OP_Data_From_JSON},
	{"DI", Extract_DI_Data_From_JSON},
	{"RC", Extract_RC_Data_From_JSON}, 
	{"CT", Extract_CT_Data_From_JSON},
	{"MP", Extract_MP_Data_From_JSON}, 
	{"MS", Extract_MS_Data_From_JSON},
	{"WR", Extract_WR_Data_From_JSON}, 	 		
	{"SI", Extract_SI_Data_From_JSON},
	{"AM", Extract_AM_Data_From_JSON}, 	 	
	{"BT", Extract_BT_Data_From_JSON}, 	
	// Add more field-function pairs here
};
const int field_count = sizeof(fields) / sizeof(fields[2]);


typedef void (*json_walk_callback_t)(
void *callback_data,
const char *name,
size_t name_len,
const char *path,
const struct json_token *token
);


/*****************************************************************************
* Function name		: my_json_callback
* Returns			: Nothing.
* Arguments			: json pointer.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Description		: Call back function to identy its token to be extracted.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr
*****************************************************************************/
void my_json_callback(void *userdata, const char *name, size_t name_len,
const char *path,  struct json_token *key_token) {
	// We're only interested in keys under "message"
	if (path != NULL && strncmp(path, ".message", 8) == 0 && name != NULL)
	{
		char key_buf[3] = {0};  // Adjust size as needed
		size_t len = name_len < sizeof(key_buf) - 1 ? name_len : sizeof(key_buf) - 1;
		memcpy(key_buf, name, len);
		key_buf[len] = '\0';

		if (name_len == 2)
		{
			char fmt[20];

			for (U8 keyIdx = 0; keyIdx < field_count; keyIdx ++)
			{
				if (strncmp(key_buf, fields[keyIdx].key , 2) == 0)
				{
					#if (DEBUG_ALL || DEBUG_JSON_EXT)
					Print_Message("\nFound Field: ");
					Print_Message(fields[keyIdx].key);
					Print_Message("\n");
					#endif

					snprintf(fmt, sizeof(fmt), "{%s: %%T}", fields[keyIdx].key);

					struct json_token token;

					if (json_scanf(message_token.ptr, message_token.len, fmt, &token) > 0)
					{
						fields[keyIdx].process_func((token));
					}
				}
			}
		}
		else
		{
			__NOP();
		}
	}
}


/*****************************************************************************
* Function name		: Extract_JsonData_Using_Frozen_Library
* Returns			: Nothing.
* Arguments			: json pointer.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Description		: Function to extract the json frame from its key values
*
* Notes				: NA
* Global Variables Affected : gen.data_arr
*****************************************************************************/
void Extract_JsonData_Using_Frozen_Library(const char *json)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nDetermine which Model is received.");
	#endif
	//Heap_Memory_Is();
	
	struct json_token token;
	U8 status_val = 0;

	// Step 1: Extract top-level "message" token
	if (json_scanf(json, strlen(json), "{message: %T}", &message_token) <= 0)
	{
		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nMessage Field NOT found\n");
		#endif
		
		return;
	}

	char fmt[20];

	// Step 2: Process ST field
	snprintf(fmt, sizeof(fmt), "{%s: %%T}", "ST");

	if (json_scanf(message_token.ptr, message_token.len, fmt, &token) > 0)
	{
		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nReceived ST:");
		Print_Message(token.ptr);
		Print_Message("\n");
		Print_Number(atoi(token.ptr));
		Print_Message("\n");
		#endif

		if ((atoi(token.ptr)) == 1)
		{
			status_val = 1;	// Set the status value to 1.
			
			// After sending an ack for cancel command.
			// Check below flag.
			if (gb_requesting_cancel == 1)
			{
				gb_requesting_cancel = 0;
				Cancel_The_Device_From_Server();
			}
			else
			__NOP();
			
			if (gb_ac_power_f)
			{
				gb_ac_power_f = 0;
				gb_dev_connect_f = 1;
			}
			else
			__NOP();
			
			if (gb_update_is_last_sent_loc_f == 1)
			{
				Update_Last_Sent_Bytes_IS();
				gb_update_is_last_sent_loc_f = 0;
			}
			else
			__NOP();
			
			if (gb_update_md_last_sent_loc_f == 1)
			{
				Update_Last_Sent_Bytes_MD();
				gb_update_md_last_sent_loc_f = 0;
			}
			else
			__NOP();
			
			if (gb_send_tl_at_conn == 1)
			gb_send_tl_at_conn = 0;
			else
			__NOP();
			
			if (gb_arm_wait_ser_ack_f == 1)
			{
				gb_arm_wait_ser_ack_f = 0;
			}
			else
			__NOP();
			
			if ((gb_op_detected_f == FLAG_SET) || (gb_op_at_pwr_on == FLAG_SET))
			{
				/* Reset Output status flags after an acknowledgment */
				gb_op_detected_f = FLAG_RST;
				gb_op_at_pwr_on = FLAG_RST;
			}
			else
			__NOP();
			
			if (gb_send_cs_to_server == FLAG_SET)
			{
				gb_send_cs_to_server = FLAG_RST;

				/* After power on, start Modbus exception (ME) checking only once slave scan is done and CS is sent */
				if ((gb_start_sending_dpt == 1) && (gb_me_check_state == ME_CHECK_DISABLED))
				{
					gb_me_check_state = ME_CHECK_ARMED;
					Resend_Stored_Write_Errors();	// Send write errors stored before power off once
				}
				else
				__NOP();
			}
			else
			__NOP();

			/* Clear ME only if the acknowledged frame carried the latest exception status */
			if ((gb_send_me_to_server == FLAG_SET) && (gb_me_sent_cnt == gb_me_change_cnt))
			{
				gb_send_me_to_server = FLAG_RST;
				memset(gb_mb_exc_clr, 0, TOTAL_SLAVES);	// Cleared slaves [SA,0] are acknowledged
			}
			else
			__NOP();
		}
		else
		__NOP();
	}

	// Step 3: Process DP field
	memset(fmt,0,20);
	snprintf(fmt, sizeof(fmt), "{%s: %%T}", "DP");

	if (json_scanf(message_token.ptr, message_token.len, fmt, &token) > 0)
	{
		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nReceived DP:");
		Print_Message(token.ptr);
		Print_Message("\n");
		Print_Number(atoi(token.ptr));
		Print_Message("\n");
		#endif

		gb_data_packets = atoi(token.ptr) ;
	}
	else
	__NOP();

	if (gb_data_packets > 0)
	{
		Update_Poll_Frequency(gb_data_packets);
	}
	else
	__NOP();

	// Step 4: Process LU field
	const char luValue[20] = {0};
	memset(fmt,0,20);
	snprintf(fmt, sizeof(fmt), "{%s: %%T}", "LU");

	if (json_scanf(message_token.ptr, message_token.len, fmt, &token) > 0)
	{
		strncpy(luValue,token.ptr,20) ;
		
		//#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nReceived LU:");
		Print_Message(luValue);
		Print_Message("\n");
		//#endif
	}
	else
	{
		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\n'LU' not found or not a string.");
		#endif
	}

	// Step 5: Find remaining keys and extract
	json_walk(json, strlen(json), my_json_callback, NULL);

	/* Process LU value if required */
	if ((luValue) && (status_val == 1))
	{
		int year;
		if (sscanf(luValue, "%4d%2d%2d%2d%2d%2d",
		&year, &gb_last_dt_tm[1], &gb_last_dt_tm[2],
		&gb_last_dt_tm[3], &gb_last_dt_tm[4], &gb_last_dt_tm[5]) == 6)
		{
			gb_last_dt_tm[0] = year % 100;  // Store last 2 digits of year
			EEPROM_Byte_Write(IS_LAST_DT_LOC, gb_last_dt_tm, 6);
		}
		else
		{
			//#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Message("\nInvalid 'LU' format.");
			//#endif

			memset(gb_last_dt_tm, 0, sizeof(gb_last_dt_tm));
			EEPROM_Byte_Write(IS_LAST_DT_LOC, gb_last_dt_tm, 6);
		}
	}
	else
	__NOP();
	
	if ((status_val == 1) && (gb_more_data_f == 1))
	{
		/* Virtually set this flag to send data to the server, if more data is present in the data base.
		i.e to send immediately after a post and its response.
		*/
		gb_rtc_send_to_server_f = 1;
		
	}
	else
	__NOP();
}


/*****************************************************************************
* Function name		: Extract_IP_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed IP array.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Updated by		: Shirisha Ankam
* Date Updated		: 21-11-2025
* Description		: Function to Extract the IP Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr , gb_Temp_CRC .
*****************************************************************************/
void Extract_IP_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nStarted Extracting IP data using Frozen JSON");
	Print_Message(token.ptr);
	Print_Message("\n");
	#endif

	struct json_token array_elements;
	uint8_t index1 = 0;
	uint8_t lcl_ip_idx = 0; // Index to track the position in the 1D array
	
	memset(extract.data_arr, 0, IP_DATA_SIZE);
	
	U8 lcl_eep_buf[IP_DATA_SIZE];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, IP_DATA_SIZE);
	lcl_eep_status = EEPROM_Byte_Read(IP_EEP_START_LOC, lcl_eep_buf, IP_DATA_SIZE);
	if (lcl_eep_status == 0)
	{
		memcpy(extract.data_arr, lcl_eep_buf, IP_DATA_SIZE);

		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nFlash data is : ");
		for (U8 idx=0;idx<IP_DATA_SIZE;idx++)
		{
			Print_Number(extract.data_arr[idx]);
			Print_Message(",");
		}
		#endif

		if (CRC_Calculate_Check(extract.data_arr, (IP_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// If CRC validated.
			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Message("\nCRC Validated for IP data");
			#endif
		}
		else
		{
			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Message("\nCRC failed for IP data");
			#endif

			memset(extract.data_arr, 0, IP_DATA_SIZE);
		}

		while (json_scanf_array_elem(token.ptr, token.len, "", index1, &array_elements) > 0)
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			struct json_token element_arry;
			uint8_t index2 = 0;
			char buf[10];

			while (json_scanf_array_elem(array_elements.ptr, array_elements.len, "", index2, &element_arry) > 0)
			{
				wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
				// Copy token into null-terminated buffer
				memset(buf, 0, sizeof(buf));
				memcpy(buf, element_arry.ptr, element_arry.len);  // element_arry.ptr is not null-terminated

				if(index2 == 0)
				{
					// Get input number, And calculate offset to store data.
					lcl_ip_idx = (((U8)atoi(buf) - 1) * 3);
										
				}
				else 
				{
					extract.data_arr[lcl_ip_idx ++] = (U8)atoi(buf);
				}
				index2++;
			}

			index1++;
		}

		CRC_Calculate_Check(extract.data_arr, (IP_DATA_SIZE - 2), CRC_CALC);
		extract.data_arr[(IP_DATA_SIZE - 2)] = gb_Temp_CRC[0];
		extract.data_arr[(IP_DATA_SIZE - 1)] = gb_Temp_CRC[1];

		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nWritable Flash data is : ");
		for (U8 idx = 0; idx < IP_DATA_SIZE; idx++)
		{
			Print_Number(extract.data_arr[idx]);
			Print_Message(",");
		}
		#endif

		EEPROM_Byte_Write(IP_EEP_START_LOC, extract.data_arr, IP_DATA_SIZE);
		Segregate_IP_Data_To_Buffer(extract.data_arr);
		
	}
	else
	{
		__NOP();
	}
	
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nCompleted extracting and storing IP data into Flash.");
	#endif
}


/*****************************************************************************
* Function name		: Extract_OP_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed OP array.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Updated by		: Shirisha Ankam
* Date Updated		: 21-11-2025
* Description		: Function to Extract the OP Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr , gb_Temp_CRC .
*****************************************************************************/
void Extract_OP_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nStarted Extracting OP data using Frozen JSON");
	Print_Message(token.ptr);
	Print_Message("\n");
	#endif

	struct json_token array_elements;
	uint8_t index1 = 0;
	U8 lcl_op_idx = 0; // Index to track the position in the 1D array

	memset(extract.data_arr, 0, OP_DATA_SIZE);
	
	U8 lcl_eep_buf[OP_DATA_SIZE];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, OP_DATA_SIZE);
	lcl_eep_status = EEPROM_Byte_Read(OP_EEP_START_LOC, lcl_eep_buf, OP_DATA_SIZE);
	if (lcl_eep_status == 0)
	{
		memcpy(extract.data_arr, lcl_eep_buf, OP_DATA_SIZE);

		#if (DEBUG_ALL || DEBUG_OP)
		Print_Message("\nFlash data is : ");
		for (U8 idx=0;idx<OP_DATA_SIZE;idx++)
		{
			Print_Number(extract.data_arr[idx]);
			Print_Message(",");
		}
		#endif

		if (CRC_Calculate_Check(extract.data_arr, (OP_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// If CRC validated.
			#if (DEBUG_ALL || DEBUG_OP)
			Print_Message("\nCRC Validated for OP data");
			#endif
		}
		else
		{
			#if (DEBUG_ALL || DEBUG_OP)
			Print_Message("\nCRC failed for OP data");
			#endif
			memset(extract.data_arr, 0, OP_DATA_SIZE);
		}
		while (json_scanf_array_elem(token.ptr, token.len, "", index1, &array_elements) > 0)
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			struct json_token element_arry;
			uint8_t index2 = 0;
			char buf[10];

			while (json_scanf_array_elem(array_elements.ptr, array_elements.len, "", index2, &element_arry) > 0)
			{
				wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
				// Copy token into null-terminated buffer
				memset(buf, 0, sizeof(buf));
				memcpy(buf, element_arry.ptr, element_arry.len);  // element_arry.ptr is not null-terminated

				if(index2 == 0)
				{
					lcl_op_idx = (((U8)atoi(buf) - 1) * 8);
				}
				else if(index2 == 5)
				{
					uint8_t hour = 0;
					uint8_t minute = 0;
					
					/* buf contains "HHMM" e.g. "0107" */
					hour   = (uint8_t)((buf[0] - '0') * 10 + (buf[1] - '0'));
					minute = (uint8_t)((buf[2] - '0') * 10 + (buf[3] - '0'));
					
					#if (DEBUG_ALL || DEBUG_JSON_EXT)
					Print_Message("\n| HOUR   | : ");
					Print_Number(hour);
					Print_Message("\n| MINUTE | : ");
					Print_Number(minute);
					#endif
					// Store hour and minute separately
					extract.data_arr[lcl_op_idx++] = hour;
					extract.data_arr[lcl_op_idx++] = minute;
				}
				else if(index2 == 6)
				{
					uint16_t duration = (uint16_t)atoi(buf);
					
					#if (DEBUG_ALL || DEBUG_JSON_EXT)
					Print_Message("\nReceived time duration is : ");
					Print_Number(duration);
					#endif
					extract.data_arr[lcl_op_idx++] = (duration >> 8) & 0xFF;
					extract.data_arr[lcl_op_idx++] = (duration) & 0xFF;
					
				}
				else
				{
					extract.data_arr[lcl_op_idx ++] = (U8)atoi(buf); // Store the value in the flattened array
				}

				index2++;
			}

			index1++;
		}
		CRC_Calculate_Check(extract.data_arr, (OP_DATA_SIZE - 2), CRC_CALC);
		extract.data_arr[(OP_DATA_SIZE - 2)] = gb_Temp_CRC[0];
		extract.data_arr[(OP_DATA_SIZE - 1)] = gb_Temp_CRC[1];

		#if (DEBUG_ALL || DEBUG_OP)
		Print_Message("\nWritable Flash data is : ");
		for (U8 idx=0;idx<OP_DATA_SIZE;idx++)
		{
			Print_Number(extract.data_arr[idx]);
			Print_Message(",");
		}
		#endif

		EEPROM_Byte_Write(OP_EEP_START_LOC, extract.data_arr, OP_DATA_SIZE);
		Segregate_OP_Data_To_Buffer(extract.data_arr);

	}
	else
	{
		__NOP();
	}
	
	gb_sync_op_at_config = 1;
	
	PowerOn_Output_Activation();

	#if (DEBUG_ALL || DEBUG_OP)
	Print_Message("\nCompleted extracting and storing OP data into Flash.");
	#endif
}


/*****************************************************************************
* Function name		: Extract_RC_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token -> passes parsed RC array.
* Created by		: Shirisha Ankam
* Date created		: 20-11-2025
* Description		: Function to Extract the Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr , gb_Temp_CRC .
*****************************************************************************/
void Extract_RC_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting relay control data using Frozen JSON");
	Print_Message(token.ptr);
	#endif

	struct json_token array_elements;
	uint8_t index1 = 0;
	uint8_t do_ix = 0;
	
	uint8_t lcl_op_state = 0;

	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nExtracted values from RC array:\n");
	#endif

	while (json_scanf_array_elem(token.ptr, token.len, "", index1, &array_elements) > 0)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		struct json_token element_arry;
		uint8_t index2 = 0;
		char buf[32];

		while (json_scanf_array_elem(array_elements.ptr, array_elements.len, "", index2, &element_arry) > 0)
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			// Copy token into null-terminated buffer
			memset(buf, 0, sizeof(buf));
			memcpy(buf, element_arry.ptr, element_arry.len);  // element_arry.ptr is not null-terminated


			/* Get output operation state from server & update in below array based on output index. */
			if(index2 == 0)
			{
				//Extraction of output Number and copying to door index
				do_ix = (U8)atoi(buf);
			}
			else if(index2 == 1)
			{
				//operation state value Extracting
				lcl_op_state = (U8)atoi(buf);
			}
			index2++;

		}
		Output_Operations_From_Server(do_ix,lcl_op_state);
		index1 ++;
	}
}

/*****************************************************************************
* Function name		: Extract_DI_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed DI array.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Updated by		: Shirisha Ankam
* Date Updated		: 21-11-2025
* Description		: Function to Extract the DI Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr
*****************************************************************************/
void Extract_DI_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nStarted Extracting DI data using Frozen JSON\n");
	Print_Message(token.ptr);
	#endif

	struct json_token t;
	char value[32];
	uint8_t index = 0;
	uint8_t diIdx = 0 ;

	memset(extract.data_arr, 0, DI_DATA_SIZE);

	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nExtracted values from DI array:\n");
	#endif

	while (json_scanf_array_elem(token.ptr, token.len, "", index, &t) > 0)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		// Copy token into null-terminated buffer
		memset(value, 0, sizeof(value));
		memcpy(value, t.ptr, t.len);  // t.ptr is not null-terminated
		value[t.len] = '\0';

		// Extract ip address from string
		if (index == 2)
		{
			memset(&extract.data_arr[diIdx], 0, DI_DNS_LENGTH);
			strncpy(&extract.data_arr[diIdx], value, t.len);  // null-terminated copy
			diIdx += DI_DNS_LENGTH;

			//Print_Data_On_Uart(value, DI_DNS_LENGTH);
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nString len : ");
			Print_Number(t.len);
			Print_Message("\nExtracted string is: ");
			Print_Message(&extract.data_arr[DNS_IDX]);
			Print_Message(" ");
			#endif


		}
		else
		{
			/*****
			NOTE : if index value (diIdx == 6) reached & if the values are in (-ve) state,
			the values are storing by 2's compliment due to declared data type U8. i.e
			if data is type casting from int to U8 internally it converts to 2's compliment.
			*****/

			extract.data_arr[diIdx] = (U8)atoi(value) ;

			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Number(extract.data_arr[diIdx]);
			Print_Message(" ");
			#endif

			diIdx++ ;
		}
		index++;
	}

	// Compute CRC and also add last two bytes of array
	CRC_Calculate_Check(extract.data_arr, (DI_DATA_SIZE - 2), CRC_CALC);
	extract.data_arr[(DI_DATA_SIZE - 2)] = gb_Temp_CRC[0];	// Higher Byte
	extract.data_arr[(DI_DATA_SIZE - 1)] = gb_Temp_CRC[1];	// Lower Byte

	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nWritable Flash data is: ");
	for (U8 idx = 0; idx < DI_DATA_SIZE; idx++)
	{
		Print_Number(extract.data_arr[idx]);
		Print_Message(",");
	}
	#endif
	
	// Store data into EEPROM
	EEPROM_Byte_Write(DI_EEP_START_LOC, extract.data_arr, DI_DATA_SIZE);

	// 	gb_sync_time_flag = 0;
	if (extract.data_arr[DEV_STS_IDX] == DEV_CANCELLED)
	{
		Print_Message("\nRequesting for cancel");
		gb_server_poll_freq = 1;	// Poll with 1 sec.
		gb_requesting_cancel = 1;	// Set last bit 1 to say requesting for device cancel.
		return ;	// Return Suddenly & reply an acknowledgment to the server.  
	}
	else
	{
		U8 dummy_arr[DI_DATA_SIZE];
		memset(dummy_arr, 0, DI_DATA_SIZE);
		Read_DI_Data_From_EEPROM(dummy_arr);	// Call this function to update RAM variables.
	}
	

	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nFinished extraction of device information data.");
	#endif
}


/*****************************************************************************
* Function name		: Extract_CT_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed CT array.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Description		: Function to Extract the CT Frozen json frame .
*
* Notes				: NA
* Global Variables Affected : NA
*****************************************************************************/
void Extract_CT_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting CT data using Frozen JSON");
	Print_Message(token.ptr);
	Print_Message("\nExtracted values from CT array:\n");
	#endif

	char *ct = NULL;
	char buf[2];  // Temporary buffer for two-digit fields
	uint8_t i =0;
	uint8_t j =0;
	U8 ct_buff[7] = {0} ;

	json_scanf((token.ptr+2), token.len,"%Q", &ct);

	for(i = 0; i < 7; i++)
	{
		strncpy(buf, (ct + j), 2);
		ct_buff[i] = (U8)atoi(buf);

		#if DEBUG_FROZEN
		Print_Number(ct_buff[i]);
		Print_Message(" ");
		#endif

		j = j + 2 ;
	}

	// Write date (DD-MM-YY-dd) and day to RTC
	Write_DtMnYy_and_Day_To_DS1339A(ct_buff[0], ct_buff[1], ct_buff[2], ct_buff[3]);
	// Write time (HH:MM:SS) to RTC
	Write_Time_hhMMss_To_DS1339A(ct_buff[4], ct_buff[5], ct_buff[6]);

	gb_req_erp_time_f = 0;	// Reset flag & Do not request new time.
	if (clr_pwr_sntp_task_f == 1)
	{
		clr_pwr_sntp_task_f = 0;
		sntp_create_task_f = 0;	// Reset the flag to re enter to update the time for next loop.
	}

}


/*****************************************************************************
* Function name		: Extract_DP_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed IP array.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Updated by		: Shirisha Ankam
* Date Updated		: 21-11-2025
* Description		: Function to Extract the IP Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr , gb_Temp_CRC .
*****************************************************************************/
void Extract_MP_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nStarted Extracting MP data using Frozen JSON");
	Print_Message(token.ptr);
	Print_Message("\n");
	#endif

	struct json_token array_elements;
	uint8_t index1 = 0;
	int lcl_dp_idx = 0; // Index to track the position in the 1D array
	
	U8 mp_buffer[MP_DATA_SIZE];
	memset(mp_buffer, 0, MP_DATA_SIZE);
	
	U8 lcl_eep_status = 0;
	lcl_eep_status = EEPROM_Byte_Read(MP_EEP_START_LOC, mp_buffer, MP_DATA_SIZE);
	if (lcl_eep_status == 0)
	{
		if (CRC_Calculate_Check(mp_buffer, (MP_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// If CRC validated.
			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Message("\nCRC Validated for MP data");
			#endif
		}
		else
		{
			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			Print_Message("\nCRC failed for data");
			#endif

			memset(mp_buffer, 0, MP_DATA_SIZE);
		}
		
		#if (DEBUG_ALL || DEBUG_JSON_EXT)
		Print_Message("\nExtracted values from MP array:\n");
		#endif

		while (json_scanf_array_elem(token.ptr, token.len, "", index1, &array_elements) > 0)
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			struct json_token element_arry;
			uint8_t index2 = 0;
			char buf[32];
			uint16_t buff_loc = 0;

			while (json_scanf_array_elem(array_elements.ptr, array_elements.len, "", index2, &element_arry) > 0)
			{
				wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
				// Copy token into null-terminated buffer
				memset(buf, 0, sizeof(buf));
				memcpy(buf, element_arry.ptr, element_arry.len);  // element_arry.ptr is not null-terminated
				buf[element_arry.len] = '\0';
				
				switch(index2)
				{
					case 0:     // DP Number
					lcl_dp_idx = ((uint8_t)atoi(buf) - 1) * MP_SIZE_PER_DATA;
					buff_loc = lcl_dp_idx;
					
					#if (DEBUG_ALL || DEBUG_JSON_EXT)
					Print_Message("\nbuffer location: ");
					Print_Number(lcl_dp_idx);
					#endif
					
					U8 buff[5];
					memset(buff,0,5);
					int read_loc1 = (MP_LAST_VAL_UPDATE_LOC + (((uint8_t)atoi(buf) - 1) * MP_LAST_VAL_UPDATE_SIZE));
					EEPROM_Byte_Write(read_loc1,buff, MP_LAST_VAL_UPDATE_SIZE);
					
					int read_loc2 = (MP_SPECIFIC_TIME_LOC + (((uint8_t)atoi(buf) - 1) * MP_SPECIFIC_TIME_SIZE));
					EEPROM_Byte_Write(read_loc2, buff, MP_SPECIFIC_TIME_SIZE);
					
					uint16_t read_loc3 = MP_PERIODIC_TIME_LOC + (((uint8_t)atoi(buf) - 1) * MP_PERIODIC_TIME_SIZE);
					EEPROM_Byte_Write(read_loc3,buff, MP_PERIODIC_TIME_SIZE);
					
					break;

					case 3:     // Register Address (Store 2 bytes)
					extract.regAdds = (uint32_t)atoi(buf);
					
					#if (DEBUG_ALL || DEBUG_JSON_EXT)
					Print_Message("\nReceived register address : ");
					Print_Number(extract.regAdds);
					#endif
					mp_buffer[lcl_dp_idx++] = (extract.regAdds >> 8) & 0xFF;
					mp_buffer[lcl_dp_idx++] = (extract.regAdds) & 0xFF;
					
					break;

					case 6:     // Time String (Store Hour, Min as 1 byte each)
					{
						uint8_t hour = 0;
						uint8_t minute = 0;

						// Example buf = "10:15"
						char *colon = strchr(buf, ':');

						if (colon != NULL)
						{
							*colon = '\0';           // Split the string in-place
							hour = (uint8_t)atoi(buf);
							minute = (uint8_t)atoi(colon + 1);
						}
						
						#if (DEBUG_ALL || DEBUG_JSON_EXT)
						Print_Message("\n| HOUR   | : ");
						Print_Number(hour);
						Print_Message("\n| MINUTE | : ");
						Print_Number(minute);
						#endif
						// Store hour and minute separately
						mp_buffer[lcl_dp_idx++] = hour;
						mp_buffer[lcl_dp_idx++] = minute;

						break;
					}
					default:    // Other 1-byte values
					mp_buffer[lcl_dp_idx++] = (uint8_t)atoi(buf);
					break;
				}

				index2++;
			}
			#if (DEBUG_ALL || DEBUG_JSON_EXT)
			uint16_t idx;
			U8 len;
			
			Print_Message("\nWritable Flash data is : ");
			for (idx = buff_loc, len = 0; len < MP_SIZE_PER_DATA; idx++, len++)
			{
				Print_Number(mp_buffer[idx]);
				Print_Message(",");
			}
			#endif
			index1++;
		}

		CRC_Calculate_Check(mp_buffer, (MP_DATA_SIZE - 2), CRC_CALC);
		mp_buffer[(MP_DATA_SIZE - 2)] = gb_Temp_CRC[0];
		mp_buffer[(MP_DATA_SIZE - 1)] = gb_Temp_CRC[1];

		EEPROM_Byte_Write(MP_EEP_START_LOC, mp_buffer, MP_DATA_SIZE);
		
	}
	else
	{
		__NOP();
	}
	
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nCompleted extracting and storing MP data into Flash.");
	#endif
}

/*****************************************************************************
* Function name		: Extract_WR_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed OP array.
* Created by		: Shirisha Ankam
* Date created		: 20-11-2025
* Description		: Function to Extract the Frozen json frame and store in Flash memory.
*
* Notes				:  There is no storage for "WR" as it is live operation.
* Global Variables Affected : gen.data_arr, gb_Temp_CRC .
*****************************************************************************/
void Extract_WR_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting write register data using Frozen JSON");
	Print_Message(token.ptr);
	#endif

	struct json_token array_elements;
	uint8_t elem_index = 0;
	char buf[32];

	while (json_scanf_array_elem(token.ptr, token.len, "", elem_index, &array_elements) > 0)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		size_t copy_len = (array_elements.len < sizeof(buf) - 1) ?
		array_elements.len : sizeof(buf) - 1;

		memcpy(buf, array_elements.ptr, copy_len);
		buf[copy_len] = '\0';

		if (elem_index == 0)
		{
			/* DPN */
			gb_mb_write_req.dpt_num = (U8)atoi(buf);
		}
		else if (elem_index == 1)
		{
			/* VALUE : HEX string ? 2-byte register */
			uint16_t value = (uint16_t)strtol(buf, NULL, 16);

			gb_mb_write_req.data[0]  = (value >> 8) & 0xFF;  // MSB
			gb_mb_write_req.data[1]  = value & 0xFF;         // LSB
		}

		elem_index++;
	}
	
	Print_Message("\nTwo Bytes Modbus value for DPT : ");
	Print_Number(gb_mb_write_req.dpt_num);
	Print_Message(" is  ");
	for(U8 i = 0; i < 2; i++)
	{
		Print_Number(gb_mb_write_req.data[i]);
		Print_Message(",");
	}
	gb_write_to_req_f = 1;
	/*Write_to_Register_Functionality(do_ix, lcl_wr_state);*/
}




/*****************************************************************************
* Function name		: Extract_MS_Data_From_JSON
* Returns			: Nothing.
* Arguments			: struct json_token token) -> passes parsed DI array.
* Created by		: Shirisha Ankam
* Date Updated		: 03-12-2025
* Description		: Function to Extract the MS Frozen json frame and store in Flash memory.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr
*****************************************************************************/
void Extract_MS_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nStarted Extracting MS data using Frozen JSON\n");
	Print_Message(token.ptr);
	#endif

	struct json_token array_elements;
	uint8_t index = 0;
	uint8_t lcl_ms_idx = 0 ;

	memset(extract.data_arr, 0, MS_DATA_SIZE);

	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nExtracted values from MS array:\n");
	#endif

	while (json_scanf_array_elem(token.ptr, token.len, "", index, &array_elements) > 0)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		char buf[32];
		// Copy token into null-terminated buffer
		memset(buf, 0, sizeof(buf));
		memcpy(buf, array_elements.ptr, array_elements.len);  // element_arry.ptr is not null-terminated
		buf[array_elements.len] = '\0';

		/* Get modbus settings from server & update in below array based on output index. */	
		extract.data_arr[lcl_ms_idx++] = (uint8_t)atoi(buf);

	index ++;
	}

	// Compute CRC and also add last two bytes of array
	CRC_Calculate_Check(extract.data_arr, (MS_DATA_SIZE - 2), CRC_CALC);
	extract.data_arr[(MS_DATA_SIZE - 2)] = gb_Temp_CRC[0];	// Higher Byte
	extract.data_arr[(MS_DATA_SIZE - 1)] = gb_Temp_CRC[1];	// Lower Byte

	// Debug output
	//#if (DEBUG_ALL || DEBUG_DI_EXT)
	Print_Message("\nWritable Flash data is: ");
	for (U8 idx = 0; idx < MS_DATA_SIZE; idx++)
	{
		Print_Number(extract.data_arr[idx]);
		Print_Message(",");
	}
	//#endif
	
	// Store data into EEPROM
	EEPROM_Byte_Write(MS_EEP_START_LOC, extract.data_arr, MS_DATA_SIZE);

	Read_MS_Data_From_EEPROM();	// Call this function to update RAM variables.
	
	gb_reinit_uart = 1;
	#if (DEBUG_ALL || DEBUG_JSON_EXT)
	Print_Message("\nFinished extraction of modbus setting data.");
	#endif
}

/*****************************************************************************
* Function Name  : Extract_SI_Data_From_JSON
* Returns        : void
* Arguments      : struct json_token token
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
*
* Description    : Extracts Safety & Intruder (SI) data received in JSON
*                  format using Frozen JSON parser and stores it into EEPROM
*                  with CRC protection.
*
*                  Functional flow:
*                  - Reads existing SI data from EEPROM.
*                  - Validates EEPROM data integrity using CRC.
*                  - Initializes default SI data if CRC validation fails.
*                  - Parses SI data array from JSON payload.
*                  - Updates local SI buffer with extracted values.
*                  - Calculates and appends CRC to extracted data.
*                  - Writes updated SI data back to EEPROM.
*                  - Updates runtime SI variables from EEPROM.
*
* Notes          :
*                  - Expects JSON token to contain an array of SI bytes.
*                  - SI_EEP_MAX_BYTE must include CRC bytes.
*                  - Watchdog is serviced during JSON parsing loop.
*
* Dependencies   :
*                  Frozen JSON library,
*                  EEPROM_Byte_Read(),
*                  EEPROM_Byte_Write(),
*                  CRC_Calculate_Check()
*
* Global Variables Affected :
*                  extract.data_arr,
*                  gb_Temp_CRC,
*                  intruder (via Update_SI_Variables)
*****************************************************************************/
void Extract_SI_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting SI data using Frozen JSON");
	Print_Message(token.ptr);
	#endif
	
	memset(extract.data_arr, 0, SI_EEP_MAX_BYTE);

	U8 si_sts = EEPROM_Byte_Read(SI_EEP_START_LOC, extract.data_arr, SI_EEP_MAX_BYTE);
	if (si_sts == 0)
	{
		if (CRC_Calculate_Check(extract.data_arr, (SI_EEP_MAX_BYTE - 2), CRC_CHECK) == 1)
		{
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validated successful");
			#endif
			__NOP();
		}
		else
		{
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validation failed.");
			#endif
			memset(extract.data_arr, 0, SI_EEP_MAX_BYTE);
			CRC_Calculate_Check(extract.data_arr, (SI_EEP_MAX_BYTE - 2), CRC_CALC);
			extract.data_arr[(SI_EEP_MAX_BYTE - 2)] = gb_Temp_CRC[0];
			extract.data_arr[(SI_EEP_MAX_BYTE - 1)] = gb_Temp_CRC[1];
		}

		/* Extract SI data from JSON array */
		U8 index = 0;
		struct json_token array_elements;

		while (json_scanf_array_elem(token.ptr, token.len, "", index, &array_elements) > 0)
		{
			wdt_restart(WDT);	// Prevent watchdog reset during parsing
			extract.data_arr[index] = atoi(array_elements.ptr);
			index++;
		}

		#if (DEBUG_ALL || DEBUG_FROZEN)
		Print_Message("\nExtracted data is:");
		Print_Data_On_Uart(extract.data_arr, (SI_EEP_MAX_BYTE - 2));
		#endif
	}
	else
	{
		__NOP();
	}

	/* Calculate CRC for extracted SI data */
	CRC_Calculate_Check(extract.data_arr, (SI_EEP_MAX_BYTE - 2), CRC_CALC);
	extract.data_arr[(SI_EEP_MAX_BYTE - 2)] = gb_Temp_CRC[0];
	extract.data_arr[(SI_EEP_MAX_BYTE - 1)] = gb_Temp_CRC[1];

	/* Store SI data into EEPROM */
	EEPROM_Byte_Write(SI_EEP_START_LOC, extract.data_arr, SI_EEP_MAX_BYTE);

	/* Update runtime SI variables */
	Update_SI_Variables();

	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nSuccessfully extracted SI data using Frozen JSON");
	#endif
}


/*****************************************************************************
* Function Name  : Extract_AM_Data_From_JSON
* Returns        : void
* Arguments      : struct json_token token
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
*
* Description    : Extracts Arm/Disarm (AM) configuration data received in
*                  JSON format using the Frozen JSON parser and stores it
*                  into EEPROM with CRC protection.
*
*                  Functional flow:
*                  - Parses AM data from JSON array structure.
*                  - Extracts ARM/DISARM status and associated input list.
*                  - Populates AM data buffer sequentially.
*                  - Calculates and appends CRC to extracted AM data.
*                  - Writes validated AM data into EEPROM.
*                  - Updates runtime AM variables received from server.
*
* Notes          :
*                  - JSON format contains a top-level array with:
*                    [ ARM_STATUS, [INPUT_LIST...] ]
*                  - AM_EEP_MAX_BYTE must include CRC bytes.
*                  - Watchdog is serviced during JSON parsing loops.
*
* Dependencies   :
*                  Frozen JSON library,
*                  EEPROM_Byte_Write(),
*                  CRC_Calculate_Check()
*
* Global Variables Affected :
*                  extract.data_arr,
*                  extract.data_len,
*                  gb_Temp_CRC,
*                  intruder (via Update_AM_ByServer_Variables)
*****************************************************************************/
void Extract_AM_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting AM data using Frozen JSON");
	Print_Message(token.ptr);
	#endif
	
	memset(extract.data_arr, 0, AM_EEP_MAX_BYTE);
	
	/* Start extracting data from here */
	U8 index = 0;
	extract.data_len = 0;
	struct json_token array_elements;
	while (json_scanf_array_elem(token.ptr, token.len, "", index, &array_elements) > 0)
	{
		wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
		
		if (index == 0)
		{
			/* First byte indicates ARM / DISARM status */
			extract.data_arr[extract.data_len++] = atoi(array_elements.ptr);
		}
		else
		{
			/* Extract inner array containing input list */
			U8 index_1 = 0;
			struct json_token array_elements1;
			while (json_scanf_array_elem(array_elements.ptr, array_elements.len, "", index_1, &array_elements1) > 0)
			{
				wdt_restart(WDT);
				extract.data_arr[extract.data_len++] = atoi(array_elements1.ptr);
				index_1++;
			}
		}
		
		index++;
	}
	
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nExtracted data is : ");
	Print_Data_On_Uart(extract.data_arr, (AM_EEP_MAX_BYTE - 2));
	#endif
	
	/* Calculate CRC of the extracted data */
	CRC_Calculate_Check(extract.data_arr, (AM_EEP_MAX_BYTE - 2), CRC_CALC);
	extract.data_arr[(AM_EEP_MAX_BYTE - 2)] = gb_Temp_CRC[0];
	extract.data_arr[(AM_EEP_MAX_BYTE - 1)] = gb_Temp_CRC[1];
	
	/* Write AM data to EEPROM */
	EEPROM_Byte_Write(AM_EEP_START_LOC, extract.data_arr, AM_EEP_MAX_BYTE);
	
	Update_AM_ByServer_Variables();
	
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nSuccessfully extracted AM data using Frozen JSON");
	#endif
}


/*****************************************************************************
* Function Name  : Extract_AM_Data_From_JSON
* Returns        : void
* Arguments      : struct json_token token
* Created by     : Suryaprakash Shendge.
* Date Created   : 20/05/2026
*
* Description    : 
* Notes          :
* Dependencies   :
*
* Global Variables Affected :
*                  
*****************************************************************************/
void Extract_BT_Data_From_JSON(struct json_token token)
{
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nStarted Extracting BT data using Frozen JSON");
	Print_Message(token.ptr);
	Print_Message("\nExtracted values from BT array:\n");
	#endif
	
	char temp[255];
	strcpy(temp,(token.ptr));
	uint8_t bt_idx = 0;
	for(bt_idx = 0; temp[bt_idx] != '"'; bt_idx++)
	{
		if (bt_idx > 100)
		{
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nBT is not valid");
			#endif
			
			return;
		}
	}
	
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Send_Frame_On_UART(temp,bt_idx - 1);
	Print_Message("\nLength is : ");
	Print_Number(bt_idx);
	#endif
	
	#if (DEBUG_ALL || DEBUG_FROZEN)
	Print_Message("\nToken string : ");
	Print_Message(ServerCfg.auth_token);
	#endif
	
	// Read data from memory and update server token and write into memory.	
	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nWriting data to EEP1");
	#endif

// 	U8 read_config_data[MAX_BYTES];
// 	read_comn_settings(read_config_data);	// Read communication configuration and write to FLASH.

	U8 read_comn_data[256] = {0};
	U8 read_comn_data_idx = 0;

	// Read the data including CRC from flash memory
	EEPROM_Page_Read(UT_EEP_START_PAGE, UT_EEP_PAGE_OFFSET, read_comn_data, COMN_CONFIG_BYTES);
	
	gb_update_sev_token = FLAG_SET;
	write_comn_settings(read_comn_data);
	gb_update_sev_token = FLAG_RST;
	
	memset(ServerCfg.auth_token,0,256);
	
	memcpy(ServerCfg.auth_token, &temp[0], bt_idx);
	ServerCfg.auth_token[bt_idx] = '\0';
	
	
	for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)                     // Read and pass the ip address.
	read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_1[ip_addr_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
	read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_2[ip_addr_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
	read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_3[ip_addr_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 ip_addr_idx = 0; ip_addr_idx < 3; ip_addr_idx++)
	read_comn_data[read_comn_data_idx++] = IpAddr.ip_addr_4[ip_addr_idx];

	//      for(U8 port_num_idx = 0; port_num_idx < 5; port_num_idx++)                  // Read and pass the port number.
	//      read_comn_data[read_comn_data_idx++] = Port_Num[port_num_idx];

	for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)                  // Read and pass the subnet mask.
	read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_1[sbnt_msk_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
	read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_2[sbnt_msk_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
	read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_3[sbnt_msk_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 sbnt_msk_idx = 0; sbnt_msk_idx < 3; sbnt_msk_idx++)
	read_comn_data[read_comn_data_idx++] = SbntMsk.sbnt_msk_4[sbnt_msk_idx];

	for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)               // Read and pass the default gateway.
	read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_1[dflt_gtwy_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
	read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_2[dflt_gtwy_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
	read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_3[dflt_gtwy_idx];
	read_comn_data[read_comn_data_idx++] = '.';
	for(U8 dflt_gtwy_idx = 0; dflt_gtwy_idx < 3; dflt_gtwy_idx++)
	read_comn_data[read_comn_data_idx++] = DfltGtwy.dflt_gtwy_4[dflt_gtwy_idx];
	
	// Read the server endpoint
	
	Print_Message("\nServer end point is : ");
	Print_Message(ServerCfg.server_endpoint);
	
	int endlen = strlen(ServerCfg.server_endpoint);
	read_comn_data[read_comn_data_idx++] = endlen;   // store end point len
	
	memcpy(&read_comn_data[read_comn_data_idx], ServerCfg.server_endpoint, endlen);
	read_comn_data_idx += endlen;
	
	// Read the authentication token
	
	Print_Message("\nServer auth is : ");
	Print_Message(ServerCfg.auth_token);
	
	int tokenlen = strlen(ServerCfg.auth_token);
	read_comn_data[read_comn_data_idx++] = tokenlen;   //store token len
	
	memcpy(&read_comn_data[read_comn_data_idx], ServerCfg.auth_token, tokenlen);
	
	read_comn_data_idx += tokenlen; 
	
	Write_ConfData_To_EEP_1_Memory(UT_EEP_START_LOC, read_comn_data, COMN_CONFIG_BYTES);

	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nData written to EEP1");
	#endif
}

/*****************************************************************************
* Function name		: Create_JSON_Frame
* Returns			: Nothing.
* Arguments			: multiple values need to be send.
* Created by		: Suryaprakash Shendge
* Date created		: 21-08-2025
* Updated by		: Shirisha Ankam
* Date Updated		: 21-11-2025
* Description		: Function to Create a json frame to send the server.
*
* Notes				: NA
* Global Variables Affected : gen.data_arr
*****************************************************************************/
void Create_JSON_Frame(const char* dev_mac_id, const char* ipAddress, U8 model_type,
U8 dev_type, const uint8_t* lastDtTime, int dp,
const uint8_t* logEntries, U8 numEntries, char* jsonBuffer, U16 bufferSize)
{
	char lcl_data_buffer[SERVER_DATA_SIZE];
	U16 lcl_data_len = 0;
	memset(lcl_data_buffer, 0, SERVER_DATA_SIZE);
	
	if (numEntries > 0)
	{
		gb_more_data_f = 1;
	}
	else
	gb_more_data_f = 0;
	
	/* Add device MAC ID field */
	char devMacIDStr[18] = {0};  // 6 bytes * 2 chars each.
	for (U8 i = 0; i < 6; i++)	// Len of MAC ID is 6 bytes.
	{
		char temp[4];
		snprintf(temp, sizeof(temp), "%02X", dev_mac_id[i]);
		strcat(devMacIDStr, temp);
	}

	lcl_data_buffer [0] = '{' ;
	lcl_data_buffer [1] = '"' ;
	lcl_data_len = strlen(lcl_data_buffer);
	strcpy((lcl_data_buffer+lcl_data_len),"MI") ;
	lcl_data_len = strlen(lcl_data_buffer);
	lcl_data_buffer [lcl_data_len] = '"' ;
	lcl_data_buffer [lcl_data_len+1] = ':' ;
	lcl_data_buffer [lcl_data_len+2] = '"' ;
	strncpy((lcl_data_buffer + lcl_data_len + 3), devMacIDStr, 18) ;
	lcl_data_len = strlen(lcl_data_buffer);
	lcl_data_buffer [lcl_data_len] = '"' ;
	lcl_data_buffer [lcl_data_len+1] = ',' ;
	/* End of Add device MAC ID field */

	/* Once the connection established with the server do not send following two fields IA and MT */
	if (gb_dev_connect_f != 1)
	{
		/* Add device IP Address field */
		unsigned long long numericIPAddress = 0; // Variable to store the numeric value
		// Convert IP address string to a numeric value
		for (const char* ptr = ipAddress; *ptr != '\0'; ++ptr)
		{
			if (*ptr >= '0' && *ptr <= '9')
			{
				// Only process numeric characters
				numericIPAddress = numericIPAddress * 10 + (*ptr - '0');
			}
		}
		// Add the numeric IP address to the JSON object

		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		lcl_data_len = strlen(lcl_data_buffer) ;
		strncpy((lcl_data_buffer+lcl_data_len),"IA",sizeof("IA"));
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = ':' ;
		lcl_data_len = strlen(lcl_data_buffer) ;
		sprintf((lcl_data_buffer + lcl_data_len),"%llu",numericIPAddress);
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = ',' ;
		/* End of Add device IP Address field */
			
		/* Add Model type field */
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		strncpy((lcl_data_buffer+lcl_data_len+1),"MT",sizeof("MT"));
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		lcl_data_buffer [lcl_data_len+1] = ':' ;
		sprintf((lcl_data_buffer + lcl_data_len+2),"%d",model_type);
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = ',' ;
		/* End of Add Model type field */
			
		/* Add Device type field */
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		strncpy((lcl_data_buffer+lcl_data_len+1),"DT",sizeof("DT"));
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = '"' ;
		lcl_data_buffer [lcl_data_len+1] = ':' ;
		sprintf((lcl_data_buffer + lcl_data_len+2),"%d", dev_type);
		lcl_data_len = strlen(lcl_data_buffer) ;
		lcl_data_buffer [lcl_data_len] = ',' ;
		/* End of Add Device type field */
	}

	/* Add last date & time of poll field */
	char dateTimeStr[15] = {0};
	snprintf(dateTimeStr, sizeof(dateTimeStr), "%02u%02u%02u%02u%02u%02u",
	lastDtTime[0] + 2000, lastDtTime[1], lastDtTime[2],
	lastDtTime[3], lastDtTime[4], lastDtTime[5]);

	lcl_data_len = strlen(lcl_data_buffer) ;
	lcl_data_buffer [lcl_data_len] = '"' ;
	strncpy((lcl_data_buffer+lcl_data_len+1),"LDT",sizeof("LDT"));
	lcl_data_len = strlen(lcl_data_buffer) ;
	lcl_data_buffer [lcl_data_len] = '"' ;
	lcl_data_buffer [lcl_data_len+1] = ':' ;
	lcl_data_buffer [lcl_data_len+2] = '"' ;
	strcpy((lcl_data_buffer + lcl_data_len+3),dateTimeStr);
	lcl_data_len = strlen(lcl_data_buffer);
	lcl_data_buffer [lcl_data_len] = '"' ;
	lcl_data_buffer [lcl_data_len+1] = ',' ;
	/* End of Add last date & time of poll field */

	/* Add DP (data packet field) */
	lcl_data_len = strlen(lcl_data_buffer) ;
	lcl_data_buffer [lcl_data_len] = '"' ;
	strncpy((lcl_data_buffer+lcl_data_len+1),"DP",strlen("DP"));
	lcl_data_len = strlen(lcl_data_buffer) ;
	lcl_data_buffer [lcl_data_len ++] = '"' ;
	lcl_data_buffer [lcl_data_len ++] = ':' ;
	sprintf((lcl_data_buffer + lcl_data_len), "%d", dp);
	lcl_data_len = strlen(lcl_data_buffer);
	lcl_data_buffer [lcl_data_len] = ',' ;
	/* End of Add DP (data packet field) */

	
	/*****
	* Base + TL + OC + OD + GA + CS Should be less than 620 //1500 Bytes.
	* 1318 + 59(1 arr) + 16(1 arr) + 26(1 arr) + 6(1 arr) + 63
	*****/
	
	//Limit for number of logs is calculated considering TCP header.
	const U8 base_size = 138;	// 131 Characters.
	const U8 is_1arr_size = 28;
 	const U8 md_1arr_size = 38;
	const U8 cs_1arr_size = 63;
	const U8 erp_tm_size = 10;
	const U8 op_1arr_size = 15;
	const U8 as_frame_size = 10;

 	U8 lcl_frame_len_exceed_f = 0;
 	U16 rem_bytes = SERVER_DATA_SIZE;
 	U16 temp_rem_bytes = 0;
 	char lcl_data_array[500] = {0};
 	U8 lcl_data_idx = 0;
	
	/* If CS or OS is present in the frame reset below flags bcz, when CS is there IS & MD frames are not sending */
	if ((gb_send_cs_to_server == FLAG_SET) || (gb_op_detected_f == FLAG_SET) ||  (gb_op_at_pwr_on == FLAG_SET) || (gb_arm_send_server_f == FLAG_SET))
	{
		gb_update_md_last_sent_loc_f = 0;
		gb_update_is_last_sent_loc_f = 0;
	}
	else
	__NOP();
	
	/* Generate transaction log if enabled. */
	if ((dm_dev_info.dev_sts == APPROVED) && (dm_dev_info.pause_rsm == RESUME)
	&& (gb_send_cs_to_server != FLAG_SET) && (gb_op_detected_f != FLAG_SET)
	&& (gb_op_at_pwr_on != FLAG_SET) && (gb_arm_send_server_f != FLAG_SET))
	{
		// Create "IS" array (outer array)
		if ((numEntries > 0) && (numEntries <= MAX_NUM_IS_LOGS))
		{
			if(gb_send_input_data_to_serverf == 1)
			{
				gb_send_input_data_to_serverf = 0;
				lcl_data_len = strlen(lcl_data_buffer) ;
				lcl_data_buffer [lcl_data_len] = '"' ;
				strncpy((lcl_data_buffer + lcl_data_len + 1),"IS",strlen("IS"));
				lcl_data_len = strlen(lcl_data_buffer) ;
				lcl_data_buffer [lcl_data_len] = '"' ;
				lcl_data_buffer [lcl_data_len+1] = ':' ;
				lcl_data_buffer [lcl_data_len+2] = '[' ;

// 				Print_Message("\nnumEntries for IS -----------: ");
// 				Print_Number(numEntries);

				// Parse each log entry 
				U8 logEntrySize = IS_BUFFER_LEN;
				for (U8 i = 0; i < numEntries; ++i)
				{
					memset(lcl_data_array, 0, sizeof(lcl_data_array));		// Clear buffer before use.
					lcl_data_idx = 0;	// Clear Index before use. (Initialize to '0').
					
					const uint8_t* entry = logEntries + (i * logEntrySize);  // Point to the current log entry

					lcl_data_array [lcl_data_idx ++] = '[' ;

					/* Format timestamp from bytes 10-16: "DDMMYYHHmmSSDY" */
					char formattedTimestamp[15];
					snprintf(formattedTimestamp, sizeof(formattedTimestamp), "%02u%02u%02u%02u%02u%02u%02u",
					entry[0], entry[1], entry[2],  // Day, Month, Year
					entry[4], entry[5], entry[6], entry[3]);  // Hour, Minute, Second, Day of Year
					lcl_data_array[lcl_data_idx++] = '"' ;
					strncpy((lcl_data_array+lcl_data_idx),formattedTimestamp,strlen(formattedTimestamp));
					lcl_data_idx = strlen(lcl_data_array) ;
					lcl_data_array[lcl_data_idx++] = '"' ;
					lcl_data_array[lcl_data_idx++] = ',' ;
					
					/* Input number (byte 7) */
					sprintf((lcl_data_array + lcl_data_idx),"%d", entry[7]);
					lcl_data_idx = strlen(lcl_data_array) ;
					lcl_data_array[lcl_data_idx++] = ',' ;
					
					/* Input state (byte 7) */
					sprintf((lcl_data_array + lcl_data_idx),"%d",entry[8]);
					lcl_data_idx = strlen(lcl_data_array) ;
					lcl_data_array[lcl_data_idx++] = ']'  ;
					lcl_data_array[lcl_data_idx++] = ',' ;

					// Add TL array to the outer log array
					lcl_data_len = strlen(lcl_data_buffer);
					strncpy((lcl_data_buffer+lcl_data_len),lcl_data_array,strlen(lcl_data_array));
				}
				lcl_data_len = strlen(lcl_data_buffer);
				lcl_data_buffer[lcl_data_len-1] = ']'  ;
				lcl_data_buffer[lcl_data_len] = ',' ;
			}
		}
		
		if (numEntries < MAX_NUM_IS_LOGS) 
		{
			/* Check if num of entries are less than maximum number of logs */
			rem_bytes = (SERVER_DATA_SIZE - (base_size + (is_1arr_size * numEntries)));
			
			if (numEntries >= MAX_NUM_MD_LOGS)
			{
				 if (((numEntries - MAX_NUM_MD_LOGS) * MD_BUFFER_LEN) > IS_BUFFER_LEN)
				 {
					 lcl_frame_len_exceed_f = FLAG_SET;
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
		}
		else
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
		
		U8 md_entries = 0;			
		if (lcl_frame_len_exceed_f != FLAG_SET)
		{
			U8 md_logs = 0;
			if (numEntries > 0)
			md_logs = (((MAX_NUM_IS_LOGS - numEntries) * is_1arr_size) / md_1arr_size);
			else
			md_logs = MAX_NUM_MD_LOGS;
			
			U8 md_entries = 0;		
			U8* mdBuffer = Send_Modbus_Data_Packets_ToServer(md_logs, &md_entries);
			
			if ((numEntries + md_entries) > 0)
			{
				gb_more_data_f = 1;
			}
			else
			gb_more_data_f = 0;
			
			
			/* Create MD array */
			if ((rem_bytes < md_1arr_size) && (gb_send_modbus_data_to_serverf == FLAG_SET))
			{
				lcl_frame_len_exceed_f = FLAG_SET;
			}
			else if (gb_send_modbus_data_to_serverf == FLAG_SET)
			{
				U8 lcl_md_loop = (U8)(rem_bytes / md_1arr_size);
				if (lcl_md_loop > 1)
				lcl_md_loop = 1;
				else
				__NOP();
				
				rem_bytes = (rem_bytes - (lcl_md_loop * md_1arr_size));
			}
			else
			{
				__NOP();
			}
			if ((md_entries > 0) && (md_entries <= MAX_NUM_MD_LOGS))
			{
				if ((gb_send_modbus_data_to_serverf == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET))
				{
					gb_send_modbus_data_to_serverf = FLAG_RST;
					lcl_frame_len_exceed_f = FLAG_RST;
						
					lcl_data_len = strlen(lcl_data_buffer) ;
					lcl_data_buffer [lcl_data_len] = '"' ;
					strncpy((lcl_data_buffer + lcl_data_len + 1),"MD",strlen("MD"));
					lcl_data_len = strlen(lcl_data_buffer) ;
					lcl_data_buffer [lcl_data_len] = '"' ;
					lcl_data_buffer [lcl_data_len+1] = ':' ;
					lcl_data_buffer [lcl_data_len+2] = '[' ;

// 					Print_Message("\nnumEntries for MD -----------: ");
// 					Print_Number(md_entries);

					// Parse each log entry (22 bytes per entry)
					U8 logEntrySize = MD_BUFFER_LEN;
					for (U8 i = 0; i < md_entries; ++i)
					{
						memset(lcl_data_array, 0, 200);		// Clear buffer before use.
						lcl_data_idx = 0;	// Clear Index before use. (Initialize to '0').
								
						const uint8_t* entry = mdBuffer + (i * logEntrySize);  // Point to the current log entry

						lcl_data_array [lcl_data_idx ++] = '[' ;

						/* Format timestamp from bytes 10-16: "DDMMYYHHmmSSDY" */
						char formattedTimestamp[15];
						snprintf(formattedTimestamp, sizeof(formattedTimestamp), "%02u%02u%02u%02u%02u%02u%02u",
						entry[0], entry[1], entry[2],  // Day, Month, Year
						entry[4], entry[5], entry[6], entry[3]);  // Hour, Minute, Second, Day of Year
						lcl_data_array[lcl_data_idx++] = '"' ;
						strncpy((lcl_data_array+lcl_data_idx),formattedTimestamp,strlen(formattedTimestamp));
						lcl_data_idx = strlen(lcl_data_array) ;
						lcl_data_array[lcl_data_idx++] = '"' ;
						lcl_data_array[lcl_data_idx++] = ',' ;
								
						/* DPT number (byte 7) */
						sprintf((lcl_data_array + lcl_data_idx),"%d", entry[7]);
						lcl_data_idx = strlen(lcl_data_array) ;
						lcl_data_array[lcl_data_idx++] = ',' ;
								
						lcl_data_array[lcl_data_idx++] = '"' ;
						/* DP value : 4 bytes ? HEX string */
						sprintf(&lcl_data_array[lcl_data_idx],
						"%02X%02X%02X%02X",
						entry[8], entry[9], entry[10], entry[11]);
						lcl_data_idx = strlen(lcl_data_array) ;
						lcl_data_array[lcl_data_idx++] = '"' ;
						lcl_data_array[lcl_data_idx++] = ']'  ;
						lcl_data_array[lcl_data_idx++] = ',' ;

						// Add TL array to the outer log array
						lcl_data_len = strlen(lcl_data_buffer);
						strncpy((lcl_data_buffer+lcl_data_len),lcl_data_array,strlen(lcl_data_array));
					}
					lcl_data_len = strlen(lcl_data_buffer);
					lcl_data_buffer[lcl_data_len-1] = ']'  ;
					lcl_data_buffer[lcl_data_len] = ',' ;
				}
			}
			
		}
		
		if (md_entries < MAX_NUM_MD_LOGS)
		{
			/* Check if num of entries are less than maximum number of logs */
			rem_bytes = (SERVER_DATA_SIZE - (base_size + (md_1arr_size * md_entries)));
		}
		else
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
						
	}				
		
	if ((dm_dev_info.dev_sts == APPROVED) && (lcl_frame_len_exceed_f != FLAG_SET))
	{
		/* Create CS array */
		if ((rem_bytes < cs_1arr_size) && (gb_send_cs_to_server == FLAG_SET))
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
		else if (gb_send_cs_to_server == FLAG_SET)
		{
			U8 lcl_cs_loop = (U8)(rem_bytes / cs_1arr_size);
			if (lcl_cs_loop > 1)
			lcl_cs_loop = 1;
			else
			__NOP();
				
			rem_bytes = (rem_bytes - (lcl_cs_loop * cs_1arr_size));
		}
		else
		{
			__NOP();
		}

		if ((gb_send_cs_to_server == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET))
		{
			/*gb_send_cs_to_server = FLAG_RST;*/
			lcl_frame_len_exceed_f = FLAG_RST;

			lcl_data_len = strlen(lcl_data_buffer) ;
			lcl_data_buffer [lcl_data_len] = '"' ;
			strncpy((lcl_data_buffer+lcl_data_len+1),"CS",strlen("CS"));
			lcl_data_len = strlen(lcl_data_buffer) ;
			lcl_data_buffer [lcl_data_len] = '"' ;
			lcl_data_buffer [lcl_data_len+1] = ':' ;
			lcl_data_buffer [lcl_data_len+2] = '[' ;
			lcl_data_len = strlen(lcl_data_buffer) ;

			for (U8 idx = 0; idx < TOTAL_SLAVES; idx ++)
			{
				sprintf((lcl_data_buffer + lcl_data_len), "%d", active_slaves[idx]);
				lcl_data_len = strlen(lcl_data_buffer);
				lcl_data_buffer[lcl_data_len ++] = ',';
			}
			lcl_data_buffer [lcl_data_len-1] = ']' ;
			lcl_data_buffer [lcl_data_len] = ',' ;
		}
		else
		__NOP();
		/* End of Create CS array */

		/* Create ME (Modbus Exception) array : "ME":[[SA,RE,WE],...], sent on change of value.
		   RE = read exception code, WE = write exception code, cleared error sent as 0 until server ack. */
		if ((gb_send_me_to_server == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET))
		{
			char lcl_me_buf[(TOTAL_SLAVES * 14) + 10];	// "ME":[ + 16 x [16,255,255], + ],
			U8 lcl_me_rd[TOTAL_SLAVES];
			U8 lcl_me_wr[TOTAL_SLAVES];
			U8 lcl_me_clr[TOTAL_SLAVES];
			U8 lcl_me_chg = 0;
			U8 lcl_me_cnt = 0;
			U16 lcl_me_len = 0;

			taskENTER_CRITICAL();	// Status is updated from Modbus task
			memcpy(lcl_me_rd, gb_mb_exc_status, TOTAL_SLAVES);
			memcpy(lcl_me_wr, gb_mb_wr_exc_status, TOTAL_SLAVES);
			memcpy(lcl_me_clr, gb_mb_exc_clr, TOTAL_SLAVES);
			lcl_me_chg = gb_me_change_cnt;
			taskEXIT_CRITICAL();

			memset(lcl_me_buf, 0, sizeof(lcl_me_buf));
			strcpy(lcl_me_buf, "\"ME\":[");
			for (U8 idx = 0; idx < TOTAL_SLAVES; idx++)
			{
				if ((lcl_me_rd[idx] != 0) || (lcl_me_wr[idx] != 0) || (lcl_me_clr[idx] == 1))	// Error present or cleared
				{
					lcl_me_len = strlen(lcl_me_buf);
					sprintf((lcl_me_buf + lcl_me_len), "[%d,%d,%d],", (idx + 1), lcl_me_rd[idx], lcl_me_wr[idx]);
					lcl_me_cnt++;
				}
				else
				__NOP();
			}

			if (lcl_me_cnt == 0)
			{
				gb_me_sent_cnt = lcl_me_chg;	// Nothing to report, ack may clear the flag
			}
			else
			{
				lcl_me_len = strlen(lcl_me_buf);
				lcl_me_buf[lcl_me_len - 1] = ']';	// Replace last comma
				lcl_me_buf[lcl_me_len] = ',';
				lcl_me_len++;

				if (lcl_me_len <= rem_bytes)
				{
					lcl_data_len = strlen(lcl_data_buffer);
					memcpy((lcl_data_buffer + lcl_data_len), lcl_me_buf, (lcl_me_len + 1));
					rem_bytes = (rem_bytes - lcl_me_len);
					gb_me_sent_cnt = lcl_me_chg;	// Ack of this frame clears ME
				}
				else
				{
					lcl_frame_len_exceed_f = FLAG_SET;	// No space, ME stays pending for next frame
				}
			}
		}
		else
		__NOP();
		/* End of Create ME array */
		
		/* Send Output Status to server */
		
		U8 lcl_op_en_f = 0;
		for (U8 idx = 0; idx < MAX_OUTPUTS; idx++)
		{
			if (gb_output[idx].op_en == 1)						// Skip disabled outputs
			{
				lcl_op_en_f = 1;
				break;
			}
			else
			__NOP();
		}
		
		if (lcl_op_en_f == 0)
		{
			gb_op_at_pwr_on = 0;
			gb_op_detected_f = 0;
		}
		else
		__NOP();
		
		if ((rem_bytes < op_1arr_size) && ((gb_op_detected_f == FLAG_SET) || (gb_op_at_pwr_on == 1)))
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
		else if ((gb_op_detected_f == FLAG_SET)  || (gb_op_at_pwr_on == 1))
		{
			U8 lcl_op_loop = (U8)(rem_bytes / op_1arr_size);
			if (lcl_op_loop > 1)
			lcl_op_loop = 1;
			else
			__NOP();
				
			rem_bytes = (rem_bytes - (lcl_op_loop * op_1arr_size));
		}
		else
		{
			__NOP();
		}
		
		if (((gb_op_detected_f == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET)) || ((gb_op_at_pwr_on == 1) && (lcl_frame_len_exceed_f != FLAG_SET)))
		{
		
			lcl_frame_len_exceed_f = FLAG_RST;

			/* Add OS key */
			lcl_data_len = strlen(lcl_data_buffer);
			sprintf((lcl_data_buffer + lcl_data_len), "\"OS\":[");
			lcl_data_len = strlen(lcl_data_buffer);

			U8 os_cnt = 0;

			for (U8 idx = 0; idx < MAX_OUTPUTS; idx++)
			{
				if(gb_output[idx].op_en == 0)						// Skip disabled outputs
				{
					continue;
				}
									
				if ((gb_previous_op_st[idx] != gb_output_state[idx]) || (gb_op_at_pwr_on == 1))
				{					
					gb_previous_op_st[idx] = gb_output_state[idx];
				
					sprintf((lcl_data_buffer + lcl_data_len),
							"[%d,%d],",
							idx + 1,
							gb_output_state[idx]);

					lcl_data_len = strlen(lcl_data_buffer);
					os_cnt++;
				}
			}
			
			/*gb_op_at_pwr_on = FLAG_RST;*/
			/*gb_op_detected_f = FLAG_RST;*/	
			/* Close OS array */
			if (os_cnt > 0)
			{
				lcl_data_buffer[lcl_data_len - 1] = ']';  // remove last comma
				lcl_data_buffer[lcl_data_len] = ',';
			}
			else
			{
				lcl_data_buffer[lcl_data_len++] = ']';
				lcl_data_buffer[lcl_data_len++] = ',';
			}
		}
		else
		{
			__NOP();
		}

		/* End of sending Output state to server" */

		/* Add "Request new time from the ERP server part" */
		if ((rem_bytes < erp_tm_size) && (gb_req_erp_time_f == FLAG_SET))
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
		else if (gb_req_erp_time_f == FLAG_SET)
		{
			U8 lcl_erp_loop = (U8)(rem_bytes / erp_tm_size);
			if (lcl_erp_loop > 1)
			lcl_erp_loop = 1;
			else
			__NOP();

			rem_bytes = (rem_bytes - (lcl_erp_loop * erp_tm_size));
		}
		else
		{
			__NOP();
		}

		if ((gb_req_erp_time_f == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET))
		{
			lcl_frame_len_exceed_f = FLAG_RST;

			lcl_data_len = strlen(lcl_data_buffer) ;
			lcl_data_buffer [lcl_data_len] = '"' ;
			strncpy((lcl_data_buffer+lcl_data_len+1),"NT",strlen("NT"));
			lcl_data_buffer [lcl_data_len+3] = '"' ;
			lcl_data_buffer [lcl_data_len+4] = ':' ;
			lcl_data_buffer [lcl_data_len+5] = '1' ;
			lcl_data_buffer [lcl_data_len+6] = ',' ;
		}
		else
		__NOP();
		/* End of Add "Request new time from the ERP server part" */
		
		/* Add AS (Arm Disarm Status) frame */
		if ((rem_bytes < as_frame_size) && (gb_arm_send_server_f == FLAG_SET))
		{
			lcl_frame_len_exceed_f = FLAG_SET;
		}
		else if (gb_arm_send_server_f == FLAG_SET)
		{
			U8 lcl_as_loop = (U8)(rem_bytes / as_frame_size);
			if (lcl_as_loop > 1)
			lcl_as_loop = 1;
			else
			__NOP();

			rem_bytes = (rem_bytes - (lcl_as_loop * as_frame_size));
		}
		else
		{
			__NOP();
		}

		if ((gb_arm_send_server_f == FLAG_SET) && (lcl_frame_len_exceed_f != FLAG_SET))
		{
			lcl_frame_len_exceed_f = FLAG_RST;
			
			gb_arm_send_server_f = FLAG_RST;
			gb_arm_wait_ser_ack_f = FLAG_SET;	// Set a flag to check an ack from the server.

			lcl_data_len = strlen(lcl_data_buffer) ;
			lcl_data_buffer [lcl_data_len ++] = '"' ;
			strncpy((lcl_data_buffer + lcl_data_len), "AS", strlen("AS"));
			lcl_data_len = strlen(lcl_data_buffer);
			lcl_data_buffer [lcl_data_len ++] = '"' ;
			lcl_data_buffer [lcl_data_len ++] = ':' ;
			// Convert intruder.arm_status to string (assuming it's an int)
			sprintf(lcl_data_buffer + lcl_data_len, "%d", intruder.arm_status);
			lcl_data_len = strlen(lcl_data_buffer);
			lcl_data_buffer [lcl_data_len ++] = ',' ;
		}
		else
		__NOP();
		/* End of AS (Arm Disarm Status) frame */
		
	}
	else
	__NOP();
	
	lcl_data_len = strlen(lcl_data_buffer)-1;
	lcl_data_buffer[lcl_data_len] = '}' ;
	
	memset(jsonBuffer, 0, SERVER_DATA_SIZE);
	
	#if 0
	Print_Message("\njsonBuffer --------: ");
	Print_Message(jsonBuffer);
	#endif
	
	memcpy(jsonBuffer, lcl_data_buffer, (strlen(lcl_data_buffer) + 1)) ;
	
	#if 0
	Print_Message("\njsonBuffer ++++++: ");
	Print_Message(jsonBuffer);
	#endif

	#if 1
	Print_Message("\n\n===========================================");
	Print_Message("\nJSON Length : ");
	Print_Number(strlen(lcl_data_buffer));
	Print_Message("\nGenerated JSON String is:\n");
	Print_Message(lcl_data_buffer);
	Print_Message("\n===========================================\n");
	#endif

	memset(lcl_data_buffer, 0, sizeof(lcl_data_buffer));
	lcl_data_len = 0 ;
}


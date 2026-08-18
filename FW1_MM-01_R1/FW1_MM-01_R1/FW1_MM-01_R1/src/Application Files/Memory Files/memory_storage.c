/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: memory_storage.c
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
#include "memory_storage.h"
#include "user_uart.h"
#include "ext_eeprom_1.h"
#include "definitions.h"
#include "memory_storage.h"
#include "CRC.h"
#include "mm_app.h"
#include "input_output.h"
#include "modbus_app.h"
#include "Modbus.h"
#include <string.h>


/*****************************************************************************
* Function Name  : Read_DI_Data_From_EEPROM
* Returns        : void
* Arguments      : U8 * di_data ---> Pointer to the DI buffer.
* Created by     : Shirisha Ankam
* Date Created   : 10/03/2025
* Description    : Reads Device Information (DI) data from EEPROM memory, validates it using
*                  CRC-16, and segregates it into a buffer. If CRC validation fails,
*                  it clears the data, recalculates the CRC, and writes the corrected data
*                  back to EEPROM memory.
* Notes          : Ensures the data integrity of Device Information (DI) data by validating it with CRC
*                  and handles errors by writing default values when necessary.
* Global Variables Affected : gb_Temp_CRC
*****************************************************************************/
void Read_DI_Data_From_EEPROM(U8 *di_data)
{
	#if (DEBUG_ALL || DEBUG_POWER_ON)
	Print_Message("\nPower on Condition for DI");
	#endif
	
	// Read Device Information (DI) data from EEPROM memory
	U8 lcl_eep_buf[DI_DATA_SIZE];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, DI_DATA_SIZE);
	lcl_eep_status = EEPROM_Byte_Read(DI_EEP_START_LOC, lcl_eep_buf, DI_DATA_SIZE);
	if (lcl_eep_status == 0)  // Check if the EEPROM read was successful
	{
		// Perform CRC validation on the data (excluding the last 2 bytes, which are CRC)
		if (CRC_Calculate_Check(lcl_eep_buf, (DI_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// CRC validation passed
			#if (DEBUG_ALL || DEBUG_POWER_ON)
			Print_Message("\nCRC Validated for DI data");
			#endif
		}
		else
		{
			// CRC validation failed, handle error
			#if (DEBUG_ALL || DEBUG_POWER_ON)
			Print_Message("\nCRC failed for DI data");
			#endif
			
			memset(lcl_eep_buf, 0, DI_DATA_SIZE);  // Clear the data buffer
			
			// Recalculate CRC for the cleared data
			CRC_Calculate_Check(lcl_eep_buf, (DI_DATA_SIZE - 2), CRC_CALC);
			lcl_eep_buf[(DI_DATA_SIZE - 2)] = gb_Temp_CRC[0];  // Store higher byte of CRC
			lcl_eep_buf[(DI_DATA_SIZE - 1)] = gb_Temp_CRC[1];  // Store lower byte of CRC

			// Write the corrected data back to EEPROM memory
			EEPROM_Byte_Write(DI_EEP_START_LOC, lcl_eep_buf, DI_DATA_SIZE);
		}
		
		// Copy the validated/corrected data into the caller’s buffer
		memcpy(di_data, lcl_eep_buf, DI_DATA_SIZE);

		dm_dev_info.poll_freq = lcl_eep_buf[POL_FRQ_IDX];	// Get poll frequency value.
		dm_dev_info.sync_time = lcl_eep_buf[SYNC_TM_IDX];	// Get sync time value. (Sync with ERP or NTP)
		dm_dev_info.dev_sts = lcl_eep_buf[DEV_STS_IDX];		// Get Device status value, Approved or not?
		dm_dev_info.pause_rsm = lcl_eep_buf[PS_RSM_IDX];	// Get transaction status, Pause or resume?
		dm_dev_info.facility_code = lcl_eep_buf[FC_IDX];	// Get facility code?
		dm_dev_info.time_zone_hr = lcl_eep_buf[TZ_IDX_H];	// Get Hour Info
		dm_dev_info.time_zone_mn = lcl_eep_buf[TZ_IDX_M];	// Get Minute Info
		
		#if 1
		Print_Message("\nFacility code is : ");
		Print_Number(dm_dev_info.facility_code);
		
		Print_Message("\nDev status is : ");
		Print_Number(dm_dev_info.dev_sts);
		
		Print_Message("\nsynct time with : ");
		Print_Number(dm_dev_info.sync_time);
		#endif
		
		// Update poll frequency. 30 secs or 60 secs.
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
	else
	{
		__NOP();
	}
}

/*****************************************************************************
* Function Name  : Read_MS_Data_From_EEPROM
* Returns        : void
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 10/03/2025
* Description    : Reads Modbus Setting (MS) data from EEPROM memory, validates it using
*                  CRC-16, and segregates it into a buffer. If CRC validation fails,
*                  it clears the data, recalculates the CRC, and writes the corrected data
*                  back to EEPROM memory.
* Notes          : Ensures the data integrity ofModbus setting (MS) data by validating it with CRC
*                  and handles errors by writing default values when necessary.
* Global Variables Affected : gb_Temp_CRC
*****************************************************************************/
void Read_MS_Data_From_EEPROM(void)
{
	#if (DEBUG_ALL || DEBUG_POWER_ON)
	Print_Message("\n************** Read Data for MS *****************");
	#endif

	// Read Device Information (DI) data from EEPROM memory
	U8 lcl_eep_buf[MS_DATA_SIZE];
	U8 lcl_eep_status = 0;
	memset(lcl_eep_buf, 0, MS_DATA_SIZE);
	lcl_eep_status = EEPROM_Byte_Read(MS_EEP_START_LOC, lcl_eep_buf, MS_DATA_SIZE);
	if (lcl_eep_status == 0)  // Check if the EEPROM read was successful
	{
		// Perform CRC validation on the data (excluding the last 2 bytes, which are CRC)
		if (CRC_Calculate_Check(lcl_eep_buf, (MS_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// CRC validation passed
			#if (DEBUG_ALL || DEBUG_POWER_ON)
			Print_Message("\nCRC Validated for MS data");
			#endif
			
			
			// Update doudrate..
			if (lcl_eep_buf[0] == 0)
			ms_info.boudrate = 1200;
			else if (lcl_eep_buf[0] == 1)
			ms_info.boudrate = 9600;
			else if (lcl_eep_buf[0] == 2)
			ms_info.boudrate = 19200;
			else
			ms_info.boudrate = 9600; //(default)
			
			ms_info.parity_bit = lcl_eep_buf[1];
			ms_info.stop_bit = lcl_eep_buf[2];
			
			// Update poll frequency. 30 secs or 60 secs..
			if (lcl_eep_buf[3] == 1)
			ms_info.poll_interval = 100;	// 100 miliSecs.
			else if (lcl_eep_buf[3] == 2)
			ms_info.poll_interval = 250;	// 250 mSec.
			else if (lcl_eep_buf[3] == 3)
			ms_info.poll_interval = 500;	// 500 mSecs.
			else if (lcl_eep_buf[3] == 4)
			ms_info.poll_interval = 1000;	// 1000 mSecs.
			else
			ms_info.poll_interval = 1000;	// 100 miliSecs.(default)
			
		}
		else
		{
			// CRC validation failed, handle error
			#if (DEBUG_ALL || DEBUG_POWER_ON)
			Print_Message("\nCRC failed for MS data");
			#endif
			
			memset(lcl_eep_buf, 0, MS_DATA_SIZE);  // Clear the data buffer
			lcl_eep_buf[0] = 1;
			
			// Recalculate CRC for the cleared data
			CRC_Calculate_Check(lcl_eep_buf, (MS_DATA_SIZE - 2), CRC_CALC);
			lcl_eep_buf[(MS_DATA_SIZE - 2)] = gb_Temp_CRC[0];  // Store higher byte of CRC
			lcl_eep_buf[(MS_DATA_SIZE - 1)] = gb_Temp_CRC[1];  // Store lower byte of CRC

			// Write the corrected data back to EEPROM memory
			EEPROM_Byte_Write(MS_EEP_START_LOC, lcl_eep_buf, MS_DATA_SIZE);
			
			ms_info.poll_interval = 1000;	// 1000 miliSecs.(default)
			ms_info.boudrate = 9600; //(default)
		}
		
		//ms_info.boudrate = lcl_eep_buf[0];			
		#if 1		
		Print_Message("\n******************* Modbus Settings ********************\n");
		Print_Message("\nModbus baudrate is : ");
		Print_Number(ms_info.boudrate);
		
		Print_Message("\nPolling interval   : ");
		Print_Number(ms_info.poll_interval);
			
		Print_Message("\nParity bit is      : ");
		Print_Number(ms_info.parity_bit);
		
		Print_Message("\nStop Bit           : ");
		Print_Number(ms_info.stop_bit);
		#endif
		
	}
	else
	{
		__NOP();
	}
	//Modbus_init();
}


/*****************************************************************************
* Function Name  : Read_IP_Data_From_EEP
* Returns        : void
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 01/01/2025
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 01/01/2025
* Description    : Reads Input (IP) data from flash memory, validates it using
*                  CRC-16, and segregates it into a buffer. If CRC validation fails,
*                  it clears the data, recalculates the CRC, and writes the corrected data
*                  back to flash memory.
* Notes          : Ensures the data integrity of Input (IP) data by validating it with CRC
*                  and handles errors by writing default values when necessary.
* Global Variables Affected : gb_Temp_CRC
*****************************************************************************/
void Read_IP_Data_From_EEP(void)
{
	unsigned char lcl_ip_data[IP_DATA_SIZE] = {0};  // Local buffer for storing Input (IP) data
	memset(lcl_ip_data, 0, IP_DATA_SIZE);  // Initialize the buffer to zero

	// Read Input (IP) data from flash memory
	U8 lcl_eep_sts = EEPROM_Byte_Read(IP_EEP_START_LOC, lcl_ip_data, IP_DATA_SIZE);

	if (lcl_eep_sts == 0)  // Check if the flash read was successful
	{
		#if (DEBUG_ALL || DEBUG_AC_MEM_IP_LOOP)
		// Debugging: Print the read Input (IP) data
		Print_Message("\nPower on Flash data for IP is : ");
		for (U8 idx = 0; idx < IP_DATA_SIZE; idx++)
		{
			Print_Number(lcl_ip_data[idx]);
			Print_Message(",");
		}
		#endif

		// Perform CRC validation on the data (excluding the last 2 bytes, which are CRC)
		if (CRC_Calculate_Check(lcl_ip_data, (IP_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// CRC validation passed
			#if (DEBUG_ALL || DEBUG_AC_MEM_IP)
			Print_Message("\nCRC Validated for IP data");
			#endif

			// Segregate the Input (IP) data into a buffer for further processing
			Segregate_IP_Data_To_Buffer(lcl_ip_data);
		}
		else
		{
			// CRC validation failed, handle error
			#if (DEBUG_ALL || DEBUG_AC_MEM_IP)
			Print_Message("\nCRC failed for IP data");
			#endif

			memset(lcl_ip_data, 0, IP_DATA_SIZE);  // Clear the data buffer
			Segregate_IP_Data_To_Buffer(lcl_ip_data);  // Segregate cleared data

			// Recalculate CRC for the cleared data
			CRC_Calculate_Check(lcl_ip_data, (IP_DATA_SIZE - 2), CRC_CALC);
			lcl_ip_data[(IP_DATA_SIZE - 2)] = gb_Temp_CRC[0];  // Store higher byte of CRC
			lcl_ip_data[(IP_DATA_SIZE - 1)] = gb_Temp_CRC[1];  // Store lower byte of CRC

			// Write the corrected data back to flash memory
			EEPROM_Byte_Write(IP_EEP_START_LOC, lcl_ip_data, IP_DATA_SIZE);
		}
	}
	else
	{
		// Wait for the flash read operation to complete
		__NOP();
	}
}


/*****************************************************************************
* Function Name  : Read_OP_Data_From_EEP
* Returns        : void
* Arguments      : None
* Created by     : Shirisha Ankam
* Date Created   : 01/01/2025
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 01/01/2025
* Description    : Reads Output (OP) data from flash memory, validates it using
*                  CRC-16, and segregates it into a buffer. If CRC validation fails,
*                  it clears the data, recalculates the CRC, and writes the corrected data
*                  back to flash memory.
* Notes          : Ensures the data integrity of Output (OP) data by validating it with CRC
*                  and handles errors by writing default values when necessary.
* Global Variables Affected : gb_Temp_CRC
*****************************************************************************/
void Read_OP_Data_From_EEP(void)
{
	U8 lcl_op_data[OP_DATA_SIZE] = {0};  // Local buffer for storing Output (OP) data
	memset(lcl_op_data, 0, OP_DATA_SIZE);  // Initialize the buffer to zero

	// Read Output (OP) data from flash memory
	U8 lcl_eep_sts = EEPROM_Byte_Read(OP_EEP_START_LOC, lcl_op_data, OP_DATA_SIZE);

	if (lcl_eep_sts == 0)  // Check if the flash read was successful
	{
		#if (DEBUG_ALL || DEBUG_AC_MEM_OP_LOOP)
		// Debugging: Print the read Output (OP) data
		Print_Message("\nPower on Flash data for OP is : ");
		for (U8 idx = 0; idx < OP_DATA_SIZE; idx++)
		{
			Print_Number(lcl_op_data[idx]);
			Print_Message(",");
		}
		#endif

		// Perform CRC validation on the data (excluding the last 2 bytes, which are CRC)
		if (CRC_Calculate_Check(lcl_op_data, (OP_DATA_SIZE - 2), CRC_CHECK) == 1)
		{
			// CRC validation passed
			#if (DEBUG_ALL || DEBUG_AC_MEM_OP)
			Print_Message("\nCRC Validated for OP data");
			#endif

			// Segregate the Output (OP) data into a buffer for further processing
			Segregate_OP_Data_To_Buffer(lcl_op_data);
			
		}
		else
		{
			// CRC validation failed, handle error
			#if (DEBUG_ALL || DEBUG_AC_MEM_OP)
			Print_Message("\nCRC failed for OP data");
			#endif

			memset(lcl_op_data, 0, OP_DATA_SIZE);  // Clear the data buffer
			Segregate_OP_Data_To_Buffer(lcl_op_data);  // Segregate cleared data

			// Recalculate CRC for the cleared data
			CRC_Calculate_Check(lcl_op_data, (OP_DATA_SIZE - 2), CRC_CALC);
			lcl_op_data[(OP_DATA_SIZE - 2)] = gb_Temp_CRC[0];  // Store higher byte of CRC
			lcl_op_data[(OP_DATA_SIZE - 1)] = gb_Temp_CRC[1];  // Store lower byte of CRC

			// Write the corrected data back to flash memory
			EEPROM_Byte_Write(OP_EEP_START_LOC, lcl_op_data, OP_DATA_SIZE);
		}
	}
	else
	{
		// Wait for the flash read operation to complete
		__NOP();
	}
	/*PowerOn_Output_Activation();*/
}

/*****************************************************************************
* Function Name  : Update_SI_Variables
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Reads Safety & Intruder (SI) configuration data from EEPROM,
*                  validates the data using CRC-16, and updates the runtime
*                  intruder configuration variables.
*                  The function:
*                  - Reads SI data bytes from EEPROM.
*                  - Verifies CRC integrity of the stored data.
*                  - If CRC validation fails, initializes SI data with default
*                    values, recalculates CRC, and writes it back to EEPROM.
*                  - Updates intruder-related variables such as operation number,
*                    alarm delay, and arm delay.
*
* Notes          :
*                  - CRC validation is mandatory before using EEPROM data.
*                  - Default values are written back to EEPROM on CRC failure.
*                  - Debug messages are printed when DEBUG_ALL or DEBUG_FROZEN
*                    flags are enabled.
*
* Global Variables Affected :
*                  intruder,
*                  gb_Temp_CRC
*****************************************************************************/
void Update_SI_Variables(void)
{
	U8 lcl_si_data[SI_EEP_MAX_BYTE];
	memset(lcl_si_data, 0, SI_EEP_MAX_BYTE);
	
	U8 si_sts = EEPROM_Byte_Read(SI_EEP_START_LOC, lcl_si_data, SI_EEP_MAX_BYTE);
	if (si_sts == 0)
	{
		// Bytes returned successful.
		if (CRC_Calculate_Check(lcl_si_data, (SI_EEP_MAX_BYTE - 2), CRC_CHECK) == 1)
		{
			// CRC validated successful.
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validated successful");
			#endif
			
			__NOP();
		}
		else
		{
			// CRC validation failed.
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validation failed.");
			#endif
			
			memset(lcl_si_data, 0, SI_EEP_MAX_BYTE);
			CRC_Calculate_Check(lcl_si_data, (SI_EEP_MAX_BYTE - 2), CRC_CALC);
			lcl_si_data[(SI_EEP_MAX_BYTE - 2)] = gb_Temp_CRC[0];
			lcl_si_data[(SI_EEP_MAX_BYTE - 1)] = gb_Temp_CRC[1];
			
			/************************************************************************/
			/* Write data to a memory with calculated CRC.                          */
			/************************************************************************/
			EEPROM_Byte_Write(SI_EEP_START_LOC, lcl_si_data, SI_EEP_MAX_BYTE);
		}
		
		intruder.op_number = lcl_si_data[0];
		intruder.alarm_delay = lcl_si_data[1];
		intruder.arm_delay = lcl_si_data[2];
	}
	else
	__NOP();
}

/*****************************************************************************
* Function Name  : Update_AM_ByServer_Variables
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Reads Arm/Disarm (AM) configuration data received from the
*                  server and stored in EEPROM, validates it using CRC-16, and
*                  updates intruder-related runtime variables accordingly.
*                  The function:
*                  - Reads AM data from EEPROM into a local buffer.
*                  - Validates data integrity using CRC-16.
*                  - On CRC failure, initializes default values, recalculates
*                    CRC, and writes the corrected data back to EEPROM.
*                  - Updates arm/disarm status, source of arm/disarm (server),
*                    monitoring flags, and feedback flags.
*                  - Updates arm input bitmask and synchronizes ARM/DISARM input
*                    states based on current arm status.
*
* Notes          :
*                  - CRC validation is mandatory before applying EEPROM data.
*                  - Default AM data is written back to EEPROM on CRC failure.
*                  - ARM/DISARM feedback is triggered to notify the server.
*                  - Input states are updated only for ITYPE_ARM_DARM inputs.
*                  - Debug messages are printed when DEBUG_ALL or DEBUG_FROZEN
*                    flags are enabled.
*
* Global Variables Affected :
*                  intruder,
*                  gb_Temp_CRC,
*                  gb_arm_feedback_f,
*                  gb_arm_pin_val,
*                  ip_state,
*                  gb_input
*****************************************************************************/
void Update_AM_ByServer_Variables(void)
{
	U8 lcl_am_data[AM_EEP_MAX_BYTE];
	memset(lcl_am_data, 0, AM_EEP_MAX_BYTE);
	intruder.arm_inputs = 0;	// Reset initially.

	U8 am_sts = EEPROM_Byte_Read(AM_EEP_START_LOC, lcl_am_data, AM_EEP_MAX_BYTE);
	if (am_sts == 0)
	{
		// Bytes returned successful.
		if (CRC_Calculate_Check(lcl_am_data, (AM_EEP_MAX_BYTE - 2), CRC_CHECK) == 1)
		{
			// CRC validated successful.
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validated successful");
			#endif
			__NOP();
		}
		else
		{
			// CRC validation failed.
			#if (DEBUG_ALL || DEBUG_FROZEN)
			Print_Message("\nCRC validation failed.");
			#endif
			memset(lcl_am_data, 0, AM_EEP_MAX_BYTE);
			CRC_Calculate_Check(lcl_am_data, (AM_EEP_MAX_BYTE - 2), CRC_CALC);
			lcl_am_data[(AM_EEP_MAX_BYTE - 2)] = gb_Temp_CRC[0];	// Store CRC MSB
			lcl_am_data[(AM_EEP_MAX_BYTE - 1)] = gb_Temp_CRC[1];	// Store CRC LSB

			/************************************************************************/
			/* Write data to memory with calculated CRC                              */
			/************************************************************************/
			EEPROM_Byte_Write(AM_EEP_START_LOC, lcl_am_data, AM_EEP_MAX_BYTE);
		}

		/* Set variables for ARMED & DIS-ARMED */
		if (lcl_am_data[0] == ARMED)
		{
			intruder.arm_status = ARMED;		// System armed
			intruder.arm_darm_by = AM_SERVER;	// Armed by server
			intruder.arm_monitor_f = 0;			// Start arm delay counter
			Write_Arm_Status_To_Memory();
			gb_arm_feedback_f = 1;				// Send status to server
		}
		else if (lcl_am_data[0] == DIS_ARMED)
		{
			intruder.arm_status = DIS_ARMED;	// System disarmed
			intruder.arm_darm_by = AM_SERVER;	// Disarmed by server
			intruder.arm_monitor_f = 1;			// Stop ongoing alarm
			Write_Arm_Status_To_Memory();
			gb_arm_feedback_f = 1;				// Send status to server
		}
		else
		__NOP();

		/* Set reset bits for inputs present in the buffer */
		for (U8 iIdx = 0; iIdx < MAX_INPUTS; iIdx++)
		{
			if (lcl_am_data[(iIdx + 1)] > 0)
			SET_BIT(&intruder.arm_inputs, (lcl_am_data[(iIdx + 1)] - 1));	// Enable input
			else
			CLEAR_BIT(&intruder.arm_inputs, (lcl_am_data[(iIdx + 1)] - 1));	// Disable input

			if (gb_input[iIdx].ip_type == ITYPE_ARM_DARM)
			{
				if (intruder.arm_status == ARMED)
				{
					ip_state[iIdx].current_state = IP_ACTIVE;	// ARM input active
					gb_arm_pin_val = 1;
				}
				else
				{
					ip_state[iIdx].current_state = IP_NOT_ACTIVE;	// ARM input inactive
					gb_arm_pin_val = 2;
				}
			}
			else
			__NOP();
		}
	}
	else
	__NOP();
}


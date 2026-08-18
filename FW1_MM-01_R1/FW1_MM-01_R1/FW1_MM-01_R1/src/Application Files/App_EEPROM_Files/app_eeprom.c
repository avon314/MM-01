/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: app_eeprom.c
* Created By	: Harshit Agnihotri
* Created Date	: 01/04/2024
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash
*					128 KB		RAM
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 02/04/2024
* Changes		: NA
*****************************************************************************/

/* System Includes */
#include "asf.h"
#include "string.h"

/* User Includes */
#include "app_eeprom.h"
#include "user_uart.h"
#include "CRC.h"
#include "ext_eeprom.h"
#include "dflt_config.h"
#include "app_utility.h"
#include "definitions.h"
 
#include "memory_storage.h"
#include "ext_eeprom_1.h"


uint8_t gb_read_pod_from_mem = 0;


/*****************************************************************************
* Function name:  void Write_ConfData_To_EEP_1_Memory(int addr, U8* data_to_write, U32 len)
* Returns		: None
* Arguments	    : int addr, U8* data_to_write, U32 len
* Created by	: Shirisha Ankam
* Date created	: 15-02-2025
* Description	: Add CRC to the data at the end and write it to flash at the specified address.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void Write_ConfData_To_EEP_1_Memory(int addr, U8* data_to_write, U32 len)
{
	if (len <= UTLTY_FRM_MX_SZ)
	{
		if (CRC_Calculate_Check(data_to_write, (len - 2), CRC_CALC))                      // Calculate and set CRC at the end.
		{
			data_to_write[(len - 2)] = gb_Temp_CRC[0];
			data_to_write[(len - 1)] = gb_Temp_CRC[1];
			
			#if DEBUG_ALL || DEBUG_APP_EEPROM
			Print_Message("\nInside read data is : ");
			Print_Data_On_Uart(data_to_write, 255);
			#endif
						
			EEPROM_Byte_Write(addr, data_to_write, len);                        // Write the data to EEP1.
		}
	}
	else
	{
		__NOP();
	}
}



/*****************************************************************************
* Function name : void Read_ConfData_From_EEP_1_Memory(int offset, U8* read_data, U32 len, U8 err_byte)
* Returns		: None
* Arguments		: int offset, U8* read_data, U32 len, U8 error byte
* Created by	: Shirisha Ankam
* Date created	: 15-02-2025
* Description	: Read the data from specified offset, verify the CRC and store it in the passed buffer.
*              :
* Notes		: NA
* Global Variables Affected : NA
*****************************************************************************/
void Read_ConfData_From_EEP_1_Memory(int offset, U8* read_data, U32 len, U8 err_byte)
{
	// Read the data including CRC from flash memory
	EEPROM_Page_Read(UT_EEP_START_PAGE, offset, read_data, len);
	
// 	Print_Message("\nRead Utility data from eeprom");
// 	for(int i = 0; i < len; i++)
// 	{
// 		Print_Number(read_data[i]);
// 		Print_Message(",");
// 	}

	if (len <= UTLTY_FRM_MX_SZ)
	{
		// Check CRC of the read data
		if (CRC_Calculate_Check(read_data, (len - 2), CRC_CHECK) == 1)
		{
			// If CRC verified, write the respective data in RAM
			switch (err_byte)
			{
				case READ_COMN:
				
				gb_read_pod_from_mem = 1;
				write_comn_settings(read_data);

				#if DEBUG_ALL || DEBUG_APP_FLASH
				Print_Message("\nCRC verification passed for communication setting flash values");
				#endif
				break;

				default:
				break;
			}
		}
		else
		{
			// If CRC check fails, write default data to RAM
			switch (err_byte)
			{
				case READ_COMN:
				write_dflt_comn_config();
				Write_Default_End_Point();
				
				U8 lcl_rd_config_data[COMN_CONFIG_BYTES] = {0};
				read_comn_settings(lcl_rd_config_data); // Read communication configuration and write to EEPROM and flash
				CRC_Calculate_Check(lcl_rd_config_data, (len - 2), CRC_CALC);
				lcl_rd_config_data[(len - 2)] = gb_Temp_CRC[0];	// Higer Byte.
				lcl_rd_config_data[(len - 1)] = gb_Temp_CRC[1];	// Lower Byte.
				EEPROM_Page_Write(UT_EEP_START_PAGE, UT_EEP_PAGE_OFFSET, lcl_rd_config_data, COMN_CONFIG_BYTES);  // write to flash

				#if DEBUG_ALL || DEBUG_APP_FLASH
				Print_Message("\nCRC verification failed for communication, set default values.");
				#endif
				break;

				default:
				break;
			}
		}
	}
	else
	{
		__NOP(); // Do nothing if length exceeds max frame size
	}
}





/*****************************************************************************
* Function name	: void Write_Config_To_EEP_1_Memory(void)
* Returns		: None
* Arguments		: None
* Created by	: Shirisha Ankam
* Date created	: 15-02-2025
* Description	: Write all configuration data to flash.
*
* Notes			: NA
* Global Variables Affected : NA
*****************************************************************************/
void Write_Config_To_EEP_1_Memory(void)
{
	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nWriting data to EEP1");
	#endif

	U8 read_config_data[MAX_BYTES];
	read_comn_settings(read_config_data);	// Read communication configuration and write to FLASH.
	
	Write_ConfData_To_EEP_1_Memory(UT_EEP_START_LOC, read_config_data, COMN_CONFIG_BYTES);

	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nData written to EEP1");
	#endif
}



/*****************************************************************************
* Function name : void Read_Config_From_EEP_1_Memory()
* Returns		: None
* Arguments 	: None
* Created by	: Shirisha Ankam
* Date created	: 15-02-2025
* Description	: Read all configuration data from flash.
*
* Notes		    : NA
* Global Variables Affected : NA
*****************************************************************************/
void Read_Config_From_EEP_1_Memory()
{
	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nReading data from EEP1");
	#endif

	U8 write_config_data[MAX_BYTES];
	Read_ConfData_From_EEP_1_Memory(UT_EEP_PAGE_OFFSET, write_config_data, COMN_CONFIG_BYTES, READ_COMN);

	Update_Manufacturing_Info();

	#if DEBUG_ALL || DEBUG_APP_FLASH
	Print_Message("\nData read from EEP1");
	#endif
}
/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: app_eeprom.h
* Created By	: Shirisha Ankam.
* Created Date	: 1/12/2025.
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025

* Module
* Description	: Header file for app_eeprom.c
				  Defines constants and macros for app_eeprom.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 30/03/2024
* Changes		: NA
*****************************************************************************/

/* User Includes */
#include "group_config.h"
#include "comn_config.h"

#ifndef APP_EEPROM_H_
#define APP_EEPROM_H_

/***** Macro definitions *****/
#define MAX_BYTES                   (512)
#define CRC_BYTES                   (2)


#define READ_COMN                    (13)

#define DEBUG_APP_FLASH              (0)
// Function prototypes
#ifdef ENABLE_EEP_FUN
void write_to_eeprom(U16 addr, U8* data_to_write, U16 len);
void read_from_eeprom(U16 addr, U8* read_data, U16 len, U8 err_byte);
void write_config_eeprom(void);
void read_config_eeprom(void);
#endif

void Write_Config_To_EEP_1_Memory(void);
void Read_Config_From_EEP_1_Memory(void);
void Write_ConfData_To_EEP_1_Memory(int addr, U8* data_to_write, U32 len);
void Read_ConfData_From_EEP_1_Memory(int addr, U8* read_data, U32 len, U8 err_byte);

#endif /* APP_EEPROM_H_ */
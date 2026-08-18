/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: group_config.c
* Created By	: Harshit Agnihotri.
* Created Date	: 28/11/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*				1024 KB		Flash.
*				128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 28/11/2023.
* Changes		: NA.
*****************************************************************************/

/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/
#include "group_config.h"
#include "string.h"
#include "user_uart.h"
#include "definitions.h"
#include "comn_config.h"
#include "app_utility.h"


// Declare and initialize the MfgInfo structure with constant values.
Mfg_Info MfgInfo=
{
     .fw_vs="R1.3.1",
     .hw_vs="R1.0",
     .ip_addr = "192.168.001.045"
};

/*****************************************************************************
* Function name: void Read_Manufacturing_Info(U8* read_mfg_info).
* Returns		: nothing.
* Arguments    : U8* read_mfg_info.
* Created by	: Harshit Agnihotri.
* Date created	: 28/11/2023.
*
* Description	: Function to read manufacturing information and copy it to a buffer.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void Read_Manufacturing_Info(U8* read_mfg_info)
{
	 Update_Manufacturing_Info();
	
     strcpy((char*)read_mfg_info, MfgInfo.fw_vs);                               // Copy manufacturing information data to the read_mfg_info array.
     strcpy((char*)(read_mfg_info + strlen(MfgInfo.fw_vs)), MfgInfo.hw_vs);
     strcpy((char*)(read_mfg_info + strlen(MfgInfo.fw_vs) + strlen(MfgInfo.hw_vs)), MfgInfo.ip_addr);

}

/*****************************************************************************
* Function name: void test_read_mfg_info(void).
* Returns		: nothing.
* Arguments    : none.
* Created by	: Harshit Agnihotri.
* Date created	: 29/11/2023.
*
* Description	: This function tests the Read_Manufacturing_Info function.
*              :
* Notes	     : NA.
* Global Variables Affected : NA.
*****************************************************************************/
void test_read_mfg_info(void)
{
     #if (TEST_READ_MFG_INF0==uDISABLE)
     U8 mfg_info[MFG_INFO_BYTES];
     Read_Manufacturing_Info(mfg_info);                     // Test the function Read_Manufacturing_Info.
     Send_Frame_On_UART(mfg_info, MFG_INFO_BYTES);
     #endif
}


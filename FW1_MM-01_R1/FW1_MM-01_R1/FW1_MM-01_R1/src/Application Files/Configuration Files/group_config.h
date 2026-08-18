/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: group_config.h
* Created By	: Harshit Agnihotri.
* Created Date	: 28/11/2023.
* Module
* Description	: Header file for group_config.c.
		       Defines constants and macros for group_config.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 28/11/2023.
* Changes		: NA.
*****************************************************************************/

#ifndef UTILITY_CONFIG_H_
#define UTILITY_CONFIG_H_

/***** User Includes *****/

/***** Definitions / Macros *****/
#define CONNECT_WITH_DEVICE_STR_LEN (20)
#define CONNECTED_STR_LEN			(9)
#define EXIT_FROM_CONFIG_STR_LEN	(16)
#define CONFIG_IS_EXIT_LEN			(14)
#define MFG_INFO_BYTES				(40)
#define FW_VS_BYTES					(10)
#define HW_VS_BYTES					(10)
#define IP_ADDR_BYTES				(15)
#define TEST_READ_MFG_INF0			(1)
#define MAX_GROUPS					(8)
#define MAX_GRP_BYTES				(425)
#define BYTE_SIZE					(8)
#define MAX_DOORS					(16)

#ifndef MAX_INPUTS
#define MAX_INPUTS				     (8)
#endif

#ifndef MAX_OUTPUTS
#define MAX_OUTPUTS				     (4)
#endif

#define STATE_NC                     (0x01)
#define STATE_NO                     (0x00)

#define ACTIVE                       (1)
#define NOT_ACTIVE                   (0)

#define TEST_GROUP_CONFIG			(0)

#ifndef SET_BIT
#define SET_BIT(num, bitposition) ((*num) |= (1u << (bitposition)))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(num, bitposition)  ((*num) &= ~(1u << (bitposition)))
#endif
#ifndef IS_BIT_SET
#define IS_BIT_SET(num, bitposition)  (((num) & (1u << (bitposition))) != 0)
#endif

/***** Structure variable that holds the manufacturing information data *****/
typedef struct
{
     const char fw_vs[FW_VS_BYTES];     // Firmware Version.
     const char hw_vs[HW_VS_BYTES];     // Hardware Version.
     char ip_addr[IP_ADDR_BYTES]; // IP Address of Device.
}Mfg_Info;

// Declare the structure variable of Mfg_Info to store the manufacturing information.
extern Mfg_Info MfgInfo;


/***** Function Declarations / Prototypes *****/
void Read_Manufacturing_Info(U8*);
void test_read_mfg_info(void);


#endif /* UTILITY_CONFIG_H_ */
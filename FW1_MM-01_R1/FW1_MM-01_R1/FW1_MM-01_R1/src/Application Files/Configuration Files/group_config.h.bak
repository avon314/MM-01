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
#define MFG_INFO_BYTES				(27)
#define FW_VS_BYTES					(4)
#define HW_VS_BYTES					(4)
#define IP_ADDR_BYTES				(15)
#define TEST_READ_MFG_INF0			(1)
#define MAX_GROUPS					(8)
#define MAX_GRP_BYTES				(425)
#define BYTE_SIZE					(8)
#define MAX_DOORS					(16)

#ifndef MAX_INPUTS
#define MAX_INPUTS				     (6)
#endif

#define TEST_GROUP_CONFIG			(0)

#define SET_BIT(num, bitposition) ((*num) |= (1u << (bitposition)))
#define CLEAR_BIT(num, bitposition)  ((*num) &= ~(1u << (bitposition)))
#define IS_BIT_SET(num, bitposition)  (((num) & (1u << (bitposition))) != 0)

/***** Structure variable that holds the manufacturing information data *****/
typedef struct
{
     const char fw_vs[FW_VS_BYTES];     // Firmware Version.
     const char hw_vs[HW_VS_BYTES];     // Hardware Version.
     char ip_addr[IP_ADDR_BYTES]; // IP Address of Device.
     U8 door_ctrllrs;             // Connected Door Controllers.
     U8 grps;                     // Number of Groups.
     U8 ips_en;                   // Digital Input Enabled.
     U8 ops_en;                   // Digital Output Enabled.
}Mfg_Info;

// Declare the structure variable of Mfg_Info to store the manufacturing information.
extern Mfg_Info MfgInfo;

/***** Structure variable that holds the information about each door *****/
typedef struct
{
     U8 itd_lb;                         // Interlocking time delay of the door.
     U8 itd_hb;
     U8 slave_addr;                     // Slave address of the door.
     U8 ils_door_lb;                    // Interlocking sequence of door with other doors.
     U8 ils_door_hb;
}Door_Info;

/***** Structure variable that holds the information about interlocking sequence of each input with doors *****/
typedef struct
{                                       // Interlocking sequence of door with inputs.
     U8 ils_ips_lb;                     // A lower byte variable to store an interlock sequence with inputs.
     U8 ils_ips_hb;                     // A higher byte variable to store an interlock sequence with inputs.
}Input_Info;

/***** Structure variable (nested Structure variable) that holds the information about each group (group of doors) *****/
typedef struct
{
     U8 group_number;                   // Group number.
     U8 doors_in_grp_lb;                // Doors enabled in the group.
     U8 doors_in_grp_hb;
     U8 ips_in_grp;                     // Inputs enabled in the group.
     Door_Info door[MAX_DOORS];         // Array to store information about interlocking sequence of each door in the group.
     Input_Info input[MAX_INPUTS];      // Array to store information about interlocking sequence of each input in the group.
}Group_Info;

//Declare Group_Info structure array variable to store the group information of all the groups.
extern Group_Info Group[MAX_GROUPS];

/***** Function Declarations / Prototypes *****/
void Read_Manufacturing_Info(U8*);
void test_read_mfg_info(void);
void write_group_configuration(U8*, U8);
void read_group_configuration(U8*, U8);
void test_group_config(void);

#endif /* UTILITY_CONFIG_H_ */
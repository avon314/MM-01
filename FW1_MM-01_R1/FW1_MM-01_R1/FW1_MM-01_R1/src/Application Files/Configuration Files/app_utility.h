/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_utility.h
* Created By	: Harshit Agnihotri.
* Created Date	: 28/12/2023.
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025
* Module
* Description	: Header file for app_utility.c.
		       Defines constants and macros for app_utility.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 28/11/2023.
* Changes		: NA.
*****************************************************************************/

#ifndef APP_UTILITY_H_
#define APP_UTILITY_H_

/***** Macro definitions *****/
//#define UTLTY_FRM_MX_SZ (0x01B3)
#define UTLTY_FRM_MX_SZ 		(0x0258)
#define SOF						(0x23)
#define EOF						(0x24)
#define DST_ADDR_RX				(0x02)
#define SRC_ADDR_RX				(0x01)
#define DST_ADDR_TX				(0x01)
#define SRC_ADDR_TX				(0x02)
#define NO_ERROR				(0x00)
#define CRC_ERROR				(0x01)
#define LEN_ERROR				(0x02)
#define DATA_ERROR				(0x03)

#define READ_MFG_IFO			(0x01)
#define READ_GRP_CONFIG			(0x02)
#define WRITE_GRP_CONFIG		(0x03)
#define READ_IP_CONFIG			(0X04)
#define WRITE_IP_CONFIG			(0x05)
#define READ_OP_CONFIG			(0X06)
#define WRITE_OP_CONFIG			(0x07)
#define READ_DOOR_CONFIG		(0x08)
#define WRITE_DOOR_CONFIG		(0x09)
#define READ_PRIVACY_CONFIG		(0x0A)
#define WRITE_PRIVACY_CONFIG	(0x0B)
#define READ_COMN_CONFIG		(0x80)
#define WRITE_COMN_CONFIG		(0x81)
#define CONNECT_WITH_DEVICE		(0X82)
#define READ_MAC_ADDRESS 		(0x83)
#define UPDATE_FIRMWARE			(0x84)
#define EXIT_FROM_CONFIG		(0x85)
#define CURRENT_DATE_TIME 		(0x0C)
#define SYNC_DATE_TIME 			(0x0D)
#define READ_DEVICE_STATUS 		(0x0E)
#define SET_FACTORY_DEF 		(0x0F)

#define RD_MFG_RPLY_LEN       	(0X0023)
#define RD_GRP_RPLY_LEN			(0X01B3)
#define WR_GRP_RPLY_LEN			(0x000B)
#define RD_IP_RPLY_LEN			(0X002E)
#define WR_IP_RPLY_LEN			(0X000A)
#define RD_OP_RPLY_LEN			(0X001E)
#define WR_OP_RPLY_LEN			(0X000A)
#define RD_DOOR_RPLY_LEN		(0X001A)
#define WR_DOOR_RPLY_LEN		(0X000A)
#define RD_PVC_RPLY_LEN			(0X008A)
#define WR_PVC_RPLY_LEN			(0X000A)
#define RD_COMN_RPLY_LEN      	(0X0072)
#define WR_COMN_RPLY_LEN		(0X000A)
#define CONN_RPLY_LEN			(0X0013)
#define EXIT_REPLY_LEN			(0x0018)
#define ERR_RPLY_LEN			(0X000A)
#define RD_MAC_ADD_LEN        	(0X0010)
#define RD_DT_AND_TM_LEN      	(0X0011)
#define SYNC_DT_AND_TIME_LEN  	(0X000A)
#define DEVICE_STATUS_LEN     	(0x000B)
#define FACTORY_DEFAULT_LEN   	(0X000A)

extern U8 gb_utility_rcv_f;             // Global flag indicating the reception of a utility frame.
extern volatile U8 gb_op_config_cplt;   // Flag indicating completion of output configuration.
extern volatile U8 gb_conn_dvc_f;       // Flag to indicate connect with device successful.
extern volatile U8 gb_err_crc_f;        // Flag to indicate error in CRC of received frame.
extern volatile U8 gb_err_len_f;        // Flag to indicate error in length of received frame.
extern volatile U8 gb_err_data_f;       // Flag to indicate error in data of received frame.
extern volatile U8 gb_fact_reset_f;	// Flag used to call power on reset pin after factory reset command received.
volatile U8 gb_utility_req_f;			// Flag used to say request is not by the utility.

// Definition of a structure representing a utility frame.
typedef struct
{
     U8 frame_buf[UTLTY_FRM_MX_SZ];     // Buffer to hold the frame data.
     U8 frame_funid;                    // Function ID within the frame.
     U16 frame_len;                     // Length of the frame.
}Utility;


// Declaration of global instances of the Utility structure for receiving and transmitting frames.
extern Utility utility_rx;
extern Utility utility_tx;

/***** Function Declarations / Prototypes *****/
void validate_utility_frame(void);
void SPLIT_BYTES(U16 cb, U8 *hb, U8 *lb);
void Find_Active_Group_Number(void);
void Update_Manufacturing_Info(void);
#endif /* APP_UTILITY_H_ */
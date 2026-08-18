/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mm_app.h
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
* Revision Date	: 17/11/2025.
* Changes		: NA.
*****************************************************************************/
#ifndef MM_APP_H_
#define MM_APP_H_

#include "tcp_server.h"
#include "group_config.h"

#define MAX_NUM_IS_LOGS			(8)		//8 Maximum numbers of logs can be send to a server, due to limit of 620 bytes of buffer length.
#define MAX_NUM_MD_LOGS         (6)		//6 Maximum numbers of logs can be send to a server, due to limit of 620 bytes of buffer length.
#define DEBUG_IP_EXT            (0)
#define DEBUG_OP_EXT            (0)
#define DEBUG_MODBUS            (0)
/***** SYNC Time With definitions *****/
#define WITH_RTC		(0)
#define NTP_SERVER		(1)
#define ERP_SERVER		(2)
/***** End of SYNC Time With definitions *****/

typedef struct
{
	U8 pause_rsm	: 1;
	U8 sync_time	: 2;
	U8 poll_freq	: 3;
	
	U8 dev_sts;
	U8 facility_code;
	U8 time_zone_hr;
	U8 time_zone_mn;
}DI;

extern DI dm_dev_info;

enum DI_IDX
{
	PAUSE			= 0,
	RESUME			= 1,
	
	PENDING_APPROVAL= 0,
	APPROVED		= 1,
	DEV_CANCELLED	= 255,
	
	POL_FRQ_IDX		= 0,
	SYNC_TM_IDX		= 1,
	DNS_IDX			= 2,
	DEV_STS_IDX		= 32,//6,
	PS_RSM_IDX		= 33,//7,
	FC_IDX			= 34,//8
	TZ_IDX_H		= 35,
	TZ_IDX_M		= 36
};


extern U8 gb_server_poll_freq;
extern U8 gb_start_sending_dpt;
extern U8 gb_send_mb_data;
extern U8 gb_reinit_uart;
extern U8 gb_requesting_cancel;
extern U8 gb_ac_power_f;
extern int gb_last_sent_is_bytes;
extern int gb_last_sent_md_bytes;
extern U8 gb_send_tl_at_conn;
extern U8 gb_update_is_last_sent_loc_f;
extern U8 gb_update_md_last_sent_loc_f;
extern U8 gb_send_input_data_to_serverf;
extern U8 gb_send_modbus_data_to_serverf;
extern U8 gb_md_empty_location_f;
extern int gb_data_packets;
extern U8 gb_dev_connect_f;
extern int mb_poll_timer;
extern U8 mb_poll_dpt;
extern U8 gb_last_dt_tm[6];
extern U8 gb_rtc_send_to_server_f;
extern U8 gb_is_reached_boundary_f;
extern U8 gb_md_reached_boundary_f;
extern char jsonBuffer[SERVER_DATA_SIZE];
extern uint8_t gs_uc_mac_address[6];
extern char gb_dev_ipAddress[13];
extern U8 gb_frame_logs_remaining;
extern  U8 gb_output_state[MAX_OUTPUTS];
extern U8 gb_current_op_st[MAX_OUTPUTS];
extern U8 gb_previous_op_st[MAX_OUTPUTS];
extern U8 gb_op_send_server_f;
extern U8 gb_op_detected_f;
extern U8 gb_op_at_pwr_on;
extern U8 input_data[MAX_INPUTS];
extern U8 ipTemp_data[MAX_INPUTS];
extern U8 gb_req_erp_time_f;
extern U8 sntp_create_task_f;
extern U8 clr_pwr_sntp_task_f;
extern U8 gb_device_power_on;

void Cancel_The_Device_From_Server(void);
void Update_Last_Sent_Bytes_IS(void);
void Write_Last_Sent_IS_Location(int rcv_last_sent_loc);
void Update_Poll_Frequency(uint16_t dp_val);
void Write_Last_Stored_Location_To_Flash(int rcv_last_store_loc);
void Send_Input_Log_ToServer(void);
void Read_MacID_And_Update_InSystem(void);
void Check_Time_To_Poll_Server(void);
void Get_Last_Dt_Tm_From_Flash(void);
void General_1_Sec_RTC_Timer_Routine(void);
void Send_Input_Status_ToServer(void);
void Update_Input_Status_To_Server(void);
void Read_Memory_For_PowerON_State(void);
void Read_IS_Boundary_Flag(void);
void Read_MD_Boundary_Flag(void);
void Flash_Store_InputState_Log( U8 *day_date, U8 *time_arr, U8 input_num, U8 input_state);

#endif /* MM_APP_H_ */
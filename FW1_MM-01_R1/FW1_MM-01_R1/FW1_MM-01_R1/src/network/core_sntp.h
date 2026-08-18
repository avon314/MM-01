/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: core_sntp.h
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

#include "asf.h"

#define SNTP_RESPONSE_TIME		(10)	// 10 Seconds.
#define SNTP_MAX_RETRY			(6)		// 6 retrials if no response within 1 min.

#define SYNC_RTC_FLAG           (0)

#define DEBUG_SNTP              (0)

extern U8 delete_sntp_task_f;
extern U8 sntp_waiting_for_response;
extern U8 sntp_retry_f;
extern U8 sntp_num_of_retry;
extern U8 sntp_resp_time;
extern	U8 gb_sntp_powerOn_f;

err_t sntp_send_request(struct udp_pcb *pcb, const ip_addr_t *server_ip);
void sntp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
void sntp_client(void);
void convert_unix_to_ist(uint32_t unix_time);
int is_leap_year(int year);
void test_udp(void); 
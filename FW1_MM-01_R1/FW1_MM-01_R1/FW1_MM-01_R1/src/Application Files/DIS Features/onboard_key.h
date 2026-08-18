/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: onboard_key.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/03/2024.
* Module
* Description	: Header file for onboard_key.c
				  Defines constants and macros for onboard_key.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/03/2024.
* Changes		: NO.
*****************************************************************************/
#ifndef ONBOARD_KEY_H_
#define ONBOARD_KEY_H_

/* User Includes */
#include "led_operation.h"

/* Constant definitions */
#define DEFAULT_IP_TIME			(20000) /* 20 Secs */
#define DEFT_IP_LED_ON_TIME		(100)	/* 100 ms */
#define DEFT_IP_LED_OFF_TIME	(300)	/* 300 ms */

/* Declaration of structures */
typedef struct  
{
	U8 key_detect_f : 1;
	U8 key_control_f : 1;
	U8 start_timer_f : 1;
	U8 key_set_for_default_ip : 1;
	U8 led_on_f : 1;
	U8 led_off_f : 1;
	U8 set_default_ip_f : 1;
	U16 timer_value;
}ON_BOARD_KEY;

/* Global structure variables */
extern ON_BOARD_KEY onb_key_flag;

/* Function Prototypes */
void Configure_Onboard_Key(void);
void Monitor_Key_For_Default_IPAddress(void);

#endif /* ONBOARD_KEY_H_ */
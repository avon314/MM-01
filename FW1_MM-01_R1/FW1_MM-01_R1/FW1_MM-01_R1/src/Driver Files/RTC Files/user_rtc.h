/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_rtc.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 14/08/2024.
* Module
* Description	: Header file for user_rtc.c
				  Defines constants and macros for user_rtc.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 14/08/2024.
* Changes		: NO.
*****************************************************************************/
#ifndef USER_RTC_H
#define USER_RTC_H

/***** Extern / Global Variable *****/

extern U8 gb_rtcTimeArr[3];
extern U8 gb_rtcDateArr[4];
extern U8 rtcUpdateArr[6];
extern U8 gb_rtc_time_update_f;
extern U8 gb_rtc_dm_1sec_f;
extern U8 gb_rtc_one_sec_flag;
extern U8 gb_rtc_1sec_IS_set_f;
extern U8 gb_rtc_1sec_delay_f;

/***** Function Prototypes *****/
void RTC_Interrupt_Pin_Configure(void);
void Get_RTC_Data_At_Every_Second(void);

void Write_Seconds_To_DS1339A(uint8_t sec);
void Write_Minutes_To_DS1339A(uint8_t min);
void Write_Hours_To_DS1339A(uint8_t hr);
void Write_DayValue_To_DS1339A(uint8_t day);
void Write_Date_To_DS1339A(uint8_t date);
void Write_Month_To_DS1339A(uint8_t month);
void Write_Year_To_DS1339A(uint8_t year);
void Write_Time_hhMMss_To_DS1339A(U8 hr, U8 mn, U8 ss);
void Write_DtMnYy_and_Day_To_DS1339A(U8 dt, U8 mon, U8 yr, U8 day);

U8 Read_Seconds_From_DS1339A(void);
U8 Read_Minutes_From_DS1339A(void);
U8 Read_Hour_From_DS1339A(void);
U8 Read_DayValue_From_DS1339A(void);
U8 Read_Date_From_DS1339A(void);
U8 Read_Month_From_DS1339A(void);
U8 Read_Year_From_DS1339A(void);

void Get_TimeDate_By_UART(U8 rcvByte);
void Update_Time_And_Date(void);
#endif // USER_RTC_
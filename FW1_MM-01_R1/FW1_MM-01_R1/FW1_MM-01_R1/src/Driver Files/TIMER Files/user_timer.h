/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_timer.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 30/10/2022.
* Module
* Description	: Header file for user_timer.c
				  Defines constants and macros for user_timer.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 30/10/2022.
* Changes		: NO.
*****************************************************************************/
#ifndef USER_TIMER_H_
#define USER_TIMER_H_

/***** Macro Definations for Watch Dog *****/
#define WDT_PERIOD			3000	// Watch dog period set is 3 seconds.
#define WDT_RESTART_PERIOD	3000	// Restart time set is 3 seconds.
/***** End of Macro Definations for Watch Dog *****/

/***** Extern variables *****/
extern volatile uint8_t gb_timer_f;
						
/***** Function Prototypes *****/
void Timer_Init(void);
void Timer_config(uint32_t freq_desired1);
void timer_delay(volatile U16 del);
void Decrement_Delay_Counts(void);
void Generate_General_Delay(void);
void Configure_WDT(void);

#endif /* USER_TIMER_H_ */





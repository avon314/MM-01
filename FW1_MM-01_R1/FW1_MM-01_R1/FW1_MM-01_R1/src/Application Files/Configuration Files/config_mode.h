/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: config_mode.h
* Created By	: Harshit Agnihotri.
* Created Date	: 20/03/2024.
* Module
* Description	: Header file for config_mode.c
				  Defines constants and macros for config_mode.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 20/03/2024.
* Changes		: NA.
*****************************************************************************/


#ifndef CONFIG_MODE_H_
#define CONFIG_MODE_H_


#define CONFIG_RST_KEY_TIME (10000)
#define CONFIG_LED_TOGGLE_TIME (500)
#define CONFIG_FRAME_RCV_TIME_OUT (60000)

extern volatile U8 gb_config_mode_f;              // Configuration mode flag.
extern volatile U8 gb_cfg_mode_keyPressed;        // To check entered in config mode or not.
extern volatile U8 gb_exit_from_cfg_f;			  // flag to check after exit from config mode, key released or not?
extern volatile int gb_config_timer;              // Configuration timer.
extern volatile U8 gb_config_timer_running_f;     // Configuration timer flag.

extern volatile U16 gb_config_led_timer;          // Configuration LED timer.
extern volatile U8 gb_config_led_on_f;            // Configuration LED flag.

extern volatile U8 gb_config_rst_key_f;           // Configuration mode reset key flag.
extern volatile int gb_config_rst_key_timer;      // Configuration mode reset key timer.

/***** Function Declarations / Prototypes *****/
void config_mode_init(void);
void enter_config_mode(void);
void Read_Utility_Config_Data(void);
void config_mode(void *pvParameters);
void exit_config_mode(void);

#endif /* CONFIG_MODE_H_ */
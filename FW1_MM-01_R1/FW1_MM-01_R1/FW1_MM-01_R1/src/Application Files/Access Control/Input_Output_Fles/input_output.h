/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: input_output.h
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

#ifndef INPUT_OUTPUT_H_
#define INPUT_OUTPUT_H_

#include "digital_input.h"


#define IP_ACTIVE				(1)
#define IP_NOT_ACTIVE			(0)
#define HIGH_TO_LOW				(0x00)
#define LOW_TO_HIGH				(0x01)

/* Macros for Input Type */
#define ITYPE_STATUS			(0)
#define ITYPE_SAFETY			(1)
#define ITYPE_INTRUDER			(2)
#define ITYPE_ARM_DARM			(3)
/* End of Macros for Input Type */

/* Macros related to Intrusion & safety */
#define AM_SYSTEM				(1)
#define AM_SERVER				(2)
#define ARMED					(1)
#define DIS_ARMED				(0)
/* End of Macros related to Intrusion & safety */

extern U8 gb_sync_op_at_config;

extern U16 output_sec[MAX_OUTPUTS];
extern U16 output_min[MAX_OUTPUTS];
extern U8 gb_start_op_timer[MAX_OUTPUTS];
extern U8 gb_arm_pin_val;
extern U8 gb_arm_feedback_f;
extern U8 gb_arm_send_server_f;
extern U8 gb_arm_wait_ser_ack_f;

typedef struct
{
	U8 ip_en;                         // Whether the input is enabled or not.
	U8 ip_activate_state;             // Whether the input functionality should be activated when input goes from low to high or high to low.
	U8 ip_type;
}input_configuration;
// Declare the structure variable of input_configuration to store the input configuration information.
extern input_configuration gb_input[MAX_INPUTS];

typedef struct
{
	U8 op_en;                    // The number assigned to this output.
	U8 op_dflt_state;            //default state of output NO or NC
	U8 op_pwr_reset_mode;        //store at power on
	U8 op_type;                  //output type
	U8 op_hour;                  //Activate from in hour
	U8 op_min;                   //Activate from in min
	U16 op_duration;             //Activation duration
}output_configuration;
// Declare the structure variable of output_configuration to store the output configuration information.
extern output_configuration gb_output[MAX_OUTPUTS];

typedef struct  
{
	U8 start_arm_delay_f	: 1;
	U8 arm_del_exceed_f		: 1;
	U8 arm_monitor_f		: 1;
	U8 start_alarm_delay_f	: 1;
	U8 alarm_del_exceed_f	: 1;
	U8 arm_status			: 1;
	U8 arm_darm_by			: 2;
	
	U8 op_number;
	U8 alarm_delay;
	U8 arm_delay;
	
	U8 arm_del_counter;		// using for secs.
	U8 alarm_del_counter;	// Using for minutes.
	U8 arm_inputs;			// bitwise operations for inputs selected.			
	U8 ip_monitor_f[MAX_INPUTS];
	U8 delay_monitor_f[MAX_INPUTS];
}SAFETY_INTRUDER;

extern U8 gb_ip_send_to_srver_f;
extern SAFETY_INTRUDER intruder;

static const uint32_t input_pin_mask[MAX_INPUTS] =
{
	DIG_INPUT_PIN_1,   // IP1 ? PD22
	DIG_INPUT_PIN_2,   // IP2 ? PD21
	DIG_INPUT_PIN_3,   // IP3 ? PD23
	DIG_INPUT_PIN_4,   // IP4 ? PA25
	DIG_INPUT_PIN_5,   // IP5 ? PD24
	DIG_INPUT_PIN_6,   // IP6 ? PA24
	DIG_INPUT_PIN_7,   // IP7 ? PD25
	DIG_INPUT_PIN_8    // IP8 ? PD26
	// Add more if MAX_INPUTS > 8
};


static  Pio* input_port[MAX_INPUTS] =
{
	PIOD,   // IP1
	PIOD,   // IP2
	PIOD,   // IP3
	PIOA,   // IP4
	PIOD,   // IP5
	PIOA,   // IP6
	PIOD,   // IP7
	PIOD    // IP8
};


void Segregate_IP_Data_To_Buffer(unsigned char *flsh_ip_data);
void Input_Functionality(void);
void Segregate_OP_Data_To_Buffer(U8 *flsh_op_data);
void Output_Operations_From_Server(U8 rec_op_num,U8 rec_op_st);
void PowerOn_Output_Activation(void);
void Input_PowerUp_Sync(void);
void monitor_output(void);
void Auto_Output_Activation(void);
void Safety_And_Intruder_Monitor(void);
void Monitor_ARM_Input(void);
void Write_Arm_Status_To_Memory(void);
void Arm_DisArm_At_Powerup(void);





#endif /* INPUT_OUTPUT_H_ */
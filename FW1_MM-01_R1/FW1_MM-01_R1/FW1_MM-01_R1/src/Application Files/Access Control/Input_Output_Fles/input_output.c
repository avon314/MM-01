/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: input_output.c
* Created By	: Shirisha  Ankam.
* Created Date	: 17/11/2025.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*                   128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 30/07/2025.
* Changes		: NA.
*****************************************************************************/
/***** System Includes *****/
#include "asf.h"

/***** User Includes *****/

#include "user_uart.h"
#include "definitions.h"
#include "digital_input.h"
#include "user_timer.h"
#include "group_config.h"
#include "input_output.h"
#include "mm_app.h"
#include "user_rtc.h"
#include "relay_output.h"
#include "ext_eeprom_1.h"
#include "memory_storage.h"
#include "CRC.h"
#include <string.h>

input_configuration gb_input[MAX_INPUTS];
output_configuration gb_output[MAX_OUTPUTS];
SAFETY_INTRUDER intruder;

U8 gb_ip_send_to_srver_f = 0;
U8 gb_sync_op_at_config = 0;
U8 gb_arm_timer_f = 0;
U16 output_sec[MAX_OUTPUTS] = {0};
U16 output_min[MAX_OUTPUTS] = {0};
U8 gb_start_op_timer[MAX_OUTPUTS] = {0};
U8 gb_arm_feedback_f = 0;
U8 gb_arm_send_server_f = 0;
U8 gb_arm_wait_ser_ack_f = 0;
U16 gb_arm_counter = 0;
U8 gb_arm_exceed_f = 0;
U8 gb_arm_pin_val = 0;

/*****************************************************************************
* Function name : void Segregate_IP_Data_To_Buffer.
* Returns		: nothing.
* Arguments     : unsigned char *flsh_ip_data.
* Created by	: Shirisha Ankam.
* Date created	: 18/11/2025.
*
* Description	: This function is to segregate input data.

* Notes	        : NA.
* Global Variables Affected : gb_ip1_func, gb_ip1_ilock_grps.
*****************************************************************************/
void Segregate_IP_Data_To_Buffer(unsigned char *flsh_ip_data)
{
	for (U8 ip_index = 0, fls_idx = 0; ip_index < MAX_INPUTS; ip_index ++)   // Loop through all inputs, fls_idx used to read flash bytes
	{
		gb_input[ip_index].ip_en = flsh_ip_data[fls_idx ++];              // Read and store Input Enable flag
		gb_input[ip_index].ip_activate_state = flsh_ip_data[fls_idx ++];  // Read and store Input Active state value
		gb_input[ip_index].ip_type = flsh_ip_data[fls_idx ++];			  // Read and store Input type value
		
		#if (DEBUG_ALL || DEBUG_IP_EXT)
		Print_Message("\nInput number  ");                               
		Print_Number(ip_index + 1);
		Print_Message(" : ");
		Print_Number(gb_input[ip_index].ip_en);Print_Message(",");        
		Print_Number(gb_input[ip_index].ip_activate_state);Print_Message(","); 
		Print_Number(gb_input[ip_index].ip_type);Print_Message(",");
		#endif
		
	}
}


/*****************************************************************************
* Function name : void Input_1_Functionality(void).
* Returns		: nothing.
* Arguments     : nothing.
* Created by	: Shirisha Ankam.
* Date created	: 20/11/2025.
*
* Description	: This function is responsible for handling inputs.

* Notes	        : NA.
* Global Variables Affected :input_configuration gb_input.
*****************************************************************************/
void Input_Functionality(void)
{
	if(gb_input_detected_f == 1)											// Check if any input change was detected
	{
		#if (DEBUG_ALL || DEBUG_IP_EXT)
		Print_Message("\nInput detected");
		#endif
		gb_input_detected_f = 0;											// Clear detection flag
		
		unsigned char lcl_ip_data[MAX_INPUTS + 2] = {0};					// Local buffer for storing IP states + CRC bytes
		memset(lcl_ip_data, 0, MAX_INPUTS + 2);								// Initialize buffer to zero
		U8 any_change = 0;													// Flag to track if any input changed
		
		EEPROM_Byte_Read(INPUT_STATE_LOC, lcl_ip_data, (MAX_INPUTS + 2));	// Read latest stored input state from EEPROM
		
		#if (DEBUG_ALL || DEBUG_IP_EXT)	
		Print_Message("\n<><><><><><><><><><><><><><><><><><\n");
		for(U8 i = 0; i < MAX_INPUTS + 2; i++)
		{
			Print_Number(lcl_ip_data[i]);									// Print stored values (only for debugging)
			Print_Message(",");
		}
		#endif
						
		for(U8 ip = 0; ip < MAX_INPUTS; ip++)								// Process each input channel
		{
			if(!gb_input[ip].ip_en)											// Skip if input is disabled
			{
				ip_state[ip].lth_flag = 0;									// Clear low-to-high flag
				ip_state[ip].htl_flag = 0;									// Clear high-to-low flag
				continue;								    
			}
				
			if (!gb_input_cotrol_f[ip])
			{
				continue;
			}							
			ip_state[ip].previous_state = lcl_ip_data[ip];					// Store previously stored state
			
			/**********************************************
				LOW TO HIGH ACTIVE MODE
			 **********************************************/
			if(gb_input[ip].ip_activate_state == LOW_TO_HIGH)				// Check if active mode = L?H
			{
				if(ip_state[ip].lth_flag)									// Low-to-high edge detected
				{
					
					if (gb_input[ip].ip_type != ITYPE_ARM_DARM)
					{
						ip_state[ip].current_state = IP_ACTIVE;					// Set state as ACTIVE
					}               
					else
					{
						
						gb_arm_timer_f = 1;	// Set flag and check in timer routine.
						gb_arm_counter = 0;	// Start counter from fresh.
					}
				}               
				else if(ip_state[ip].htl_flag)								// High-to-low edge detected
				{
					
					if (gb_input[ip].ip_type != ITYPE_ARM_DARM)
					{
						ip_state[ip].current_state = IP_NOT_ACTIVE;				// Set state as NOT ACTIVE
					}
					else
					{
						if (gb_arm_timer_f == 1)
						{
							gb_arm_timer_f = 0;		// Stop incrementing counter in timer.
							gb_arm_counter = 0;		// Reset counter value.
							gb_arm_exceed_f = 0;	// Reset flag to start fresh.
						}
						else
						__NOP();
					}
				}
				else
				{
					__NOP();                                  
				}
               
			}

			/**********************************************
				HIGH TO LOW ACTIVE MODE
			 **********************************************/
			else if(gb_input[ip].ip_activate_state == HIGH_TO_LOW)			// Active mode = H?L
			{
				if(ip_state[ip].htl_flag)									// High-to-low detected
				{
					
					if (gb_input[ip].ip_type != ITYPE_ARM_DARM)
					{
						ip_state[ip].current_state = IP_ACTIVE;					// ACTIVE on HL
					}               
					else
					{
						
						gb_arm_timer_f = 1;	// Set flag and check in timer routine.
						gb_arm_counter = 0;	// Start counter from fresh.
					}
				}               
				else if(ip_state[ip].lth_flag)								// Low-to-high detected
				{
					if (gb_input[ip].ip_type != ITYPE_ARM_DARM)
					{
						ip_state[ip].current_state = IP_NOT_ACTIVE;				// NOT ACTIVE on LH
					}
					else
					{
						
						if (gb_arm_timer_f == 1)
						{
							
							gb_arm_timer_f = 0;		// Stop incrementing counter in timer.
							gb_arm_counter = 0;		// Reset counter value.
							gb_arm_exceed_f = 0;	// Reset flag to start fresh.
						}
						else
						__NOP();
						
						
					}
				}
				else
				{
					__NOP();                                    
				}
                
			}
			else                                           
			{
				__NOP();
			}
			 			 				
			/**********************************************
				 UPDATE EEPROM ONLY IF STATE CHANGED
		     **********************************************/
			if(ip_state[ip].previous_state != ip_state[ip].current_state)   // State change detected?
			{
				any_change = 1;												// Mark that something changed
				lcl_ip_data[ip] = ip_state[ip].current_state;				// Update new state in buffer
				
				Flash_Store_InputState_Log(									// Store event log in flash
					gb_rtcDateArr, 
					gb_rtcTimeArr,
					ip + 1,
					ip_state[ip].current_state
				);
				
				gb_ip_send_to_srver_f = 1;
			}

			ip_state[ip].lth_flag = 0;										// Clear LH flag
			ip_state[ip].htl_flag = 0;										// Clear HL flag
			gb_input_cotrol_f[ip] = 0;
		}

		/************* WRITE UPDATED DATA TO EEPROM ONLY IF CHANGED *************/
		if (any_change)														// Prevent unnecessary EEPROM wear
		{
			CRC_Calculate_Check(lcl_ip_data, MAX_INPUTS, CRC_CALC);			// Calculate CRC for data
			lcl_ip_data[MAX_INPUTS]     = gb_Temp_CRC[0];				    // Store CRC MSB
			lcl_ip_data[MAX_INPUTS + 1] = gb_Temp_CRC[1];					// Store CRC LSB

			EEPROM_Byte_Write(INPUT_STATE_LOC, lcl_ip_data, MAX_INPUTS + 2); // Save to EEPROM
			
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\n<><><><><><><><><><><><><><><><><><\n");
			for(U8 i = 0; i < MAX_INPUTS + 2; i++)
			{
				Print_Number(lcl_ip_data[i]);								 // Print updated data
				Print_Message(",");
			}
			#endif
		}
	}
	else
	__NOP();
	
}


/*****************************************************************************
* Function name : monitor_output.
* Returns		: nothing.
* Arguments     : void.
* Created by	: Shirisha Ankam.
* Date created	: 21/11/2025.
*
* Description	: Monitors output states and detects changes to trigger
*                 server notification.
*
* Notes	        : Uses static variable to track previous states.
* Global Variables Affected : gb_op_send_server_f, gb_op_detected_f
*****************************************************************************/
void monitor_output(void)
{
	static U8 prv_op_st[MAX_OUTPUTS] = {0};                                 // Previous output states (persists across calls)
	U8 change_detected = 0;                                                 // Flag to indicate any output state changed
	for (U8 i = 0; i < MAX_OUTPUTS; i++)                                    // Loop through all outputs
	{
		if (gb_output_state[i] != prv_op_st[i])                             // Compare current state with previous state
		{
			prv_op_st[i] = gb_output_state[i];                              // Update previous state to current
			change_detected = 1;                                            // Mark that a change was detected
		}
	}
	if (change_detected)                                                    // If any output changed
	{
		gb_op_send_server_f = 1;                                            // Set flag to send output status to server
		gb_op_detected_f    = 1;                                            // Set flag indicating output change detected
	}
}


/*****************************************************************************
* Function name : PowerOn_Output_Activation
* Returns       : nothing
* Arguments     : void
* Created by    : Shirisha Ankam
* Date created  : 21/11/2025
*
* Description   : Operates and restores output states at power-on.
*                 - Handles auto outputs with configured duration.
*                 - Resumes outputs if duration not expired.
*                 - Restores last state from EEPROM for manual outputs.
*                 - Applies default states (NO/NC) depending on configuration.
*
* Notes         : Synchronizes outputs with RTC and EEPROM values.
* Global Variables Affected : gb_output_state, gb_start_op_timer,
*                             output_min, gb_sync_op_at_config
*****************************************************************************/
void PowerOn_Output_Activation(void)
{
	#if (DEBUG_ALL || DEBUG_OP_EXT)
	Print_Message("\nPower On Output Activation");       
	#endif
	gb_op_at_pwr_on = 1;										// Flag indicating outputs activated at power-on

	for (U8 op_index = 0; op_index < MAX_OUTPUTS; op_index++)	// Loop through all outputs
	{
		if(gb_output[op_index].op_en == 0)						// Skip disabled outputs
		{
			continue;
		}
		
		if(gb_output[op_index].op_type == 1)					// Auto output type
		{
			/* Validate duration: must be between 1 and 1080 minutes */
			if (gb_output[op_index].op_duration == 0 ||
			gb_output[op_index].op_duration > 1080)
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nInvalid duration for output: ");
				Print_Number(op_index + 1);
				#endif
				continue;
			}
			
			/* Convert configured time to minutes */
			U16 config_min = (gb_output[op_index].op_hour * 60) + gb_output[op_index].op_min;

			/* Convert RTC time to minutes */
			U16 rtc_min = (gb_rtcTimeArr[0] * 60) + gb_rtcTimeArr[1];
			
			#if (DEBUG_ALL || DEBUG_OP_EXT)
			Print_Message("\nConfigure time in min : ");
			Print_Number(config_min);
			Print_Message(" |RTC time in min: ");
			Print_Number(rtc_min);
			#endif
			
			/* Calculate elapsed duration since configured time */
			U16 dur_gap;
			if (rtc_min >= config_min)
			{
				dur_gap = rtc_min - config_min;           // Same day
			}
			else
			{
				dur_gap = (1440 - config_min) + rtc_min;  // Crossed midnight
			}
			
			/* If duration already completed, set to default state */
			if (dur_gap >= gb_output[op_index].op_duration)
			{
				if (gb_output[op_index].op_dflt_state == STATE_NO)
				{
					Operate_Relay(op_index + 1, OFF);     // Relay OFF for NO
				}
				else
				{
					Operate_Relay(op_index + 1, ON);      // Relay ON for NC
				}
				gb_output_state[op_index] = NOT_ACTIVE;
				continue;
			}
			
			/******** Resume output ********/
			gb_start_op_timer[op_index] = 1;              // Start timer for resumed output
			output_min[op_index] = dur_gap;               // Track elapsed minutes
			
			if (gb_output[op_index].op_dflt_state == STATE_NO)
			{
				Operate_Relay(op_index + 1, ON);          // Resume relay ON
			}
			else
			{
				Operate_Relay(op_index + 1, OFF);         // Resume relay OFF
			}
			gb_output_state[op_index] = ACTIVE;           // Mark output as active

			#if (DEBUG_ALL || DEBUG_OP_EXT)
			Print_Message("\nAuto output resumed: ");
			Print_Number(op_index + 1);
			Print_Message(" | Elapsed min: ");
			Print_Number(dur_gap);
			#endif
		}
		else
		{
			__NOP();                                      
		}
		
		if(gb_output[op_index].op_type == 0)              // Manual output type
		{
			U8 lcl_eep_buf[MAX_OUTPUTS];                  // Buffer for EEPROM read
			U8 lcl_eep_status = 0;                        // EEPROM read status
			memset(lcl_eep_buf, 0, MAX_OUTPUTS);          // Clear buffer
			
			if((gb_output[op_index].op_pwr_reset_mode == 1) || (gb_sync_op_at_config == 1)) // Maintain last state on power reset
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nMaintain last State of Output at Power Reset");
				Print_Number(op_index + 1);
				#endif
				
				if(gb_output[op_index].op_dflt_state == STATE_NO)          // Default state is Normally Open (NO)
				{
					#if (DEBUG_ALL || DEBUG_OP_EXT)
					Print_Message("\nConfigured output state is ( NO ) for: ");
					Print_Number(op_index + 1);
					#endif
					
					lcl_eep_status = EEPROM_Byte_Read(OUTPUT_STATE_LOC, lcl_eep_buf, MAX_OUTPUTS);  // Read previous states
					
					if(((lcl_eep_buf[op_index] == ACTIVE) && (gb_output[op_index].op_pwr_reset_mode == 1)) || (gb_output_state[op_index]  ==  ACTIVE)) // Last state ACTIVE
					{						
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nOutput State is ACTIVE");
						#endif
						
						Operate_Relay((op_index + 1), ON);            // Relay ON at power-up
						gb_output_state[op_index] = ACTIVE;
					}
					else if((lcl_eep_buf[op_index] == NOT_ACTIVE) || (gb_output_state[op_index]  ==  NOT_ACTIVE)) // Last state NOT ACTIVE
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nOutput State is NOT ACTIVE");
						#endif
						
						Operate_Relay((op_index + 1), OFF);           // Relay OFF at power-up
						gb_output_state[op_index] = NOT_ACTIVE;
					}
					else
					{
						__NOP();                                     
					}
				}
				else if(gb_output[op_index].op_dflt_state == STATE_NC)    // Default state is Normally Closed (NC)
				{
					#if (DEBUG_ALL || DEBUG_OP_EXT)
					Print_Message("\nConfigured output state is ( NC ) for: ");
					Print_Number(op_index + 1);
					#endif
					
					lcl_eep_status = EEPROM_Byte_Read(OUTPUT_STATE_LOC, lcl_eep_buf, MAX_OUTPUTS); // Read stored states
					
					if((lcl_eep_buf[op_index] == ACTIVE) || (gb_output_state[op_index]  ==  ACTIVE)) // Last state ACTIVE
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nOutput State is ACTIVE");
						#endif
						
						Operate_Relay((op_index + 1), OFF);          // NC logic: ACTIVE ? relay OFF
						gb_output_state[op_index] = ACTIVE;
					}
					else if((lcl_eep_buf[op_index] == NOT_ACTIVE) || (gb_output_state[op_index]  ==  NOT_ACTIVE)) // Last state NOT ACTIVE
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nOutput State is NOT ACTIVE");
						#endif
						
						Operate_Relay((op_index + 1), ON);           // NC logic: NOT ACTIVE ? relay ON
						gb_output_state[op_index] = NOT_ACTIVE;
					}
					else
					{
						__NOP();                                     
					}
				}
				else
				{
					__NOP();                                        
				}
			}
			else                                                  // Always OFF mode
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nAlways OFF the output : ");
				Print_Number(op_index + 1);
				#endif
				
				Operate_Relay((op_index + 1), ON);               // For NC, OFF state means relay ON
				gb_output_state[op_index] = NOT_ACTIVE;
				memset(lcl_eep_buf, 0, MAX_OUTPUTS);
				EEPROM_Byte_Write(OUTPUT_STATE_LOC, lcl_eep_buf, MAX_OUTPUTS);
			}
		}
		else
		{
			__NOP();                                             
		}
	}
	gb_sync_op_at_config = 0;                                    // Reset sync flag
}



/*****************************************************************************
* Function name : void Segregate_OP_Data_To_Buffer.
* Returns		: nothing.
* Arguments     : unsigned char *flsh_ip_data.
* Created by	: Shirisha Ankam.
* Date created	: 18/11/2025.
*
* Description	: This function is to segregate output data.

* Notes	        : NA.
* Global Variables Affected : output_configuration.
*****************************************************************************/
void Segregate_OP_Data_To_Buffer(U8 *flsh_op_data)
{
	for (U8 op_index = 0, fls_idx = 0; op_index < MAX_OUTPUTS; op_index ++)   // Loop through each output and use fls_idx to read sequential bytes
	{
		gb_output[op_index].op_en = flsh_op_data[fls_idx ++];                 // Load output enable flag
		gb_output[op_index].op_dflt_state = flsh_op_data[fls_idx ++];         // Load default NO/NC state
		gb_output[op_index].op_pwr_reset_mode = flsh_op_data[fls_idx ++];     // Load power reset mode (retain or always off/on)
		gb_output[op_index].op_type = flsh_op_data[fls_idx ++];               // Load output type as Auto or manual
		gb_output[op_index].op_hour = flsh_op_data[fls_idx ++];               // Load output activation time in hpur       
		gb_output[op_index].op_min = flsh_op_data[fls_idx ++];				  // Load output activation time  in minute
		gb_output[op_index].op_duration = ((flsh_op_data[fls_idx ++] << 8) | flsh_op_data[fls_idx ++]);  // Load output duration
		
		#if (DEBUG_ALL || DEBUG_OP_EXT)
		Print_Message("\nOutput number  ");
		Print_Number(op_index + 1);
		Print_Message(" : ");
		Print_Number(gb_output[op_index].op_en);Print_Message(",");
		Print_Number(gb_output[op_index].op_dflt_state);Print_Message(",");
		Print_Number(gb_output[op_index].op_pwr_reset_mode);Print_Message(",");
		Print_Number(gb_output[op_index].op_type);Print_Message(",");
		Print_Number(gb_output[op_index].op_hour);Print_Message(",");
		Print_Number(gb_output[op_index].op_min);Print_Message(",");
		Print_Number(gb_output[op_index].op_duration);Print_Message(",");
		#endif
	}

}


/*****************************************************************************
* Function name : void Output_Operations_From_Server.
* Returns		: nothing.
* Arguments     : U8 --> ouput number which to be operated.
                  U8 --> output state on or off.
* Created by	: Shirisha Ankam.
* Date created	: 20/11/2025.
*
* Description	: This function is responsible Operating Outputs when RC frame is received .

* Notes	        : NA.
* Global Variables Affected : gb_output_state, output_configuration.
*****************************************************************************/
void Output_Operations_From_Server(U8 rec_op_num,U8 rec_op_st)
{	
	#if (DEBUG_ALL || DEBUG_OP_EXT)
	Print_Message("\nInside Output Operation from Sever");
	#endif
	
	if (gb_output[rec_op_num - 1].op_en == 1)                                       // Check if output is enabled.
	{
		#if (DEBUG_ALL || DEBUG_OP_EXT)
		Print_Message("\nOutput is enabled: ");
		Print_Number(rec_op_num);
		#endif
		if(rec_op_st == ACTIVE)														// received state is active
		{
			#if (DEBUG_ALL || DEBUG_OP_EXT)
			Print_Message("\nReceived output state Activation for: ");
			Print_Number(rec_op_num);
			#endif
			if(gb_output[rec_op_num - 1].op_dflt_state == STATE_NC)                 // Check if the default state of output is set to NC.
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nConfigured output state is ( NC ) for: ");
				Print_Number(rec_op_num);
				#endif
				
				Operate_Relay(rec_op_num, OFF);                                     //activate relay      
				
				gb_output_state[rec_op_num - 1] = ACTIVE;							//update output state in global variable
				
				if(gb_output[rec_op_num - 1].op_pwr_reset_mode == 1)
				{
					EEPROM_Byte_Write(OUTPUT_STATE_LOC, gb_output_state, MAX_OUTPUTS); // store updated output state in memory	
				}
				
					
				#if (DEBUG_ALL || DEBUG_OP_EXT)		
				Print_Message("\nRelay Activated at ( NC ) for: ");
				Print_Number(rec_op_num);
				#endif
			}
			else if(gb_output[rec_op_num - 1].op_dflt_state == STATE_NO)			// Check if the default state of output is set to NC.
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nConfigured output state is ( NO ) for: ");
				Print_Number(rec_op_num);
				#endif
				
				Operate_Relay(rec_op_num, ON);                                      //activate relay   
							
				gb_output_state[rec_op_num - 1] = ACTIVE;                           //update output state in global variable
				
				if(gb_output[rec_op_num - 1].op_pwr_reset_mode == 1)
				{
					EEPROM_Byte_Write(OUTPUT_STATE_LOC, gb_output_state, MAX_OUTPUTS); // store updated output state in memory
				}
				
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nRelay Activated at ( NO ) for: ");
				Print_Number(rec_op_num);
				#endif
			}
				
		}
		else if(rec_op_st == NOT_ACTIVE)                                            // received state is de-activate
		{
			#if (DEBUG_ALL || DEBUG_OP_EXT)
			Print_Message("\nReceived output state Deactivation for: ");
			Print_Number(rec_op_num);
			#endif		
			if(gb_output[rec_op_num - 1].op_dflt_state == STATE_NO)                 // Check if the default state of output is set to NO.
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nConfigured output state is ( NO ) for: ");
				Print_Number(rec_op_num);
				#endif
					
				Operate_Relay(rec_op_num, OFF);                                     //relay deactivate
				
				gb_output_state[rec_op_num - 1] = NOT_ACTIVE;
				
				if(gb_output[rec_op_num - 1].op_pwr_reset_mode == 1)
				{
					EEPROM_Byte_Write(OUTPUT_STATE_LOC, gb_output_state, MAX_OUTPUTS); // store updated output state in memory
				}
					
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nRelay Di - Activated at ( NO ) for: ");
				Print_Number(rec_op_num);
				#endif
			}
			else if(gb_output[rec_op_num - 1].op_dflt_state == STATE_NC)            // Check if the default state of output is set to NO.
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nConfigured output state is ( NC ) for: ");
				Print_Number(rec_op_num);
				#endif
				 
				Operate_Relay(rec_op_num, ON);                                       //relay deactivate
							
				gb_output_state[rec_op_num - 1] = NOT_ACTIVE;
				
				if(gb_output[rec_op_num - 1].op_pwr_reset_mode == 1)
				{
					EEPROM_Byte_Write(OUTPUT_STATE_LOC, gb_output_state, MAX_OUTPUTS); // store updated output state in memory
				}
				
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nRelay Di-Activated at ( NC ) for: ");
				Print_Number(rec_op_num);
				#endif
			}
			else
			{
				__NOP();
			}
		}
	}

	#if (DEBUG_ALL || DEBUG_OP_EXT)
	Print_Message("\nStored Output States are ");
	for(U8 i = 0; i < MAX_OUTPUTS; i++)
	{
		Print_Number(gb_output_state[i]);
		Print_Message(",");
	}
	#endif
}


/*****************************************************************************
* Function name : Auto_Output_Activation
* Returns       : Nothing.
* Arguments     : None.
* Created by    : Shirisha Ankam
* Date created  : 20/11/2025
*
* Description   : Operates outputs if output type is Auto.
*                 - Starts outputs at configured RTC time.
*                 - Runs for configured duration (1–1080 minutes).
*                 - Turns outputs OFF once duration expires.
*
* Notes         : Duration and default state (NO/NC) determine relay behavior.
* Global Variables Affected : gb_output_state, gb_start_op_timer, output_min
*****************************************************************************/
void Auto_Output_Activation()
{
	for(U8 op_num = 0; op_num < MAX_OUTPUTS; op_num++)          // Loop through all outputs
	{
		if (gb_output[op_num].op_en && gb_output[op_num].op_type == 1)   // Check if enabled and Auto type
		{
			if(gb_output[op_num].op_duration == 0 || gb_output[op_num].op_duration > 1080) // Validate duration
			{
				#if (DEBUG_ALL || DEBUG_OP_EXT)
				Print_Message("\nInvalid Duration");             // Debug: invalid duration
				Print_Number(op_num);
				#endif
				continue;                                        // Skip invalid configuration
			}
			
			if(gb_output_state[op_num] == NOT_ACTIVE)            // If output is currently inactive
			{
				if((gb_output[op_num].op_hour == gb_rtcTimeArr[0]) &&
				(gb_output[op_num].op_min == gb_rtcTimeArr[1])) // Match RTC time with configured time
				{
					#if (DEBUG_ALL || DEBUG_OP_EXT)
					Print_Message("\n<><><><>Timer On for<><><><><><: "); // Debug: timer started
					Print_Number(op_num);
					#endif
					
					gb_start_op_timer[op_num] = 1;               // Start timer
					output_min[op_num] = 0;                      // Reset elapsed minutes
					
					if(gb_output[op_num].op_dflt_state == STATE_NC)   // Default state NC
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nConfigured output state is ( NC ) for: ");
						Print_Number(op_num);
						#endif
						
						Operate_Relay(op_num + 1, OFF);          // Activate relay (NC logic)
						gb_output_state[op_num] = ACTIVE;        // Update state to active
					}
					else if(gb_output[op_num].op_dflt_state == STATE_NO) // Default state NO
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\nConfigured output state is ( NO ) for: ");
						Print_Number(op_num + 1);
						#endif
						
						Operate_Relay(op_num + 1, ON);           // Activate relay (NO logic)
						gb_output_state[op_num] = ACTIVE;        // Update state to active
					}
				}
			}
			else if(gb_output_state[op_num] == ACTIVE)           // If output is currently active
			{
				if (output_min[op_num] >= gb_output[op_num].op_duration) // Check if duration expired
				{
					gb_start_op_timer[op_num] = 0;               // Stop timer
					output_min[op_num] = 0;                      // Reset elapsed minutes
					
					if(gb_output[op_num].op_dflt_state == STATE_NC)   // Default NC
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\n * Configured output state is ( NC ) for: ");
						Print_Number(op_num);
						#endif
						
						Operate_Relay(op_num + 1, ON);           // Deactivate relay (NC logic)
						gb_output_state[op_num] = NOT_ACTIVE;    // Update state to not active
					}
					else if(gb_output[op_num].op_dflt_state == STATE_NO) // Default NO
					{
						#if (DEBUG_ALL || DEBUG_OP_EXT)
						Print_Message("\n * Configured output state is ( NO ) for: ");
						Print_Number(op_num + 1);
						#endif
						
						Operate_Relay(op_num + 1, OFF);          // Deactivate relay (NO logic)
						gb_output_state[op_num] = NOT_ACTIVE;    // Update state to not active
					}
				}
			}
		}
	}
}



/*****************************************************************************
* Function name : Read_Input_Pin.
* Returns		: nothing.
* Arguments     : uint8_t - > input number.
* Created by	: Shirisha Ankam.
* Date created	: 22/11/2025.
*
* Description	: This function is reading pin state at power on.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_output_state, output_configuration.
*****************************************************************************/
uint8_t Read_Input_Pin(uint8_t ip)
{	
	if(ip >= MAX_INPUTS)
	return IP_NOT_ACTIVE;

	uint32_t mask = input_pin_mask[ip];
	Pio *port     = input_port[ip];
	
	if(gb_input[ip].ip_activate_state == LOW_TO_HIGH)    //configured state if Low to High
	{
		if (pio_get(port, PIO_TYPE_PIO_INPUT, mask))
		//if(port->PIO_PDSR & mask)		
			return IP_ACTIVE;        // HIGH		
		else	
			return IP_NOT_ACTIVE;    // LOW		
	}
	else if(gb_input[ip].ip_activate_state == HIGH_TO_LOW)  //configured state is high to low
	{
		if (pio_get(port, PIO_TYPE_PIO_INPUT, mask))
		//if(port->PIO_PDSR & mask)
		return IP_NOT_ACTIVE;        // HIGH
		else
		return IP_ACTIVE;    // LOW
	}
	  
}



/*****************************************************************************
* Function name : Input_PowerUp_Sync.
* Returns		: nothing.
* Arguments     : void.
* Created by	: Shirisha Ankam.
* Date created	: 22/11/2025.
*
* Description	: This function is responsible for sync pin state with previous state at power on.
*              :
* Notes	     : NA.
* Global Variables Affected : gb_output_state, output_configuration.
*****************************************************************************/
void Input_PowerUp_Sync(void)
{
	#if (DEBUG_ALL || DEBUG_IP_EXT)
	Print_Message("\nSync Input State at Power On");
	#endif
	
	unsigned char eep_ip[MAX_INPUTS + 2] = {0};

	// Read previously stored input states + CRC from EEPROM
	EEPROM_Byte_Read(INPUT_STATE_LOC, eep_ip, MAX_INPUTS + 2);
	
	#if (DEBUG_ALL || DEBUG_IP_EXT)
	Print_Message("\nPower on read eeprom data for Input state is: ");
	for (U8 idx = 0; idx < MAX_INPUTS + 2; idx++)
	{
		Print_Number(eep_ip[idx]);
		Print_Message(",");
	}
	#endif
	
	// Perform CRC validation on the data (excluding the last 2 bytes, which are CRC)
	if (CRC_Calculate_Check(eep_ip, MAX_INPUTS, CRC_CHECK) == 1)
	{
		// CRC validation passed
		#if (DEBUG_ALL || DEBUG_IP_EXT)
		Print_Message("\nCRC Validated for IP data");
		#endif
		__NOP();		
	}
	else
	{
		// CRC validation failed, handle error
		#if (DEBUG_ALL || DEBUG_IP_EXT)
		Print_Message("\nCRC failed for IP data");
		#endif

		memset(eep_ip, 255, MAX_INPUTS + 2);  // Clear the data buffer

		// Recalculate CRC for the cleared data
		CRC_Calculate_Check(eep_ip, MAX_INPUTS, CRC_CALC);
		eep_ip[MAX_INPUTS] = gb_Temp_CRC[0];  // Store higher byte of CRC
		eep_ip[MAX_INPUTS + 1] = gb_Temp_CRC[1];  // Store lower byte of CRC

		// Write the corrected data back to flash memory
		EEPROM_Byte_Write(INPUT_STATE_LOC, eep_ip, MAX_INPUTS + 2);
		
		return;
	}
	
	// Compare each input pin’s current state with previously saved state
	for(uint8_t ip = 0; ip < MAX_INPUTS; ip++)
	{
		// Skip processing for disabled inputs
		if(!gb_input[ip].ip_en)
		continue;
		
		// Read actual hardware GPIO state at power-up
		uint8_t actual_state = Read_Input_Pin(ip);
		
		// If previous EEPROM state is different from actual hardware state
		if(actual_state != eep_ip[ip])
		{
			// Debug printing: show mismatch
			#if (DEBUG_ALL || DEBUG_IP_EXT)
			Print_Message("\n| Input pin      | : ");
			Print_Number(ip + 1);
			Print_Message("\n| Actual State   | : ");
			Print_Number(actual_state);
			Print_Message("\n| previous State | : ");
			Print_Number(eep_ip[ip]);
			#endif
			// Log this difference because state changed while power was OFF
			Flash_Store_InputState_Log(gb_rtcDateArr, gb_rtcTimeArr, ip + 1, actual_state);

			// Update EEPROM buffer with the new state
			eep_ip[ip] = actual_state;
		}
	}

	// Recalculate CRC on updated input states
	CRC_Calculate_Check(eep_ip, MAX_INPUTS, CRC_CALC);

	// Store new CRC bytes
	eep_ip[MAX_INPUTS]     = gb_Temp_CRC[0];
	eep_ip[MAX_INPUTS + 1] = gb_Temp_CRC[1];

	// Write updated states + CRC back to EEPROM
	EEPROM_Byte_Write(INPUT_STATE_LOC, eep_ip, MAX_INPUTS + 2);
	
	#if (DEBUG_ALL || DEBUG_IP_EXT)
	Print_Message("\nPower on updated eeprom data for Input state is: ");
	for (U8 idx = 0; idx < MAX_INPUTS + 2; idx++)
	{
		Print_Number(eep_ip[idx]);
		Print_Message(",");
	}
	#endif
}

/*****************************************************************************
* Function Name  : Safety_And_Intruder_Monitor
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Monitors ARM/DISARM and intruder input conditions and controls
*                  alarm generation based on configured delays and system state.
*                  The function performs the following operations:
*                  - Monitors ARM/DISARM input status and updates system arm state.
*                  - Tracks intruder input activation when the system is ARMED.
*                  - Starts and manages arm-delay and alarm-delay timers.
*                  - Generates alarm output after arm-delay expiry for valid inputs.
*                  - Stops alarm generation on DISARM or after alarm-delay expiry.
*                  - Resets monitoring flags to allow re-triggering of alarms.
*                  - Uses a 1-second RTC tick to update delay counters.
*
* Notes          :
*                  - ARM delay is applied before enabling intruder alarm detection.
*                  - Alarm delay controls how long the alarm remains active.
*                  - Supports ARM/DISARM initiated by system inputs or server.
*                  - Intruder inputs are validated against server-configured input list.
*                  - Uses Output_Operations_From_Server() to control alarm output.
*                  - Debug messages are printed for state transitions and delays.
*
* Global Variables Affected :
*                  intruder,
*                  ip_state,
*                  gb_input,
*                  gb_rtc_1sec_delay_f
*****************************************************************************/
void Safety_And_Intruder_Monitor(void)
{
	/************************************************************************/
	/* Check for the ARM-DISARM input                                       */
	/************************************************************************/
	static U8 lcl_secs;			// Static seconds counter for alarm delay timing
	Monitor_ARM_Input();		// Monitor ARM/DISARM hardware input
	for (U8 ip_num = 0; ip_num < MAX_INPUTS; ip_num++)	// Loop through all inputs
	{
		if ((intruder.arm_status == ARMED)/* && (intruder.arm_monitor_f == 0)*/)	// System armed check
		{
			if (intruder.arm_monitor_f == 0)	// First ARM detection
			{
				intruder.arm_monitor_f = 1;		// Mark ARM signal monitored
			}
			else
			__NOP();	// No operation

			if ((ip_state[ip_num].current_state == IP_ACTIVE)	// Input active
			&& (gb_input[ip_num].ip_type == ITYPE_INTRUDER)		// Intruder type input
			&& (intruder.delay_monitor_f[ip_num] == 0))			// Delay not started yet
			{
				intruder.delay_monitor_f[ip_num] = 1;	// Start monitoring delay for this input
				if (intruder.start_arm_delay_f == 0)	// ARM delay not started
				{
					intruder.start_arm_delay_f = 1;	// Start ARM delay
					intruder.arm_del_counter = 0;	// Reset ARM delay counter
					intruder.arm_del_exceed_f = 0;	// Clear delay exceed flag
				}
				else
				__NOP();	// No operation
			}
			else if ((ip_state[ip_num].current_state == IP_NOT_ACTIVE)	// Input inactive
			&& (gb_input[ip_num].ip_type == ITYPE_INTRUDER)				// Intruder type input
			&& (intruder.delay_monitor_f[ip_num] == 1))					// Delay was active
			{
				intruder.delay_monitor_f[ip_num] = 0;	// Stop delay monitoring

				U8 lcl_all_ip_inactive = 1;	// Assume all intruder inputs inactive
				for (U8 iIdx = 0; iIdx < MAX_INPUTS; iIdx++)	// Loop through all inputs
				{
					if ((ip_state[iIdx].current_state == IP_ACTIVE)	// Any active input
					&& (gb_input[iIdx].ip_type == ITYPE_INTRUDER))	// Intruder type
					{
						lcl_all_ip_inactive = 0;	// Clear all-inactive flag
						break;						// Exit loop
					}
					else
					__NOP();	// No operation
				}

				if (lcl_all_ip_inactive == 1)	// All intruder inputs inactive
				{
					lcl_all_ip_inactive = 0;	// Reset local flag
				}
				else
				__NOP();	// No operation
			}
		}
		else if ((((ip_state[ip_num].current_state == IP_NOT_ACTIVE)	// ARM/DISARM input inactive
		&& (gb_input[ip_num].ip_type == ITYPE_ARM_DARM)				// ARM/DISARM type
		&& (intruder.arm_darm_by == AM_SYSTEM))						// Disarmed by system
		|| ((intruder.arm_status == DIS_ARMED)						// System disarmed
		&& (intruder.arm_darm_by == AM_SERVER)))					// Disarmed by server
		&& (intruder.arm_monitor_f == 1))							// ARM was previously monitored
		{
			intruder.arm_monitor_f = 0;		// Reset ARM monitor flag
			intruder.start_arm_delay_f = 0;	// Stop ARM delay
			intruder.arm_del_exceed_f = 0;	// Clear ARM delay exceed
			Output_Operations_From_Server(intruder.op_number, NOT_ACTIVE);	// Turn OFF alarm output
			intruder.start_alarm_delay_f = 0;	// Stop alarm delay
			intruder.alarm_del_counter = 0;		// Reset alarm delay counter
			for (U8 iIdx = 0; iIdx < MAX_INPUTS; iIdx++)	// Loop through all inputs
			{
				intruder.ip_monitor_f[iIdx] = 0;		// Reset IP monitor flag
				intruder.delay_monitor_f[iIdx] = 0;	// Reset delay monitor flag
			}
			Write_Arm_Status_To_Memory();	// Store ARM status to memory
		}
		else
		__NOP();	// No operation

		if ((ip_state[ip_num].current_state == IP_ACTIVE)	// Input active
		&& (gb_input[ip_num].ip_type == ITYPE_INTRUDER)		// Intruder type
		&& (intruder.arm_del_exceed_f == 1)					// ARM delay exceeded
		&& (intruder.ip_monitor_f[ip_num] == 0))				// Input not yet processed
		{
			intruder.ip_monitor_f[ip_num] = 1;	// Mark input as processed
			U8 lcl_input_present = 0;			// Reset presence flag
			if ((intruder.arm_darm_by == AM_SYSTEM)	// Armed by system
			|| ((intruder.arm_darm_by == AM_SERVER)	// Armed by server
			&& (IS_BIT_SET(intruder.arm_inputs, ip_num))))	// Input present in server list
			{
				lcl_input_present = 1;	// Mark input as valid
			}
			else
			__NOP();	// No operation

			if ((lcl_input_present == 1)	// Input valid
			&& (intruder.start_alarm_delay_f == 0))	// Alarm delay not started
			{
				Output_Operations_From_Server(intruder.op_number, ACTIVE);	// Activate alarm output
				intruder.start_alarm_delay_f = 1;	// Start alarm delay
				intruder.alarm_del_counter = 0;		// Reset alarm delay counter
				intruder.alarm_del_exceed_f = 0;	// Clear alarm exceed flag
				lcl_secs = 0;						// Reset seconds counter
				Write_Arm_Status_To_Memory();		// Store status to memory
			}
			else
			__NOP();	// No operation
		}
		else if ((ip_state[ip_num].current_state == IP_NOT_ACTIVE)	// Input inactive
		&& (gb_input[ip_num].ip_type == ITYPE_INTRUDER)				// Intruder type
		&& (intruder.arm_del_exceed_f == 1)							// ARM delay exceeded
		&& (intruder.ip_monitor_f[ip_num] == 1))					// Input already processed
		{
			intruder.ip_monitor_f[ip_num] = 0;	// Reset IP monitor flag
		}
		else
		__NOP();	// No operation
	}

	// Check 1-second RTC delay flag
	if (gb_rtc_1sec_delay_f == 1)	// RTC tick occurred
	{
		gb_rtc_1sec_delay_f = 0;	// Clear RTC flag
		if (intruder.start_arm_delay_f == 1)	// ARM delay active
		{
			intruder.arm_del_counter++;	// Increment ARM delay counter
			if (intruder.arm_del_counter >= intruder.arm_delay)	// ARM delay reached
			{
				intruder.start_arm_delay_f = 0;	// Stop ARM delay
				intruder.arm_del_counter = 0;	// Reset counter
				intruder.arm_del_exceed_f = 1;	// Set exceed flag
			}
			else
			__NOP();	// No operation
		}
		else
		__NOP();	// No operation

		if (intruder.start_alarm_delay_f == 1)	// Alarm delay active
		{
			lcl_secs++;	// Increment seconds counter
			if (lcl_secs >= 60)	// One minute elapsed
			{
				lcl_secs = 0;	// Reset seconds
				intruder.alarm_del_counter++;	// Increment alarm delay counter
				if (intruder.alarm_del_counter >= intruder.alarm_delay)	// Alarm delay reached
				{
					intruder.start_alarm_delay_f = 0;	// Stop alarm delay
					intruder.alarm_del_counter = 0;		// Reset counter
					intruder.alarm_del_exceed_f = 1;	// Set exceed flag
					Write_Arm_Status_To_Memory();		// Store status to memory
				}
				else
				__NOP();	// No operation
			}
			else
			__NOP();	// No operation
		}
		else
		__NOP();	// No operation
	}
	else
	__NOP();	// No operation

	if (intruder.alarm_del_exceed_f == 1)	// Alarm delay exceeded
	{
		intruder.alarm_del_exceed_f = 0;	// Clear alarm exceed flag
		intruder.arm_del_exceed_f = 0;		// Clear ARM delay exceed flag
		Output_Operations_From_Server(intruder.op_number, NOT_ACTIVE);	// Deactivate alarm output
	}
	else
	__NOP();	// No operation
}

/*****************************************************************************
* Function Name  : Monitor_ARM_Input
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated By     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Monitors the ARM/DISARM input pulses and updates the system
*                  arm state accordingly. The function detects valid ARM timing
*                  exceed events and interprets successive pulses to toggle
*                  between ARMED and DIS-ARMED states.
*
*                  Functional flow:
*                  - Checks for ARM timer exceed flag (gb_arm_exceed_f).
*                  - Identifies ARM/DISARM type inputs.
*                  - Uses pulse count (gb_arm_pin_val) to determine:
*                      • First pulse  ? ARM the system
*                      • Second pulse ? DIS-ARM the system
*                  - Updates intruder system state and source (system pin).
*                  - Stores arm status persistently in memory.
*                  - Logs input state changes into flash with RTC timestamp.
*                  - Updates EEPROM with the latest input state and CRC.
*                  - Sets flags to notify the server about arm status and input
*                    state changes.
*
* Notes          :
*                  - Pulse-based ARM/DISARM logic relies on gb_arm_exceed_f.
*                  - ARM state changes are logged only on state transitions.
*                  - CRC is recalculated before storing input state in EEPROM.
*                  - Server update flags are set after successful state changes.
*
* Global Variables Affected :
*                  gb_arm_exceed_f,
*                  gb_arm_pin_val,
*                  gb_arm_feedback_f,
*                  gb_ip_send_to_srver_f,
*                  intruder,
*                  ip_state,
*                  gb_input,
*                  gb_Temp_CRC
*****************************************************************************/
void Monitor_ARM_Input(void)
{
	for (U8 ip_num = 0; ip_num < MAX_INPUTS; ip_num ++)	// Loop through all inputs
	{
		if ((gb_arm_exceed_f == 1) && (gb_input[ip_num].ip_type == ITYPE_ARM_DARM))	// ARM timer exceeded and ARM/DISARM input
		{
			gb_arm_exceed_f = 0;	// Reset ARM exceed flag
			gb_arm_pin_val ++;		// Increment pulse counter
			if (gb_arm_pin_val > 2)	// Pulse count rollover check
			{
				gb_arm_pin_val = 1;	// Reset pulse count to first pulse
			}
			else
			__NOP();	// No operation

			if (gb_arm_pin_val == 1)	// First pulse detected
			{
				// First Pulse detected.
				ip_state[ip_num].current_state = IP_ACTIVE;	// Mark ARM input active
				intruder.arm_status = ARMED;				// Set system as armed
				intruder.arm_darm_by = AM_SYSTEM;			// Armed by system input
				Write_Arm_Status_To_Memory();				// Store ARM status
				gb_arm_feedback_f = 1;						// Enable feedback to server
			}
			else if (gb_arm_pin_val == 2)	// Second pulse detected
			{
				// Second pulse detected.
				ip_state[ip_num].current_state = IP_NOT_ACTIVE;	// Mark ARM input inactive
				intruder.arm_status = DIS_ARMED;				// Set system as disarmed
				intruder.arm_darm_by = AM_SYSTEM;				// Disarmed by system input
				Write_Arm_Status_To_Memory();					// Store ARM status
				gb_arm_feedback_f = 1;							// Enable feedback to server
			}
			else
			__NOP();	// No operation

			if (ip_state[ip_num].previous_state != ip_state[ip_num].current_state)	// Detect state change
			{
				Flash_Store_InputState_Log(					// Store input state log
				gb_rtcDateArr,								// RTC date
				gb_rtcTimeArr,								// RTC time
				ip_num + 1,									// Input number
				ip_state[ip_num].current_state				// Current state
				);

				// Read latest stored input state from EEPROM
				U8 lcl_ip_data[(MAX_INPUTS + 2)];			// Local buffer for EEPROM data
				memset(lcl_ip_data, 0, (MAX_INPUTS + 2));	// Clear buffer
				EEPROM_Byte_Read(INPUT_STATE_LOC, lcl_ip_data, (MAX_INPUTS + 2));	// Read EEPROM
				lcl_ip_data[ip_num] = ip_state[ip_num].current_state;	// Update current input state

				CRC_Calculate_Check(lcl_ip_data, MAX_INPUTS, CRC_CALC);	// Calculate CRC
				lcl_ip_data[(MAX_INPUTS + 0)] = gb_Temp_CRC[0];			// Store CRC MSB
				lcl_ip_data[(MAX_INPUTS + 1)] = gb_Temp_CRC[1];			// Store CRC LSB
				EEPROM_Byte_Write(INPUT_STATE_LOC, lcl_ip_data, (MAX_INPUTS + 2));	// Write EEPROM

				gb_ip_send_to_srver_f = 1;	// Trigger input status update to server
			}
			else
			__NOP();	// No operation
		}
	}
}

/*****************************************************************************
* Function Name  : Write_Arm_Status_To_Memory
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated by     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Writes the current ARM/DIS-ARM status information of the
*                  intruder system into EEPROM with CRC protection.
*
*                  Data stored:
*                  - System ARM / DIS-ARM status
*                  - Source of ARM/DISARM (system pin or server)
*                  - Alarm delay start flag
*                  - CRC-16 checksum for data integrity
*
*                  Functional flow:
*                  - Clears local buffer.
*                  - Copies intruder arm-related parameters into buffer.
*                  - Calculates CRC over valid data length.
*                  - Appends CRC bytes at the end of the buffer.
*                  - Writes complete data block into EEPROM.
*
* Notes          :
*                  - CRC is mandatory to ensure EEPROM data integrity.
*                  - ARM_MAX_BYTES must include space for CRC (2 bytes).
*
* Global Variables Affected :
*                  intruder,
*                  gb_Temp_CRC
*****************************************************************************/
void Write_Arm_Status_To_Memory(void)
{
	U8 temp_arm_arr[ARM_MAX_BYTES];			// Local buffer to store arm status data
	memset(temp_arm_arr, 0, ARM_MAX_BYTES);	// Clear buffer before use

	temp_arm_arr[0] = intruder.arm_status;			// Store system ARM / DISARM status
	temp_arm_arr[1] = intruder.arm_darm_by;			// Store ARM/DISARM source (system/server)
	temp_arm_arr[2] = intruder.start_alarm_delay_f;	// Store alarm delay start flag

	CRC_Calculate_Check(temp_arm_arr, (ARM_MAX_BYTES - 2), CRC_CALC);	// Calculate CRC for data
	temp_arm_arr[(ARM_MAX_BYTES - 2)] = gb_Temp_CRC[0];	// Store CRC MSB
	temp_arm_arr[(ARM_MAX_BYTES - 1)] = gb_Temp_CRC[1];	// Store CRC LSB

	EEPROM_Byte_Write(ARM_EEP_START_LOC, temp_arm_arr, ARM_MAX_BYTES);	// Write arm status data to EEPROM
}

/*****************************************************************************
* Function Name  : Arm_DisArm_At_Powerup
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 06/02/2026
* Updated by     : Ranjitkumar Ainapure
* Date Updated   : 06/02/2026
*
* Description    : Restores ARM / DIS-ARM state of the intruder system at
*                  power-up from EEPROM with CRC validation.
*
*                  Functional flow:
*                  - Reads ARM status data from EEPROM.
*                  - Validates data integrity using CRC.
*                  - If CRC fails, initializes EEPROM with default values.
*                  - Restores ARM/DISARM status, source, and alarm delay flag.
*                  - Synchronizes ARM/DISARM input pin states.
*                  - Restores intruder input states from hardware pins.
*                  - Re-triggers alarm sequence if alarm was active before
*                    power-down.
*
* Notes          :
*                  - ARM_MAX_BYTES must include CRC bytes.
*                  - This function must be called once during system power-up.
*
* Global Variables Affected :
*                  intruder,
*                  ip_state,
*                  gb_arm_pin_val,
*                  gb_Temp_CRC
*****************************************************************************/
void Arm_DisArm_At_Powerup(void)
{
	U8 temp_arm_arr[ARM_MAX_BYTES];
	memset(temp_arm_arr, 0, ARM_MAX_BYTES);

	EEPROM_Byte_Read(ARM_EEP_START_LOC, temp_arm_arr, ARM_MAX_BYTES);

	if (CRC_Calculate_Check(temp_arm_arr, (ARM_MAX_BYTES - 2), CRC_CHECK) == 1)
	{
		// CRC validated.
		__NOP();
	}
	else
	{
		// CRC failed – initialize default values.
		memset(temp_arm_arr, 0, ARM_MAX_BYTES);
		CRC_Calculate_Check(temp_arm_arr, (ARM_MAX_BYTES - 2), CRC_CALC);
		temp_arm_arr[(ARM_MAX_BYTES - 2)] = gb_Temp_CRC[0];
		temp_arm_arr[(ARM_MAX_BYTES - 1)] = gb_Temp_CRC[1];
		EEPROM_Byte_Write(ARM_EEP_START_LOC, temp_arm_arr, ARM_MAX_BYTES);
	}

	/* Restore ARM/DISARM parameters */
	intruder.arm_status           = temp_arm_arr[0];	// System ARM/DISARM status
	intruder.arm_darm_by          = temp_arm_arr[1];	// ARM/DISARM source
	intruder.start_alarm_delay_f  = temp_arm_arr[2];	// Alarm delay flag

	if (intruder.arm_darm_by == AM_SYSTEM)
	{
		// ARM/DISARM done by hardware before power-up.
		for (U8 iIdx = 0; iIdx < MAX_INPUTS; iIdx++)
		{
			if ((gb_input[iIdx].ip_type == ITYPE_ARM_DARM) &&
			(intruder.arm_status == ARMED))
			{
				ip_state[iIdx].current_state = IP_ACTIVE;
				gb_arm_pin_val = 1;
			}
			else if ((gb_input[iIdx].ip_type == ITYPE_ARM_DARM) &&
			(intruder.arm_status == DIS_ARMED))
			{
				ip_state[iIdx].current_state = IP_NOT_ACTIVE;
				gb_arm_pin_val = 2;
			}
			else
			{
				__NOP();
			}
		}
	}
	else if (intruder.arm_darm_by == AM_SERVER)
	{
		// ARM/DISARM done by server.
		Update_AM_ByServer_Variables();
	}
	else
	{
		__NOP();
	}

	/* Restore intruder input states from hardware pins */
	for (U8 iIdx = 0; iIdx < MAX_INPUTS; iIdx++)
	{
		if ((gb_input[iIdx].ip_type == ITYPE_INTRUDER) &&
		(Read_Input_Pin(iIdx) == 1))
		{
			ip_state[iIdx].current_state = IP_ACTIVE;
		}
		else if ((gb_input[iIdx].ip_type == ITYPE_INTRUDER) &&
		(Read_Input_Pin(iIdx) == 0))
		{
			ip_state[iIdx].current_state = IP_NOT_ACTIVE;
		}
		else
		{
			__NOP();
		}
	}

	/* Restart alarm if it was active before power-down */
	if (intruder.start_alarm_delay_f == 1)
	{
		Output_Operations_From_Server(intruder.op_number, ACTIVE);
		intruder.start_alarm_delay_f = 1;
		intruder.alarm_del_counter   = 0;
		intruder.alarm_del_exceed_f  = 0;
	}
	else
	{
		__NOP();
	}
}

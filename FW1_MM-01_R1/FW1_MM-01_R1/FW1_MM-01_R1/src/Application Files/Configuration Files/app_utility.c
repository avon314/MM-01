/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_utility.c
* Created By	: Harshit Agnihotri.
* Created Date	: 28/12/2023.
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*				1024 KB		Flash.
*				128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 28/11/2023.
* Changes		: NA.
*****************************************************************************/

/* System Includes */
#include "asf.h"
#include "string.h"

/* User Includes */
#include "app_utility.h"
#include "definitions.h"
#include "user_uart.h"
#include "tcp_server.h"
#include "CRC.h"
#include "group_config.h"
#include "comn_config.h"
#include "config_mode.h"
#include "user_rtc.h"
#include "dflt_config.h"
 
#include "app_eeprom.h"
#include "memory_storage.h"
#include "ext_eeprom_1.h"
#include "bootloader.h"
#include "mm_app.h"


U8 gb_utility_rcv_f = 0;            // Global flag indicating the reception of a utility frame.
volatile U8 gb_op_config_cplt = 0;  // Flag indicating completion of output configuration.
volatile U8 gb_conn_dvc_f = 0;      // Flag to indicate connect with device successful.
volatile U8 gb_err_crc_f = 0;       // Flag to indicate error in CRC of received frame.
volatile U8 gb_err_len_f = 0;       // Flag to indicate error in length of received frame.
volatile U8 gb_err_data_f = 0;      // Flag to indicate error in data of received frame.
U8 gb_exit_cnf_f = 0;

volatile U8 gb_utility_req_f = 0;	// Flag used to say request is not by the utility.
volatile U8 gb_fact_reset_f = 0;
extern U8 gb_boot_jump_start_timer ;   //globale variable

// Declaration of global instances of the Utility structure for receiving and transmitting frames.
Utility utility_rx;
Utility utility_tx;

extern char gb_def_ipAddress[13];
extern char gb_dev_ipAddress[13];

/*****************************************************************************
* Function name: void validate_utility_frame(void).
* Returns		: nothing.
* Arguments    : None.
* Created by	: Harshit Agnihotri.
* Date created	: 28/12/2023.
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025
*
* Description	: Implementation of the utility frame validation function and create response frame to be transmitted on tcp ip.
*              :
* Notes		: NA.
* Global Variables Affected : utility_rx, utility_tx and gb_utility_rcv_f.
*****************************************************************************/
void validate_utility_frame(void)
{
     U8 rcv_frame_idx = 0;                                                      // Index to traverse received frame buffer.
     if (gb_mbTcp_rcv_buf[rcv_frame_idx++] == SOF)                               // Check for Start of Frame (SOF).
     {
          // Macros to be updated.
          memcpy(utility_rx.frame_buf, gb_mbTcp_rcv_buf, gb_mbTCP_rcvbuf_len);  // Copy the entire received frame to the utility_rx frame buffer.
          
		  #if DEBUG_ALL || DEBUG_UTILITY
          Print_Message("\nSOF verified");
          #endif
		  
          U8 lc_hb = utility_rx.frame_buf[rcv_frame_idx++];                     // Extract the high and low bytes of the frame length.
          U8 lc_lb = utility_rx.frame_buf[rcv_frame_idx++];
          utility_rx.frame_len = COMBINE_BYTES(lc_hb, lc_lb);
          
		  #if DEBUG_ALL || DEBUG_UTILITY
          Print_Message("\nFrame length = ");
          Print_Number(utility_rx.frame_len);
          #endif
		  
          if(gb_mbTCP_rcvbuf_len == utility_rx.frame_len)                       // Check if the received frame length matches the expected length.
          {
               #if DEBUG_ALL || DEBUG_UTILITY
               Print_Message("\nFrame length verified");
               #endif
			   
               if(utility_rx.frame_buf[rcv_frame_idx++] == DST_ADDR_RX)         // Check the destination address.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nDST ADDR verified");
                    #endif
					
                    if(utility_rx.frame_buf[rcv_frame_idx++] == SRC_ADDR_RX)    // Check the source address.
                    {
                         #if DEBUG_ALL || DEBUG_UTILITY
                         Print_Message("\nSRC ADDR verified");
                         #endif
						 
                         utility_rx.frame_funid = utility_rx.frame_buf[rcv_frame_idx++];  // Extract the function ID.
                         #if DEBUG_ALL || DEBUG_UTILITY
                         Print_Message("\nFunction id received");
                         #endif
						 
                         if(utility_rx.frame_buf[utility_rx.frame_len - 1] == EOF)   // Check for End of Frame (EOF).
                         {
                              #if DEBUG_ALL || DEBUG_UTILITY
                              Print_Message("\nEOF verified");
                              #endif
							  
                              U8 temp = utility_rx.frame_buf[utility_rx.frame_len - 3];   // Modify the frame for CRC Verification.
                              utility_rx.frame_buf[utility_rx.frame_len - 3] = utility_rx.frame_buf[utility_rx.frame_len - 1];
                              utility_rx.frame_buf[utility_rx.frame_len - 1] = utility_rx.frame_buf[utility_rx.frame_len - 2];
                              utility_rx.frame_buf[utility_rx.frame_len - 2] = temp;
                              
							  #if DEBUG_ALL || DEBUG_UTILITY
                              Print_Message("\nModified frame = ");
                              Send_Frame_On_UART(utility_rx.frame_buf, utility_rx.frame_len);
                              #endif
							  
							  if (utility_rx.frame_len <= UTLTY_FRM_MX_SZ)
							  {
								  if(CRC_Calculate_Check(utility_rx.frame_buf, utility_rx.frame_len - 2, CRC_CHECK))   // Verify CRC.
								  {
									   gb_utility_rcv_f = 1;                        // Set the utility receive flag to indicate successful frame reception.
									   gb_config_timer = 0;                         // Reload configuration timer after each frame is received from utility.
									   
									   #if DEBUG_ALL || DEBUG_UTILITY
									   Print_Message("\nCRC verified");
									   Print_Message("\nUtility receive flag Set = ");
									   Print_Number(gb_utility_rcv_f);
									   #endif
								  }
							  }
                              else
                              {
                                   gb_err_crc_f = 1;                         // Set the CRC error flag.
                              }
                         }
                         else
                         {
                              gb_err_data_f = 1;                             // Set the data error flag.
                         }
                    }
                    else
                    {
                         gb_err_data_f = 1;                                  // Set the data error flag.
                    }
               }
               else
               {
                    gb_err_data_f = 1;                                       // Set the data error flag.
               }
          }
          else
          {
               gb_err_len_f = 1;                                             // Set the length error flag.
          }
     }
	 else
	 {
		 gb_utility_req_f = 0;
		 return;
	 }

     if (gb_utility_rcv_f && (gb_conn_dvc_f == 0))                           // Check for the connect with device data from utility if the utility receive flag is set by switching function id.
     {
          switch (utility_rx.frame_funid)
          {
               case CONNECT_WITH_DEVICE:                                        // Analyze the received string and create response frame including response string for connect with device command.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for connect with device command");
                    #endif
					
					if ((dm_dev_info.dev_sts == DEV_CANCELLED)
					&& (strcmp(gb_dev_ipAddress, gb_def_ipAddress) != 0))
					{
						gb_utility_rcv_f = 0;	// Reset the utility receive flag after processing.
						
						/* Break case immediately, do not connect with the device. Because the 
						device status is canceled & configured device IP address is not default. */
						break;
					}
					else
					__NOP();
					
					
                    char conn_dvc_str[CONNECT_WITH_DEVICE_STR_LEN];             // Char array to store connect with device string.
                    char conn_str[9] = {"Connected"};
                    U16 tx_frame_idx = 0;                                       // Index to track the received and transmitted frame.
                    for(U8 data_idx = 0; data_idx < CONNECT_WITH_DEVICE_STR_LEN; data_idx++)     // Parse the received string for connect with device.
                    {
                         conn_dvc_str[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    if(!strncmp(conn_dvc_str, "Connect_Avon_DOR2200", CONNECT_WITH_DEVICE_STR_LEN))          // Compare the received string with expected string.
                    {
                         gb_conn_dvc_f = 1;
                         utility_tx.frame_buf[tx_frame_idx++] = SOF;            // Start building the transmit frame (SOF).
                         utility_tx.frame_len = CONN_RPLY_LEN;                  // Set the frame length.
                         U16 cb = CONN_RPLY_LEN;
                         U8 lb;
						 U8 hb;
                         SPLIT_BYTES(cb, &hb, &lb);
                         utility_tx.frame_buf[tx_frame_idx++] = hb;
                         utility_tx.frame_buf[tx_frame_idx++] = lb;
                         utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;    // Set destination and source addresses.
                         utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                         utility_tx.frame_buf[tx_frame_idx++] = CONNECT_WITH_DEVICE;      // Set function ID.
                         for(U8 data_idx = 0; data_idx < CONNECTED_STR_LEN; data_idx++)   // Fill the transmit frame with the response string.
                         {
                              utility_tx.frame_buf[tx_frame_idx++] = conn_str[data_idx];
                         }
                         utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;       // Set error byte and EOF.
                         utility_tx.frame_buf[tx_frame_idx++] = EOF;
                         
						 if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
						 {
							 if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))         // Calculate and set CRC and add EOF at the end.
							 {
								  utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
								  utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
								  utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
							 }
						 }
						 else
						 {
							 __NOP();
						 }
						 
                         #if DEBUG_ALL || DEBUG_UTILITY
                         Print_Message("\nFrame sent on tcp server is = ");
                         Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                         #endif
                         gb_utility_rcv_f = 0;                                  // Reset the utility receive flag after processing.
						 
						 gb_config_mode_f = 1;                         // Set configuration mode flag.
						 /* Create the Config mode Task. */
						 xTaskCreate(config_mode, (const signed char *const) "CONFIG_MODE",
						 (mainTCP_TASK_STACK_SIZE), NULL, (mainTCP_TASK_PRIORITY + 1), NULL);
                    }
                    else
                    gb_conn_dvc_f = 0;
                    break;
               }
               default:
               break;
          }
     }
     else if (gb_utility_rcv_f && (gb_conn_dvc_f == 1))                       // Create the response frame if the utility receive flag and connect with device flag is set by switching function id.
     {
          switch (utility_rx.frame_funid)
          {
               case READ_MFG_IFO:                                               // Create response frame for read manufacturing information function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read manufacturing information");
                    #endif
					
                    U8 read_mfg_frame[MFG_INFO_BYTES];                          // Array to store read_mfg_info data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = 0;// RD_MFG_RPLY_LEN;                     // Set the frame length.
                    U16 cb = 0;// RD_MFG_RPLY_LEN;
                    U8 lb;
					U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_MFG_IFO;        // Set function ID.
                    Read_Manufacturing_Info(read_mfg_frame);                    // Fill the transmit frame with data from read_mfg_frame.
                    for(U8 data_idx = 0; data_idx < strlen(read_mfg_frame); data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_mfg_frame[data_idx];
                    }
                    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					// increment the length by 2 bytes for crc.
					tx_frame_idx++;
					tx_frame_idx++;
					utility_tx.frame_len = tx_frame_idx;
					
					
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nLength of the frame ------: ");
					Print_Number(utility_tx.frame_len);
					#endif
					
					// Add length of the frame in the frame at the location 1 and 2
					cb = utility_tx.frame_len;
					SPLIT_BYTES(cb, &hb, &lb);
					utility_tx.frame_buf[1] = hb;
					utility_tx.frame_buf[2] = lb;
					
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
						{
							 utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							 utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							 utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						}
					}
					else
					{
						
					}
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFrame sent on tcp server is = ");
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					 for(int i = 0; i< utility_tx.frame_len;i++ )
					 {
						 Print_ASCII_HEX(utility_tx.frame_buf[i]);
						 Print_Message(",");
					 }
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing.
                    break;
               }

               case READ_COMN_CONFIG:                                           // Create response frame for read communication configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read communication configuration");
                    #endif
					
                    U8 read_comn_data[COMN_CONFIG_BYTES] = {0};                       // Array to store read_comn_settings data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = 0;//RD_COMN_RPLY_LEN;                    // Set the frame length.
                    U16 cb = 0;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_COMN_CONFIG;    // Set function ID.
                    read_comn_settings(read_comn_data);                         // Fill the transmit frame with data from read_comn_data.
					

					
                    for(U8 data_idx = 0; data_idx < strlen(read_comn_data) ; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_comn_data[data_idx];
                    }
					
					// Add error byte and EOF
					utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					// increment the length by 2 bytes for crc.
					tx_frame_idx++;
					tx_frame_idx++;
					utility_tx.frame_len = tx_frame_idx;
					
							
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nLength of the frame ------: ");
					Print_Number(utility_tx.frame_len);
					#endif
					
					// Add length of the frame in the frame at the location 1 and 2
                    cb = utility_tx.frame_len;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[1] = hb;
                    utility_tx.frame_buf[2] = lb;
					
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
					
						if(CRC_Calculate_Check(utility_tx.frame_buf, (utility_tx.frame_len - 2), CRC_CALC) == 1)    // Calculate and set CRC and add EOF at the end.
						{
							 utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							 utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							 utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						}
					}
					else
					{
						__NOP();
					}
					
                    #if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF\n");
					for(int i = 0;i < utility_tx.frame_len;i++)
					{
						Print_ASCII_HEX(utility_tx.frame_buf[i]);
						Print_Message(",");
					}
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_COMN_CONFIG:                                          // Create response frame for write communication configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write communication configuration");
                    #endif
					
					if (dm_dev_info.dev_sts == DEV_CANCELLED)
					{
						gb_utility_rcv_f = 0;
						
						// If an user trying to write to the dev, but the device status is canceled.
						// Do not allow to write, Hence break the case.
						break;
					}
					
                    U8 received_comn_data[COMN_CONFIG_BYTES];                   // Array to store received communication settings data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < COMN_CONFIG_BYTES; data_idx++)   // Parse the received communication settings data.
                    {
                         received_comn_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nReceived rx frame is ");
					for(int i = 0; i < COMN_CONFIG_BYTES ;i++)
					{
						Print_ASCII_HEX(utility_rx.frame_buf[i]);
						Print_Message(",");
					}
					Print_Message("\nupdated frame isOOOOOOOOOOOOOOO");
					for(int i = 0; i < COMN_CONFIG_BYTES ;i++)
					{
						Print_ASCII_HEX(received_comn_data[i]);
						Print_Message(",");
					}
					#endif
					
					
                    write_comn_settings(received_comn_data);                    // Write the communication settings data.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_COMN_RPLY_LEN;                    // Set the frame length.
                    U16 cb = WR_COMN_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_COMN_CONFIG;   // Set function ID.
                    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
						{
							 utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							 utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							 utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						}
					}
					else
					{
						__NOP();
					}
                    
					#if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFrame sent on tcp server is = ");
					for(int i = 0;i < utility_tx.frame_len;i++)
					{
						Print_ASCII_HEX(utility_tx.frame_buf[i]);
						Print_Message(",");
					}
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case CONNECT_WITH_DEVICE:                                        // Analyze the received string and create response frame including response string for connect with device command.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for connect with device command");
                    #endif
					
                    char conn_dvc_str[CONNECT_WITH_DEVICE_STR_LEN];             // Char array to store connect with device string.
                    char conn_str[9] = {"Connected"};
                    U16 tx_frame_idx = 0;                                       // Index to track the received and transmitted frame.
                    for(U8 data_idx = 0; data_idx < CONNECT_WITH_DEVICE_STR_LEN; data_idx++)     // Parse the received string for connect with device.
                    {
                         conn_dvc_str[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    if(!strncmp(conn_dvc_str, "Connect_Avon_DOR2200", CONNECT_WITH_DEVICE_STR_LEN))      // Compare the received string with expected string.
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = SOF;            // Start building the transmit frame (SOF).
                         utility_tx.frame_len = CONN_RPLY_LEN;                  // Set the frame length.
                         U16 cb = CONN_RPLY_LEN;
                         U8 lb;
                         U8 hb;
                         SPLIT_BYTES(cb, &hb, &lb);
                         utility_tx.frame_buf[tx_frame_idx++] = hb;
                         utility_tx.frame_buf[tx_frame_idx++] = lb;
                         utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;    // Set destination and source addresses.
                         utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                         utility_tx.frame_buf[tx_frame_idx++] = CONNECT_WITH_DEVICE;      // Set function ID.
                         for(U8 data_idx = 0; data_idx < CONNECTED_STR_LEN; data_idx++)   // Fill the transmit frame with the response string.
                         {
                              utility_tx.frame_buf[tx_frame_idx++] = conn_str[data_idx];
                         }
                         utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;       // Set error byte and EOF.
                         utility_tx.frame_buf[tx_frame_idx++] = EOF;
                         
						 if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
						 {
							 if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))         // Calculate and set CRC and add EOF at the end.
							 {
								  utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
								  utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
								  utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
							 }
						 }
						 else
						 {
							 __NOP();
						 }
						 
                         #if DEBUG_ALL || DEBUG_UTILITY
                         Print_Message("\nFrame sent on tcp server is = ");
                         Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                         #endif
                         gb_utility_rcv_f = 0;                                  // Reset the utility receive flag after processing.
                    }
                    break;
               }
			    case READ_MAC_ADDRESS:                                           // Create response frame for read mac address function.
			    {
				    #if DEBUG_ALL || DEBUG_UTILITY
				    Print_Message("\nFun id verified for read mac address configuration");
				    #endif
			
				    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
				    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
				    utility_tx.frame_len = RD_MAC_ADD_LEN;                    // Set the frame length.
				    U16 cb = RD_MAC_ADD_LEN;
				    U8 lb;
				    U8 hb;
				    SPLIT_BYTES(cb, &hb, &lb);
				    utility_tx.frame_buf[tx_frame_idx++] = hb;
				    utility_tx.frame_buf[tx_frame_idx++] = lb;
				    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
				    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
				    utility_tx.frame_buf[tx_frame_idx++] = READ_MAC_ADDRESS;    // Set function ID.
		   
					for (U8 data_idx = 0; data_idx < 6; data_idx++)
					{
						utility_tx.frame_buf[tx_frame_idx++] = gs_uc_mac_address[data_idx];
						
					}

					
				    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
				    utility_tx.frame_buf[tx_frame_idx++] = EOF;
				    
				    if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
				    {
					    if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
					    {
						    utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
						    utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
						    utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
					    }
				    }
				    else
				    {
					    __NOP();
				    }
				    
				    #if DEBUG_ALL || DEBUG_UTILITY
				    Print_Message("\nFrame sent on tcp server is = ");
					 for(int i = 0 ; i< utility_tx.frame_len ; i++)
					 {
						 Print_ASCII_HEX(utility_tx.frame_buf[i]);
						 Print_Message(",");
					 }
				    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
				    #endif
				    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
				    break;
			    }
				
				  case CURRENT_DATE_TIME:                                           // Create response frame for read mac address function.
				  {
					  #if DEBUG_ALL || DEBUG_UTILITY
					  Print_Message("\nFun id verified for read current Date and Time");
					  #endif
					  
					      
					  U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
					  utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
					  utility_tx.frame_len = RD_DT_AND_TM_LEN;                                // Set the frame length.
					  U16 cb = RD_DT_AND_TM_LEN;
					  U8 lb;
					  U8 hb;
					  SPLIT_BYTES(cb, &hb, &lb);
					  
					utility_tx.frame_buf[tx_frame_idx++] = hb;
					utility_tx.frame_buf[tx_frame_idx++] = lb;
					utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
					utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
					utility_tx.frame_buf[tx_frame_idx++] = CURRENT_DATE_TIME;    // Set function ID.
					 
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcDateArr[0];//Read_Date_From_DS1339A();
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcDateArr[1];//Read_Month_From_DS1339A();
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcDateArr[2];//Read_Year_From_DS1339A();
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcDateArr[3];//Read_DayValue_From_DS1339A();
															
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcTimeArr[0];//Read_Hour_From_DS1339A();
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcTimeArr[1];//Read_Minutes_From_DS1339A();
					utility_tx.frame_buf[tx_frame_idx++] = gb_rtcTimeArr[2];//Read_Seconds_From_DS1339A();

					  
					  utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
					  utility_tx.frame_buf[tx_frame_idx++] = EOF;
					  
					  if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					  {
						  if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
						  {
							  utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							  utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							  utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						  }
					  }
					  else
					  {
						  __NOP();
					  }
					  
					  #if DEBUG_ALL || DEBUG_UTILITY
 					  Print_Message("\nFrame sent on tcp server is = ");
					  for(int i = 0;i< utility_tx.frame_len;i++)
					  {
						 	  Print_Number(utility_tx.frame_buf[i]);
							  Print_Message(",");
					  }
				
					  Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					  #endif
					  gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
					  break;
				  }
				  
				  
				  
				   case SYNC_DATE_TIME:                                           // Create response frame for read mac address function.
				  {
					  #if DEBUG_ALL || DEBUG_UTILITY
					  Print_Message("\nFun id verified for Sync current Date and Time ");
					  #endif
					   U8 received_dt_amd_tm_data[DATE_TIME_SIZE];                   // Array to store received communication settings data.
					   U16 tx_frame_idx = 0;
					                                          // Index to track the transmitted frame.
					   for(U8 data_idx = 0; data_idx < DATE_TIME_SIZE; data_idx++)   // Parse the received communication settings data.
					   {
						   received_dt_amd_tm_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
					   }
					   
					   Print_Message("\nWritten date and time data");
					   for(int i = 0;i < DATE_TIME_SIZE; i++)
					   {
						   Print_Number(received_dt_amd_tm_data[i]);
						   Print_Message(",");
					   }
					   
					   	// Update RTC registers with received time
					   	Write_Date_To_DS1339A(received_dt_amd_tm_data[0]);  // Date
					   	Write_Month_To_DS1339A(received_dt_amd_tm_data[1]); // Month
					   	Write_Year_To_DS1339A(received_dt_amd_tm_data[2]);  // Year
						Write_DayValue_To_DS1339A(received_dt_amd_tm_data[3]);  // Day
					   	Write_Hours_To_DS1339A(received_dt_amd_tm_data[4]);  // Hour
					   	Write_Minutes_To_DS1339A(received_dt_amd_tm_data[5]); // Minutes
					   	Write_Seconds_To_DS1339A(received_dt_amd_tm_data[6]); // Seconds
					                                   
					  utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
					  utility_tx.frame_len = SYNC_DT_AND_TIME_LEN;                              // Set the frame length.
					  U16 cb = SYNC_DT_AND_TIME_LEN;
					  U8 lb;
					  U8 hb;
					  SPLIT_BYTES(cb, &hb, &lb);
					  
					utility_tx.frame_buf[tx_frame_idx++] = hb;
					utility_tx.frame_buf[tx_frame_idx++] = lb;
					utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
					utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
					utility_tx.frame_buf[tx_frame_idx++] = SYNC_DATE_TIME;      // Set function ID.
				    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
				    utility_tx.frame_buf[tx_frame_idx++] = EOF;
					  
					  if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					  {
						  if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
						  {
							  utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							  utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							  utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						  }
					  }
					  else
					  {
						  __NOP();
					  }
					  
					  #if DEBUG_ALL || DEBUG_UTILITY
						Print_Message("\nFrame sent on tcp server is = ");
    					for(int i = 0;i< utility_tx.frame_len;i++)
    					{
	    					Print_Number(utility_tx.frame_buf[i]);
	    					Print_Message(",");
    					}
					  Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					  #endif
					  gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
					  break;
				  }
				case READ_DEVICE_STATUS:
				{
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nFun id verified for Read Device Status ");
					#endif
					
					U16 tx_frame_idx = 0;
					
					utility_tx.frame_buf[tx_frame_idx++] = SOF;
					utility_tx.frame_len = DEVICE_STATUS_LEN;
					U16 cb = DEVICE_STATUS_LEN;
					U8 lb;
					U8 hb;
					SPLIT_BYTES(cb, &hb, &lb);
				
					utility_tx.frame_buf[tx_frame_idx++] = hb;
					utility_tx.frame_buf[tx_frame_idx++] = lb;
					utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;
					utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
					utility_tx.frame_buf[tx_frame_idx++] = READ_DEVICE_STATUS;
					utility_tx.frame_buf[tx_frame_idx++] = dm_dev_info.dev_sts;
					utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;
					utility_tx.frame_buf[tx_frame_idx++] = EOF;
					
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))
						{
							utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						}
					}
					else
					{
						__NOP();
					}
					
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nFrame sent on tcp server is = ");
					for(int i = 0; i< utility_tx.frame_len; i++)
					{
						Print_Number(utility_tx.frame_buf[i]);
						Print_Message(",");
					}
					Print_Message("\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
					for(int i = 0; i< utility_tx.frame_len; i++)
					{
						Print_ASCII_HEX(utility_tx.frame_buf[i]);
						Print_Message(",");
					}
					Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					#endif

					gb_utility_rcv_f = 0;

					break;
				}


				  case SET_FACTORY_DEF:
				  {
					  #if DEBUG_ALL || DEBUG_UTILITY
					  Print_Message("\nFun id verified for Set Factory Default ");
					  #endif
					  
					  U16 tx_frame_idx = 0;
					  
					  U8 factory_default_value = utility_rx.frame_buf[6];  // Extract factory default value (e.g., 0x00)
					  
					  if (factory_default_value == 0x00)
					  {
						 /* Read boot flag value from the mem before erasing chip */
						 U8 boot_flag[1];
						 U8 lcl_eep_sts = 0;
						 lcl_eep_sts = EEPROM_Byte_Read(BL_APP_STATUS_LOC, boot_flag, 1);
						 
						 EEPROM_Chip_Erase();
						 
						 /* Re write the boot flag value to the mem after erasing the chip */
						 EEPROM_Byte_Write(BL_APP_STATUS_LOC, boot_flag, 1);
						 
						 // Perform actions for setting default IP
						 write_dflt_comn_config(); // Write default communication configuration
						 Write_Default_End_Point();
						 
						 U8 lcl_rd_config_data[COMN_CONFIG_BYTES] = {0};
						 read_comn_settings(lcl_rd_config_data); // Read communication configuration and write to EEPROM and flash
						 CRC_Calculate_Check(lcl_rd_config_data, (COMN_CONFIG_BYTES - 2), CRC_CALC);
						 lcl_rd_config_data[(COMN_CONFIG_BYTES - 2)] = gb_Temp_CRC[0];	// Higer Byte.
						 lcl_rd_config_data[(COMN_CONFIG_BYTES - 1)] = gb_Temp_CRC[1];	// Lower Byte.
						 EEPROM_Page_Write(UT_EEP_START_PAGE, UT_EEP_PAGE_OFFSET, lcl_rd_config_data, COMN_CONFIG_BYTES);  // write to flash
						 
						 gb_fact_reset_f = FLAG_SET;	// Set this flag to reset system (By calling on chip software reset function).
					  }

					  utility_tx.frame_buf[tx_frame_idx++] = SOF;
					  utility_tx.frame_len = FACTORY_DEFAULT_LEN;
					  U16 cb = FACTORY_DEFAULT_LEN;
					  U8 lb;
					  U8 hb;
					  SPLIT_BYTES(cb, &hb, &lb);

					  utility_tx.frame_buf[tx_frame_idx++] = hb;
					  utility_tx.frame_buf[tx_frame_idx++] = lb;
					  utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;
					  utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
					  utility_tx.frame_buf[tx_frame_idx++] = SET_FACTORY_DEF;
					  utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;
					  utility_tx.frame_buf[tx_frame_idx++] = EOF;

					  if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					  {
						  if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))
						  {
							  utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
							  utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
							  utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						  }
					  }
					  else
					  {
						  __NOP();
					  }

					  #if DEBUG_ALL || DEBUG_UTILITY
					  Print_Message("\nFrame sent on TCP server is = ");
					  for(int i = 0; i < utility_tx.frame_len; i++)
					  {
						  Print_Number(utility_tx.frame_buf[i]);
						  Print_Message(",");
					  }
					  Print_Message("\nAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA");
					  for(int i = 0; i< utility_tx.frame_len; i++)
					  {
					  	Print_ASCII_HEX(utility_tx.frame_buf[i]);
					  	Print_Message(",");
					  }
					  Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					  #endif

					  gb_utility_rcv_f = 0;

					  break;
				  }
				case UPDATE_FIRMWARE :
				{
					Print_Message("\r\n firmware update command received  ") ;
						
					U8 tx_frame_idx = 0;                                                  // Index to track the transmitted frame.
					utility_tx.frame_buf[tx_frame_idx++] = SOF;                           // Start building the transmit frame (SOF).
					utility_tx.frame_len = ERR_RPLY_LEN;                                  // Set the frame length.
					U16 cb = ERR_RPLY_LEN;
					U8 lb;
					U8 hb;
					SPLIT_BYTES(cb, &hb, &lb);
					utility_tx.frame_buf[tx_frame_idx++] = hb;
					utility_tx.frame_buf[tx_frame_idx++] = lb;
					utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;                   // Set destination and source addresses.
					utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
					utility_tx.frame_buf[tx_frame_idx++] = 0x00;           // Set function ID.
						
					utility_tx.frame_buf[tx_frame_idx++] = 0x00;
					utility_tx.frame_buf[tx_frame_idx++] = EOF;
						
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
						{
							utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[1];
							utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[0];
							utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
						}
					}
					else
					{
						__NOP();
					}
						
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("Frame sent on tcp server is = ") ;
					Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
					#endif
					gb_utility_rcv_f = 0;                                                 // Reset the utility receive flag after processing the frame.
					gb_err_crc_f = 0;                                                     // Reset the error flags.
					gb_err_len_f = 0;
					gb_err_data_f = 0;
						
					gb_boot_jump_start_timer = 1 ;
				}
				break;
				case EXIT_FROM_CONFIG :
				{
					#if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nFun id verified for Exit from configuration command.");
					#endif
					
					char conn_dvc_str[EXIT_FROM_CONFIG_STR_LEN];             // Char array to store Exit from config string.
					char conn_str[CONFIG_IS_EXIT_LEN] = {"Config_is_Exit"};
					U16 tx_frame_idx = 0;                                       // Index to track the received and transmitted frame.
					for(U8 data_idx = 0; data_idx < EXIT_FROM_CONFIG_STR_LEN; data_idx++)     // Parse the received string for Exit from config.
					{
						conn_dvc_str[data_idx] = utility_rx.frame_buf[data_idx + 6];
					}
					
					if(!strncmp(conn_dvc_str, "Exit_From_Config", EXIT_FROM_CONFIG_STR_LEN))          // Compare the received string with expected string.
					{
						utility_tx.frame_buf[tx_frame_idx++] = SOF;            // Start building the transmit frame (SOF).
						utility_tx.frame_len = EXIT_REPLY_LEN;                  // Set the frame length.
						U16 cb = EXIT_REPLY_LEN;
						U8 lb;
						U8 hb;
						SPLIT_BYTES(cb, &hb, &lb);
						utility_tx.frame_buf[tx_frame_idx++] = hb;
						utility_tx.frame_buf[tx_frame_idx++] = lb;
						utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;    // Set destination and source addresses.
						utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
						utility_tx.frame_buf[tx_frame_idx++] = EXIT_FROM_CONFIG;      // Set function ID.
						for(U8 data_idx = 0; data_idx < CONFIG_IS_EXIT_LEN; data_idx++)   // Fill the transmit frame with the response string.
						{
							utility_tx.frame_buf[tx_frame_idx++] = conn_str[data_idx];
						}
						utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;       // Set error byte and EOF.
						utility_tx.frame_buf[tx_frame_idx++] = EOF;
							
						if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
						{
							if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))         // Calculate and set CRC and add EOF at the end.
							{
								utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
								utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
								utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
							}
						}
						else
						{
							__NOP();
						}
							
						#if DEBUG_ALL || DEBUG_UTILITY
						Print_Message("\nFrame sent on tcp server is = ");
						Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
						#endif
						
						gb_utility_rcv_f = 0;                                  // Reset the utility receive flag after processing.
						gb_exit_cnf_f = 1;	// Set a flag to exit from config utility.
					}
					else
					__NOP();
				}
				break;

               default:
               {
                    gb_utility_rcv_f = 0;
                    gb_err_data_f = 1;
                    break;
               }
          }
     }

     if ((gb_utility_rcv_f == 0) && (gb_err_crc_f || gb_err_len_f || gb_err_data_f))
     {
          U8 tx_frame_idx = 0;                                                  // Index to track the transmitted frame.
          utility_tx.frame_buf[tx_frame_idx++] = SOF;                           // Start building the transmit frame (SOF).
          utility_tx.frame_len = ERR_RPLY_LEN;                                  // Set the frame length.
          U16 cb = ERR_RPLY_LEN;
          U8 lb;
          U8 hb;
          SPLIT_BYTES(cb, &hb, &lb);
          utility_tx.frame_buf[tx_frame_idx++] = hb;
          utility_tx.frame_buf[tx_frame_idx++] = lb;
          utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;                   // Set destination and source addresses.
          utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
          utility_tx.frame_buf[tx_frame_idx++] = gb_mbTcp_rcv_buf[5];           // Set function ID.
          if(gb_err_crc_f)                                                      // Set error byte and EOF.
          utility_tx.frame_buf[tx_frame_idx++] = CRC_ERROR;
          else if(gb_err_len_f)
          utility_tx.frame_buf[tx_frame_idx++] = LEN_ERROR;
          else if(gb_err_data_f)
          utility_tx.frame_buf[tx_frame_idx++] = DATA_ERROR;
          utility_tx.frame_buf[tx_frame_idx++] = EOF;
          
		  if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
		  {
			  if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and add EOF at the end.
			  {
				   utility_tx.frame_buf[utility_tx.frame_len - 3] = gb_Temp_CRC[0];
				   utility_tx.frame_buf[utility_tx.frame_len - 2] = gb_Temp_CRC[1];
				   utility_tx.frame_buf[utility_tx.frame_len - 1] = EOF;
			  }
		  }
		  else
		  {
			  __NOP();
		  }
		  
          #if DEBUG_ALL || DEBUG_UTILITY
          Print_Message("\nError frame sent on tcp server is = ");
          Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
          #endif
		  
          gb_utility_rcv_f = 0;                                                 // Reset the utility receive flag after processing the frame.
          gb_err_crc_f = 0;                                                     // Reset the error flags.
          gb_err_len_f = 0;
          gb_err_data_f = 0;
     }
}

/*****************************************************************************
* Function name: void SPLIT_BYTES(U16 cb, U8 *hb, U8 *lb).
* Returns		: nothing.
* Arguments    : None.
* Created by	: Harshit Agnihotri.
* Date created	: 06/04/2024.
*
* Description	: Function to split U16 into two U8 bytes.
*              :
* Notes		: NA.
* Global Variables Affected : NA.
*****************************************************************************/
void SPLIT_BYTES(U16 cb, U8 *hb, U8 *lb)
{
     *hb = (U8)((cb >> 8) & 0xFF);      // Extract high byte
     *lb = (U8)(cb & 0xFF);             // Extract low byte
}



/*****************************************************************************
* Function name	: void Update_Manufacturing_Info(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 05/07/2024.
*
* Description	: Function written to update current IP address, Number of active
*				  groups, number of active inputs, outputs and configured
*				  number of doors.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Update_Manufacturing_Info(void)
{
	U8 dotArr[1] = ".";
	
	/* Fill Present IP Address. */
	memcpy(&MfgInfo.ip_addr[0], IpAddr.ip_addr_1, 3);
	MfgInfo.ip_addr[3] = dotArr[0];
	memcpy(&MfgInfo.ip_addr[4], IpAddr.ip_addr_2, 3);
	MfgInfo.ip_addr[7] = dotArr[0];
	memcpy(&MfgInfo.ip_addr[8], IpAddr.ip_addr_3, 3);
	MfgInfo.ip_addr[11] = dotArr[0];
	memcpy(&MfgInfo.ip_addr[12], IpAddr.ip_addr_4, 3);

}
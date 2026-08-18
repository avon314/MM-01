/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: app_utility.c
* Created By	: Harshit Agnihotri.
* Created Date	: 28/12/2023.
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
#include "input_config.h"
#include "output_config.h"
#include "door_config.h"
#include "privacy_config.h"
#include "comn_config.h"
#include "config_mode.h"
#include "interlock.h"

U8 gb_utility_rcv_f = false;            // Global flag indicating the reception of a utility frame.
volatile U8 gb_op_config_cplt = false;  // Flag indicating completion of output configuration.
volatile U8 gb_conn_dvc_f = false;      // Flag to indicate connect with device successful.
volatile U8 gb_err_crc_f = false;       // Flag to indicate error in CRC of received frame.
volatile U8 gb_err_len_f = false;       // Flag to indicate error in length of received frame.
volatile U8 gb_err_data_f = false;      // Flag to indicate error in data of received frame.

// Declaration of global instances of the Utility structure for receiving and transmitting frames.
Utility utility_rx;
Utility utility_tx;
MANFACT_INFO updateMnfactInfo;

/*****************************************************************************
* Function name: void validate_utility_frame(void).
* Returns		: nothing.
* Arguments    : None.
* Created by	: Harshit Agnihotri.
* Date created	: 28/12/2023.
*
* Description	: Implementation of the utility frame validation function and create response frame to be transmitted on tcp ip.
*              :
* Notes		: NA.
* Global Variables Affected : utility_rx, utility_tx and gb_utility_rcv_f.
*****************************************************************************/
void validate_utility_frame(void)
{
     U8 rcv_frame_idx = 0;                                                      // Index to traverse received frame buffer.
     if(gb_mbTcp_rcv_buf[rcv_frame_idx++] == SOF)                               // Check for Start of Frame (SOF).
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
                                   gb_err_crc_f = true;                         // Set the CRC error flag.
                              }
                         }
                         else
                         {
                              gb_err_data_f = true;                             // Set the data error flag.
                         }
                    }
                    else
                    {
                         gb_err_data_f = true;                                  // Set the data error flag.
                    }
               }
               else
               {
                    gb_err_data_f = true;                                       // Set the data error flag.
               }
          }
          else
          {
               gb_err_len_f = true;                                             // Set the length error flag.
          }
     }





     if(gb_utility_rcv_f && (gb_conn_dvc_f == false))                           // Check for the connect with device data from utility if the utility receive flag is set by switching function id.
     {
          switch(utility_rx.frame_funid)
          {
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
                    if(!strncmp(conn_dvc_str, "Connect_Avon_DOR2200", CONNECT_WITH_DEVICE_STR_LEN))          // Compare the received string with expected string.
                    {
                         gb_conn_dvc_f = true;
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
                    else
                    gb_conn_dvc_f = false;
                    break;
               }

               default:
               break;
          }
     }
     else if(gb_utility_rcv_f && (gb_conn_dvc_f == true))                       // Create the response frame if the utility receive flag and connect with device flag is set by switching function id.
     {
          switch(utility_rx.frame_funid)
          {
               case READ_MFG_IFO:                                               // Create response frame for read manufacturing information function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read manufacturing information");
                    #endif
                    U8 read_mfg_frame[MFG_INFO_BYTES];                          // Array to store read_mfg_info data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_MFG_RPLY_LEN;                     // Set the frame length.
                    U16 cb = RD_MFG_RPLY_LEN;
                    U8 lb;
					U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_MFG_IFO;        // Set function ID.
                    Read_Manufacturing_Info(read_mfg_frame);                              // Fill the transmit frame with data from read_mfg_frame.
                    for(U8 data_idx = 0; data_idx < MFG_INFO_BYTES; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_mfg_frame[data_idx];
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
						
					}
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFrame sent on tcp server is = ");
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing.
                    break;
               }

               case READ_GRP_CONFIG:                                            // Create response frame for read group configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read group configuration");
                    #endif
                    U8 read_grp_data[MAX_GRP_BYTES];
                    U16 tx_frame_idx = 0;
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_GRP_RPLY_LEN;                     // Set the frame length.
                    U16 cb = RD_GRP_RPLY_LEN;
                    U8 lb;
					U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_GRP_CONFIG;     // Set function ID.
                    read_group_configuration(read_grp_data, utility_rx.frame_buf[6]);     // Fill the transmit frame with data from read_group_configuration.
                    for(U16 data_idx = 0; data_idx < MAX_GRP_BYTES; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_grp_data[data_idx];
                    }
                    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and EOF at the same time.
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
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_GRP_CONFIG:                                           // Create response frame for write group configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write group configuration");
                    #endif
                    U8 received_grp_data[MAX_GRP_BYTES];                        // Array to store received group configuration data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U16 data_idx = 0; data_idx < MAX_GRP_BYTES; data_idx++) // Parse the received group configuration data.
                    {
                         received_grp_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    write_group_configuration(received_grp_data, received_grp_data[0]);    // Write the group configuration data.

                    U8 lclGroupNum = (received_grp_data[0] - 1);
					Find_iLockSequence_Of_Group(lclGroupNum);

                         #if DEBUG_ALL || DEBUG_UTILITY
					Print_Message("\nGroup number is : ");
					Print_Number(received_grp_data[0]);
					Print_Message("\nNumber of devices in group : ");
					Print_Number(grpData[lclGroupNum].numOfDevs);

					Print_Message("\nSlave address and iLock sequence are : ");
					for (U8 idx = 0; idx < grpData[lclGroupNum].numOfDevs; idx++)
					{
						Print_Message("\n");
						Print_Number(grpData[lclGroupNum].devices[idx]);
						Print_Message(" with ");

						for (U8 jdx = 0; jdx < grpData[lclGroupNum].iLockInfo[idx].numOfiLckDevs; jdx++)
						{
							Print_Number(grpData[lclGroupNum].iLockInfo[idx].ilockSeq[jdx]);
							Print_Message(",");
						}
						Print_Message(" itd is : ");
						Print_Number(grpData[lclGroupNum].iLockInfo[idx].defItdValue);
					}
                         #endif

                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_GRP_RPLY_LEN;                     // Set the frame length.
                    U16 cb = WR_GRP_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_GRP_CONFIG;    // Set function ID.
                    utility_tx.frame_buf[tx_frame_idx++] = received_grp_data[0];     // Set group no.
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case READ_IP_CONFIG:                                             // Create response frame for read input configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read input configuration");
                    #endif
                    U8 read_input_data[IP_CONFIG_BYTES];                        // Array to store read_ip_config data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_IP_RPLY_LEN;                      // Set the frame length.
                    U16 cb = RD_IP_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_IP_CONFIG;      // Set function ID.
                    read_ip_config(read_input_data);                            // Fill the transmit frame with data from read_input_data.
                    for(U8 data_idx = 0; data_idx < IP_CONFIG_BYTES; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_input_data[data_idx];
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_IP_CONFIG:                                            // Create response frame for write input configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write input configuration");
                    #endif
                    U8 received_input_data[IP_CONFIG_BYTES];                    // Array to store received input configuration data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < IP_CONFIG_BYTES; data_idx++)     // Parse the received input configuration data.
                    {
                         received_input_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    write_ip_config(received_input_data);                       // Write the input configuration data.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_IP_RPLY_LEN;                      // Set the frame length.
                    U16 cb = WR_IP_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_IP_CONFIG;     // Set function ID.
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case READ_OP_CONFIG:                                             // Create response frame for read output configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read output configuration");
                    #endif
                    U8 read_output_data[OP_CONFIG_BYTES];                       // Array to store read_op_config data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_OP_RPLY_LEN;                      // Set the frame length.
                    U16 cb = RD_OP_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_OP_CONFIG;      // Set function ID.
                    read_op_config(read_output_data);                           // Fill the transmit frame with data from read_output_data.
                    for(U8 data_idx = 0; data_idx < OP_CONFIG_BYTES; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_output_data[data_idx];
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_OP_CONFIG:                                            // Create response frame for write output configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write output configuration");
                    #endif
                    U8 received_output_data[OP_CONFIG_BYTES];                   // Array to store received output configuration data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < OP_CONFIG_BYTES; data_idx++)     // Parse the received output configuration data.
                    {
                         received_output_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    write_op_config(received_output_data);                      // Write the output configuration data.
                    gb_op_config_cplt = true;
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_OP_RPLY_LEN;                      // Set the frame length.
                    U16 cb = WR_OP_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_OP_CONFIG;     // Set function ID.
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case READ_DOOR_CONFIG:                                           // Create response frame for read door configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read door configuration");
                    #endif
                    U8 read_door_data[MAX_DOORS];
                    U16 tx_frame_idx = 0;
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_DOOR_RPLY_LEN;                    // Set the frame length.
                    U16 cb = RD_DOOR_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_DOOR_CONFIG;    // Set function ID.
                    read_door_config(read_door_data);                           // Fill the transmit frame with data from read_door_config.
                    for(U8 data_idx = 0; data_idx < MAX_DOORS; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_door_data[data_idx];
                    }
                    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and EOF at the same time.
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
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_DOOR_CONFIG:                                          // Create response frame for write door configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write door configuration");
                    #endif
                    U8 received_door_data[MAX_DOORS];                           // Array to store received door configuration data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < MAX_DOORS; data_idx++)      // Parse the received door configuration data.
                    {
                         received_door_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    write_door_config(received_door_data);                      // Write the door configuration data.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_DOOR_RPLY_LEN;                    // Set the frame length.
                    U16 cb = WR_DOOR_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_DOOR_CONFIG;   // Set function ID.
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case READ_PRIVACY_CONFIG:                                        // Create response frame for read privacy configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read privacy configuration");
                    #endif
                    U8 read_pvc_grp_data[MAX_PVC_GRPS * MAX_DOORS];
                    U16 tx_frame_idx = 0;
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_PVC_RPLY_LEN;                     // Set the frame length.
                    U16 cb = RD_PVC_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_PRIVACY_CONFIG; // Set function ID.
                    read_pvc_grp_config(read_pvc_grp_data);                     // Fill the transmit frame with data from read_pvc_grp_config.
                    for(U8 data_idx = 0; data_idx < (MAX_DOORS * MAX_PVC_GRPS); data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_pvc_grp_data[data_idx];
                    }
                    utility_tx.frame_buf[tx_frame_idx++] = NO_ERROR;            // Set error byte and EOF.
                    utility_tx.frame_buf[tx_frame_idx++] = EOF;
                    
					if (utility_tx.frame_len <= UTLTY_FRM_MX_SZ)
					{
						if(CRC_Calculate_Check(utility_tx.frame_buf, utility_tx.frame_len - 2, CRC_CALC))    // Calculate and set CRC and EOF at the same time.
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
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case WRITE_PRIVACY_CONFIG:                                       // Create response frame for write privacy configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for write privacy configuration");
                    #endif
                    U8 received_pvc_grp_data[MAX_PVC_GRPS * MAX_DOORS];         // Array to store received privacy configuration data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < (MAX_PVC_GRPS * MAX_DOORS); data_idx++)      // Parse the received privacy configuration data.
                    {
                         received_pvc_grp_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
                    write_pvc_grp_config(received_pvc_grp_data);                // Write the privacy configuration data.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = WR_PVC_RPLY_LEN;                     // Set the frame length.
                    U16 cb = WR_PVC_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = WRITE_PRIVACY_CONFIG;// Set function ID.
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
                    Send_Frame_On_UART(utility_tx.frame_buf, utility_tx.frame_len);
                    #endif
                    gb_utility_rcv_f = 0;                                       // Reset the utility receive flag after processing the frame.
                    break;
               }

               case READ_COMN_CONFIG:                                           // Create response frame for read communication configuration function.
               {
                    #if DEBUG_ALL || DEBUG_UTILITY
                    Print_Message("\nFun id verified for read communication configuration");
                    #endif
                    U8 read_comn_data[COMN_CONFIG_BYTES];                       // Array to store read_comn_settings data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    utility_tx.frame_buf[tx_frame_idx++] = SOF;                 // Start building the transmit frame (SOF).
                    utility_tx.frame_len = RD_COMN_RPLY_LEN;                    // Set the frame length.
                    U16 cb = RD_COMN_RPLY_LEN;
                    U8 lb;
                    U8 hb;
                    SPLIT_BYTES(cb, &hb, &lb);
                    utility_tx.frame_buf[tx_frame_idx++] = hb;
                    utility_tx.frame_buf[tx_frame_idx++] = lb;
                    utility_tx.frame_buf[tx_frame_idx++] = DST_ADDR_TX;         // Set destination and source addresses.
                    utility_tx.frame_buf[tx_frame_idx++] = SRC_ADDR_TX;
                    utility_tx.frame_buf[tx_frame_idx++] = READ_COMN_CONFIG;    // Set function ID.
                    read_comn_settings(read_comn_data);                         // Fill the transmit frame with data from read_comn_data.
                    for(U8 data_idx = 0; data_idx < COMN_CONFIG_BYTES; data_idx++)
                    {
                         utility_tx.frame_buf[tx_frame_idx++] = read_comn_data[data_idx];
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
                    U8 received_comn_data[COMN_CONFIG_BYTES];                   // Array to store received communication settings data.
                    U16 tx_frame_idx = 0;                                       // Index to track the transmitted frame.
                    for(U8 data_idx = 0; data_idx < COMN_CONFIG_BYTES; data_idx++)   // Parse the received communication settings data.
                    {
                         received_comn_data[data_idx] = utility_rx.frame_buf[data_idx + 6];
                    }
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

               default:
               {
                    gb_utility_rcv_f = 0;
                    gb_err_data_f = true;
                    break;
               }
          }
     }





     if((gb_utility_rcv_f == false) && (gb_err_crc_f || gb_err_len_f || gb_err_data_f))
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
* Description	: Function written to Find number of active groups.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Find_Active_Group_Number(void)
{
	updateMnfactInfo.numOfActiveGroups = 0;	/* Init a variable */
	
	/* Iterate through all the groups */
	for (U8 gpIdx = 0; gpIdx < MAX_GROUPS; gpIdx++)
	{
		if (grpData[gpIdx].numOfDevs > 0)
		{
			/*	If a single device is present in the group then update
				number of active groups.
			*/
			updateMnfactInfo.numOfActiveGroups ++;
		}
	}
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
	
	/***********************************************/
	/* Call a below function, to find Number of Controllers configured. */
	Find_Polling_Devices();
	MfgInfo.door_ctrllrs = polling_dev_len;	/* Fill number of controllers to actual variable */
	/***********************************************/
	
	/***********************************************/
	/* Below two functions are called to find number of active groups. */
	for (U8 grpIdx = 0; grpIdx < MAX_GROUPS; grpIdx ++)
	Find_iLockSequence_Of_Group(grpIdx);
	Find_Active_Group_Number();
	/* Update actual variable. */
	MfgInfo.grps = updateMnfactInfo.numOfActiveGroups;
	/***********************************************/
	
	/* Number of Active Inputs */
	MfgInfo.ips_en = updateMnfactInfo.numOfActiveIps;
	/* Number of Active Outputs */
	MfgInfo.ops_en = updateMnfactInfo.numOfActiveOps;
}
 /*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: tcp_server.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/11/2023.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/11/2023.
* Changes		: NA.
*****************************************************************************/
#include <string.h>
//#include "task_defs.h"
#include "stdio_serial.h"
#include "ethernet.h"
#include "sysclk.h"
#include "lwip/opt.h"
#include "lwip/tcpip.h"
#include "tc.h"

#include "lwip/stats.h"
#include "user_uart.h"
#include "user_timer.h"
#include "rs485_uart.h"
#include "rs485_driver.h"
#include "tcp_server.h"
#include "user_rtos.h"
#include "app_utility.h"
#include "config_mode.h"
#include "json_extraction.h"
#include "user_rtc.h"
#include "lwip/init.h"
#include "lwip/dns.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/ip_addr.h"
#include "netif/etharp.h"
#include "dns.h"
#include <stdio.h>
#include <string.h>

#include "mpu_wrappers.h"
#include "definitions.h"
#include "comn_config.h"
#include "mm_app.h"
#include "modbus_app.h"

#define LOCAL_SERVER	(0)
#define REMOTE_SERVER	(1)

char server_buffer[SERVER_DATA_SIZE];
int buffer_offset = 0;
U8 client_server_running_f = 0;
//U8 gb_send_dr_sts_at_conn = 0;
//U8 gb_send_ip_sts_at_conn = 0;
U8 gb_send_tl_at_conn = 0;
//static struct netif netif_data;
extern U8 gb_exit_cnf_f;

/***** Function Prototypes *****/
//static void Receive_Data_From_Client(void *pvParameters);
void Send_Data_Client_To_Server(void *pvParameters);

/***** Global Function Prototypes *****/
extern void Validate_Modbus_Frame(void);
extern void config_mode(void *pvParameters);
extern void Accept_Connections_From_Client(void *pvParameters);

/***** Variables *****/
U8 gb_mbTcp_rcv_buf[RCV_NUM_OF_CHR] = {0};
U16 gb_mbTCP_rcvbuf_len = 0;
U8 gb_mbTCP_rcvd_f = 0;
	
U8 gb_mbTcp_tx_buf[TX_NUM_OF_CHR] = {0};
U8 gb_mbTCP_txbuf_len = 0;
U8 gb_mbTCP_tx_f = 0;

U8 gb_conn_closed = 0;

#if LWIP_STATS
/** Used to compute LwIP bandwidth */
extern uint32_t lwip_tx_count;
extern uint32_t lwip_rx_count;
extern uint32_t lwip_tx_rate;
extern uint32_t lwip_rx_rate;
#endif


#if LWIP_STATS
// TC3 means TC1 channel 0.
void TC3_Handler(void)
{
	if ((tc_get_status(TC1, 0) & TC_SR_CPCS) == TC_SR_CPCS) {
		lwip_tx_rate = lwip_tx_count;
		lwip_rx_rate = lwip_rx_count;
		lwip_tx_count = 0;
		lwip_rx_count = 0;
	}
}

/**
 * \brief Enable TC1 channel 0 to trigger each second to compute GMAC stats.
 */
static void configure_timer_for_bandwidth_stats(void)
{
	pmc_enable_periph_clk(ID_TC3);		// Warning TC number is the channel not TC number.
										// Hence TC3 means TC1 channel 0.
	tc_init(TC1, 0,						// Init timer counter 1 channel 0.
			TC_CMR_WAVE |				// Waveform Mode is enabled.
			TC_CMR_CPCTRG |				// UP mode with automatic trigger on RC Compare
			TC_CMR_TCCLKS_TIMER_CLOCK5	// Use slow clock to avoid overflow.
	);

	tc_write_rc(TC1, 0, 32768);			// Load the highest possible value into TC.

	/* Configure TC interrupts for TC TC_CHANNEL_CAPTURE only */
	NVIC_SetPriority(TC3_IRQn, 0);		// TC3 means TC1 channel 0.
	NVIC_EnableIRQ(TC3_IRQn);			// TC3 means TC1 channel 0.
	tc_enable_interrupt(TC1, 0, TC_IER_CPCS);
	tc_start(TC1, 0);					// Start Timer counter 0 channel 0.
}
#endif

/*****************************************************************************
* Function name	: void Create_TCP_Server_Task(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/11/2023.
*
* Description	: Create a task to receive the tcp data.
*				  stack size and priority is assigned.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Create_TCP_Server_Task(void)
{
	/* Create the task for sending data to a server. */
	xTaskCreate(Send_Data_Client_To_Server, (const signed char *const) "Send_Data_Client_To_Server",
	(1536), NULL, mainTCP_TASK_PRIORITY, NULL);	// 2048 (8 KB), 2560 is 10 KB, 3584 (14KB)
		
	/* Create the TCP-IP Server Task. */
	xTaskCreate(Accept_Connections_From_Client, (const signed char *const) "Accept_Connections_From_Client",
	(/*mainTCP_TASK_STACK_SIZE*/768/*1024*/), NULL, (mainTCP_TASK_PRIORITY + 1), NULL);	
}

/*****************************************************************************
* Function name	: void Accept_Connections_From_Client(void *pvParameters)
* Returns		: nothing.
* Arguments    	: void *pvParameters (Holds the pointer)
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/11/2023.
*
* Description	: Configure the TCP-IP server. using Netconn API.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Accept_Connections_From_Client(void *pvParameters)
{
	//Print_Message("\nHit Device as Server rtosTask.");
	
	struct netconn *conn;
	struct netconn *newconn;
	struct netbuf *inbuf;
	char *buf;
	u16_t buflen;
	err_t err;

	UNUSED(pvParameters);

	#if LWIP_STATS
	configure_timer_for_bandwidth_stats();
	#endif

	/* Create a new TCP connection handle (listening socket) */
	conn = netconn_new(NETCONN_TCP);
	if (conn == NULL)
	{
		Print_Message("\nhttp_task: invalid conn");
		vTaskDelete(NULL);
	}

	/* Bind to port 502 (Modbus TCP / example) */
	err = netconn_bind(conn, NULL, 502);
	if (err != ERR_OK)
	{
		Print_Message("\nBind failed.");
		netconn_delete(conn);
		vTaskDelete(NULL);
	}

	/* Put the connection into LISTEN state */
	err = netconn_listen(conn);
	if (err != ERR_OK)
	{
		Print_Message("\nListen failed.");
		netconn_delete(conn);
		vTaskDelete(NULL);
	}

	for (;;)
	{
		if (client_server_running_f == FLAG_RST)
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
			/* Accept a new client connection (blocking) */
			err = netconn_accept(conn, &newconn);
			if (err == ERR_OK)
			{
				#if DEBUG_ALL || DEBUG_TCP_IP
				Print_Message("\nConnection Established.");
				#endif
				
				/* Receive data from this client */
				while ((err = netconn_recv(newconn, &inbuf)) == ERR_OK)
				{
					wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
					
					if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
					{
						Print_Message("\nSemaphore taken.");

						netbuf_data(inbuf, (void **)&buf, &buflen);

						memcpy(gb_mbTcp_rcv_buf, buf, buflen);
						gb_mbTCP_rcvbuf_len = buflen;
						
						U8 frame_type = 0;
						frame_type = Inspect_Received_Frame(gb_mbTcp_rcv_buf, gb_mbTCP_rcvbuf_len);

						if (frame_type == 0)
						{
							// An utility frame received.
							Print_Message("\nAn utility frame received.");
							validate_utility_frame();
							netconn_write(newconn, utility_tx.frame_buf, utility_tx.frame_len, NETCONN_COPY);

							if (gb_fact_reset_f == FLAG_SET)
							{
								gb_fact_reset_f = FLAG_RST;
								RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
							}
							else if (gb_exit_cnf_f == FLAG_SET)
							{
								gb_exit_cnf_f = FLAG_RST;

								// Call this function to exit from the configuration mode after receiving command "Exif_From_Config".
								exit_config_mode();
							}
							else
							__NOP();
						}
						else if ((frame_type == 1) && (gb_config_mode_f == 0))
						{
							// A modbus frame received.
							Print_Message("\nA modbus frame received.");
							gb_mbTCP_rcvd_f = 1;
							Validate_Modbus_Frame();
							netconn_write(newconn, gb_mbTcp_tx_buf, gb_mbTCP_txbuf_len, NETCONN_COPY);
						}
						else
						__NOP();

						netbuf_delete(inbuf);

						xSemaphoreGive(xSemaphore);
						Print_Message("\nSemaphore released.");
					}
					
					vTaskDelay(1);
				}

				/* Client disconnected or error */
				#if DEBUG_ALL || DEBUG_TCP_IP
				Print_Message("\nConnection Closed.");
				#endif
				
				netconn_close(newconn);
				netconn_delete(newconn);
			}
			else
			{
				#if DEBUG_ALL || DEBUG_TCP_IP
				Print_Message("\nnetconn_accept error: ");
				Print_Number(err);
				#endif
				__NOP();
			}
		}
		
		vTaskDelay(5);
	}

	/* Should never reach here */
	netconn_close(conn);
	netconn_delete(conn);
	vTaskDelete(NULL);
}


/*****************************************************************************
* Function name	: static void Receive_Data_From_Client(void *pvParameters)
* Returns		: nothing.
* Arguments    	: void *pvParameters (Holds the pointer)
* Created by	: Ranjitkumar Ainapure.
* Date created	: 08/11/2023.
*
* Description	: Once established the tcp server connection, then receive
*				  the data over tcp here in this function.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
// static void Receive_Data_From_Client(void *pvParameters)
// {
// 	Print_Message("\nInside the 'Receive_Data_From_Client' function.");
// 
// 	struct netconn *conn = (struct netconn *)pvParameters;
// 	struct netbuf *inbuf;
// 	char *buf;
// 	u16_t buflen;
// 	err_t err;
// 
// 	while (1)
// 	{
// 		err = netconn_recv(conn, &inbuf);
// 		if (err == ERR_OK)
// 		{
// 			if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
// 			{
// 				Print_Message("\nSemaphore taken.");
// 
// 				netbuf_data(inbuf, (void **)&buf, &buflen);
// 
// 				memcpy(gb_mbTcp_rcv_buf, buf, buflen);
// 				gb_mbTCP_rcvbuf_len = buflen;
// 				gb_mbTCP_rcvd_f = 1;
// 
// 				if (gb_config_mode_f)
// 				{
// 					validate_utility_frame();
// 					netconn_write(conn, utility_tx.frame_buf, utility_tx.frame_len, NETCONN_COPY);
// 					
// 					if (gb_fact_reset_f == FLAG_SET)
// 					{
// 						gb_fact_reset_f = FLAG_RST;
// 						
// 						// Trigger a software reset
// 						RSTC->RSTC_CR = RSTC_CR_KEY_PASSWD | RSTC_CR_PROCRST;
// 					}
// 					else
// 					__NOP();
// 				}
// 				else
// 				{
// 					//Validate_Modbus_Frame();
// 					netconn_write(conn, gb_mbTcp_tx_buf, gb_mbTCP_txbuf_len, NETCONN_COPY);
// 				}
// 
// 				netbuf_delete(inbuf);
// 
// 				xSemaphoreGive(xSemaphore);
// 				Print_Message("\nSemaphore released.");
// 			}
// 		}
// 		else if (err == ERR_CLSD || err == ERR_CONN)
// 		{
// 			gb_conn_closed = 1;
// 
// 			#if DEBUG_ALL || DEBUG_TCP_IP
// 			Print_Message("\nConnection Closed.");
// 			#endif
// 			
// 			break;  // Break the loop to clean up
// 		}
// 		else
// 		{
// 			// Optional: handle other errors or retry
// 			break;
// 		}
// 	}
// 
// 	/* Cleanup after connection is closed or error occurs */
// 	netconn_close(conn);
// 	netconn_delete(conn);
// 	vTaskDelete(NULL);
// }


// Function to convert hexadecimal string to an integer
int hex_to_int(const char *hex_str) {
	int value = 0;
	sscanf(hex_str, "%x", &value);  // Convert hex string to integer
	return value;
}

void Heap_Memory_Is(void)
{
	// Get total heap size from FreeRTOSConfig.h
	Print_Message("\nAllocated heap size in Bytes : ");
	Print_Number(configTOTAL_HEAP_SIZE);
	// Calculate used heap size
	Print_Message("\nUsed heap size in Bytes : ");
	Print_Number((configTOTAL_HEAP_SIZE - xPortGetFreeHeapSize()));
	// Get current free heap size
	Print_Message("\nAvailable free heap size in Bytes : ");
	Print_Number(xPortGetFreeHeapSize());
	
	/**************************************************/
	Print_Message("\nRemaining stack size in words : ");
	Print_Number(uxTaskGetStackHighWaterMark(NULL));  // This shows unused stack words
}

void Send_Data_Client_To_Server(void *pvParameters)
{
	//Print_Message("\nHit Device as Client rtosTask.");
	
	/* Just to avoid compiler warnings. */
	UNUSED(pvParameters);
	
	struct netconn *conn;
	struct netbuf *inbuf;
	ip_addr_t server_ip;
	err_t err;
	//char *buf;
	//u16_t buflen;
	char *body;
			
	#if (REMOTE_SERVER)
	// Convert server IP address
	// 202.21.32.129:8010
//  	IP4_ADDR(&server_ip, 202, 21, 32, 129); // Public IP address
//  	U16 dvPort = 8010;
	
	// 26/10/2024
// 	IP4_ADDR(&server_ip, 103, 133, 231, 125); // Public IP address
// 	U16 dvPort = 8005;
	
	// 18/02/2025
// 	IP4_ADDR(&server_ip, 103, 133, 231, 125); // Public IP address
// 	U16 dvPort = 8010;

	// 21/04/2025
// 	IP4_ADDR(&server_ip, 110, 227, 214, 55); // Public IP address
// 	U16 dvPort = 8005;
	//U16 dvPort = 8010;
	
	// 26/05/2025	(Config from Utility)
	int ip1, ip2, ip3, ip4, port;
	U16 dvPort = 0;
	if (sscanf(ServerCfg.server_endpoint, "%d.%d.%d.%d:%d", &ip1, &ip2, &ip3, &ip4, &port) == 5)
	{
		#if 1
		Print_Message("\nServer IP Address is : ");
		Print_Number(ip1);Print_Message(".");
		Print_Number(ip2);Print_Message(".");
		Print_Number(ip3);Print_Message(".");
		Print_Number(ip4);Print_Message(":");
		Print_Number(port);
		#endif
		
		IP4_ADDR(&server_ip, ip1, ip2, ip3, ip4);
		dvPort = (u16_t)port;
	}
	else
	{
		#if 1
		Print_Message("\nError in the configured Server IP Address.");
		#endif
	}
	#endif
	
	
	#if (LOCAL_SERVER)
	IP4_ADDR(&server_ip, 192, 168, 100, 71); // Public IP address
	U16 dvPort = 80;
	#endif
	
	while (1)
	{
		/* Client Side Code */
		if ((dm_dev_info.dev_sts != DEV_CANCELLED) && (gb_config_mode_f == 0) && (gb_start_sending_dpt == 1) && (gb_rtc_send_to_server_f == 1))
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
						
			if (gb_rtc_send_to_server_f == 1)
			{
				gb_rtc_send_to_server_f = 0;
				gb_send_tl_at_conn = 1;
			}
			else
			__NOP();
			
			client_server_running_f = FLAG_SET;
			static U8 retry_connection = 0;
			const U8 num_of_retry = 1;
			U8 conn_established_f = 0;
				
			while (retry_connection <= num_of_retry)
			{
				retry_connection ++;
				// Create a new TCP connection handle
				conn = netconn_new(NETCONN_TCP);
				if (conn == NULL)
				{
					Print_Message("\nFailed to create netconn.");
					vTaskDelay(10);
					continue;
				}
				else
				{
					/* If TCP connection created successfully, execute next instruction. */
					__NOP();
				}

				// Connect to the server
				err = netconn_connect(conn, &server_ip, dvPort);
				if (err != ERR_OK)
				{
					Print_Message("\nFailed to connect to server.");
					
						
					client_server_running_f = FLAG_RST;
					netconn_close(conn);
					netconn_delete(conn);
					vTaskDelay(10);
					continue;
				}
				else
				{
					/* If connected to the server break the while loop, execute next instruction. */
					conn_established_f = 1;
					break;
				}
			}
			retry_connection = 0;

			#if (LOCAL_SERVER)
			const char *http_request_template =
			"POST / HTTP/1.1\r\n"
			"Host: 192.168.100.71\r\n"
			"Content-Type: application/json\r\n"
			"Content-Length: %d\r\n"
			"Connection: close\r\n"
			"\r\n"
			"%s";
			#endif
				
			#if (REMOTE_SERVER)
			const char *http_request_template =
			// 			"POST /api/method/dor.api.log_transaction HTTP/1.1\r\n"
			// 			"Host: dor-uat.b-zone.in\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			"Authorization: Bearer token 398445b3f1f8266:c07bc759c99b8b5\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";

			// 			"POST /api/method/dor.api.device_data HTTP/1.1\r\n"
			// 			"Host: 202.21.32.129:8010\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token cbfc89180e0c92c:452514952f6e34d\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";

			// 26-10-2024
			// 			"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
			// 			"Host: 103.133.231.125:8005\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token cbfc89180e0c92c:159ab361e776c0b\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";
				
			// 18-02-2025
			// 			"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
			// 			"Host: 103.133.231.125:8010\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token 2e3c2af3cccb2f3:a1cd9d0aa016dc4\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";

			// 21-04-2025
			// 			"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
			// 			"Host: 110.227.214.55:8005\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token cbfc89180e0c92c:159ab361e776c0b\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";

			// 21-04-2025
			// 			"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
			// 			"Host: 110.227.214.55:8010\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token 2e3c2af3cccb2f3:a1cd9d0aa016dc4\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";

// 			26/05/2025
// 						"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
// 						"Host: %s\r\n"
// 						"Content-Type: application/json\r\n"
// 						"Authorization: Bearer token %s\r\n"
// 						"Content-Length: %d\r\n"
// 						"Connection: close\r\n\r\n"
// 						"%s";
			
			// 01/06/2026
			"POST /api/method/avon_iot.apis.iot.endpoint HTTP/1.1\r\n"
			"Host: %s\r\n"
			"Content-Type: application/json\r\n"
			"Authorization: token %s\r\n"
			"Content-Length: %d\r\n"
			"Connection: close\r\n\r\n"
			"%s";
				
			// 14-04-2025			static IP for testing
			// 			"POST /api/method/dor.api.device_api HTTP/1.1\r\n"
			// 			"Host: 192.168.1.9:8000\r\n"
			// 			"Content-Type: application/json\r\n"
			// 			//"cookie: SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest,SameSite=Lax; sid=Guest; SameSite=Lax,user_image\r\n"
			// 			"Authorization: Bearer token 2e3c2af3cccb2f3:6071646b564c4b2\r\n"
			// 			"Content-Length: %d\r\n"
			// 			"Connection: close\r\n\r\n"
			// 			"%s";
			#endif
				
			if (conn_established_f == 1)
			{
				/* Attempt to take the semaphore (wait for it to be available) */
				//if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
				{
					U8 data_send_error_f = 1;
					/* Attempt to take the semaphore (wait for it to be available) */
					if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
					{
						Print_Message("\nSema-1 taken.");
						if (gb_send_tl_at_conn == 1)
						{
							Send_Input_Log_ToServer();
						}
						else
						__NOP();
								
						snprintf(server_buffer, sizeof(server_buffer),
						http_request_template, ServerCfg.server_endpoint, ServerCfg.auth_token,
						strlen(jsonBuffer), jsonBuffer);
						
						Print_Message("\n\nServerCfg.auth_token:");
						Print_Message(ServerCfg.auth_token);
						
						err = netconn_write(conn, server_buffer, strlen(server_buffer), NETCONN_COPY);
						if (err != ERR_OK)
						{
							Print_Message("Failed to send HTTP request.");
							__NOP();
						}
						else
						data_send_error_f = 0;
						
						#if 1
						Print_Message("\n========== Sending final buffer ===========\n");
						Print_Message(server_buffer);
						#endif
						
						Print_Message("\nSema-1 Released.");
						/* Release the semaphore when done with the critical section */
						xSemaphoreGive(xSemaphore);
					}
					else
					__NOP();
						
					if (data_send_error_f == 0)
					{
						#if (REMOTE_SERVER)
						// After sending data to the server clear the below buffer to receive the data in
						// the same buffer.
						memset(server_buffer, 0, sizeof(server_buffer));
							buffer_offset = 0;
						U8 lcl_body_f = 0;
						
						U8 lcl_chunk_num = 1;
						
						wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
						netconn_set_recvtimeout(conn, 2000); // Set a 2-second timeout for receiving data.
						while((err = netconn_recv(conn, &inbuf)) == ERR_OK)
						{
							wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
							if (lcl_chunk_num == 1)
							{
								/* Attempt to take the semaphore (wait for it to be available) */
								xSemaphoreTake(xSemaphore, portMAX_DELAY);
								Print_Message("\nSema-2 taken.");
							}
							else
							__NOP();
							
							char *data = NULL;
							uint16_t len = 0;
							
							netbuf_first(inbuf);
							do
							{
								netbuf_data(inbuf, (void**)&data, &len);
								
								#if 0
								Print_Message("\nReceiving Data in chunks");
								Print_Message("\nChunk number is : ");
								Print_Number(lcl_chunk_num);
								Print_Message("\n\n");
								
								for (int i = 0; i<len; i++)
								{
									UART_Debug_PutChar(data[i]);
								}
								#endif
								lcl_chunk_num ++;
								
								// Extract the body from the HTTP response
								body = strstr(data, "\r\n\r\n");
								if ((body) && (lcl_body_f == 0))
								{
									body += 4; // Skip past the "\r\n\r\n" to get to the body
									
									#if 0
									Print_Message("\nHTTP body is present, And data is:-\n");
									for (uint16_t i = 0; i < len - (body - data); i++)
									{
										UART_Debug_PutChar(body[i]);
									}
									#endif
									
									unsigned int lcl_body_len = 0;
									lcl_body_len = (unsigned int)(len - (body - data));
									// Print_Message("\nLength of the body : ");
									// Print_Number(lcl_body_len);
									
									if ((lcl_body_len >= 0) && (lcl_body_len < sizeof(server_buffer)))
									{
										lcl_body_f = 1;
										memcpy(&server_buffer[buffer_offset], body, lcl_body_len);
										buffer_offset += lcl_body_len;
										body = 0;
									}
								}
								else if ((lcl_body_f == 1) && (len < sizeof(server_buffer)))
								{
									memcpy(&server_buffer[buffer_offset], data, len);
									buffer_offset += len;
								}
								else
								{
									__NOP();
								}
							}
							while (netbuf_next(inbuf) >= 0);
							
							netbuf_delete(inbuf);
						}
						
						if ((buffer_offset < sizeof(server_buffer)) && (lcl_body_f == 1))
						{
							server_buffer[buffer_offset] =  '\0'; // Ensure null-terminated string
							
							#if 1
							Print_Message("\n\nreceived length of the body is : ");
							Print_Number(buffer_offset);
							Print_Message("\nReceived Data from the server is :-\n");
							for (int i = 0; i < buffer_offset; i ++)
							{
								UART_Debug_PutChar(server_buffer[i]);
							}
							Print_Message("\n---------- Complete reception ------------\n\n");
							#endif
							
							Extract_JsonData_Using_Frozen_Library(server_buffer);
							
							Print_Message("\nSema-2 Released.");
							/* Release the semaphore when done with the critical section */
							xSemaphoreGive(xSemaphore);
						}
						else
						{
							gb_update_is_last_sent_loc_f = 0;	// Clear this flag if body not present or if not response from the server.
							gb_update_md_last_sent_loc_f = 0;	// Clear this flag if body not present or if not response from the server.
							
							Print_Message("\nSema-2 Released.");
							/* Release the semaphore when done with the critical section */
							xSemaphoreGive(xSemaphore);
						}
						
						client_server_running_f = FLAG_RST;
						// Close the connection
						netconn_close(conn);
						netconn_delete(conn);
						#endif						
						#if (LOCAL_SERVER)
						// Receive the response
						inbuf = 0;
						err = netconn_recv(conn, &inbuf);
						if (err != ERR_OK)
						{
							Print_Message("\nError! Failed to receive data.");
						}
						buf = 0;
						
						// Read the data from the buffer
						do
						{
							netbuf_data(inbuf, (void**)&buf, &buflen);
							if (buflen > 0)
							{
								
								/*
								Print_Message("\n++++++++++++++++++++++++++");
								for (int i = 0; i<buflen; i++)
								{
								UART_Debug_PutChar(buf[i]);
								}
								*/
								buf[buflen] = '\0'; // Ensure null-terminated string

								// Print the received data
								Print_Message("\nLength of the buffer is : ");
								Print_Number(buflen);
								/*
								Print_Message("\nReceived data: ");
								for (uint16_t i = 0; i < buflen; i++)
								{
								UART_Debug_PutChar(buf[i]);
								}
								*/
								// Extract the body from the HTTP response
								body = strstr(buf, "\r\n\r\n");
								if (body)
								{
									body += 4; // Skip past the "\r\n\r\n" to get to the body
									
									Print_Message("\nReceived Body \n");
									for (uint16_t i = 0; i < buflen - (body - buf); i++)
									{
										UART_Debug_PutChar(body[i]);
									}
									
									Extract_JsonData_Using_Frozen_Library(body);
									
									break;
								}
								else
								{
									Print_Message("\nThere is nothing in the body.");
								}
							}
						} while (netbuf_next(inbuf) >= 0);

						netbuf_delete(inbuf);
						client_server_running_f = FLAG_RST;
						// Close the connection
						netconn_close(conn);
						netconn_delete(conn);
						#endif
					}
					else
					{
						netbuf_delete(inbuf);
						client_server_running_f = FLAG_RST;
						// Close the connection
						netconn_close(conn);
						netconn_delete(conn);
					}
						
					conn_established_f = 0;
					/* Release the semaphore when done with the critical section */
					//xSemaphoreGive(xSemaphore);
				}
			}
		}
		vTaskDelay(5);
	}	// End of function while loop
}	// End of function

// Callback function for DNS resolution
void Mydns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
	//Print_Message("\nHere.");

	if (ipaddr != NULL)
	{
		// Print resolved IP address
// 		char ip_message[128];
// 		snprintf(ip_message, sizeof(ip_message), "DNS resolved: %s -> %s", name, ipaddr_ntoa(ipaddr)); // Removed \n
// 		Print_Message(ip_message);

		Print_Message("\nDNS 1resolved.");
		
		// Proceed to send HTTP POST request using resolved IP
		struct netconn *conn;
		struct netbuf *inbuf;
		err_t err;

		Print_Message("\nCreating netconn...");
		conn = netconn_new(NETCONN_TCP);
		if (conn == NULL)
		{
			Print_Message("Failed to create netconn.\n");
			return;
		}

		Print_Message("\nConnecting to server...");
		err = netconn_connect(conn, ipaddr, 80);  // Port 80 for HTTP
		if (err != ERR_OK)
		{
			char err_message[64];
			snprintf(err_message, sizeof(err_message), "Failed to connect to server: %d", err); // Removed \n
			Print_Message(err_message);
			netconn_delete(conn);
			return;
		}

		Print_Message("\nPreparing JSON body...");
		const char *json_body = "{\"dev_id\": \"123456\", \"card_uid\": \"11,12,13,aa,bb,cc,38\", \"date\": \"130824\", \"time\": \"165111\"}";

		// Prepare HTTP POST request
		Print_Message("\nPreparing HTTP POST request...");
		char http_request[512];
		snprintf(http_request, sizeof(http_request),
		"POST /app/device-transaction-log HTTP/1.1\r\n"
		"Host: shakti.b-zone.in\r\n"
		"Content-Type: application/json\r\n"
		"Authorization: Bearer token 398445b3f1f8266:c07bc759c99b8b5\r\n"
		"Content-Length: %d\r\n"
		"Connection: close\r\n\r\n"
		"%s", strlen(json_body), json_body);

		Print_Message("\nSending HTTP request...");
		err = netconn_write(conn, http_request, strlen(http_request), NETCONN_COPY);
		if (err != ERR_OK)
		{
			char err_message[64];
			snprintf(err_message, sizeof(err_message), "Failed to send HTTP request: %d", err); // Removed \n
			Print_Message(err_message);
			netconn_close(conn);
			netconn_delete(conn);
			return;
		}

		Print_Message("\nReceiving response...");
		err = netconn_recv(conn, &inbuf);
		if (err == ERR_OK)
		{
			char *buf;
			u16_t buflen;

			// Print received data
			do
			{
				netbuf_data(inbuf, (void **)&buf, &buflen);
				if (buflen > 0)
				{
					char response_message[512];
					snprintf(response_message, sizeof(response_message), "Received response:\n%.*s", buflen, buf); // Removed \n
					Print_Message(response_message);
				}
			} while (netbuf_next(inbuf) >= 0);

			netbuf_delete(inbuf);
		}
		else
		{
			char err_message[64];
			snprintf(err_message, sizeof(err_message), "Failed to receive response: %d", err); // Removed \n
			Print_Message(err_message);
		}

		// Close the connection
		netconn_close(conn);
		netconn_delete(conn);
	}
	else
	{
// 		char fail_message[128];
// 		snprintf(fail_message, sizeof(fail_message), "DNS resolution failed for: %s", name); // Removed \n
// 		Print_Message(fail_message);

		Print_Message("\nDNS 2resolution failed.");
	}
}

// Function to perform DNS lookup and send HTTP POST request
void perform_dns_lookup_and_post(void)
{
	Print_Message("\nInside perform_dns_lookup_and_post function.");
	
	//const char *hostname = "shakti.b-zone.in";
	//const char *hostname = "webhook.site";
	//const char *hostname = "dor-uat.b-zone.in";
	const char *hostname = "202.21.32.129:8010";
	
	ip_addr_t resolved_ip;

	// Perform DNS lookup
	//err_t err = dns_gethostbyname(hostname, &resolved_ip, Mydns_found_callback, NULL);
	//err_t err = netconn_gethostbyname("dor-uat.b-zone.in", &resolved_ip);
	err_t err = netconn_gethostbyname("202.21.32.129:8010", &resolved_ip);
	
	if (err == ERR_OK)
	{
		Print_Message("\nDNS resolution success block");
		// DNS resolution completed immediately, send HTTP POST
// 		char success_message[256];
// 		snprintf(success_message, sizeof(success_message), "Immediate DNS resolution: %s -> %s\n", hostname, ipaddr_ntoa(&resolved_ip));
// 		Print_Message(success_message);

		// Call the callback manually since the resolution is done
		Mydns_found_callback(hostname, &resolved_ip, NULL);
	}
	else if (err == ERR_INPROGRESS)
	{
		Print_Message("\nDNS lookup in progress block");
		// DNS lookup in progress, wait for the callback
		//Print_Message("DNS lookup in progress...\n");
	}
	else
	{
		Print_Message("\nDNS lookup failed block - ");
		Print_Number(err);
// 		char fail_message[256];
// 		snprintf(fail_message, sizeof(fail_message), "DNS lookup failed for %s\n", hostname);
// 		Print_Message(fail_message);
	}
	
	// Wait for DNS resolution
	while (1) {
		// Wait for DNS resolution
		vTaskDelay(100);
	}
}


// void Send_Data_Client_To_Server(void *pvParameters)
// {
// 	struct netconn *conn;
// 	struct netbuf *inbuf;
// 	ip_addr_t server_ip;
// 	err_t err;
// 	char *buf;
// 	u16_t buflen;
// 	char *body;
// 	
// 	// Convert server IP address
// // 	IP4_ADDR(&server_ip, 192, 168, 100, 120); // Public IP address
//  	U16 dvPort = 8080;
// 	
// 	// Set up DNS server manually (Optional, if not using DHCP)
// 	ip_addr_t dnsserver;
// 	IP4_ADDR(&dnsserver, 8, 8, 8, 8); // Google's public DNS server
// 	dns_setserver(0, &dnsserver);     // Set DNS server 0 (primary)
// 	
// 	ip_addr_t dnsserver1;
// 	IP4_ADDR(&dnsserver1, 4, 2, 2, 2); // Google's public DNS server
// 	dns_setserver(1, &dnsserver1);     // Set DNS server 0 (primary)
// 
// 	// Perform DNS lookup and send the POST request
// 	perform_dns_lookup_and_post();
// 	
// 	
// 	while (1)
// 	{
// 		vTaskDelay(10);
// 	}
// 	
// // 	while (1)
// // 	{
// // 		if (gb_rtc_send_to_server_f == 1)
// // 		{
// // 			gb_rtc_send_to_server_f = 0;
// // 			
// // 			// Create a new TCP connection handle
// // 			conn = netconn_new(NETCONN_TCP);
// // 			if (conn == NULL) {
// // 				Print_Message("\nFailed to create netconn.");
// // 				/*vTaskDelay(1000);*/
// // 				vTaskDelay(10);
// // 				continue;
// // 			}
// // 
// // 			// Connect to the server
// // 			err = netconn_connect(conn, &server_ip, dvPort);
// // 			if (err != ERR_OK) {
// // 				Print_Message("\nFailed to connect to server.");
// // 				Print_Number(err);
// // 				netconn_delete(conn);
// // 				/*vTaskDelay(1000);*/
// // 				vTaskDelay(10);
// // 				continue;
// // 			}
// // 			
// // 			// Variables to hold the timestamp values
// // 			static uint8_t hour = 16;
// // 			static uint8_t minute = 50;
// // 			static uint8_t second = 15;
// // 			
// // 			// Update the timestamp (increment seconds, handle minute and hour overflow)
// // 			second++;
// // 			if (second >= 60) {
// // 				second = 0;
// // 				minute++;
// // 				if (minute >= 60) {
// // 					minute = 0;
// // 					hour++;
// // 					if (hour >= 24) {
// // 						hour = 0;
// // 					}
// // 				}
// // 			}
// // 			
// // 			// Prepare the HTTP GET request
// // 			const char *get_request = "GET / HTTP/1.1\r\n"
// // 			"Host: 192.168.100.120\r\n"  // Replace with your server IP address
// // 			"Connection: close\r\n\r\n";
// // 
// // 			static U8 devId = 2;
// // 			//devId ++;
// // 			
// // 			static U8 UIDArray[7] = {0};
// // 			
// // 			UIDArray[0] = 0x11;
// // 			UIDArray[1] = 0x12;
// // 			UIDArray[2] = 0x13;
// // 			UIDArray[3] = 0x22;
// // 			UIDArray[4] = 0x33;
// // 			UIDArray[5] = 0x44;
// // 			UIDArray[6] = 0x77;
// // 			
// // 			//UIDArray[6] ++;
// // 			
// // 			static U8 dd = 13;
// // 			static U8 mm = 8;
// // 			static U8 yy = 24;
// // 			
// // 			// Prepare the HTTP POST request with updated JSON data
// // 			char json_data[128];
// // 			snprintf(json_data, sizeof(json_data),
// // 			"{\"dev_id\": %d, \"card_uid\": [%02x,%02x,%02x,%02x,%02x,%02x,%02x], "
// // 			"\"date\": \"%02d%02d%02d\", \"time\": \"%02d%02d%02d\"}",
// // 			devId, UIDArray[0], UIDArray[1], UIDArray[2], UIDArray[3], UIDArray[4], UIDArray[5], UIDArray[6],
// // 			gb_rtcDateArr[0], gb_rtcDateArr[1], gb_rtcDateArr[2], gb_rtcTimeArr[0], gb_rtcTimeArr[1], gb_rtcTimeArr[2]);
// // 
// // 			const char *http_request_template =
// // 			"POST / HTTP/1.1\r\n"
// // 			"Host: 192.168.100.120\r\n"
// // 			"Content-Type: application/json\r\n"
// // 			"Content-Length: %d\r\n"
// // 			"Connection: close\r\n"
// // 			"\r\n"
// // 			"%s";
// // 
// // 			// Calculate the length of the HTTP request
// // 			char http_request[512];
// // 			snprintf(http_request, sizeof(http_request), http_request_template, strlen(json_data), json_data);
// // 
// // 			static U8 send_f = 0;
// // 			if (send_f == 1)
// // 			{
// // 				// Send the HTTP GET request
// // 				send_f = 0;
// // 				err = netconn_write(conn, get_request, strlen(get_request), NETCONN_COPY);
// // 				if (err != ERR_OK)
// // 				{
// // 					Print_Message("\nFailed to send GET request");
// // 				}
// // 			}
// // 			else
// // 			{
// // 				// Send the HTTP POST request
// // 				send_f = 0;
// // 				err = netconn_write(conn, http_request, strlen(http_request), NETCONN_COPY);
// // 				if (err != ERR_OK)
// // 				{
// // 					Print_Message("Failed to send HTTP request.");
// // 				}
// // 			}
// // 
// // 			// Receive the response
// // 			err = netconn_recv(conn, &inbuf);
// // 			if (err != ERR_OK)
// // 			{
// // 				break;
// // 			}
// // 
// // 			// Read the data from the buffer
// // 			do
// // 			{
// // 				netbuf_data(inbuf, (void**)&buf, &buflen);
// // 				if (buflen > 0) {
// // 					buf[buflen] = '\0'; // Ensure null-terminated string
// // 
// // 					// Print the received data
// // 					// 					Print_Message("\nLength of the buffer is : ");
// // 					// 					Print_Number(buflen);
// // 					// 					Print_Message("\nReceived data: ");
// // 					// 					for (uint16_t i = 0; i < buflen; i++)
// // 					// 					{
// // 					// 						UART_Debug_PutChar(buf[i]);
// // 					// 					}
// // 
// // 					// Extract the body from the HTTP response
// // 					body = strstr(buf, "\r\n\r\n");
// // 					if (body)
// // 					{
// // 						body += 4; // Skip past the "\r\n\r\n" to get to the body
// // 						Print_Message("Received Body: ");
// // 						for (uint16_t i = 0; i < buflen - (body - buf); i++)
// // 						{
// // 							UART_Debug_PutChar(body[i]);
// // 						}
// // 						break;
// // 					}
// // 				}
// // 			} while (netbuf_next(inbuf) >= 0);
// // 
// // 			netbuf_delete(inbuf);
// // 
// // 			// Close the connection
// // 			netconn_close(conn);
// // 			netconn_delete(conn);
// // 		}
// // 
// // 		vTaskDelay(10);
// // 	}
// }

// Callback for DNS resolution
void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg)
{
	if (ipaddr != NULL)
	{
		Print_Message("\nDNS resolved successfully.");

		// Cast callback_arg to the original connection structure
		struct netconn *conn = (struct netconn *)callback_arg;

		// Now we can connect to the server using the resolved IP
		err_t err = netconn_connect(conn, ipaddr, 80);  // Use port 80 for HTTP
		if (err != ERR_OK)
		{
			Print_Message("\nFailed to connect to the server after DNS resolution.");
			netconn_delete(conn);
			return;
		}

		// Proceed to send the HTTP request after successful connection
		// The rest of the HTTP POST sending process happens here (same as your existing code)
	}
	else
	{
		Print_Message("\nDNS resolution failed.");
	}
}

// void Send_Data_Client_To_Server(void *pvParameters)
// {
// 	struct netconn *conn;
// 	struct netbuf *inbuf;
// 	err_t err;
// 	char *buf;
// 	u16_t buflen;
// 	char *body;
// 
// 	ip_addr_t server_ip;
// 	
// 	// DNS Hostname
// 	const char *hostname = "dor-uat.b-zone.in";
// 	
// 	while (1)
// 	{
// 		if (gb_rtc_send_to_server_f == 1)
// 		{
// 			gb_rtc_send_to_server_f = 0;
// 
// 			// Create a new TCP connection handle
// 			conn = netconn_new(NETCONN_TCP);
// 			if (conn == NULL)
// 			{
// 				Print_Message("\nFailed to create netconn.");
// 				vTaskDelay(10);
// 				continue;
// 			}
// 
// 			// Perform DNS lookup
// 			Print_Message("\nPerforming DNS lookup...");
// 			//err = dns_gethostbyname(hostname, NULL, dns_callback, conn);
// 			err = netconn_gethostbyname(hostname, &server_ip);
// 			
// 			if (err == ERR_INPROGRESS)
// 			{
// 				// DNS lookup in progress, wait for the callback to handle the rest
// 				Print_Message("\nDNS lookup in progress...");
// 				vTaskDelay(100);  // Introduce some delay for DNS to complete
// 			}
// 			else if (err == ERR_OK)
// 			{
// 				// DNS resolution completed immediately, call the callback directly
// 				dns_callback(hostname, NULL, conn);
// 			}
// 			else
// 			{
// 				// DNS resolution failed
// 				Print_Message("\nDNS resolution failed immediately.");
// 				netconn_delete(conn);
// 				vTaskDelay(10);
// 				continue;
// 			}
// 
// 			// The rest of the logic happens in the DNS callback, including sending the HTTP POST request
// 		}
// 
// 		vTaskDelay(10);  // Adjust delay as necessary
// 	}
// }


U8 Inspect_Received_Frame(U8 *rcv_frame, U16 rcv_len)
{
	if ((rcv_frame[0] != SOF) && (rcv_frame[2] == 0) && (rcv_frame[3] == 0))
	{
		// Its a modbus TCP frame.
		return 1;
	}
	else if ((rcv_frame[0] == SOF) && (rcv_frame[3] == DST_ADDR_RX))
	{
		// Its an utility frame.
		return 0;
	}
	else
	{
		__NOP();
	}
	
	return 0;
}
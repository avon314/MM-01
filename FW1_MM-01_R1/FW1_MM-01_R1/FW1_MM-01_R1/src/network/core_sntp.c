/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: core_sntp.c
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
#include "lwip/udp.h"
#include "lwip/netdb.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"
#include "api.h"
#include "user_uart.h"
#include "lwipopts.h"
#include <string.h>
#include <stdio.h>
#include "core_sntp.h"
#include "user_rtc.h"
#include "memory_storage.h"
#include "ext_eeprom_1.h"
#include "mm_app.h"

#define SNTP_PORT              123
//#define SNTP_SERVER_IP         "ntp.mobatime.ru"  // or your preferred SNTP server
#define SNTP_SERVER_IP         "time.google.com"  // or your preferred SNTP server
#define SNTP_REQUEST_LEN       48
#define SNTP_TIMESTAMP_OFFSET  40
#define NTP_TO_UNIX_OFFSET     2208988800UL   // Difference between NTP and UNIX epoch in seconds

static struct udp_pcb *sntp_pcb = NULL;
U8 delete_sntp_task_f = 0;
U8 sntp_waiting_for_response = 0;
U8 sntp_retry_f = 0;
U8 sntp_num_of_retry = 0;
U8 sntp_resp_time = 0;
U8 gb_sntp_powerOn_f = 0;
extern U8 sntp_create_task_f;
extern U8 clr_pwr_sntp_task_f;
extern xSemaphoreHandle xSemaphore;
extern char gb_dev_ipAddress[13];
U32 dev_ip_add;

void Task_Get_SNTP_Data(void *pvParameters);



/*****************************************************************************
* Function name		: Task_Get_SNTP_Data
* Returns			: Nothing.
* Arguments			: void --> pvParameters
* Created by		: Shirisha Ankam
* Date created		: 14-04-2025
* Description		: Function to Run SNTP task parallel at specific 
					  time for specific period.
*
* Notes				: NA
* Global Variables Affected : sntp_waiting_for_response,sntp_num_of_retry,sntp_retry_f,
							  delete_sntp_task_f,sntp_pcb
*****************************************************************************/
void Task_Get_SNTP_Data(void *pvParameters)
{
	while (1)
	{
		/* Attempt to take the semaphore (wait for it to be available) */
		if (xSemaphoreTake(xSemaphore, portMAX_DELAY))
		{
			wdt_restart(WDT);	// Restart the watch dog timer. Otherwise controller will restart.
			
			if (delete_sntp_task_f == 1)
			{
				delete_sntp_task_f = 0;
				if (clr_pwr_sntp_task_f == 1)
				{
					clr_pwr_sntp_task_f = 0;
					sntp_create_task_f = 0;
				}
				else
				__NOP();
				
				if (sntp_pcb != NULL)
				{
					udp_remove(sntp_pcb);
					sntp_pcb = NULL;
				}
				else
				__NOP();
				
				/* Release the semaphore when done with the critical section */
				xSemaphoreGive(xSemaphore);
				// Delete current task
				vTaskDelete(NULL);
			}
			else if ((sntp_num_of_retry < SNTP_MAX_RETRY) && (sntp_retry_f == 1))
			{
				// Cleanup manually if needed
				if (sntp_pcb != NULL)
				{
					udp_remove(sntp_pcb);
					sntp_pcb = NULL;
				}
				else
				__NOP();
			
				sntp_client();
				sntp_retry_f = 0;		// Reset flag for next check.
				sntp_num_of_retry ++;	// Increment counter value up to max.
			}
			else if ((sntp_waiting_for_response == 0) && (sntp_retry_f == 0))
			{
				sntp_client();
			}
			else if (sntp_retry_f == 1)
			{
				// if exceed number of retrial's. delete the task and buffer.
				if (sntp_pcb != NULL)
				{
					udp_remove(sntp_pcb);
					sntp_pcb = NULL;
				}
				else
				__NOP();
			
				// Reset retry counter before deleting
				sntp_num_of_retry = 0;
				sntp_retry_f = 0;
				
				if (clr_pwr_sntp_task_f == 1)
				{
					clr_pwr_sntp_task_f = 0;
					sntp_create_task_f = 0;
				}
				else
				__NOP();

				/* Release the semaphore when done with the critical section */
				xSemaphoreGive(xSemaphore);
				// Delete current task
				vTaskDelete(NULL);
			}
			else
			__NOP();
		
			/* Release the semaphore when done with the critical section */
			xSemaphoreGive(xSemaphore);
		}
		
		vTaskDelay(1);
	}
}


/*****************************************************************************
* Function name : sntp_send_request
* Returns       : err --> Error value if request fails.
* Arguments     : pcb --> Pointer to UDP protocol control block.
*                 server_ip --> Pointer to SNTP server IP address.
* Created by    : Shirisha Ankam
* Date created  : 12-04-2025
* Description   : Function to send SNTP request packet to a server.
*
* Notes         : This function forms a basic SNTP request and sends it over
*                 UDP to the specified server IP using the provided PCB.
* Global Variables Affected : None
*****************************************************************************/
err_t sntp_send_request(struct udp_pcb *pcb, const ip_addr_t *server_ip)
{
	#if DEBUG_SNTP
	Print_Message("\nInside SNTP send request function");
	#endif

	struct pbuf *p;
	u8_t sntp_packet[SNTP_REQUEST_LEN] = {0};  // SNTP packet buffer initialized to 0

	sntp_packet[0] = 0x1B;  // LI = 0 (no warning), VN = 3 (version), Mode = 3 (client)

	// Allocate memory for UDP payload
	p = pbuf_alloc(PBUF_TRANSPORT, SNTP_REQUEST_LEN, PBUF_RAM);
	if (!p) return ERR_MEM;  // Return memory error if allocation fails

	// Copy SNTP packet data into pbuf payload
	memcpy(p->payload, sntp_packet, SNTP_REQUEST_LEN);

	// Send SNTP request packet to the server on SNTP port
	err_t err = udp_sendto(pcb, p, server_ip, SNTP_PORT);

	// Free the allocated pbuf memory
	pbuf_free(p);

	return err;  // Return status of send operation
}

/*****************************************************************************
* Function name : sntp_recv_callback
* Returns       : None
* Arguments     : arg  --> Pointer to user-defined data (unused).
*				  pcb  --> Pointer to the UDP protocol control block.
*				  p    --> Pointer to the received pbuf containing the SNTP response.
*                 addr --> Pointer to the IP address of the sender.
*                 port --> Port number of the sender.
* Created by    : Shirisha Ankam
* Date created  : 12-04-2025
* Description   : Callback function to handle SNTP server response.
*
* Notes         : This function extracts the transmit timestamp from the
*                 received SNTP packet and converts it to UNIX time.

* Global Variables Affected : sntp_waiting_for_response, delete_sntp_task_f
*                             sntp_num_of_retry, sntp_retry_f,sntp_pcb
*****************************************************************************/
void sntp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port)
{
	#if DEBUG_SNTP
	Print_Message("\nInside SNTP receive call back function");
	#endif

	if (p->len >= SNTP_REQUEST_LEN)
	{
		U32 seconds;

		// SNTP transmit timestamp starts at byte 40 of the SNTP response
		u8_t *data = (u8_t *)p->payload;
		seconds = (data[40] << 24) | (data[41] << 16) | (data[42] << 8) | data[43];

		// Convert NTP time to UNIX time by subtracting offset
		seconds -= NTP_TO_UNIX_OFFSET;
		
		#if DEBUG_SNTP
		// Print UNIX time in seconds
		Print_Message("\nSNTP Time (UNIX) in seconds : ");
		Print_Number(seconds);
		#endif

		// Convert and apply the time in IST (or any required format)
		convert_unix_to_ist(seconds);

		sntp_waiting_for_response = 0;	// Clear the waiting flag
		delete_sntp_task_f = 1;			// Signal to delete the SNTP task
		sntp_num_of_retry = 0;			// Reset retry count
		sntp_retry_f = 0;				// Clear retry flag
	}
	else
	{
		#if DEBUG_SNTP
		Print_Message("\nBuffer allocation failed");
		#endif
	}

	// Free the received pbuf memory
	pbuf_free(p);

	// Remove the UDP PCB after single-use sync
	udp_remove(pcb);
	sntp_pcb = NULL;
}

/*****************************************************************************
* Function name : sntp_client
* Returns       : None
* Arguments     : None
* Created by    : Shirisha Ankam
* Date created  : 12-04-2025
* Description   : Entry point for the SNTP client to initiate time synchronization.
*
* Notes         : This function initializes a UDP PCB, binds it to a local IP,
*                 performs DNS resolution for the SNTP server, and sends an SNTP
*                 request. On success, it sets up the receive callback.

* Global Variables Affected : sntp_pcb , sntp_waiting_for_response
*****************************************************************************/
void sntp_client(void)
{
	#if DEBUG_SNTP
	Print_Message("\nInside SNTP client Function");
	#endif

	ip_addr_t local_ip;
	err_t err;

	// Create a new UDP protocol control block
	sntp_pcb = udp_new();

	if (!sntp_pcb)
	{
		#if DEBUG_SNTP
		Print_Message("\nFailed to create UDP PCB");
		#endif
		return;
	}
	#if DEBUG_SNTP
	Print_Message("\nSuccessfully created UDP PCB");
	#endif

	// Set local bind IP (e.g., static IP assigned to device)
	//IP4_ADDR(&local_ip, 192, 168, 100, 77/*98*/);
	local_ip.addr = dev_ip_add;
	// Bind the UDP PCB to the specified local IP and port 5555
	err = udp_bind(sntp_pcb, &local_ip, 5555);
	if (err != ERR_OK)
	{
		#if DEBUG_SNTP
		Print_Message("\nUDP bind failed");
		#endif
		udp_remove(sntp_pcb);
		return;
	}
	#if DEBUG_SNTP
	Print_Message("\nUDP bind Success");
	#endif
	
	#if DEBUG_SNTP
	Print_Message("\nBound to source address : ");
	Send_Frame_On_UART(ipaddr_ntoa(&local_ip), 15);
	#endif
	
	/***** Read DI from the EEPROM, to get the dns info & Time zone *****/
	U8 lcl_eep_arr[DI_DATA_SIZE];
	memset(lcl_eep_arr, 0, DI_DATA_SIZE);
	U8 lcl_eep_sts = 0;
	lcl_eep_sts = EEPROM_Byte_Read(DI_EEP_START_LOC, lcl_eep_arr, DI_DATA_SIZE);
	if (lcl_eep_sts == 0)  // Check if the EEPROM read was successful
	{
		#if DEBUG_SNTP
		Print_Message("\nDomain name to resolve = ");
		Print_Message(&lcl_eep_arr[DNS_IDX]);
		#endif
	}
	else
	__NOP();
	/***** End of Read DI from the EEPROM, to get the dns info & Time zone *****/
	
	U8 is_dns_resolved = 0;
	// Perform DNS resolution for SNTP server
	//err = netconn_gethostbyname(SNTP_SERVER_IP, &local_ip);
	err = netconn_gethostbyname(&lcl_eep_arr[DNS_IDX], &local_ip);

	if (err == ERR_OK)
	{
		#if DEBUG_SNTP
		// DNS resolved immediately
		Print_Message("\nDNS resolved immediately, and resolved IP address is : ");
		//dns_found_cb(hostname, &resolved_ip, udp_pcb);  // Callback (commented)
		Send_Frame_On_UART(ipaddr_ntoa(&local_ip), strlen(ipaddr_ntoa(&local_ip)));
		#endif
			
		is_dns_resolved = 1;
		// Now send UDP packet
		//udp_send_data(sntp_pcb, "Hello from netconn DNS!", &server_ip, 1023);  // (commented)
	}
	else if (err == ERR_INPROGRESS)
	{
		#if DEBUG_SNTP
		Print_Message("\nDNS resolution in progress...\n");
		#endif
		// Callback will be called once resolution is done
	}
	else
	{
		#if DEBUG_SNTP
		Print_Message("\nDNS resolution failed");
		#endif
		
		delete_sntp_task_f = 1;
	}
	

	// Send SNTP request to the resolved server IP
	if (is_dns_resolved == 1)
	{
		err = sntp_send_request(sntp_pcb, &local_ip);
		if (err != ERR_OK)
		{
			#if DEBUG_SNTP
			Print_Message("\nFailed to send SNTP request");
			#endif
			//udp_remove(sntp_pcb); 
		}
		else
		{
			#if DEBUG_SNTP
			Print_Message("\nSNTP send Request successfully");
			#endif

			sntp_waiting_for_response = 1;	// Set flag indicating waiting for SNTP response
			udp_recv(sntp_pcb, sntp_recv_callback, NULL);	// Register receive callback
		}
	}
	else
	__NOP();
}


int is_leap_year(int year)
{
	return ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
}

/*****************************************************************************
* Function name : convert_unix_to_ist
* Returns       : void
* Arguments     : uint32_t unix_time - UNIX timestamp (seconds since 1970)
* Created by    : Shirisha Ankam
* Date created  : 14-04-2025
* Description   : Converts UNIX time to IST (Indian Standard Time), formats it
*                 into DD-MM-YYYY and HH:MM:SS format, prints the result, and
*                 syncs the converted time and date to the DS1339A RTC module.
* Notes         : IST is UTC + 5:30, i.e., 19800 seconds offset
*****************************************************************************/
void convert_unix_to_ist(uint32_t unix_time)
{
	// Convert UNIX time to IST by adding 19800 seconds (5 hours 30 minutes)
	//uint32_t ist_time = unix_time + 19800;
	if ((dm_dev_info.time_zone_hr >= 244) && (dm_dev_info.time_zone_hr <= 255))
	{
		// Subtract unix time by time zone. 
		// if values in between -12 to -1, those are stored as 2's compliment.
		U8 negValue = (U8) ~(dm_dev_info.time_zone_hr - 1);
		unix_time = (unix_time - ((negValue * 3600) + (dm_dev_info.time_zone_mn * 60)));
	}
	else
	{
		// Add unix time by time zone.
		unix_time = (unix_time + ((dm_dev_info.time_zone_hr * 3600) + (dm_dev_info.time_zone_mn * 60)));
	}
	

	// Calculate number of days since epoch and remaining seconds in the day
	uint32_t days = unix_time / 86400;
	uint32_t seconds_in_day = unix_time % 86400;

	// Convert seconds in the day to hours, minutes, and seconds
	uint8_t hour = seconds_in_day / 3600;
	uint8_t minute = (seconds_in_day % 3600) / 60;
	uint8_t second = seconds_in_day % 60;

	// Calculate year starting from 1970 and adjusting for leap years
	uint16_t year = 1970;
	const uint8_t month_days[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
	while (1)
	{
		uint16_t days_in_year = is_leap_year(year) ? 366 : 365;
		if (days >= days_in_year)
		{
			days -= days_in_year;
			year++;
		}
		else
		{
			break;
		}
	}

	// Determine the correct month based on remaining days
	uint8_t month = 0;
	while (1)
	{
		uint8_t dim = month_days[month];
		if (month == 1 && is_leap_year(year)) dim = 29; // Adjust for leap year in February

		if (days >= dim)
		{
			days -= dim;
			month++;
		}
		else
		{
			break;
		}
	}

	// Remaining days are the date (1-based)
	uint8_t date = days + 1;
	
	#if DEBUG_SNTP
	// Print final IST date and time
	Print_Message("\nDD-MM-YYYY\t");
	Print_Number(date);
	Print_Message("-");
	Print_Number(month + 1); // Month is 0-based
	Print_Message("-");
	Print_Number(year);

	Print_Message("\nHH:MM:SS\t");
	Print_Number(hour);
	Print_Message(":");
	Print_Number(minute);
	Print_Message(":");
	Print_Number(second);
	#endif

	/***** RTC SYNC CODE BLOCK *****/

	// Compute day of the week using Tomohiko Sakamoto’s Algorithm
	uint16_t year1 = year;
	static const int day_offsets[12] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
	if (month < 2)
	{
		year1 -= 1; // Adjust for Jan and Feb
	}
	else
	{
		__NOP(); // Placeholder NOP
	}
	U8 day_of_week = ((year1 + (year1 / 4) - (year1 / 100) + (year1 / 400) + day_offsets[month] + date) % 7);
	day_of_week = (day_of_week == 0) ? 7 : day_of_week; // Convert 0 (Sunday) to 7 for compatibility

	// Write date (DD-MM-YY) and day to RTC
	Write_DtMnYy_and_Day_To_DS1339A(date, (month + 1), (year % 100), (day_of_week + 1));

	// Write time (HH:MM:SS) to RTC
	Write_Time_hhMMss_To_DS1339A(hour, minute, second);

	/***** END OF RTC SYNC BLOCK *****/
}


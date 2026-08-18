/**
 * \file
 *
 * \brief Ethernet management for LwIP (threaded mode).
 *
 * Copyright (c) 2013-2018 Microchip Technology Inc. and its subsidiaries.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Subject to your compliance with these terms, you may use Microchip
 * software and any derivatives exclusively with Microchip products.
 * It is your responsibility to comply with third party license terms applicable
 * to your use of third party software (including open source software) that
 * may accompany Microchip software.
 *
 * THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE,
 * INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY,
 * AND FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT WILL MICROCHIP BE
 * LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, INCIDENTAL OR CONSEQUENTIAL
 * LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND WHATSOEVER RELATED TO THE
 * SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP HAS BEEN ADVISED OF THE
 * POSSIBILITY OR THE DAMAGES ARE FORESEEABLE.  TO THE FULLEST EXTENT
 * ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL CLAIMS IN ANY WAY
 * RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT OF FEES, IF ANY,
 * THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS SOFTWARE.
 *
 * \asf_license_stop
 *
 */

#include <string.h>
#include "board.h"
#include "ethernet.h"
#if SAM3XA
# include "ethernet_phy.h"
# include "emac.h"
#elif SAM4E
# include "ethernet_phy.h"
# include "gmac.h"
#elif SAM4S
# include "ksz8851snl.h"
# include "ksz8851snl_reg.h"
# include "netif/sam_spi_ksz8851snl.h"
#else
# error Unsupported chip type
#endif
#include "sysclk.h"
/* lwIP includes */
#include "lwip/sys.h"
#include "lwip/api.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "lwip/memp.h"
#include "lwip/dhcp.h"
#include "lwip/dns.h"
#include "lwip/stats.h"
#include "lwip/init.h"
#include "lwip/ip_frag.h"
#if ( (LWIP_VERSION) == ((1U << 24) | (3U << 16) | (2U << 8) | (LWIP_VERSION_RC)) )
#include "netif/loopif.h"
#else
#include "lwip/inet.h"
#include "lwip/tcp_impl.h"
#endif
#include "netif/etharp.h"

/***** User Includes *****/
#include "user_uart.h"
#include "onboard_key.h"

#include "comn_config.h"
#include "definitions.h"

/* Global variable containing MAC Config (hw addr, IP, GW, ...) */
struct netif gs_net_if;

int8_t g_c_ipconfig[];
uint32_t g_ip_mode = 1; // 0 unselected, 1 static, 2 dhcp.
char gb_dev_ipAddress[13] = "abc";
char gb_def_ipAddress[13] = "192168001045";
extern U32 dev_ip_add;
U8 eth_conn_establish_f = 0;
/**
 * \brief Configure ethernet interface.
 */
void ethernet_configure_interface(void)
{
	struct ip_addr x_ip_addr;
	struct ip_addr x_net_mask;
	struct ip_addr x_gateway;
	extern err_t ethernetif_init(struct netif *netif);
	
	if (g_ip_mode == 2)
	{
		/* DHCP mode. */
		x_ip_addr.addr = 0;
		x_net_mask.addr = 0;
	}
	else
	{
		U8 lclIPAddress[4] = {0};
		U8 lclSubNetMask[4] = {0};
		U8 lclDefGateway[4] = {0};
			
		char *endPtr;
		long int num = 0;
		U8 lclIdx = 0;
		
		/****************************************************/
		num = strtol(IpAddr.ip_addr_1, &endPtr, 10);
		lclIPAddress[lclIdx++] = (U8) num;
		num = strtol(IpAddr.ip_addr_2, &endPtr, 10);
		lclIPAddress[lclIdx++] = (U8) num;
		num = strtol(IpAddr.ip_addr_3, &endPtr, 10);
		lclIPAddress[lclIdx++] = (U8) num;
		num = strtol(IpAddr.ip_addr_4, &endPtr, 10);
		lclIPAddress[lclIdx++] = (U8) num;
		
		#if DEBUG_ALL || DEBUG_ETHERNET
		Print_Message("\nIP Address is : ");
		Print_Number(lclIPAddress[0]);
		Print_Message(":");
		Print_Number(lclIPAddress[1]);
		Print_Message(":");
		Print_Number(lclIPAddress[2]);
		Print_Message(":");
		Print_Number(lclIPAddress[3]);
		#endif
		
		/* Fixed IP mode. */
		/* Default IP addr */
 		IP4_ADDR(&x_ip_addr, lclIPAddress[0], lclIPAddress[1],
			 				lclIPAddress[2], lclIPAddress[3]);
		
		/*******************************************************/
		lclIdx = 0;
		num = strtol(SbntMsk.sbnt_msk_1, &endPtr, 10);
		lclSubNetMask[lclIdx++] = (U8) num;
		num = strtol(SbntMsk.sbnt_msk_2, &endPtr, 10);
		lclSubNetMask[lclIdx++] = (U8) num;
		num = strtol(SbntMsk.sbnt_msk_3, &endPtr, 10);
		lclSubNetMask[lclIdx++] = (U8) num;
		num = strtol(SbntMsk.sbnt_msk_4, &endPtr, 10);
		lclSubNetMask[lclIdx++] = (U8) num;
		
		#if DEBUG_ALL || DEBUG_ETHERNET
		Print_Message("\nSubnet Mask is : ");
		Print_Number(lclSubNetMask[0]);
		Print_Message(":");
		Print_Number(lclSubNetMask[1]);
		Print_Message(":");
		Print_Number(lclSubNetMask[2]);
		Print_Message(":");
		Print_Number(lclSubNetMask[3]);
		#endif	
						 
		/* Default subnet mask. */
		IP4_ADDR(&x_net_mask, lclSubNetMask[0], lclSubNetMask[1],
		 				lclSubNetMask[2], lclSubNetMask[3]);
		
		/*******************************************************/
		lclIdx = 0;
		num = strtol(DfltGtwy.dflt_gtwy_1, &endPtr, 10);
		lclDefGateway[lclIdx++] = (U8) num;
		num = strtol(DfltGtwy.dflt_gtwy_2, &endPtr, 10);
		lclDefGateway[lclIdx++] = (U8) num;
		num = strtol(DfltGtwy.dflt_gtwy_3, &endPtr, 10);
		lclDefGateway[lclIdx++] = (U8) num;
		num = strtol(DfltGtwy.dflt_gtwy_4, &endPtr, 10);
		lclDefGateway[lclIdx++] = (U8) num;
		
		#if DEBUG_ALL || DEBUG_ETHERNET
		Print_Message("\nDefault gateway is : ");
		Print_Number(lclDefGateway[0]);
		Print_Message(":");
		Print_Number(lclDefGateway[1]);
		Print_Message(":");
		Print_Number(lclDefGateway[2]);
		Print_Message(":");
		Print_Number(lclDefGateway[3]);
		#endif
						 
		/* Default gateway addr. */
 		IP4_ADDR(&x_gateway, lclDefGateway[0], lclDefGateway[1],
						lclDefGateway[2], lclDefGateway[3]);
	}
// 	else {
// 		/* Fixed IP mode. */
// 		/* Default IP addr */
// 		IP4_ADDR(&x_ip_addr, ETHERNET_CONF_IPADDR0, ETHERNET_CONF_IPADDR1,
// 				ETHERNET_CONF_IPADDR2, ETHERNET_CONF_IPADDR3);
// 
// 		/* Default subnet mask. */
// 		IP4_ADDR(&x_net_mask, ETHERNET_CONF_NET_MASK0, ETHERNET_CONF_NET_MASK1,
// 				ETHERNET_CONF_NET_MASK2, ETHERNET_CONF_NET_MASK3);
// 
// 		/* Default gateway addr. */
// 		IP4_ADDR(&x_gateway, ETHERNET_CONF_GATEWAY_ADDR0,
// 				ETHERNET_CONF_GATEWAY_ADDR1,
// 				ETHERNET_CONF_GATEWAY_ADDR2,
// 				ETHERNET_CONF_GATEWAY_ADDR3);
// 	}

	/* Add data to netif. */
	/* Use ethernet_input as input method for standalone lwIP mode. */
	/* Use tcpip_input as input method for threaded lwIP mode. */
	if (NULL == netif_add(&gs_net_if, &x_ip_addr, &x_net_mask, &x_gateway, NULL,
			ethernetif_init, tcpip_input)) {
		LWIP_ASSERT("NULL == netif_add", 0);
	}

	/* Setup callback function for netif status change. */
	netif_set_status_callback(&gs_net_if, status_callback);
	/* Setup callback function for netif link status change. */
	netif_set_link_callback(&gs_net_if, link_callback);
	
	/* Make it the default interface. */
	netif_set_default(&gs_net_if);

	/* Bring it up. */
	if (g_ip_mode == 2) {
		/* DHCP mode. */
		if (ERR_OK != dhcp_start(&gs_net_if)) {
			LWIP_ASSERT("ERR_OK != dhcp_start", 0);
		}
	}
	else {
		/* Static mode. */
		netif_set_up(&gs_net_if);
	}
}

/**
 * \brief Initialize ethernet stask.
 */
void init_ethernet(void)
{
	/* Initialize lwIP. */
	/* Call tcpip_init for threaded lwIP mode. */
	tcpip_init(NULL, NULL);
	dns_init();
	/* Set hw and IP parameters, initialize MAC too */
	ethernet_configure_interface();
	
	// Optional: Set DNS server manually
// 	ip_addr_t dnsserver;
// 	IP4_ADDR(&dnsserver, 8, 8, 8, 8); // Google's public DNS server
// 	dns_setserver(0, &dnsserver
}

/**
 * \brief Status callback used to print assigned IP address.
 *
 * \param netif Instance to network interface.
 */
// void status_callback(struct netif *netif)
// {
// 	if (netif_is_up(netif)) {
// 		strcat((char*)g_c_ipconfig, inet_ntoa(*(struct in_addr *)&(netif->ip_addr)));
// 		g_ip_mode = 3;
// 	}
// }

void status_callback(struct netif *netif)
{
	if (netif_is_up(netif))
	{
		if (netif->ip_addr.addr)
		{
			sprintf(gb_dev_ipAddress, "%03d%03d%03d%03d",
			ip4_addr1(&netif->ip_addr),
			ip4_addr2(&netif->ip_addr),
			ip4_addr3(&netif->ip_addr),
			ip4_addr4(&netif->ip_addr));
			
			dev_ip_add = netif->ip_addr.addr;
			gb_dev_ipAddress[12] = '\0';
			
			//#if DEBUG_ALL || DEBUG_ETHERNET
			Print_Message("\nAssigned IP address is : ");
			Print_Message(gb_dev_ipAddress);
			Print_Message("\n");
			//#endif
		}
		else
		__NOP();
	}
	else
	{
		//#if DEBUG_ALL || DEBUG_ETHERNET
		Print_Message("\nNetwork interface is down.");
		//#endif
	}
}

void link_callback(struct netif *netif)
{
	if (netif_is_link_up(netif))
	{
		Print_Message("Ethernet cable is connected (Link UP)\n");
	}
	else
	{
		Print_Message("Ethernet cable is NOT connected (Link DOWN)\n");
	}
}

/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: dflt_config.c
* Created By	: Harshit Agnihotri
* Created Date	: 05/04/2024
* Updated By    : Shirisha Ankam
* Date Updated  : 12/02/2025
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash
*					128 KB		RAM
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 05/04/2024
* Changes		: NA
*****************************************************************************/

 /* System Includes */
 #include "asf.h"
 #include "string.h"

/* User Includes */
 #include "dflt_config.h"
 #include "definitions.h"
 #include "group_config.h"
 #include "comn_config.h"
 #include "user_uart.h"

extern char gb_def_ipAddress[13];

/*****************************************************************************
 * Function name: void write_dflt_comn_config(void)
 * Returns	 : None
 * Arguments	 : None
 * Created by	 : Harshit Agnihotri
 * Date created : 06-04-2024
 * Description	 : Write the default configuration to communication settings.
 *              :
 * Notes		 : NA
 * Global Variables Affected : IpAddr, Port_Num, SbntMsk, Dflt_Gtwy.
 *****************************************************************************/
void write_dflt_comn_config(void)
{
	 #if DEBUG_UTILITY
	 Print_Message("\nSetting Default Configuration");
	 #endif
     
	 strcpy(gb_def_ipAddress, "192168001045");	// Set Default Ip_Address.
	 
	 strcpy(IpAddr.ip_addr_1, "192");                                           // Write the default values for ip address.
     strcpy(IpAddr.ip_addr_2, "168");
     strcpy(IpAddr.ip_addr_3, "001");
     strcpy(IpAddr.ip_addr_4, "045");
     IpAddr.ip_addr_1[3] = '\0';
     IpAddr.ip_addr_2[3] = '\0';
     IpAddr.ip_addr_3[3] = '\0';
     IpAddr.ip_addr_4[3] = '\0';

     strcpy(Port_Num, "00502");                                                 // Write the default values for port number.
     Port_Num[5] = '\0';

     strcpy(SbntMsk.sbnt_msk_1, "255");                                         // Write the default values for subnet mask.
     strcpy(SbntMsk.sbnt_msk_2, "255");
     strcpy(SbntMsk.sbnt_msk_3, "255");
     strcpy(SbntMsk.sbnt_msk_4, "000");
     SbntMsk.sbnt_msk_1[3] = '\0';
     SbntMsk.sbnt_msk_2[3] = '\0';
     SbntMsk.sbnt_msk_3[3] = '\0';
     SbntMsk.sbnt_msk_4[3] = '\0';

     strcpy(DfltGtwy.dflt_gtwy_1, "192");                                       // Write the default values for default gateway.
     strcpy(DfltGtwy.dflt_gtwy_2, "168");
     strcpy(DfltGtwy.dflt_gtwy_3, "001");
     strcpy(DfltGtwy.dflt_gtwy_4, "001");
     DfltGtwy.dflt_gtwy_1[3] = '\0';
     DfltGtwy.dflt_gtwy_2[3] = '\0';
     DfltGtwy.dflt_gtwy_3[3] = '\0';
     DfltGtwy.dflt_gtwy_4[3] = '\0';
}



/*****************************************************************************
 * Function name: Write_Default_End_Point
 * Returns	    : None
 * Arguments	 : None
 * Created by	 : Shirisha Ankam
 * Date created :  06-04-2025
 * Description	 : Write the default end point to communication settings.
 *              :
 * Notes		 : NA
 * Global Variables Affected : ServerCfg.server_endpoint, ServerCfg.auth_token.
 *****************************************************************************/
void Write_Default_End_Point(void)
{
	strcpy(ServerCfg.server_endpoint, "https://dor-uat.b-zone.in/");
	ServerCfg.server_endpoint[strlen(ServerCfg.server_endpoint)] = '\0';

	strcpy(ServerCfg.auth_token, "90d5f57fda74f00:1013503ebd52556");
	ServerCfg.auth_token[strlen(ServerCfg.auth_token)] = '\0';
}
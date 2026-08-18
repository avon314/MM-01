/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: definitions.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 18/08/2023.
* Module
* Description	: Defines constants and macros. This file is common file
*				  for the entire project.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef DEFINITIONS_H_
#define DEFINITIONS_H_

/***** Select Access Control Model Types *****/
#define DWAR_01			(01)	// DOR2200
#define DWAR_02			(02)	// (Access Control + Interlock) (16 Device) (DOR2300)
#define DWAR_03			(03)	// (Basic Access Control) (2 Device) (DOR2400-Weigand)
#define DWAR_04			(04)	// (Access Control + Interlock) (2 Device) (DOR2500-IEX)
#define MM_01			(05)	// (Monitor Module-01, 8 Inputs + 4 Outputs + Modbus)
#define MM_02			(06)	// (Monitor Module-02, 16 Inputs + 4 Outputs)

#define MODEL_TYPE		(MM_01)
/***** End of Select Access Control Model Types *****/

/***** Select Access Control Device Types *****/
#define ACS_CNTRL		(01)	// Access Control Device
#define ATTENDANCE		(02)	// Attendance Device
#define CANTEEN			(03)	// Canteen Device
#define MONITOR			(04)	// Monitoring Device
#define VIRDHI			(05)	// Virdhi

#define DEVICE_TYPE		(MONITOR)
/***** End of Select Access Control Device Types *****/

/***** User defined Definitions *****/
#define uTRUE				(1)
#define uFALSE				(0)
#define uSET				(1)
#define uRESET				(0)
#define uENABLE				(1)
#define uDISABLE			(0)
#define IP_ENABLED			(1)
#define IP_DISABLED			(0)

/***** ENABLE-DISABLE AREA RULES *****/
#define EN_DIS_DOOR_RULES		(uENABLE)
#define EN_DIS_AREA_RULES		(uENABLE)
#define EN_DIS_ILOK_RULES		(uENABLE)

/***** Make debug messages ON-OFF *****/
#define DEBUG_ALL				(uENABLE)

#define DEBUG_ETHERNET			(uDISABLE)
#define DEBUG_ARP_PKT			(uDISABLE)
#define DEBUG_IP_PKT			(uDISABLE)
#define DEBUG_TCP_IP			(uDISABLE)
#define DEBUG_INTERLOCK			(uDISABLE)
#define DEBUG_APP_OSDP_TX		(uDISABLE)
#define DEBUG_APP_OSDP_RX		(uDISABLE)
#define DEBUG_EMERGENCY			(uDISABLE)
#define DEBUG_FIRE_FUN			(uDISABLE)
#define DEBUG_AUX_INPUT			(uDISABLE)
#define DEBUG_ONBOARD_KEY		(uDISABLE)
#define DEBUG_EXT_EEPROM		(uDISABLE)
#define DEBUG_OP				(uENABLE)
#define DEBUG_CONFIG_MODE		(uDISABLE)
#define DEBUG_UTILITY			(uDISABLE)
#define DEBUG_APP_EEPROM		(uDISABLE)
#define DEBUG_DIG_IP			(uDISABLE)
#define DEBUG_RTC				(uDISABLE)
#define DEBUG_POWER_ON			(uDISABLE)


/***** DEV TYPE INFO *****/
#define DOR2100					(1)
#define DOR2100R				(2)
#define DOR2100P				(3)
#define DOR27					(4)
#define DOR27R					(5)
#define REX						(6)
#define BEP2					(7)
#define AY_H6355BT				(8)
/***** End of DEV TYPE INFO *****/

/***** Interface Type *****/
#define OSDP_V2_AVON			(0)
#define OSDP_V2_STD				(1)
#define WEIGAND					(2)

#define IFACE_REQ				(OSDP_V2_STD)
#define IFACE_LC				(OSDP_V2_AVON)
/***** End of an Interface Type *****/

/***** LED & BUZZER Control Flags *****/
#define SET0X80FLAG				(0x80)
#define RST0X80FLAG				(0x7F)
/***** End of LED & BUZZER Control Flags *****/

#ifndef SET_BIT
#define SET_BIT(num, bitposition) ((*num) |= (1u << (bitposition)))
#endif
#ifndef CLEAR_BIT
#define CLEAR_BIT(num, bitposition)  ((*num) &= ~(1u << (bitposition)))
#endif
#ifndef IS_BIT_SET
#define IS_BIT_SET(num, bitposition)  (((num) & (1u << (bitposition))) != 0)
#endif

#define COMBINE_BYTES(hb, lb) (((U16)(hb) << 8) | (U16)(lb))     // Macro to combine two bytes into a 16-bit value.

#define FLAG_SET      (1)
#define FLAG_RST      (0)

#define TRUE          (1)
#define FALSE         (0)

#define READ_U32_FROM_BUF(buf) \
( ((uint32_t)(buf)[0] << 24) | ((uint32_t)(buf)[1] << 16) | \
((uint32_t)(buf)[2] << 8)  |  (uint32_t)(buf)[3] )

#endif /* DEFINITIONS_H_ */
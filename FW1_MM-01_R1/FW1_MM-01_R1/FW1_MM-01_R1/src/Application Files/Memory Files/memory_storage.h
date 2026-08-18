#include "asf.h"

/* General macros */
#ifndef FLAG_SET
#define FLAG_SET	(1)
#endif
#ifndef FLAG_RST
#define FLAG_RST	(0)
#endif

#define MAX_DATA_PACKETS        (100)

/***** Macros for Configuration Data *****/
#define UT_EEP_START_PAGE		(0)		// Page 0 is used to store configuration data (Utility)
#define UT_EEP_END_PAGE		    (0)		//End page 0
#define UT_EEP_PAGE_OFFSET		(0)		//Page 0 offset 0
#define UT_EEP_START_LOC		(0)		// Start address (Byte location for page 0).
#define UT_EEP_END_LOC          (255)	// End address (Byte location for page 0).
/***** End of macros for Configuration Data *****/


/***** Macros for Modbus Setting configuration *****/
#define MS_DATA_SIZE			(6)		// With 2 bytes of CRC
// ---------------------------- ***** -------------------------------- //
#define MS_EEP_START_PAGE		(0)		// Page 0 is used to store configuration data (Utility)
#define MS_EEP_END_PAGE		    (0)
#define MS_EEP_PAGE_OFFSET		(256)	//page 0 offset 256
#define MS_EEP_START_LOC        (256)	// Start address (Byte location for page 0).
#define MS_EEP_END_LOC          (261)	// End address (Byte location for page 0)
/***** End of macros for madbus setting configuration data *****/


/***** Macros for Modbus Data configuration *****/
#define MP_SIZE_PER_DATA        (8)      
#define MP_DATA_SIZE			(802)	//MP_SIZE_PER_DATA + MAX_DATA_PACKETS + With 2 bytes of CRC 
// ---------------------------- ***** -------------------------------- //
#define MP_EEP_START_PAGE		(1)     //page 1 and 2 are used to stored modbus data packets data
#define MP_EEP_END_PAGE		    (2)
#define MP_EEP_PAGE_OFFSET		(0)
#define MP_EEP_START_LOC		(512)	// Start address (Byte location for page 1).
#define MP_EEP_END_LOC		    (1313)	// End address (Byte location for page 2)
/***** End of macros for madbus configuration data *****/


/***** Macros for Input configuration data *****/
#define IP_DATA_SIZE			(26)	// With 2 bytes of CRC
// ---------------------------- ***** -------------------------------- //
#define IP_EEP_START_PAGE		(3)		// Page 3 is used to store input configuration data 
#define IP_EEP_END_PAGE		    (3)
#define IP_EEP_PAGE_OFFSET		(0)     //Start from offset 0
#define IP_EEP_START_LOC		(1536)  // Start address (Byte location for page 3).
#define IP_EEP_END_LOC		    (1561)	// End address (Byte location for page 3)
/***** End of macros for Input configuration data *****/


/***** Macros for Output configuration data *****/
#define OP_DATA_SIZE			(34)	// 8 * 4 With 2 bytes of CRC
// ---------------------------- ***** -------------------------------- //
#define OP_EEP_START_PAGE		(3)     // Page 3 is used to store output configuration data 
#define OP_EEP_END_PAGE		    (3)
#define OP_EEP_PAGE_OFFSET		(256)   //Start from offset 256
#define OP_EEP_START_LOC		(1792)  // Start address (Byte location for page 3).
#define OP_EEP_END_LOC		    (1825)  // End address (Byte location for page 3).	
/***** End of macros for Output configuration data *****/


/***** Macros for Periodic Time Storage*****/
//Period value is storing in seconds in 2 bytes for max 100 dpts
//So, 2 Bytes * 100 dpt = 200 Bytes.
#define MP_PERIODIC_TIME_SIZE   (2)
#define MP_TOTAL_PRIODIC_TIME   (2 * MAX_DATA_PACKETS)
// ---------------------------- ***** -------------------------------- //
#define MP_PERIODIC_TIME_PAGE	(4)    // Page 4 is used to store periodic time data 
#define MP_PERIODIC_END_PAGE	(4)
#define MP_PERIODIC_TIME_OFFSET	(0)    //page 4 offset 0
#define MP_PERIODIC_TIME_LOC	(2048) // Start address (Byte location for page 4)
#define MP_PERIODIC_END_LOC	    (2247) // End address (Byte location for page 4)
/***** End of macros for Periodic Time Storage *****/


/***** Macros for Specific Time Storage*****/
//Time is stored in date only in 1 byte as previous date for max 100 dpts.
//So, 1 * 100 = 100 Bytes.
#define MP_SPECIFIC_TIME_SIZE   (1)
#define MP_TOTAL_SP_TIME_SIZE   (1 * MAX_DATA_PACKETS)
// ---------------------------- ***** -------------------------------- //
#define MP_SPECIFIC_TIME_PAGE	(4)    // Page 4 is used to store specific time data 
#define MP_SPECIFIC_END_PAGE	(4)
#define MP_SPECIFIC_TIME_OFFSET	(256)  //page 4 offset 256
#define MP_SPECIFIC_TIME_LOC	(2304) // Start address (Byte location for page 4)
#define MP_SPECIFIC_END_LOC	    (2403) // End address (Byte location for page 4)
/***** End of macros for Specific Time Storage *****/


/***** Macros for Change in Value Storage*****/
//To fetch change in a value of DPT a last value is stored in 4 bytes for 100 dpts.
//So, 4 * 100 = 400 bytes
#define MP_LAST_VAL_UPDATE_SIZE (4)
#define MP_TOTAL_LV_DATA_SIZE   (4 * MAX_DATA_PACKETS)
// ---------------------------- ***** -------------------------------- //
#define MP_LAST_VAL_UPDATE_PAGE	(5)    // Page 5 is used to store last change in value data 
#define MP_LAST_VAL_END_PAGE	(5)
#define MP_LAST_VAL_OFFSET	    (0)    //page 5 offset 0
#define MP_LAST_VAL_UPDATE_LOC	(2560) // Start address (Byte location for page 5)
#define MP_LAST_VAL_END_LOC	    (2959) // End address (Byte location for page 5)
/***** End of macros for Change in Value Storage *****/


/***** Macros for Power On States *****/
//Base page is 6
#define OUTPUT_STATE_LOC        (3072)  //requires 1 byte x 4 outputs So, 1 x 4 = 4 Bytes.
//boundary is                   (3075)
#define INPUT_STATE_LOC         (3328)  //(offset 256) requires 1 byte x 8 inputs So, 1 x 8 = 8 Bytes. + 2 Bytes of CRC = 10 Bytes.
//boundary is                   (3337)

/***** Macros for SI(Safety & Intruder) Configuration *****/
//Only one output can be configured at a time so, requires 3 bytes to maintain configuration. + 2 Bytes of CRC = 5 Bytes.
#define SI_EEP_MAX_BYTE			(5)
// ---------------------------- ***** -------------------------------- //
#define SI_EEP_START_PAGE		(7)     // Page 7 is used to store SI(Safety & Intruder) configuration data
#define SI_EEP_END_PAGE		    (7)
#define SI_EEP_OFFSET			(0)
#define SI_EEP_START_LOC		(3584)  // Start address (Byte location for page 7).
#define SI_EEP_END_LOC		    (3588)  // End address (Byte location for page 7).
/***** End of macros SI(Safety & Intruder) Configuration *****/

/***** Macros for AM(Arm Dis-Arm) Configuration by the server. *****/
// Requires 9 bytes to store Arm Disarm information in the memory. So, 9 bytes + 2 CRC bytes = 11 Bytes.
#define AM_EEP_MAX_BYTE			(11)
// ---------------------------- ***** -------------------------------- //
#define AM_EEP_START_PAGE		(7)     // Page 7 is used to store AM(Arm Dis-Arm) configuration data
#define AM_EEP_END_PAGE		    (7)
#define AM_EEP_OFFSET			(256)
#define AM_EEP_START_LOC		(3840)  // Start address (Byte location for page 7).
#define AM_EEP_END_LOC		    (3850)  // End address (Byte location for page 7).
/***** End of macros for AM(Arm Dis-Arm) Configuration by the server. *****/

/*****************************************************
		EMPTY SPACE (Reserved for future use.) 
		page 8 to page 10 (3 pages reserved)
*****************************************************/

/***** Macros for Input Transaction log *****/
// 9 bytes are required to store Transaction log, storing 50,000 transactionsSo, requires 879 total pages..
#define IS_BUFFER_LEN			(9)		    // Length of the buffer to store in flash for a single input state Transaction.
// ---------------------------- ***** -------------------------------- //
#define IS_EEP_START_PAGE		(11)		// page 11 offset 0
#define IS_EEP_END_PAGE			(890)		// Last Page
#define IS_EEP_START_LOC		(5632)	    // Byte Address / EEP location.
#define IS_EEP_END_LOC			(455631)	// EEP last location IS storage. (9 * 50,000 (transactions) = 4,50,000 byts required).
 /***** End for Macros for Input Transaction log *****/
 
 
 /*****************************************************
		EMPTY SPACE (Reserved for future use.) 
		page 891 to page 1769 (879 pages reserved)
*****************************************************/
 
 
 /***** Macros for Data Packets Transaction log *****/
 // 12 bytes are required to store Transaction log, storing 50,000 transactionsSo, requires 1172 total pages..
 #define MD_BUFFER_LEN			(12)		// Length of the buffer to store in flash for a single modbus Transaction.
 // ---------------------------- ***** -------------------------------- //
 #define MD_EEP_START_PAGE		(1770)		// page 1770 offset 0
 #define MD_EEP_END_PAGE		(2942)		// Last Page
 #define MD_EEP_START_LOC		(906240)	// Byte Address / EEP location.
 #define MD_EEP_END_LOC			(1506239)	// EEP last location MD storage. (12 * 50,000 (transactions) = 6,00,000 byts required).
 /***** End for Macros for Data Packets Transaction log *****/
 
  /*****************************************************
		EMPTY SPACE (Reserved for future use.) 
		page 2943 to page 4114 (1172 pages reserved)
*****************************************************/
 
 /***** Macros for DI (Device Info) Configuration *****/
 #define DI_DATA_SIZE			(39)	// With 2 bytes of CRC.
 #define DI_DNS_LENGTH			(30)	// 30 Bytes long DNS can be entered.
 // ---------------------------- ***** -------------------------------- //
 #define DI_EEP_START_PAGE		(4115)
 #define DI_EEP_END_PAGE		(4115)     //page 4115 offset 0
 #define DI_EEP_START_LOC		(2106880)  // Start address (Byte location for page 4115)
 #define DI_EEP_END_LOC			(2106918)  // End address (Byte location for page 4115)
 /***** End of Macros for DI (Device Info) Configuration *****/
 
// 5 bytes are required to store, last location of stored IS. (Base page is 4116).
#define IS_STORED_LAST_ADDS		(2107392)	// EEP location to store IS last location.
// boundary is					(2107396)
// ---------------------------------------------------------------------------------//
// 5 bytes are required to store, last sent address of stored IS. (Base page is 4116).
#define IS_SENT_LAST_ADDS		(2107397)	// EEP location to store TL sent till this address.
// boundary is					(2107401)
// ---------------------------------------------------------------------------------//
// 1 byte require for a flag.	(Base page is 4116).
#define IS_BOUNDARY_FLAG		(2107402)	// EEP location to store if boundary reached while storing IS Log.
/***** End of macros for Transaction log *****/


//Requires 3 bytes to store last stored address of the IS log to retain while storing next time. So, 3 Bytes + 2 CRC = 5 bytes
// 5 bytes are required to store, last location of stored MD. (Base page is 4116).
#define MD_STORED_LAST_ADDS		(2107403)	// EEP location to store MD last location. 
// boundary is					(2107407)
// ---------------------------------------------------------------------------------//
// 5 bytes are required to store, last sent address of stored MD. (Base page is 4116).
#define MD_SENT_LAST_ADDS		(2107408)	// EEP location to store DP sent till this address.
// boundary is					(2107412)
// ---------------------------------------------------------------------------------//
// 1 byte require for a flag.	(Base page is 4116).
#define MD_BOUNDARY_FLAG		(2107413)	// EEP location to store if boundary reached while storing MD Log.
/***** End of macros for Transaction log *****/


// 6 bytes are required to store, date & time of last received configuration. (Base page is 4116).
#define IS_LAST_DT_LOC			(2107414)	// EEP location to store the data of last date & time.
// boundary is					(2107419)
// ---------------------------------------------------------------------------------//

/***** Macros for ARM_DISARM VALUE *****/
//Requires 5 byte with 2 bytes of CRC values.
//1 byte for arm status. 1 byte for arm activated by system or by server, 1 byte for alarm state. 
#define ARM_MAX_BYTES			(5)
// ---------------------------- ***** -------------------------------- //
#define ARM_EEP_START_PAGE		(4116)
#define ARM_EEP_END_PAGE		(4116)     // page 4116
#define ARM_EEP_START_LOC		(2107420)  // Byte Address.
#define ARM_EEP_END_LOC			(2107424)  // Byte Address.
/***** End of Macros for ARM_DISARM VALUE *****/

/***** Macros for ALARM VALUE *****/
//Requires 1 byte to store in memory before power off for alarm value.
// ---------------------------- ***** -------------------------------- //
#define ALRM_EEP_START_PAGE		(4116)
#define ALRM_EEP_END_PAGE		(4116)     // page 4116
#define ALRM_EEP_START_LOC		(2107421)  // Byte Address.
#define ALRM_EEP_END_LOC		(2107421)  // Byte Address.
/***** End of Macros for ALARM VALUE *****/

/*****************************************************
		EMPTY SPACE (Reserved for future use.) 
		page 4117 to page 8191 (4075 pages unused)
		8191 is the last page for eeprom
*****************************************************/

/***** Macros for Debug *****/
#define DEBUG_AC_MEM_IP_LOOP    (0)
#define DEBUG_AC_MEM_IP         (0) 
#define DEBUG_AC_MEM_OP_LOOP    (0)
#define DEBUG_AC_MEM_OP         (0)
/***** End of Macros for Debug *****/

/***** Bootloader Bytes *****/
// A page (6909) is reserved for boot loader info storage. 
#define BL_APP_STATUS_LOC		(3537408)	// is the 6909 page address
/***** End of Bootloader Bytes *****/

void Read_DI_Data_From_EEPROM(U8 *di_data);
void Read_MS_Data_From_EEPROM(void);
void Read_IP_Data_From_EEP(void);
void Read_OP_Data_From_EEP(void);
void Update_SI_Variables(void);
void Update_AM_ByServer_Variables(void);
///////////////
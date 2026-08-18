/*
 * bootloader.c
 *
 * Created: 14-07-2025 10:52:15 AM
 *  Author: admin
 */
#include "bootloader.h"
#include "definitions.h"
#include "ext_eeprom_1.h"
#include "memory_storage.h"

U8 boot_flag[8]           = {0};

extern volatile uint16_t jsec ;

/*****************************************************************************
* Function name	: void Jump_To_Bootloader( void)
* Returns		: Nothing.
* Arguments    	: Nothing.
* Created By	: Suryaprakash Parmeshwar Shendge
* Created Date	: 14/07/2025.
*
*
* Description	:  Function to jump to bootloader.
*
*               :
* Notes			: NA
* Global Variables Affected : None .
*****************************************************************************/
void Jump_To_Bootloader(void)
{
	uint32_t MSP = *(uint32_t *)(BOOT_START_ADDRESS) ;
	uint32_t reset_vector = *(uint32_t *)(BOOT_START_ADDRESS + 4);

	deinit_peripherals();

	// Clear pending system exceptions (PendSV, SysTick, etc.)
	SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
	SCB->ICSR |= SCB_ICSR_PENDSTCLR_Msk;

	SCB->VTOR = BOOT_START_ADDRESS;          // Set vector table

	__set_MSP(MSP);                  // Set Main Stack Pointer
	Print_Message("\nMSP Value : ");
	Print_Number(MSP);
	__set_CONTROL(0);                // Use MSP, privileged mode
	__ISB();                         // Flush instruction pipeline

	void (*app_reset_handler)(void) = (void (*)(void))reset_vector;
	__enable_irq();			// enable all interrupts before jumping to application
	app_reset_handler();


}

/*****************************************************************************
* Function name	: void Run_Bootlodar(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created By	: Suryaprakash Parmeshwar Shendge
* Created Date	: 14/07/2025..
*
*
* Description	:  Function to jump from application to bootloade
*
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Run_Bootlodar(void)
{
	#if DEBUG_BOOT_APP
	Print_Message("\n\nJumping To Bootloader............. ");
	#endif
	
	boot_flag[0] = NEW_FIRM_VALUE;    // Update Boot flag
	EEPROM_Byte_Write(BL_APP_STATUS_LOC, boot_flag, 1); // Write Boot flag into external Flash

	tc_stop(TC0, 0);          // Stop Timer Counter 0 (assuming TC0 was used by bootloader)
	tc_stop(TC0, 1);         // Stop Timer Counter 1 (assuming TC0 was used by bootloader)

	__disable_irq();        // Disable all interrupts before jumping to application

	Jump_To_Bootloader() ;

}

/*****************************************************************************
* Function name	: void Check_Boot_Command(void)
* Returns		: Nothing.
* Arguments    	: None.
* Created By	: Suryaprakash Parmeshwar Shendge
* Created Date	: 14/07/2025..
*
*
* Description	:  Function checking boot command is receive
*
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Check_Boot_Command(void)
{
//	#ifdef DEBUG_BOOT_APP
//	Print_Message("\n\nWating for Boot Command......");
//	#endif
	            
	if (jsec >= 3)
	{
		#if DEBUG_BOOT_APP
		Print_Message("\n\nBoot Command Received");
		#endif
		
		Run_Bootlodar() ;      // Jump into Bootloader
	}
}


/*****************************************************************************
* Function name	: void Download_New_Firmware(U8 *bin_buffer, int Byte_Count)
* Returns			: NA
* Arguments    	: U8 *bin_buffer ---> buffer reciving from tcp server
*				: int Byte_Count ---> Toatal number of bytes recived
* Created by		: Anup Silvan Mascarenhas
* Date created		: 11/04/2025.
*
* Description		: This function is written to get ready to receive new firmware/.bin bin file
*				:
* Notes			: Call this Function in tcp_server_recv() from httpd.c driver
* Global Variables Affected :gb_fRead_Array	---> Buffer Recives from Flash
*****************************************************************************/
//void Download_New_Firmware(U8 *bin_buffer, int Byte_Count)
//{
//	U8 updt_firmware[ACK_FRM_LEN] = {0};
//
//	updt_firmware[0] = BTLD_SOF;
//	updt_firmware[1] = ((ACK_FRM_LEN >> 8)& 0xFF);
//	updt_firmware[2] = (ACK_FRM_LEN & 0xFF);
//	updt_firmware[3] = BTLD_DEST_ADD;
//	updt_firmware[4] = BTLD_SRC_ADD;
//	updt_firmware[5] = FUNC_ID;
//	updt_firmware[6] = BTLD_EOF;
//
//	CRC_Calculate_Check(updt_firmware,(ACK_FRM_LEN - 2),CRC_CALC);
//	updt_firmware[6] = gb_Temp_CRC[0];
//	updt_firmware[7] = gb_Temp_CRC[1];
//	updt_firmware[8] = BTLD_EOF;
//
//	#if DEBUG_BOOT_APP
//	Send_Frame_On_UART(bin_buffer,Byte_Count);
//	Send_Frame_On_UART(updt_firmware,ACK_FRM_LEN);
//	#endif
//
//	if(memcmp(updt_firmware,bin_buffer,Byte_Count) == 0)
//	{
//		btld_ack(CRC_OK,1);
//		gb_download_frame = 1;
//
//	}
//	else
//	{
//		btld_ack(CRC_NOK,0);
//		gb_download_frame = 0;
//
//		#if DEBUG_BOOT_APP
//		Print_Message("Frame NOK");
//		#endif
//	}
//}

/*****************************************************************************
* Function name	: void deinit_peripherals(void)
* Returns			: NA
* Arguments    	: NA
* Created by		: Anup Silvan Mascarenhas
* Date created		: 11/04/2025.
*
* Description		: This function disables IRQ when booted to new firmware
*				:
* Notes			: NA
* Global Variables Affected : NA
*****************************************************************************/
void deinit_peripherals(void) {
	// Disable SysTick
	SysTick->CTRL = 0;
	SysTick->LOAD = 0;
	SysTick->VAL  = 0;

	// Disable all NVIC interrupts
	for (int i = 0; i < 8; i++) {
		NVIC->ICER[i] = 0xFFFFFFFF;
		NVIC->ICPR[i] = 0xFFFFFFFF;
	}

	// Optional: reset peripherals if needed
	// PM->PM_CTRLA.bit.SWRST = 1; // Example for SAMD, not SAM4E
}

/*****************************************************************************
* Function name	: void btld_ack(U8 err, U8 fuc_id)
* Returns			: NA
* Arguments    	: U8 err		---> Error code
*				: U8 fuc_id	---> runs in two functionalities
* Created by		: Anup Silvan Mascarenhas
* Date created		: 11/04/2025.
*
* Description		: This Function is written to send back ack to tcp-ip
*				:
*				:
* Notes			: NA
* Global Variables Affected : gb_fRead_Array	---> Buffer received from Extrenal Flash
*****************************************************************************/
//void btld_ack(U8 err, U8 fuc_id)
//{
//	U8 len     = fuc_id ? FRM_LEN : ACK_FRM_LEN;
//	U8 crc_idx = fuc_id ? 7 : 6;
//	U8 eof_idx = crc_idx + 2;
//
//	gb_ack_buff[0] = BTLD_SOF;
//	gb_ack_buff[1] = (len >> 8) & 0xFF;
//	gb_ack_buff[2] = len & 0xFF;
//	gb_ack_buff[3] = BTLD_DEST_ADD;
//	gb_ack_buff[4] = BTLD_SRC_ADD;
//	gb_ack_buff[5] = err;
//
//	if (fuc_id)
//	{
//		gb_ack_buff[6] = FUNC_ID;
//		gb_ack_buff[7] = BTLD_EOF;
//	}
//	else
//	gb_ack_buff[6] = BTLD_EOF;
//
//	#if DEBUG_NEW_APP
//	Print_Message("\nokokokokokok ______>>>>>");
//	Print_Frame_On_UART(gb_ack_buff,(len - 2));
//	#endif
//
//	CRC_Calculate_Check(gb_ack_buff, (len - 2) , CRC_CALC);
//
//	#if DEBUG_NEW_APP
//	Print_Message("\n??????????? : ");
//	Print_ASCII_HEX(gb_Temp_CRC[0]);
//	Print_Message(" : ");
//	Print_ASCII_HEX(gb_Temp_CRC[1]);
//	#endif
//
//	gb_ack_buff[crc_idx]     = gb_Temp_CRC[1];
//	gb_ack_buff[crc_idx + 1] = gb_Temp_CRC[0];
//	gb_ack_buff[eof_idx]     = BTLD_EOF;
//
//	#if DEBUG_NEW_APP
//	Print_Message("\nAck ______>>>>>");
//	Print_Frame_On_UART(gb_ack_buff,len);
//	#endif
//}
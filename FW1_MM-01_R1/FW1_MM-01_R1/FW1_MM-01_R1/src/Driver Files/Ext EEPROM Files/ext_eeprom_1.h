/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED
*
* Module Name	: ext_eeprom_1.h
* Created By	: Shirisha Ankam
* Created Date	: 06/01/2025
* Module
* Description	: Header file for ext_eeprom.c
				  Defines constants and macros for ext_eeprom_1.c
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date: 30/03/2024
* Changes		: NA
*****************************************************************************/
#ifndef EXT_EEPROM_1_H_
#define EXT_EEPROM_1_H_

#include "asf.h"

// SPI Chip Select and Write Protect Pins
#define EP_CS_PIN_LOW				pio_clear(PIOA, PIO_PA16)
#define EP_CS_PIN_HIGH				pio_set(PIOA, PIO_PA16)

#define WR_PTR_LOW					pio_clear(PIOA, PIO_PA21)
#define WR_PTR_HIGH					pio_set(PIOA, PIO_PA21)

// SPI Data Transfer Macros
#define Data_To_SPI(data, len)		spi_write_packet(SPI, data, len)

// EEPROM Configuration
#define EP_MAX_PAGES				(8191)
#define MX_BYTE_SIZE				(4194304)
#define EEPROM_PAGE_SIZE			(512)
#define EEPROM_MAX_PAGES			(MX_BYTE_SIZE / EEPROM_PAGE_SIZE)
#define READ_ONCE					(EEPROM_PAGE_SIZE * 2)	// Read Upto 2 pages at once.

// EEPROM Commands
#define EEPROM_CMD_WRITE_ENABLE		(0x06)
#define EEPROM_CMD_WRITE_DISABLE	(0x04)
#define EEPROM_CMD_READ_STATUS		(0x05)
#define EEPROM_CMD_READ_DATA		(0x03)
#define EEPROM_CMD_WRITE_DATA		(0x02)
#define EEPROM_CMD_PAGE_ERASE		(0xDB)
#define EEPROM_CMD_CHIP_ERASE		(0xC7)

// EEPROM debug commands
#define DEBUG_EEPROM_STATUS			(0)
#define DEBUG_EEPROM_PWRITE			(0)
#define DEBUG_EEPROM_PREAD			(0)
#define DEBUG_EEPROM_ENABLE			(0)
#define DEBUG_EEPROM_BWRITE			(0)
#define DEBUG_EEPROM_BREAD			(0)
#define DEBUG_EEPROM				(0)

// Public Functions
void EXT_EEPROM_Init(void);
uint8_t EEPROM_Page_Write(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len);
uint8_t EEPROM_Page_Read(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len);
uint8_t EEPROM_Byte_Write(int loc, uint8_t *data, uint32_t len);
uint8_t EEPROM_Byte_Read(uint32_t loc, uint8_t *buffer, uint32_t len);
uint8_t EEPROM_Page_Erase(uint32_t page_num);
void EEPROM_Chip_Erase(void);

uint8_t Is_eeprom_Ready(void);
void Wait_For_eeprom_Ready(void);
uint8_t Read_eeprom_Status_Register(void);
void Write_eeprom_Status_Register(uint8_t status);
void Write_Enable(void);
void Disable_Full_Protection(void);
void Enable_Full_Protection(void);
void Wait_For_Write_Completion(void);
void Write_Disable(void);
uint8_t EEPROM_Page_Write_Without_HW(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len);

#endif // EEPROM_DRIVER_H

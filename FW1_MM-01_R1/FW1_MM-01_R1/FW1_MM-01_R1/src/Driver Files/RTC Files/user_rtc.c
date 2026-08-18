/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_rtc.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 14/08/2024.
* Module
* Description	: RTC driver file, in which programmer can set the RTC time, date and day
*				  and also read the time, date and day from the DS1339A RTC chip.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 14/08/2024.
* Changes		: NA.
*****************************************************************************/
#include "asf.h"
#include "user_rtc.h"
#include "definitions.h"
#include "user_uart.h"
#include "string.h"

/***** Definitions *****/
#define DS1339A_SLAVE_ADDRESS (0x68)

#define DS1339A_SEC_REG		(0x00)
#define DS1339A_MIN_REG		(0x01)
#define DS1339A_HOUR_REG	(0x02)
#define DS1339A_DAY_REG		(0x03)
#define DS1339A_DATE_REG	(0x04)
#define DS1339A_MONTH_REG	(0x05)
#define DS1339A_YEAR_REG	(0x06)
#define DS1339A_CONTROL_REG	(0x0E)
#define DS1339A_STATUS_REG	(0x0F)

U8 gb_read_stsReg_at_pwrOn_f = 1;
volatile U8 gb_rtc_1secInt_triggered_f = 0;


U8 gb_rtcTimeArr[3] = {0};
U8 gb_rtcDateArr[4] = {0};
U8 rtcUpdateArr[6] = {0};
U8 gb_rtc_time_update_f = 0;
U8 gb_rtc_dm_1sec_f = 0;
U8 gb_rtc_one_sec_flag = 0;
U8 gb_rtc_1sec_IS_set_f = 0;
U8 gb_rtc_1sec_delay_f = 0;


/*****************************************************************************
* Function Name  : DecimalToBCD
* Returns        : int - The BCD (Binary-Coded Decimal) equivalent of the input decimal value.
* Arguments      : int Decimal ---> The decimal number to be converted to BCD format.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Converts a decimal number into its BCD (Binary-Coded Decimal) equivalent.
*                  In BCD, each digit of the decimal number is represented by its own binary
*                  sequence, making it easy to encode and decode individual digits.
*
* Notes          : This function assumes the input decimal number is less than 100.
* Global Variables Affected : NA
*****************************************************************************/
static int DecimalToBCD(int Decimal)
{
	// Divide the decimal number by 10 to get the tens digit, shift it left by 4 bits,
	// and combine it with the ones digit (Decimal % 10) using a bitwise OR.
	return (((Decimal / 10) << 4) | (Decimal % 10));
}

/*****************************************************************************
* Function Name  : BCDToDecimal
* Returns        : int - The decimal equivalent of the input BCD (Binary-Coded Decimal) value.
* Arguments      : int BCD ---> The BCD value to be converted to decimal format.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Converts a BCD (Binary-Coded Decimal) value into its decimal equivalent.
*                  In BCD format, each digit of a decimal number is represented by a 4-bit binary
*                  value. This function decodes the BCD format into a regular decimal integer.
*
* Notes          : The input BCD value should represent a valid decimal number encoded in BCD.
* Global Variables Affected : NA
*****************************************************************************/
static int BCDToDecimal(int BCD)
{
	// Shift the BCD value right by 4 bits to extract the tens digit (upper nibble),
	// then multiply by 10 to get its decimal equivalent.
	// Use bitwise AND with 0xF to extract the ones digit (lower nibble),
	// and add it to the tens digit to form the final decimal number.
	return (((BCD >> 4) * 10) + (BCD & 0xF));
}

/*****************************************************************************
* Function Name  : Update_I2C_Packet_To_Send
* Returns        : void - This function does not return a value.
* Arguments      : U8 RegAdd ---> The register address on the I2C slave device where the data will be written.
*				   U8 byteValue ---> The decimal value that needs to be converted to BCD and sent over I2C.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
* Description    : Prepares and sends a data packet over I2C to the specified register address of
*                  the DS1339A RTC. The provided decimal value is first converted to BCD format
*                  before being sent.
*
* Notes          : The function assumes that the TWI (I2C) interface has already been initialized.
* Global Variables Affected : NA
*****************************************************************************/
static void Update_I2C_Packet_To_Send(U8 RegAdd, U8 byteValue)
{
	// Convert the given decimal value to BCD format before sending over I2C.
	uint8_t bcd_seconds = DecimalToBCD(byteValue);
	
	// Declare a structure to hold the TWI (I2C) packet information.
	twi_package_t packet_write;

	// Initialize the TWI packet structure fields:
	packet_write.addr[0] = RegAdd;              // Set the I2C register address where data will be written.
	packet_write.addr_length = 1;               // Specify the address length (1 byte).
	packet_write.buffer = &bcd_seconds;         // Set the buffer pointer to the BCD-converted data.
	packet_write.length = 1;                    // Set the length of data to be written (1 byte).
	packet_write.chip = DS1339A_SLAVE_ADDRESS;  // Set the I2C address of the DS1339A RTC device.
	
	// Attempt to send the data packet over I2C using the TWI interface.
	if (twi_master_write(TWI0, &packet_write) != TWI_SUCCESS)
	{
		// If the write operation fails, print an error message.
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nTWI writing is unsuccessful.");
		#endif
		
		return;
	}
	else
	{
		// If the write operation succeeds, execute a no-operation instruction (for debugging or delay).
		__NOP();
	}
}

/*****************************************************************************
* Function Name  : Update_I2C_Packet_To_Read
* Returns        : U8 - The value read from the specified register in decimal format.
* Arguments      : U8 RegAdd ---> The register address from which to read the data.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads a byte of data from the specified register of the DS1339A RTC.
*                  The read value is converted from BCD to decimal format before being returned.
*
* Notes          :
*					- The function uses the I2C master interface to read from the DS1339A RTC.
*					- In case of a read failure, it prints an error message and returns 0.
* Global Variables Affected : NA
*****************************************************************************/
static U8 Update_I2C_Packet_To_Read(U8 RegAdd)
{
	uint8_t read_val = 0;
	
	// Prepare the TWI package for reading data from the specified register
	twi_package_t packet_read;
	packet_read.addr[0] = RegAdd;                  // Register address to read from
	packet_read.addr_length = 1;                   // Length of the register address
	packet_read.buffer = &read_val;                // Buffer to store the read value
	packet_read.length = 1;                       // Number of bytes to read
	packet_read.chip = DS1339A_SLAVE_ADDRESS;     // Slave address of the DS1339A RTC
	
	// Perform the TWI read operation
	if (twi_master_read(TWI0, &packet_read) != TWI_SUCCESS)
	{
		// If the read operation fails, print an error message and return 0
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nTWI is read unsuccessfully.");
		#endif
		return 0;
	}
	
	// Convert the read value from BCD to decimal and return it
	return BCDToDecimal(read_val);
}

/*****************************************************************************
* Function Name  : Read_Status_Register
* Returns        : U8 - The value of the status register read from the DS1339A RTC.
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the status register of the DS1339A RTC and returns its value.
*
* Notes          : This function reads a single byte from the status register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
static U8 Read_Status_Register(void)
{
	uint8_t status_reg;  // Variable to hold the status register value

	// Prepare the packet to read the Status Register from the DS1339A RTC
	twi_package_t packet_rx;
	packet_rx.addr[0] = DS1339A_STATUS_REG;  // Address of the status register
	packet_rx.addr_length = 1;               // Length of the address (1 byte)
	packet_rx.buffer = &status_reg;          // Buffer to store the read status register value
	packet_rx.chip = DS1339A_SLAVE_ADDRESS;  // I2C slave address of the DS1339A RTC
	packet_rx.length = 1;                   // Number of bytes to read (1 byte)

	// Perform the TWI read operation to get the status register
	if (twi_master_read(TWI0, &packet_rx) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nFailed to read Status Register."); // Print error message if read operation fails
		#endif
		
		return 0; // Return 0 if reading fails
	}
	
	return status_reg;  // Return the value of the status register
}

/*****************************************************************************
* Function Name  : Write_To_Status_Register
* Returns        : void
* Arguments      : U8 byteValue : Value to be written to the status register of the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes a specified value to the status register of the DS1339A RTC.
*
* Notes          : This function is used to clear or update the status register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
static void Write_To_Status_Register(U8 byteValue)
{
	// Prepare the packet to write the specified value to the status register of the DS1339A RTC
	twi_package_t packet_tx;
	packet_tx.addr[0] = DS1339A_STATUS_REG; // Address of the status register
	packet_tx.addr_length = 1;              // Length of the address (1 byte)
	packet_tx.buffer = &byteValue;          // Buffer containing the value to write to the status register
	packet_tx.chip = DS1339A_SLAVE_ADDRESS; // I2C slave address of the DS1339A RTC
	packet_tx.length = 1;                  // Number of bytes to write (1 byte)

	// Perform the TWI write operation to update the status register
	if (twi_master_write(TWI0, &packet_tx) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nFailed to write data into Status Register."); // Print error message if write operation fails
		#endif
		
		return; // Exit the function if writing fails
	}
}

/*****************************************************************************
* Function Name  : RTC_Interrupt_Handler
* Returns        : Nothing
* Arguments      : const uint32_t id    ---> ID of the interrupting peripheral.
*                  const uint32_t index ---> Pin index that triggered the interrupt.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Handles the interrupt triggered by the RTC. This function checks
*                  if the interrupt was caused by the correct pin (PD28) on the correct
*                  port (PIOD). If the pin is low, it sets the global flag `gb_rtc_1secInt_triggered_f`
*                  to indicate that an RTC interrupt has occurred.
*
* Notes          : This function is specific to handling interrupts from the RTC connected
*                  to pin PD28 on the PIOD port.
* Global Variables Affected : gb_rtc_1secInt_triggered_f
*****************************************************************************/
static void RTC_Interrupt_Handler(const uint32_t id, const uint32_t index)
{
	// Check if the interrupt was triggered by the PIOD peripheral and the PD28 pin
	if ((id == ID_PIOD) && (index == PIO_PD28))
	{
		// Check if the PD28 pin is low (0) indicating a falling edge has occurred
		if (pio_get(PIOD, PIO_TYPE_PIO_INPUT, PIO_PD28) != 1)
		{
			// Set the global flag to indicate that an RTC interrupt has occurred
			gb_rtc_1secInt_triggered_f = 1;
		}
	}
}

/*****************************************************************************
* Function Name  : Configure_Interrupt_Logic_For_RTC
* Returns        : Nothing
* Arguments      : U8 byteVal ---> Pass value to control register.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Configures interrupt output on the pin instead of square wave output.
*				   Enables alarm 1 output and an alarm is set for every one sec.
*
* Notes          : NA.
* Global Variables Affected : NA.
*****************************************************************************/
static void Configure_Interrupt_Logic_For_RTC(U8 byteVal)
{
	// Prepare the packet to write the specified value to the control and Alarm register of the DS1339A RTC
	twi_package_t packet_tx;
	
	U8 oneSecAlmValue[4] = {0};
	oneSecAlmValue[0] = 0x80;	// Set mask bit for the register 0x07
	oneSecAlmValue[1] = 0x80;	// Set mask bit for the register 0x08
	oneSecAlmValue[2] = 0x80;	// Set mask bit for the register 0x09
	oneSecAlmValue[3] = 0x80;	// Set mask bit for the register 0x0A
	
	packet_tx.addr[0] = 0x07;				// Start Address of the alarm register
	packet_tx.addr_length = 1;              // Length of the address (1 byte)
	packet_tx.buffer = oneSecAlmValue;		// Buffer containing the value to write to the alarm register
	packet_tx.chip = DS1339A_SLAVE_ADDRESS; // I2C slave address of the DS1339A RTC
	packet_tx.length = 4;					// Number of bytes to write (4 byte)
	
	if (twi_master_write(TWI0, &packet_tx) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nFailed to write data into alarm Register."); // Print error message if write operation fails
		#endif
		
		return; // Exit the function if writing fails
	}
	else
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nWrite success to alarm Register.");
		#endif		
	}
	
	packet_tx.addr[0] = DS1339A_CONTROL_REG; // Address of the control register
	packet_tx.addr_length = 1;              // Length of the address (1 byte)
	packet_tx.buffer = &byteVal;			// Buffer containing the value to write to the control register
	packet_tx.chip = DS1339A_SLAVE_ADDRESS; // I2C slave address of the DS1339A RTC
	packet_tx.length = 1;					// Number of bytes to write (1 byte)
	
	// Perform the TWI write operation to update the control register
	if (twi_master_write(TWI0, &packet_tx) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nFailed to write data into Control Register."); // Print error message if write operation fails
		#endif
		
		return; // Exit the function if writing fails
	}
	else
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nWrite success to Control Register.");
		#endif
	}
}

/*****************************************************************************
* Function Name  : RTC_Interrupt_Pin_Configure
* Returns        : Nothing
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 13/08/2024
*
* Description    : Configures and sets up an external interrupt for the RTC (Real-Time Clock).
*                  This function enables the necessary peripheral clocks, configures the pin
*                  as an input with a pull-up resistor, sets the interrupt handler, and enables
*                  the interrupt for a falling edge signal.
*
* Notes          : This setup is specific to handling interrupts from the RTC connected to pin PD28.
* Global Variables Affected : NA
*****************************************************************************/
void RTC_Interrupt_Pin_Configure(void)
{
	// Enable the peripheral clock for the PIOD port
	pmc_enable_periph_clk(ID_PIOD);
	// Configure the pin PD28 on PIOD as an input with a pull-up resistor
	pio_set_input(PIOD, PIO_PD28, PIO_PULLUP);
	// Set up an interrupt handler for PD28 on a falling edge
	pio_handler_set(PIOD, ID_PIOD, PIO_PD28, PIO_IT_FALL_EDGE, RTC_Interrupt_Handler);
	// Enable the interrupt for PD28
	pio_enable_interrupt(PIOD, PIO_PD28);
	// Enable the PIOD interrupt in the Nested Vectored Interrupt Controller (NVIC)
	NVIC_EnableIRQ(PIOD_IRQn);
	
	Configure_Interrupt_Logic_For_RTC(0x05);	// Set an alarm for an every one second.
}

/*****************************************************************************
* Function Name  : Write_Seconds_To_DS1339A
* Returns        : Nothing.
* Arguments      : uint8_t sec ---> The seconds value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided seconds value to the DS1339A RTC by sending it to the
*                  appropriate register (0x00). The seconds value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The I2C packet preparation and sending is handled by the `Update_I2C_Packet_To_Send` function.
*					- The function is expected to interact with the DS1339A RTC clock module.
*					- It currently does not return a status code, though its signature suggests that it might
*						return a status code in the future.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Seconds_To_DS1339A(uint8_t sec)
{
	// Call the function that prepares and sends the I2C packet to write the seconds value
	// to register 0x00 of the DS1339A RTC.
	// The 'sec' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_SEC_REG, sec);
}

/*****************************************************************************
* Function Name  : Write_Minutes_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t min ---> The minutes value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided minutes value to the DS1339A RTC by sending it to the
*                  appropriate register (0x01). The minutes value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*					- The function interacts with the DS1339A RTC clock module and updates the minutes register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Minutes_To_DS1339A(uint8_t min)
{
	// Call the function that prepares and sends the I2C packet to write the minutes value
	// to register 0x01 of the DS1339A RTC.
	// The 'min' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_MIN_REG, min);
}

/*****************************************************************************
* Function Name  : Write_Hours_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t hr ---> The hours value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided hours value to the DS1339A RTC by sending it to the
*                  appropriate register (0x02). The hours value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*					- The function interacts with the DS1339A RTC clock module and updates the hours register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Hours_To_DS1339A(uint8_t hr)
{
	// Call the function that prepares and sends the I2C packet to write the hours value
	// to register 0x02 of the DS1339A RTC.
	// The 'hr' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_HOUR_REG, hr);
}

/*****************************************************************************
* Function Name  : Write_DayValue_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t day ---> The day value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided day value to the DS1339A RTC by sending it to the
*                  appropriate register (0x03). The day value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*					- The function interacts with the DS1339A RTC clock module and updates the day register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_DayValue_To_DS1339A(uint8_t day)
{
	// Call the function that prepares and sends the I2C packet to write the day value
	// to register 0x03 of the DS1339A RTC.
	// The 'day' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_DAY_REG, day);
}

/*****************************************************************************
* Function Name  : Write_Date_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t date ---> The date value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 13/08/2024
*
* Description    : Writes the provided date value to the DS1339A RTC by sending it to the
*                  appropriate register (0x04). The date value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*					- The function interacts with the DS1339A RTC clock module and updates the date register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Date_To_DS1339A(uint8_t date)
{
	// Call the function that prepares and sends the I2C packet to write the date value
	// to register 0x04 of the DS1339A RTC.
	// The 'date' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_DATE_REG, date);
}

/*****************************************************************************
* Function Name  : Write_Month_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t month ---> The month value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided month value to the DS1339A RTC by sending it to the
*                  appropriate register (0x05). The month value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*- The function interacts with the DS1339A RTC clock module and updates the month register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Month_To_DS1339A(uint8_t month)
{
	// Call the function that prepares and sends the I2C packet to write the month value
	// to register 0x05 of the DS1339A RTC.
	// The 'month' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_MONTH_REG, month);
}

/*****************************************************************************
* Function Name  : Write_Year_To_DS1339A
* Returns        : void - This function does not return a value.
* Arguments      : uint8_t year ---> The year value (in decimal) to write to the DS1339A RTC.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Writes the provided year value to the DS1339A RTC by sending it to the
*                  appropriate register (0x06). The year value is converted to BCD format
*                  before transmission using the I2C protocol.
*
* Notes          :
*					- The function calls `Update_I2C_Packet_To_Send` to prepare and send the I2C packet.
*					- The function interacts with the DS1339A RTC clock module and updates the year register.
* Global Variables Affected : NA
*****************************************************************************/
void Write_Year_To_DS1339A(uint8_t year)
{
	// Call the function that prepares and sends the I2C packet to write the year value
	// to register 0x06 of the DS1339A RTC.
	// The 'year' parameter will be converted from decimal to BCD format inside the function.
	Update_I2C_Packet_To_Send(DS1339A_YEAR_REG, year);
}

/*****************************************************************************
* Function Name  : Read_Seconds_From_DS1339A
* Returns        : U8 - The seconds value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the seconds value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x00.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the seconds register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Seconds_From_DS1339A(void)
{
	// Read the seconds value from register 0x00 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_SEC_REG);
}

/*****************************************************************************
* Function Name  : Read_Minutes_From_DS1339A
* Returns        : U8 - The minutes value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the minutes value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x01.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the minutes register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Minutes_From_DS1339A(void)
{
	// Read the minutes value from register 0x01 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_MIN_REG);
}

/*****************************************************************************
* Function Name  : Read_Hour_From_DS1339A
* Returns        : U8 - The hour value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the hour value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x02.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the hour register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Hour_From_DS1339A(void)
{
	// Read the hour value from register 0x02 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_HOUR_REG);
}

/*****************************************************************************
* Function Name  : Read_DayValue_From_DS1339A
* Returns        : U8 - The day value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the day value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x03.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the day value register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_DayValue_From_DS1339A(void)
{
	// Read the day value from register 0x03 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_DAY_REG);
}

/*****************************************************************************
* Function Name  : Read_Date_From_DS1339A
* Returns        : U8 - The date value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the date value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x04.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the date value register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Date_From_DS1339A(void)
{
	// Read the date value from register 0x04 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_DATE_REG);
}

/*****************************************************************************
* Function Name  : Read_Month_From_DS1339A
* Returns        : U8 - The month value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the month value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x05.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the month value register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Month_From_DS1339A(void)
{
	// Read the month value from register 0x05 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_MONTH_REG);
}

/*****************************************************************************
* Function Name  : Read_Year_From_DS1339A
* Returns        : U8 - The year value read from the DS1339A RTC (in decimal format).
* Arguments      : None.
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
*
* Description    : Reads the year value from the DS1339A RTC by calling the
*                  `Update_I2C_Packet_To_Read` function to read from register 0x06.
*                  The returned value is in decimal format.
*
* Notes          : This function reads the year value register of the DS1339A RTC.
* Global Variables Affected : NA
*****************************************************************************/
U8 Read_Year_From_DS1339A(void)
{
	// Read the year value from register 0x06 of the DS1339A RTC.
	// The read value is converted to decimal format by the `Update_I2C_Packet_To_Read` function.
	return Update_I2C_Packet_To_Read(DS1339A_YEAR_REG);
}

/*****************************************************************************
* Function Name	: Write_Time_hhMMss_To_DS1339A
* Returns       : void
* Arguments     : U8 hr : Hours value to be written to DS1339A RTC (in decimal format).
*				  U8 mn : Minutes value to be written to DS1339A RTC (in decimal format).
*				  U8 ss : Seconds value to be written to DS1339A RTC (in decimal format).
* Created by    : Ranjitkumar Ainapure
* Date Created  : 14/08/2024
*
* Description   : Writes the provided time values (hours, minutes, seconds)
*                 into the DS1339A RTC. The values are converted to BCD format
*                 before sending over I2C.
*
* Notes         : This function updates the time registers of the DS1339A RTC (registers 0x00 to 0x02).
* Global Variables Affected : NA
*****************************************************************************/
void Write_Time_hhMMss_To_DS1339A(U8 hr, U8 mn, U8 ss)
{
	U8 bcd_time_arr[3] = {0};
	
	// Convert seconds, minutes, and hours to BCD format
	bcd_time_arr[0] = DecimalToBCD(ss);  // Convert seconds to BCD
	bcd_time_arr[1] = DecimalToBCD(mn);  // Convert minutes to BCD
	bcd_time_arr[2] = DecimalToBCD(hr);  // Convert hours to BCD

	// Setup I2C packet to write time data to DS1339A
	twi_package_t packet_write;
	packet_write.addr[0] = DS1339A_SEC_REG;           // Set the starting register address to 0x00 (seconds)
	packet_write.addr_length = 1;          // Address length is 1 byte
	packet_write.buffer = &bcd_time_arr;   // Set the data buffer to the BCD time array
	packet_write.length = 3;               // Write 3 bytes (seconds, minutes, hours)
	packet_write.chip = DS1339A_SLAVE_ADDRESS; // Set the slave address for DS1339A
	
	// Perform the I2C write operation
	if (twi_master_write(TWI0, &packet_write) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nTWI writing is unsuccessful."); // Print error message if write fails
		#endif
		
		return;
	}
}

/*****************************************************************************
* Function Name : Write_DtMnYy_and_Day_To_DS1339A
* Returns       : void
* Arguments     : U8 dt  : Date value to be written to DS1339A RTC (in decimal format).
*				  U8 mon : Month value to be written to DS1339A RTC (in decimal format).
*				  U8 yr  : Year value to be written to DS1339A RTC (in decimal format).
*				  U8 day : Day of the week to be written to DS1339A RTC (in decimal format).
* Created by    : Ranjitkumar Ainapure
* Date Created  : 14/08/2024
*
* Description   : Writes the provided date, month, year, and day values
*                 into the DS1339A RTC. The values are converted to BCD format
*                 before sending over I2C.
*
* Notes         : This function updates the day, date, month, and year registers of the
*				  DS1339A RTC (registers 0x03 to 0x06).
* Global Variables Affected : NA
*****************************************************************************/
void Write_DtMnYy_and_Day_To_DS1339A(U8 dt, U8 mon, U8 yr, U8 day)
{
	U8 bcd_date_arr[4] = {0};

	// Convert day, date, month, and year to BCD format
	bcd_date_arr[0] = DecimalToBCD(day);  // Convert day to BCD
	bcd_date_arr[1] = DecimalToBCD(dt);   // Convert date to BCD
	bcd_date_arr[2] = DecimalToBCD(mon);  // Convert month to BCD
	bcd_date_arr[3] = DecimalToBCD(yr);   // Convert year to BCD

	// Setup I2C packet to write date, month, year, and day data to DS1339A
	twi_package_t packet_write;
	packet_write.addr[0] = DS1339A_DAY_REG;   // Set the starting register address to 0x03 (Day)
	packet_write.addr_length = 1;          // Address length is 1 byte
	packet_write.buffer = &bcd_date_arr;   // Set the data buffer to the BCD date array
	packet_write.length = 4;               // Write 4 bytes (Day, Date, Month, Year)
	packet_write.chip = DS1339A_SLAVE_ADDRESS; // Set the slave address for DS1339A
	
	// Perform the I2C write operation
	if (twi_master_write(TWI0, &packet_write) != TWI_SUCCESS)
	{
		#if (DEBUG_ALL || DEBUG_RTC)
		Print_Message("\nTWI writing is unsuccessful."); // Print error message if write fails
		#endif
		
		return;
	}
}

/*****************************************************************************
* Function Name  : Get_RTC_Data_At_Every_Second
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 14/08/2024
* Description    : Reads and prints the current time and date from the DS1339A RTC
*                   if the RTC interrupt flag is set. The data is printed every
*                   second if an alarm interrupt occurs.
* Notes          : This function handles reading and printing RTC data when an interrupt
*				   indicates that the data should be fetched.
* Global Variables Affected : gb_rtc_1secInt_triggered_f
*****************************************************************************/
void Get_RTC_Data_At_Every_Second(void)
{
	U8 lcl_rtc_read_f = 0;  // Local flag to indicate if RTC data should be read

	// Check if the RTC interrupt flag is set
	if ((gb_rtc_1secInt_triggered_f == 1) || (gb_read_stsReg_at_pwrOn_f == 1))
	{
		U8 status_reg = 0;
		status_reg = Read_Status_Register();  // Read the status register to check for interrupts

		// Check if the alarm interrupt flag (bit 0) is set in the status register
		if (status_reg & 0x01)
		{
			gb_rtc_1secInt_triggered_f = 0;  // Reset RTC interrupt flags
			gb_read_stsReg_at_pwrOn_f = 0;
			
			uint8_t clear_flag;  // Variable to hold the status register value after clearing the interrupt flag
			// Clear the alarm interrupt flag in the status register
			clear_flag = status_reg & 0xFE;  // Clear the least significant bit (alarm interrupt flag)
			Write_To_Status_Register(clear_flag);  // Write back the cleared status register value
			
			lcl_rtc_read_f = 1;  // Set flag to read RTC data
			gb_rtc_dm_1sec_f = 1;		// Used this variable for DM functionality.
			gb_rtc_one_sec_flag = 1;
			gb_rtc_1sec_IS_set_f = 1;	// Used this variable for OD functionality.
			gb_rtc_1sec_delay_f = 1;
		}
	}

	// If RTC data needs to be read
	if (lcl_rtc_read_f == 1)
	{
		lcl_rtc_read_f = 0;  // Reset the local RTC read flag
		
		gb_rtcTimeArr[0] = Read_Hour_From_DS1339A();
		gb_rtcTimeArr[1] = Read_Minutes_From_DS1339A();
		gb_rtcTimeArr[2] = Read_Seconds_From_DS1339A();
		
		gb_rtcDateArr[0] = Read_Date_From_DS1339A();
		gb_rtcDateArr[1] = Read_Month_From_DS1339A();
		gb_rtcDateArr[2] = Read_Year_From_DS1339A();
		gb_rtcDateArr[3] = Read_DayValue_From_DS1339A();
		
		#if 0
		// Print the time
		Print_Message("\nTime       : HH:MM:SS   | ");
		if (gb_rtcTimeArr[0] < 10)
		Print_Number(0);	// Print Dummy.
		Print_Number(gb_rtcTimeArr[0]);    // Read and print the hour
		Print_Message(":");
		if (gb_rtcTimeArr[1] < 10)
		Print_Number(0);	// Print Dummy.
		Print_Number(gb_rtcTimeArr[1]); // Read and print the minutes
		Print_Message(":");
		if (gb_rtcTimeArr[2] < 10)
		Print_Number(0);	// Print Dummy.
		Print_Number(gb_rtcTimeArr[2]); // Read and print the seconds

		// Print the date
		Print_Message("\nDate       : DD:MM:YYYY | ");
		if (gb_rtcDateArr[0] < 10)
		Print_Number(0);	// Print Dummy.
		Print_Number(gb_rtcDateArr[0]);   // Read and print the date
		Print_Message(":");
		if (gb_rtcDateArr[1] < 10)
		Print_Number(0);	// Print Dummy.
		Print_Number(gb_rtcDateArr[1]);  // Read and print the month
		Print_Message(":20");
		Print_Number(gb_rtcDateArr[2]);   // Read and print the year
		
		// Print day of the week
		Print_Message("\nWeekday is : ---- ");
		Print_Number(gb_rtcDateArr[3]);
		Print_Message(" --- | ");
		switch (gb_rtcDateArr[3])
		{
			case 1:
				Print_Message("Sunday.");
			break;
			case 2:
				Print_Message("Monday.");
			break;
			case 3:
				Print_Message("Tuesday.");
			break;
			case 4:
				Print_Message("Wednesday.");
			break;
			case 5:
				Print_Message("Thursday.");
			break;
			case 6:
				Print_Message("Friday.");
			break;
			case 7:
				Print_Message("Saturday.");
			break;
			default:
				Print_Message("Holiday");
			break;
		}
		#endif
		
		static int send_f;
		send_f ++;
		if (send_f >= 10)
		{
			send_f = 0;
			
		}
	}
}

/*****************************************************************************
* Function Name  : Get_TimeDate_By_UART
* Returns        : void
* Arguments      : U8 rcvByte
* Created by     : Ranjitkumar Ainapure
* Date Created   : 03/10/2024
* Description    : Receives time and date data via UART. If the start byte ('#')
*                  is received, the function begins capturing data. Once the
*                  end byte ('$') is received, it triggers the RTC time update
*                  flag. Data received between '#' and '$' is stored in
*                  rtcUpdateArr.
* Notes          : The function captures data between '#' and '$' bytes and
*                  sets a flag when complete for RTC time update.
* Global Variables Affected : rtcUpdateArr, gb_rtc_time_update_f
*****************************************************************************/
void Get_TimeDate_By_UART(U8 rcvByte)
{
	static U8 startRcv_f;  // Flag to indicate the start of reception
	static U8 rcvIdx;      // Index for storing received data
	
	if ((rcvByte == '#') && (startRcv_f == 0))
	{
		startRcv_f = 1;  // Start receiving data
		rcvIdx = 0;      // Reset index to store from the beginning
		memset(rtcUpdateArr, 0, sizeof(rtcUpdateArr)); // Clear the buffer for new data
	}
	else if ((rcvByte == '$') && (startRcv_f == 1))
	{
		startRcv_f = 0;   // Stop receiving data
		gb_rtc_time_update_f = 1; // Set the flag to indicate time update is ready
	}
	else
	{
		rtcUpdateArr[rcvIdx++] = rcvByte; // Store received data in the array
	}
}

/*****************************************************************************
* Function Name  : Update_Time_And_Date
* Returns        : void
* Arguments      : None
* Created by     : Ranjitkumar Ainapure
* Date Created   : 03/10/2024
* Description    : Updates the DS1339A RTC with the time and date received via UART.
*                  If the first byte of the data is 'T', it updates the time.
*                  If the first two bytes are 'DT', it updates the date and day.
* Notes          : The function checks for 'T' to update the time, and 'DT' to
*                  update the date and day in the RTC.
* Global Variables Affected : rtcUpdateArr
*****************************************************************************/
void Update_Time_And_Date(void)
{
	// If the first byte is 'T', update time in hh:mm:ss format
	if (rtcUpdateArr[0] == 'T')
	{
		Write_Time_hhMMss_To_DS1339A(rtcUpdateArr[1], rtcUpdateArr[2], rtcUpdateArr[3]);
	}
	
	// If the first two bytes are 'DT', update date (dd:mm:yy) and day
	if ((rtcUpdateArr[0] == 'D') && (rtcUpdateArr[1] == 'T'))
	{
		Write_DtMnYy_and_Day_To_DS1339A(rtcUpdateArr[2], rtcUpdateArr[3], rtcUpdateArr[4], rtcUpdateArr[5]);
	}
}


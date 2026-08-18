/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: ext_eeprom_1.c
* Created By	: Shirisha Ankam.
* Created Date	: 06-01-2025.
* Module
* Description	: Driver to External Eeprom.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 05/10/2023.
* Changes		: NA.
*****************************************************************************/
#include "ext_eeprom_1.h"
#include "user_uart.h"
#include <string.h>

uint8_t eeprom_command_data[8]			= {0};		// Used to modify command, page num, byte address.
uint8_t epwrite_arr[EEPROM_PAGE_SIZE]	= {0};	// Used to write eeprom data.
uint8_t epread_arr[EEPROM_PAGE_SIZE]	= {0};	// Used to read eeprom data.

/*****************************************************************************
* Function name	: void EXT_EEPROM_Init(void) 
* Returns		: None.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 06-01-2025
* Description	: Function to Initialize eeprom.
                  Presently default settings are there.
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/	
void EXT_EEPROM_Init(void) 
{
	// Initialize GPIO for CS and WR
	pmc_enable_periph_clk(ID_PIOA); // Enable clock for PIOA
	pio_set_output(PIOA, PIO_PA16, LOW, DISABLE, ENABLE);  // CS
	pio_set_output(PIOA, PIO_PA21, HIGH, DISABLE, ENABLE); // WR_PTR

	Write_Enable();
	Disable_Full_Protection();
	Write_Enable();
    Enable_Full_Protection();
	
	WR_PTR_LOW; //Enable Hardware Write Protection.
	Write_Disable();
}

/*****************************************************************************************
* Function name : void Write_eeprom_Status_Register(uint8_t status)
* Returns       : None
* Arguments     : uint8_t status -> Value to write to the status register.
* Created by    : Shirisha Ankam
* Date created  : 06-01-2025
* Description   : Function to write to the EEPROM status register.
* Notes         : NA
* Global Variables Affected : NA
******************************************************************************************/
void Write_eeprom_Status_Register(uint8_t status) 
{
	#if DEBUG_EEPROM_STATUS
	Print_Message("\nInside Write_eeprom_Status_Register Function.\n");
	#endif
	
	memset(eeprom_command_data, 0, 8);
	eeprom_command_data[0] = 0x01;  // Command for writing to the status register
	eeprom_command_data[1] = status;  // Status register value to be written

	EP_CS_PIN_LOW;
	delay_us(10);  // Adjust delay based on datasheet
	Data_To_SPI(eeprom_command_data, 2);
	while (!spi_is_tx_empty(SPI));
	EP_CS_PIN_HIGH;
	Wait_For_Write_Completion();  // Wait for the write operation to complete

	#if DEBUG_EEPROM_STATUS
	Print_Message("\nWrite to Status Register completed.\n");
	#endif
}


/*****************************************************************************
* Function name	: void Enable_Full_Protection()
* Returns		: None.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 10-01-2025
* Description	: Function to enable full protection for eeprom.
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
void Enable_Full_Protection()
{
	#if DEBUG_EEPROM_STATUS
	Print_Message("\nEnabling full memory protection with W pin.\n");
	#endif

	uint8_t protection_value = 0x9C;  //  BP3:BP0 = 1(Full Protection)
	Write_eeprom_Status_Register(protection_value);  // Write protection to status register

	#if DEBUG_EEPROM_STATUS
	// Verify the status register
	uint8_t status = Read_eeprom_Status_Register();
	if (status == protection_value)
	{
		Print_Message("\nFull protection enabled successfully.\n");
	}
	else
	{
		Print_Message("\nError: Failed to enable full protection.\n");
	}
	#endif
}

/*****************************************************************************
* Function name	: void Disable_Full_Protection()
* Returns		: None.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 10-01-2025
* Description	: Function to disable full protection for eeprom.
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
void Disable_Full_Protection()
{
	#if DEBUG_EEPROM_STATUS
	Print_Message("\nDisabling full memory protection with W pin.\n");
	#endif

	uint8_t protection_value = 0x00;  //  BP3:BP0 = 0 remove protection
	Write_eeprom_Status_Register(protection_value);  // Write protection to status register

	#if DEBUG_EEPROM_STATUS
	// Verify the status register
	uint8_t status = Read_eeprom_Status_Register();
	if (status == protection_value)
	{
		Print_Message("\nFull protection disabled successfully.\n");
	}
	else
	{
		Print_Message("\nError: Failed to disable full protection.\n");
	}
	#endif
}



/*****************************************************************************************
* Function name	: uint8_t Read_eeprom_Status_Register(void)
* Returns		: uint8_t ---> returns status register data.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 06-01-2025
* Description	: Function is written for reading status register value.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
uint8_t Read_eeprom_Status_Register(void) 
{
	#if DEBUG_EEPROM_STATUS
	Print_Message("\nInside Read_Status_Register Function.\n");
	#endif

	uint8_t status;
	uint8_t eeprom_command_data[1];
	eeprom_command_data[0] = EEPROM_CMD_READ_STATUS;

	EP_CS_PIN_LOW;
	delay_us(10);  // Adjust delay based on datasheet
	Data_To_SPI(eeprom_command_data, 1);
	while (!spi_is_tx_empty(SPI));
	spi_read_packet(SPI, &status, 1);
	EP_CS_PIN_HIGH;

	#if DEBUG_EEPROM_STATUS
	Print_Message("\nStatus register value is ");
	Print_Number(status);
	#endif

	return status;
}

/*****************************************************************************************
* Function name	: void Write_Enable()
* Returns		: Noting.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 06-01-2025
* Description	: Function to Enable Write command for eeprom and set WEL bit.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
void Write_Enable()
{
	#if DEBUG_EEPROM_ENABLE
	Print_Message("\nSending Write Enable Command..."); // Debug: Indicate entry into the function and sending the write enable command
	#endif

	uint8_t cmd_data = EEPROM_CMD_WRITE_ENABLE; // Prepare the Write Enable command byte 0x06
	EP_CS_PIN_LOW;  // Start communication by pulling chip select (CS) low
	Data_To_SPI(&cmd_data, 1);  // Send the Write Enable command via SPI
	//delay_ms(5); // Wait for the command to be processed
	while (!spi_is_tx_empty(SPI));	// Wait for the command to be processed
	EP_CS_PIN_HIGH;  // End communication by pulling chip select (CS) high

	
	#if DEBUG_EEPROM_ENABLE
	// Read the status register to check if the Write Enable Latch (WEL) bit is set
	uint8_t status = Read_eeprom_Status_Register();
	if (status & 0x02) // Check if WEL bit (bit 1) is set
	{
		Print_Message("\nWEL bit set successfully."); // Debug: Indicate successful setting of WEL bit
	}
	else
	{
		Print_Message("\nError: WEL bit not set."); // Debug: Indicate failure to set WEL bit
	}
	Print_Message("\nWrite Enable Command Sent."); // Debug: Indicate that the command has been sent
	#endif
}

/*****************************************************************************************
* Function name	: void Write_Disable();
* Returns		: Noting.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 26-02-2025
* Description	: Function to Disable Write command for eeprom and clear WEL bit.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
void Write_Disable() 
{
	#if DEBUG_EEPROM_ENABLE
	Print_Message("\nSending Write Disable Command...");
	#endif

	uint8_t cmd_data = EEPROM_CMD_WRITE_DISABLE;

	EP_CS_PIN_LOW;
	delay_us(10);  
	Data_To_SPI(&cmd_data, 1);
	while (!spi_is_tx_empty(SPI));
	EP_CS_PIN_HIGH;

	#if DEBUG_EEPROM_ENABLE
	uint8_t status = Read_eeprom_Status_Register();
	if (!(status & 0x02)) {
		Print_Message("\nWEL bit cleared successfully.");
		} else {
		Print_Message("\nError: WEL bit not cleared.");
	}
	Print_Message("\nWrite Disable Command Sent.");
	#endif
}

/*****************************************************************************************
* Function name	: void Wait_For_Write_Completion(void);
* Returns		: Noting.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 26-02-2025
* Description	: Function to Wait while the Write In Progress (WIP) bit is set.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
void Wait_For_Write_Completion(void) 
{
	uint8_t status;
	do 
	{
		status = Read_eeprom_Status_Register();
	} 
	while (status & 0x01); // Wait while the Write In Progress (WIP) bit is set
}




/*****************************************************************************************
* Function name	: uint8_t EEPROM_Page_Write(uint32_t page_num, uint16_t byte_add, 
				  uint8_t *data, uint16_t len) 
* Returns		: uint8_t ---> returns 1 if error occurs. else returns 0;
* Arguments		: uint32_t page_num ---> Send Page number to be write.
* 				  uint16_t byte_add ---> Send Byte address of the page.
* 				  uint8_t *data ---> Holds the address of the data buffer.
* 				  uint16_t len	---> Send total length to be write.
* Created by	: Shirisha Ankam
* Date created	: 06-01-2025
* Description	: Function is written for writing data to a page.
* 				  Developer can write from 0th location to maximum of EEPROM_PAGE_SIZE.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
uint8_t EEPROM_Page_Write(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len) 
{
	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nInside EEPROM_Page_Write Function.");
	#endif

	// Validate length and boundary
	if (len > EEPROM_PAGE_SIZE || byte_add + len > EEPROM_PAGE_SIZE) {
		Print_Message("\nError: Write exceeds page boundary or invalid length.\n");
		return 1;  // Error: Invalid length or boundary
	}

	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nPage Number : ");
	Print_Number(page_num);
	Print_Message("\nByte Address : ");
	Print_Number(byte_add);
	Print_Message("\nEntered length : ");
	Print_Number(len);
	Print_Message("\nData to be written is\n");
	for (int idx = 0; idx < len; idx++) {
		Print_Number(data[idx]);
		Print_Message(",");
	}
	#endif

	WR_PTR_HIGH;
	delay_us(10);
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Disable full protection to allow writing
	Disable_Full_Protection();
	Write_Enable();
	// Wait for the EEPROM to be ready (WIP = 0)
	Wait_For_Write_Completion();

	// Calculate the full address using the page number and byte offset
	uint32_t ADDRESS_BYTE = (page_num << 9) | (byte_add & 0x01FF);

	// Prepare the command buffer for the write operation
	eeprom_command_data[0] = EEPROM_CMD_WRITE_DATA; // Write data command
	eeprom_command_data[1] = (uint8_t)((ADDRESS_BYTE & 0xFF0000) >> 16); // Address MSB
	eeprom_command_data[2] = (uint8_t)((ADDRESS_BYTE & 0x00FF00) >> 8);  // Address middle byte
	eeprom_command_data[3] = (uint8_t)(ADDRESS_BYTE & 0x0000FF);         // Address LSB

	// Start SPI transaction
	EP_CS_PIN_LOW; // Pull chip select (CS) pin low to start the SPI transaction
	//WR_PTR_LOW;
	delay_us(10);  // Small delay for CS stabilization
	Data_To_SPI(eeprom_command_data, 4); // Send the command and address bytes via SPI
	while (!spi_is_tx_empty(SPI)); // Wait for the SPI transmit buffer to be empty
	Data_To_SPI(data, len); // Send the data bytes to be written
	while (!spi_is_tx_empty(SPI)); // Wait for the SPI transmit buffer to be empty
	EP_CS_PIN_HIGH; // Pull chip select (CS) pin high to end the SPI transaction

	// Wait for the write operation to complete
	Wait_For_Write_Completion();
	WR_PTR_LOW;
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Re-enable full protection after writing
	Enable_Full_Protection();
	Write_Disable();

	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nWrite successful.");
	#endif

	return 0;  // Success: Return success code
}


//Function EEPROM_Page_Write_Without_HW (); is used for testing purpose only and not to be included in final driver.
uint8_t EEPROM_Page_Write_Without_HW(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len)
{
	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nInside EEPROM_Page_Write Function.");
	#endif

	// Validate length and boundary
	if (len > EEPROM_PAGE_SIZE || byte_add + len > EEPROM_PAGE_SIZE) {
		Print_Message("\nError: Write exceeds page boundary or invalid length.\n");
		return 1;								// Error: Invalid length or boundary
	}

	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nPage Number : ");
	Print_Number(page_num);
	Print_Message("\nByte Address : ");
	Print_Number(byte_add);
	Print_Message("\nEntered length : ");
	Print_Number(len);
	Print_Message("\nData to be written is\n");
	for (int idx = 0; idx < len; idx++) {
		Print_Number(data[idx]);
		Print_Message(",");
	}
	#endif

// 	
	WR_PTR_HIGH;							  //Disabled Hardware Write Protection.
	delay_us(10);
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Disable full protection to allow writing
	Disable_Full_Protection();
	Write_Enable();
	// Wait for the EEPROM to be ready (WIP = 0)
	Wait_For_Write_Completion();

	// Calculate the full address using the page number and byte offset
	uint32_t ADDRESS_BYTE = (page_num << 9) | (byte_add & 0x01FF);

	// Prepare the command buffer for the write operation
	eeprom_command_data[0] = EEPROM_CMD_WRITE_DATA; // Write data command
	eeprom_command_data[1] = (uint8_t)((ADDRESS_BYTE & 0xFF0000) >> 16); // Address MSB
	eeprom_command_data[2] = (uint8_t)((ADDRESS_BYTE & 0x00FF00) >> 8);  // Address middle byte
	eeprom_command_data[3] = (uint8_t)(ADDRESS_BYTE & 0x0000FF);         // Address LSB

	// Start SPI transaction
	EP_CS_PIN_LOW;							// Pull chip select (CS) pin low to start the SPI transaction
	//WR_PTR_LOW;
	delay_us(10);							// Small delay for CS stabilization
	Data_To_SPI(eeprom_command_data, 4);    // Send the command and address bytes via SPI
	while (!spi_is_tx_empty(SPI));			// Wait for the SPI transmit buffer to be empty
	Data_To_SPI(data, len);					// Send the data bytes to be written
	while (!spi_is_tx_empty(SPI));			// Wait for the SPI transmit buffer to be empty
	EP_CS_PIN_HIGH; 						// Pull chip select (CS) pin high to end the SPI transaction

	// Wait for the write operation to complete
	Wait_For_Write_Completion();
	WR_PTR_LOW;								//Enable Hardware Write Protection.
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Re-enable full protection after writing
	Enable_Full_Protection();
	Write_Disable();

	#if DEBUG_EEPROM_PWRITE
	Print_Message("\nWrite successful.");
	#endif

	return 0;  // Success: Return success code
}




/*****************************************************************************************
* Function name	: uint8_t EEPROM_Page_Read(uint32_t page_num, uint16_t byte_add,
                  uint8_t *data, uint16_t len)
* Returns		: uint8_t ---> returns 1 if error occurs. else returns 0;
* Arguments		: uint32_t page_num ---> Send Page number to be read.
* 				  uint16_t byte_add ---> Send Byte address of the page.
* 				  uint8_t *data ---> Holds the address of the read data buffer.
* 				  uint16_t len	---> Send total length to be read.
* Created by	: Shirisha Ankam
* Date created	: 06-01-2025
* Description	: Function is written for reading data from the page.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
uint8_t EEPROM_Page_Read(uint32_t page_num, uint16_t byte_add, uint8_t *data, uint16_t len) 
{
	#if DEBUG_EEPROM_PREAD
	Print_Message("\nInside EEPROM_Page_Read Function."); // Debug: Indicate entry into the function
	#endif

	// Validate length and boundary to ensure it doesn't exceed the page size
	if (len > EEPROM_PAGE_SIZE || byte_add + len > EEPROM_PAGE_SIZE) {
		Print_Message("\nRead exceeds page boundary or invalid length.\n"); // Debug: Length or boundary error
		return 1; // Error: Invalid length or address
	}

	#if DEBUG_EEPROM_PREAD
	Print_Message("\nPage Number : "); // Debug: Print the page number
	Print_Number(page_num);
	Print_Message("\nByte Address : "); // Debug: Print the byte address
	Print_Number(byte_add);
	Print_Message("\nEntered length : "); // Debug: Print the length to be read
	Print_Number(len);
	#endif

	uint32_t addr = page_num;
	addr <<= 9;
	addr |= (byte_add & 0x01FF); // Apply the byte offset mask (to ensure it's within the page)

	// Prepare the command buffer for the read operation
	eeprom_command_data[0] = EEPROM_CMD_READ_DATA; // Read data command
	eeprom_command_data[1] = (uint8_t)((addr & 0xFF0000) >> 16); // Address MSB
	eeprom_command_data[2] = (uint8_t)((addr & 0x00FF00) >> 8);  // Address middle byte
	eeprom_command_data[3] = (uint8_t)(addr & 0x0000FF);         // Address LSB

	// Send the read command and receive data via SPI
	EP_CS_PIN_LOW;  // Start communication by pulling chip select (CS) low
	Data_To_SPI(eeprom_command_data, 4);  // Send the command and address bytes via SPI
	spi_read_packet(SPI, data, len);  // Receive the data into the provided buffer
	EP_CS_PIN_HIGH;  // End communication by pulling chip select (CS) high

	#if DEBUG_EEPROM_PREAD
	Print_Message("\nData read from EEPROM:\n"); // Debug: Indicate start of received data
	for (uint16_t i = 0; i < len; i++) { // Loop through the received data
		Print_Number(data[i]);  // Print each received byte
		Print_Message(",");
	}
	#endif

	return 0; // Success: Return success code
}



/***********************************************************************************
* Function name	: uint8_t EEPROM_Byte_Write(int loc, uint8_t *data, uint32_t len)
* Returns		: uint8_t ---> returns 0 on success;
* Arguments		: int loc ---> Send byte location.
* 				  uint8_t *fdata ---> pointer holds an address of data buffer.
* 				  uint16_t len	---> Send total length to be written.
* Created by	: Shirisha Ankam
* Date created	: 07-01-2025
* Description	: Function is written for Writing a Byte data or multiple bytes.
* 				  Developer can write from 0th location to MAX_Byte size.
* 				  Only need to send byte location , data and length of data. Pages
* 				  are selected automatically based on byte location/address.
*               :
* Notes			: NA
* Global Variables Affected	: NA
***********************************************************************************/
uint8_t EEPROM_Byte_Write(int loc, uint8_t *data, uint32_t len)
{
	#if DEBUG_EEPROM_BWRITE
	Print_Message("\nInside eeprom_Byte_Write Function");  // Debug: Entry into function
	#endif

	// Calculate the page number and byte address based on the location
	uint32_t page_num = (loc / EEPROM_PAGE_SIZE);  // Page number based on location
	uint32_t byte_add = loc % EEPROM_PAGE_SIZE;  // Calculate the byte offset within the page

	#if DEBUG_EEPROM_BWRITE
	Print_Message("\nEntered write location : ");
	Print_Number(loc);  // Debug: Print the write location
	Print_Message("\nEntered total length : ");
	Print_Number(len);  // Debug: Print the total length to be written
	#endif

	uint32_t add_counts = 0;  // Initialize the address counter

	while (1)  // Start writing the data
	{
		#if DEBUG_EEPROM_BWRITE
		Print_Message("\nPage Number : ");
		Print_Number(page_num);  // Debug: Print the page number
		Print_Message("\nByte location : ");
		Print_Number(byte_add);  // Debug: Print the byte offset
		Print_Message("\nWritable data is-->\n");
		#endif

		uint32_t temp_len = 0;  // Temporary length to track how much data to write

		// Loop to write the data within the page
		for (uint16_t idx = byte_add; idx < EEPROM_PAGE_SIZE; idx++)
		{
			#if DEBUG_EEPROM_BWRITE
			Print_Number(data[add_counts]);  // Debug: Print the data being written
			Print_Message(",");
			#endif

			add_counts++;  // Increment the address counter
			temp_len++;  // Increment the temporary length

			if (add_counts >= len)  // Stop when the total length is written
			{
				break;
			}
		}

		#if DEBUG_EEPROM_BWRITE
		Print_Message("\nAddress counter : ");
		Print_Number(add_counts);  // Debug: Print the address counter
		Print_Message("\nTemp length : ");
		Print_Number(temp_len);  // Debug: Print the temporary length
		#endif
		
		WR_PTR_HIGH;//Disabled Hardware Write Protection.
		delay_us(10);
		// Enable writing to the EEPROM by setting the Write Enable Latch
		Write_Enable();
		// Disable full protection to allow writing
		Disable_Full_Protection();
		Write_Enable();
		// Wait for the EEPROM to be ready (WIP = 0)
		Wait_For_Write_Completion();

		// Prepare the command to write data
		eeprom_command_data[0] = EEPROM_CMD_WRITE_DATA;
		uint32_t addr = 0;
		// Calculate the address based on page number and byte offset
		addr = page_num;
		addr <<= 9;
		addr |= (byte_add & 0x01FF);

		// Populate the command with the address
		eeprom_command_data[1] = (uint8_t)((addr & 0xFF0000) >> 16);
		eeprom_command_data[2] = (uint8_t)((addr & 0x00FF00) >> 8);
		eeprom_command_data[3] = (uint8_t)(addr & 0x0000FF);

		// Start communication with the EEPROM and write the data
		EP_CS_PIN_LOW;  // Activate chip select
		delay_us(10);  // Delay for command execution
		Data_To_SPI(eeprom_command_data, 4);  // Send command with address
		while (!spi_is_tx_empty(SPI));  // Wait for transmission to complete
		Data_To_SPI(&data[(add_counts - temp_len)], temp_len);  // Send the actual data
		while (!spi_is_tx_empty(SPI));  // Wait for transmission to complete
		EP_CS_PIN_HIGH;  // Deactivate chip select
		
		// Wait for the write operation to complete
		Wait_For_Write_Completion();
		WR_PTR_LOW;                        //Enable Hardware Write Protection.
		// Enable writing to the EEPROM by setting the Write Enable Latch
		Write_Enable();
		// Re-enable full protection after writing
		Enable_Full_Protection();
		Write_Disable();

		// If there is more data to write, update the page number and byte offset
		if (add_counts < len)
		{
			page_num++;  // Move to the next page
			byte_add = 0;  // Reset the byte offset for the next page
		}
		else
		break;  // Exit the loop when all data has been written
	}

	return 0;  // Return success
}




/*************************************************************************************
* Function name   : uint8_t EEPROM_Byte_Read(uint32_t loc, uint8_t *buffer, uint32_t len)
* Returns         : uint8_t ---> returns 0 on success; 1 on error (invalid location/length)
* Arguments       : uint32_t loc   ---> Starting byte location in EEPROM.
*                   uint8_t *buffer ---> Pointer to user-provided buffer where data will be stored.
*                   uint32_t len   ---> Total number of bytes to be read.
* Created by      : Shirisha Ankam
* Date created    : 07-01-2025
* Description     : Function is written for reading single or multiple bytes from EEPROM.
*                   Developer can read from 0th location up to maximum EEPROM size (MX_BYTE_SIZE).
*                   Only need to send the starting byte location and length of data.
*                   Pages are selected automatically based on byte location/address.
* Notes           : Caller must provide a buffer of at least 'len' bytes.
*                   Function internally handles page boundaries (e.g., 512-byte pages).
* Global Variables Affected : NA
*************************************************************************************/
uint8_t EEPROM_Byte_Read(uint32_t loc, uint8_t *buffer, uint32_t len)
{
	#if DEBUG_EEPROM_BREAD
	Print_Message("\nInside EEPROM_Byte_Read Function.\n");  // Debug: Entry into function
	#endif

	// Validate the location and length
	if ((loc > MX_BYTE_SIZE) || (len < 1))
	return 1;  // Error: Invalid location or length

	// Calculate the page number and byte offset based on the logical location
	uint32_t page_num = (loc / EEPROM_PAGE_SIZE);   // Page number
	uint32_t byte_add = loc % EEPROM_PAGE_SIZE;     // Offset within the page

	#if DEBUG_EEPROM_BREAD
	Print_Message("\nRead location : ");
	Print_Number(loc);   // Debug: Print the read location
	Print_Message("\nRead length Data : ");
	Print_Number(len);   // Debug: Print the requested length
	#endif

	uint32_t add_counts = 0;        // Total number of bytes read so far
	uint8_t *dest = buffer;         // Destination pointer (caller’s buffer)

	// Loop until all requested bytes are read
	while (add_counts < len)
	{
		#if DEBUG_EEPROM_BREAD
		Print_Message("\nPage Number : ");
		Print_Number(page_num);     // Debug: Print current page number
		Print_Message("\nByte location : ");
		Print_Number(byte_add);     // Debug: Print current byte offset
		#endif

		// Calculate how many bytes can be read from the current page
		uint32_t temp_len = EEPROM_PAGE_SIZE - byte_add;  // Remaining bytes in this page
		if (temp_len > (len - add_counts))
		temp_len = (len - add_counts);  // Limit to remaining requested length

		// Perform the actual page read into the caller’s buffer
		EEPROM_Page_Read(page_num, byte_add, dest, temp_len);

		// Update counters and pointers
		add_counts += temp_len;     // Increase total bytes read
		dest       += temp_len;     // Advance destination buffer pointer
		page_num++;                 // Move to next page
		byte_add   = 0;             // Reset offset for next page
	}

	return 0;                       // Return success
}


/*****************************************************************************************
* Function name	: uint8_t EEPROM_Page_ERASE(uint32_t page_num)
* Returns		: uint8_t ---> returns 0 on success;
* Arguments		: uint32_t page_num ---> Send Page number to erase.
* Created by	: Shirisha Ankam
* Date created	: 01-01-2025
* Description	: Function is written for erasing the page.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
uint8_t EEPROM_Page_Erase(uint32_t page_num)
{
	#if DEBUG_EEPROM
	Print_Message("\nInside Erase_Page Function.\nPlease Wait...");
	#endif

	// Initialize variables
	uint32_t ADD_BYTE = 0;
	uint8_t eeprom_command_data[4];
	
	WR_PTR_HIGH;							  //Disabled Hardware Write Protection.
	delay_us(10);
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Disable full protection to allow writing
	Disable_Full_Protection();
	Write_Enable();
	// Wait for the EEPROM to be ready (WIP = 0)
	Wait_For_Write_Completion();
	
	// Prepare the erase command
	eeprom_command_data[0] = EEPROM_CMD_PAGE_ERASE;
	// Calculate the address (page number to byte address)
	ADD_BYTE = page_num << 9; // Convert page number to byte address for a 512-byte page.

	// Split the 24-bit address into 3 bytes
	eeprom_command_data[1] = (uint8_t)((ADD_BYTE & 0xFF0000) >> 16); // MSB
	eeprom_command_data[2] = (uint8_t)((ADD_BYTE & 0x00FF00) >> 8);  // Middle byte
	eeprom_command_data[3] = (uint8_t)(ADD_BYTE & 0x0000FF);         // LSB
	
	// Start the SPI transaction
	EP_CS_PIN_LOW;
	delay_us(10); // Optional delay for chip select stabilization
	// Transmit the command and address
	Data_To_SPI(eeprom_command_data, 4);
	// Wait for the SPI transaction to complete
	while (!spi_is_tx_empty(SPI));
	// End the SPI transaction
	EP_CS_PIN_HIGH;
	
	// Wait for the write operation to complete
	Wait_For_Write_Completion();
	WR_PTR_LOW;						 //Enabled Hardware Write Protection.
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Re-enable full protection after writing
	Enable_Full_Protection();
	Write_Disable();
	
	#if DEBUG_EEPROM
	Print_Message("\nPage Erase Complete");
	#endif

	return 0; // Return success
}






/*****************************************************************************************
* Function name	: void EEPROM_Chip_Erase(void)
* Returns		: Nothing.
* Arguments		: None.
* Created by	: Shirisha Ankam
* Date created	: 07-01-2025
* Description	: Function is written for erasing whole chip.
*               :
* Notes			: NA
* Global Variables Affected	: NA
******************************************************************************************/
void EEPROM_Chip_Erase(void)
{
	#if DEBUG_EEPROM
	Print_Message("\nInside Chip_Erase Function.\nPlease Wait...");
	#endif
	
	WR_PTR_HIGH;							  //Disabled Hardware Write Protection.
	delay_us(10);
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Disable full protection to allow writing
	Disable_Full_Protection();
	Write_Enable();
	// Wait for the EEPROM to be ready (WIP = 0)
	Wait_For_Write_Completion();

	// Prepare Chip Erase Command
	eeprom_command_data[0] = EEPROM_CMD_CHIP_ERASE; 

	// Send the Chip Erase Command
	EP_CS_PIN_LOW;
	delay_us(10);
	Data_To_SPI(eeprom_command_data, 1); 
	while (!spi_is_tx_empty(SPI)); // Ensure transmission is complete
	EP_CS_PIN_HIGH;
	
	// Wait for the write operation to complete
	Wait_For_Write_Completion();
	WR_PTR_LOW;						 //Enabled Hardware Write Protection.
	// Enable writing to the EEPROM by setting the Write Enable Latch
	Write_Enable();
	// Re-enable full protection after writing
	Enable_Full_Protection();
	Write_Disable();
	
	#if DEBUG_EEPROM
	Print_Message("\nChip Erase Successful.");
	#endif
}

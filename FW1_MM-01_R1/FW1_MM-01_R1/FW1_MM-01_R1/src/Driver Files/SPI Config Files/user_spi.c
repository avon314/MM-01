/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_spi.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 30/10/2022.
* Module
* Description	:	Initialization of spi driver.
*
* Controller	: 	ATSAM4S8B
*					512 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 30/10/2022.
* Changes		: NA.
*****************************************************************************/
#include <asf.h>

#include "user_spi.h"

static void configure_spi_cs_pin(void);


/*****************************************************************************
* Function name	: void configure_spi_master(void)
* Returns		: None.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 20-09-2022
* Description	: .
*               :
* Notes			: NA
* Global Variables Affected	: NA
*****************************************************************************/
void configure_spi_master(void)
{
	// Configure the SPI pins
	ioport_set_pin_mode(SPI_MISO_PIN, SPI_MISO_FLAG);
	ioport_disable_pin(SPI_MISO_PIN);
	ioport_set_pin_mode(SPI_MOSI_PIN, SPI_MOSI_FLAG);
	ioport_disable_pin(SPI_MOSI_PIN);
	ioport_set_pin_mode(SPI_SPCK_PIN, SPI_SPCK_FLAG);
	ioport_disable_pin(SPI_SPCK_PIN);
	ioport_set_pin_mode(SPI_NPCS0_PIN, SPI_NPCS0_FLAG);
	ioport_disable_pin(SPI_NPCS0_PIN);

	// Enable the SPI peripheral clock
	pmc_enable_periph_clk(ID_SPI);

	// Configure the SPI as master
	spi_enable_clock(SPI_MASTER_BASE);
	spi_reset(SPI_MASTER_BASE);
	spi_set_master_mode(SPI_MASTER_BASE);
	spi_disable_mode_fault_detect(SPI_MASTER_BASE);

	// Set fixed peripheral select (NSS)
	spi_set_peripheral_chip_select_value(SPI_MASTER_BASE, SPI_CHIP_SEL);

	// Set clock phase and polarity
	spi_set_clock_polarity(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_POLARITY);
	spi_set_clock_phase(SPI_MASTER_BASE, SPI_CHIP_SEL, SPI_CLK_PHASE);

	// Set the baudrate
	spi_set_baudrate_div(SPI_MASTER_BASE, SPI_CHIP_SEL, (sysclk_get_cpu_hz() / SPI_BAUDRATE));

	// Enable the SPI
	spi_enable(SPI_MASTER_BASE);
	
	// Configure chip select pin.
	configure_spi_cs_pin();
}

/*****************************************************************************
* Function name	: static void configure_spi_cs_pin(void)
* Returns		: Nothing.
* Arguments		: None.
* Created by	: Ranjitkumar Ainapure
* Date created	: 17-05-2024.
* Description	: Configure SPI chip select pin.
*               :
* Notes			: It is a user defined Chip Select Pin.
* Global Variables Affected	: NA
*****************************************************************************/
static void configure_spi_cs_pin(void)
{
	pmc_enable_periph_clk(ID_PIOA);
	pio_set_output(PIOA, PIO_PA15, HIGH, DISABLE, ENABLE);
}

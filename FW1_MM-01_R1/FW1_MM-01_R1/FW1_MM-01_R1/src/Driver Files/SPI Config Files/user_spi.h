#ifndef USER_SPI_H_
#define USER_SPI_H_

#define SPI_MASTER_BASE       SPI
#define SPI_CHIP_SEL          0
#define SPI_CLK_POLARITY      0
#define SPI_CLK_PHASE         1
#define SPI_BAUDRATE          1000000

#define SPI_MISO_PIN          PIO_PA12_IDX
#define SPI_MOSI_PIN          PIO_PA13_IDX
#define SPI_SPCK_PIN          PIO_PA14_IDX
#define SPI_NPCS0_PIN         PIO_PA11_IDX

#define SPI_MISO_FLAG         IOPORT_MODE_MUX_A
#define SPI_MOSI_FLAG         IOPORT_MODE_MUX_A
#define SPI_SPCK_FLAG         IOPORT_MODE_MUX_A
#define SPI_NPCS0_FLAG        IOPORT_MODE_MUX_A


void configure_spi_master(void);


#endif /* USER_SPI_H_ */
/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mb_tcp_server.c
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 09/11/2023.
* Module
* Description	: The on board LED functionalities are written here.
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*					128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 09/11/2023.
* Changes		: NA.
*****************************************************************************/
/***** Systems Includes *****/
#include "asf.h"
#include "string.h"

/***** User Includes *****/
#include "mb_tcp_server.h"
#include "user_uart.h"
#include "rs485_driver.h"

/***** Macros / Definitions *****/
/***** Function Prototypes / Declarations *****/

/***** Structure Variables *****/
MB_TCP_IP mbRecieve;
MB_TCP_IP mbTransmit;
MB_TCP_IP mbTemp;

/***** Function Prototypes *****/
void Create_MBTransmit_Frame(void);
void Update_mbFrame_length(U8);
U8 Fill_Header(void);
U8 Response_MB_Exception_Frame(void);
U8 Response_MB_Transmission_Frame(void);

/*****************************************************************************
* Function name	: void Validate_Modbus_Frame(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Once Modbus data validate that in this function.
*               :
* Notes			: NA
* Global Variables Affected : gb_mbTCP_rcvd_f ---> flag resets after check.
*****************************************************************************/
void Validate_Modbus_Frame(void)
{
	if (gb_mbTCP_rcvd_f == 1)	/* If received the modbus data over the TCP */
	{
		gb_mbTCP_rcvd_f = 0;	/* Reset the flag for next reception of the data. */
		
		/*********************************************************************/
		U16 lcl_mbrcvIdx = 0;	/* Initialize receiving index */
		
		/******************** MBAP Header ***************************/
		/* Get transaction ID */
		mbRecieve.txId = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		mbRecieve.txId = (mbRecieve.txId << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		/* Get Protocol ID */
		mbRecieve.protID = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		mbRecieve.protID = (mbRecieve.protID << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		/* Get Frame length */
		mbRecieve.frameLen = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		mbRecieve.frameLen = (mbRecieve.frameLen << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		/* Get Unit ID */
		mbRecieve.unitId = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		
		/******************** PDU ***********************/
		/* Get function code */
		mbRecieve.funCode = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		/* Get Starting address of register */
		mbRecieve.regAddress = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		mbRecieve.regAddress = (mbRecieve.regAddress << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		
		if (mbRecieve.funCode == PRESET_SINGLE_REG)
		{
			/* If function code is preset single register, Get Data from the client */
			mbRecieve.regDataBuf[0] = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
			mbRecieve.regDataBuf[1] = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		}
		else if (mbRecieve.funCode == PRESET_MULTIPLE_REG)
		{
			/* If function code is preset multiple registers */
			/* Get number of points or number of registers to preset */
			mbRecieve.numPoints = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
			mbRecieve.numPoints = (mbRecieve.numPoints << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
			/* Get number of bytes to preset */
			mbRecieve.regByteCount = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
			
			/* Get Data from the client */
			memcpy(mbRecieve.regDataBuf, &gb_mbTcp_rcv_buf[lcl_mbrcvIdx], (mbRecieve.numPoints * 2));
			lcl_mbrcvIdx = lcl_mbrcvIdx + (mbRecieve.numPoints * 2); /* Update buffer index here */
		}
		else
		{
			/* Get number of points or number of registers */
			mbRecieve.numPoints = gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
			mbRecieve.numPoints = (mbRecieve.numPoints << 8) | gb_mbTcp_rcv_buf[lcl_mbrcvIdx++];
		}
		Create_MBTransmit_Frame();	/* Create appropriate modbus frame */
	}
	else
	{
		/*Do nothing*/
	}
}

/*****************************************************************************
* Function name	: void Create_MBTransmit_Frame(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Once Modbus frame validated then create an appropriate
*				  reply / response frame.
*               :
* Notes			: NA
* Global Variables Affected : NA.
*****************************************************************************/
void Create_MBTransmit_Frame(void)
{
	/* Create local variables */
	U8 lcl_regAddIdx = 0;
	U8 lcl_regByteCount = 0;
	
	/* Initialize exception flag */
	mbTransmit.exception_f = FLAG_RST;
	
	/* Create the Header */
	/******************** MBAP Header ***************************/
	mbTransmit.txId = mbRecieve.txId;		/* Echo back the transaction ID */
	mbTransmit.protID = mbRecieve.protID;	/* Echo back the protocol ID */
	/* "mbRecieve.frameLen" Fill at the last */
	mbTransmit.unitId = mbRecieve.unitId;	/* Echo back the Unit ID */
	
	/******************** PDU ***********************/
	mbTransmit.funCode = mbRecieve.funCode;	/* Echo back the function ID */
	mbTransmit.regByteCount = (mbRecieve.numPoints * 2);	/* Calculate number of byte counts */
	
	switch (mbRecieve.funCode)
	{
		case READ_COIL_STATUS :
			// Skip
			/* Simply reply an acknowledge */
			mbTransmit.exception_f = FLAG_SET;
			mbTransmit.exceptionCode = EX_SERVER_ACK;
			
		break;
		case READ_HOLDING_REG :
			/* Simply reply an acknowledge */
			mbTransmit.exception_f = FLAG_SET;
			mbTransmit.exceptionCode = EX_SERVER_ACK;
			
			/* If the function code is Read holding register */
// 			if (mbRecieve.numPoints > 0)
// 			{
// 				/* If number of points valid? */
// 				if ((mbRecieve.regAddress >= minHoldRegAdd) && (mbRecieve.regAddress <= maxHoldRegAdd))
// 				{
// 					/* If requested register address is withn the range */
// 					lcl_regAddIdx = (U8)((mbRecieve.regAddress - minHoldRegAdd) * 2);	/* Calculate local index to read data from the register. */
// 					lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
// 					
// 					if (mbTransmit.regByteCount > lcl_regByteCount)
// 					{
// 						/* If requested byte counts are greater than available bytes then reply exception */
// 						mbTransmit.exception_f = FLAG_SET;
// 						mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
// 					}
// 					else
// 					{
// 						/* Copy data from the application data register to transmit register data buffer */
// 						memcpy(mbTransmit.regDataBuf, &mbTransmit.appHoldingBuf[lcl_regAddIdx], mbTransmit.regByteCount);
// 					}
// 					
// 				}
// 				else
// 				{
// 					/* If invalid address is requested then reply exception */
// 					mbTransmit.exception_f = FLAG_SET;
// 					mbTransmit.exceptionCode = EX_ILLIGAL_DATA_ADD;
// 				}
// 			}
// 			else
// 			{
// 				/* If invalid address is requested then reply exception */
// 				mbTransmit.exception_f = FLAG_SET;
// 				mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
// 			}
		break;
		case READ_INPUT_REG :
			/* If the function code is Read Input register */
			if (mbRecieve.numPoints > 0)
			{
				/* If number of points valid? */
				
				/* Initialize address error flag locally. */
				U8 lcl_addr_err_f = FLAG_RST;
			
				if ((mbRecieve.regAddress >= mnIpRgAd_DrState) && (mbRecieve.regAddress <= mxIpRgAd_DrState))
				{
					/* If requested register address is within the range */
					lcl_regAddIdx = (U8)((mbRecieve.regAddress - mnIpRgAd_DrState) *2);	/* Calculate local index to read data from the register. */
					lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
				}
// 				else if ((mbRecieve.regAddress >= mnIpRgAd_GrpState) && (mbRecieve.regAddress <= mxIpRgAd_GrpState))
// 				{
// 					/* If requested register address is within the range */
// 					lcl_regAddIdx = (U8)((mbRecieve.regAddress - mnIpRgAd_GrpState) *2);	/* Calculate local index to read data from the register. */
// 					lcl_regByteCount = (U8)((MAX_REG_BYTES / 2) - (lcl_regAddIdx));	/* Calculate local bytes counts */
// 					lcl_regAddIdx = lcl_regAddIdx + ipRgGrpIdx;	/* After calculating reg byte counts update index value again. */
// 				}
				else if ((mbRecieve.regAddress >= mnIpRgAd_DrAlarm) && (mbRecieve.regAddress <= mxIpRgAd_DrAlarm))
				{
					/* If requested register address is within the range */
					lcl_regAddIdx = (U8)((mbRecieve.regAddress - mnIpRgAd_DrAlarm) * 2);	/* Calculate local index to read data from the register. */
					lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
					lcl_regAddIdx = lcl_regAddIdx + ipRgAlrmIdx;	/* After calculating reg byte counts update index value again. */
				}
				else if ((mbRecieve.regAddress >= mnIpRgAd_DrOprt) && (mbRecieve.regAddress <= mxIpRgAd_DrOprt))
				{
					/* If requested register address is within the range */
					lcl_regAddIdx = (U8)((mbRecieve.regAddress - mnIpRgAd_DrOprt) * 2);	/* Calculate local index to read data from the register. */
					lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
					lcl_regAddIdx = lcl_regAddIdx + ipRgOprtIdx;	/* After calculating reg byte counts update index value again. */
				}
				else
				{
					/* Set an error flag. */
					lcl_addr_err_f = FLAG_SET;
				}
			
				if (!lcl_addr_err_f)
				{
					/* If there is no error in register address */
					if (mbTransmit.regByteCount > lcl_regByteCount)
					{
						/* If requested byte counts are greater than available bytes then reply exception */
						mbTransmit.exception_f = FLAG_SET;
						mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
					}
					else
					{
						/* Copy data from the application data register to transmit register data buffer */
						memcpy(mbTransmit.regDataBuf, &mbTransmit.appIpRegBuf[lcl_regAddIdx], mbTransmit.regByteCount);
					}
				}
				else
				{
					/* If invalid address is requested then reply exception */
					mbTransmit.exception_f = FLAG_SET;
					mbTransmit.exceptionCode = EX_ILLIGAL_DATA_ADD;
				}
			}
			else
			{
				/* If invalid address is requested then reply exception */
				mbTransmit.exception_f = FLAG_SET;
				mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
			}
		break;
		case PRESET_SINGLE_REG : 
			/* Simply reply an acknowledge */
			mbTransmit.exception_f = FLAG_SET;
			mbTransmit.exceptionCode = EX_SERVER_ACK;
			
			/* If the function code is preset single holding register */
// 			if ((mbRecieve.regAddress >= minHoldRegAdd) && (mbRecieve.regAddress <= maxHoldRegAdd))
// 			{
// 				/* If requested register address is withn the range */
// 				lcl_regAddIdx = (U8)((mbRecieve.regAddress - minHoldRegAdd) * 2);	/* Calculate local index to read data from the register. */
// 				lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
// 				mbTransmit.regByteCount = 2;	/* As single register there is always 2 bytes */
// 				
// 				/*
// 				* Update application holding register with received values
// 				* And also copy the same data to transmit register buffer to echo back
// 				*/
// 				memcpy(&mbTransmit.appHoldingBuf[lcl_regAddIdx], mbRecieve.regDataBuf, mbTransmit.regByteCount);
// 				memcpy(mbTransmit.regDataBuf, &mbTransmit.appHoldingBuf[lcl_regAddIdx], mbTransmit.regByteCount);
// 			}
// 			else
// 			{
// 				/* If invalid address is requested then reply exception */
// 				mbTransmit.exception_f = FLAG_SET;
// 				mbTransmit.exceptionCode = EX_ILLIGAL_DATA_ADD;
// 			}
		break;
		case PRESET_MULTIPLE_REG :
			/* Simply reply an acknowledge */
			mbTransmit.exception_f = FLAG_SET;
			mbTransmit.exceptionCode = EX_SERVER_ACK;
			
			/* If the function code is preset multiple holding registers */
// 			if (mbRecieve.numPoints > 0)
// 			{
// 				/* If number of points valid? */
// 				if ((mbRecieve.regAddress >= minHoldRegAdd) && (mbRecieve.regAddress <= maxHoldRegAdd))
// 				{
// 					/* If requested register address is within the range */
// 					lcl_regAddIdx = (U8)((mbRecieve.regAddress - minHoldRegAdd) * 2);	/* Calculate local index to read data from the register. */
// 					lcl_regByteCount = (U8)(MAX_REG_BYTES - lcl_regAddIdx);	/* Calculate local bytes counts */
// 				
// 					if (mbTransmit.regByteCount > lcl_regByteCount)
// 					{
// 						/* If requested byte counts are greater than available bytes then reply exception */
// 						mbTransmit.exception_f = FLAG_SET;
// 						mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
// 					}
// 					else
// 					{
// 						/* Copy the data to transmit register buffer to echo back */
// 						memcpy(&mbTransmit.appHoldingBuf[lcl_regAddIdx], mbRecieve.regDataBuf, mbTransmit.regByteCount);
// 						mbTransmit.preset_mReg_f = 1;
// 					}
// 				
// 				}
// 				else
// 				{
// 					/* If invalid register address is requested then reply exception */
// 					mbTransmit.exception_f = FLAG_SET;
// 					mbTransmit.exceptionCode = EX_ILLIGAL_DATA_ADD;
// 				}
// 			}
// 			else
// 			{
// 				/* If invalid register byte count is requested then reply exception */
// 				mbTransmit.exception_f = FLAG_SET;
// 				mbTransmit.exceptionCode = EX_ILLIGAL_DATA_VAL;
// 			}
		break;
		default:
			/* if default, simply reply an acknowledge */
			mbTransmit.exception_f = FLAG_SET;
			mbTransmit.exceptionCode = EX_SERVER_ACK;
		break;
	}
	
	if (mbTransmit.exception_f == FLAG_SET)
	{
		/* If an exception flag is set then reply an exception frame.
		   And update transmit frame length */
		Update_mbFrame_length(Response_MB_Exception_Frame());
	}
	else
	{
		/* Create response frame / reply frame.
		   And update transmit frame length */
		Update_mbFrame_length(Response_MB_Transmission_Frame());
	}
	
	gb_mbTCP_tx_f = 1;
}

/*****************************************************************************
* Function name	: U8 Fill_Header(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Fill modbus tcp header data in the transmit buffer.
*               :
* Notes			: NA
* Global Variables Affected : gb_mbTcp_tx_buf[] ---> Updates buffer.
*****************************************************************************/
U8 Fill_Header(void)
{
	/* Initialize local index */
	U8 lcl_mbtxIdx = 0;
	
	/* Fill two bytes of transaction id in the buffer */
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbTransmit.txId & 0xFF00) >> 8;
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.txId & 0x00FF;
	/* Fill two bytes of protocol id in the buffer */
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbTransmit.protID & 0xFF00) >> 8;
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.protID & 0x00FF;
	/* Frame length (2 Bytes )  (Fill at the end) */
	lcl_mbtxIdx++;	// Increment idx by one (Higher Byte).
	lcl_mbtxIdx++;	// Increment idx by one (Lower Byte).
	/* Fill unit id in the buffer */
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.unitId;
	
	if (mbTransmit.exception_f == FLAG_SET)
	{
		/* If exception flag is set, set the msb bit of the function code and reply */
		gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbTransmit.funCode | 0x80);
	}
	else
	{
		/* Fill function code */
		gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.funCode;		
		
		if (mbTransmit.funCode == PRESET_SINGLE_REG)
		{
			/* Fill register address to echo back */
			/*mbRecieve.regAddress += 1;*/
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.regAddress & 0xFF00) >> 8;
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.regAddress & 0x00FF);
		}
		else if (mbTransmit.funCode == PRESET_MULTIPLE_REG)
		{
			/* Fill register address and number of points to echo back */
			/*mbRecieve.regAddress += 1;*/
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.regAddress & 0xFF00) >> 8;
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.regAddress & 0x00FF);
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.numPoints & 0xFF00) >> 8;
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = (mbRecieve.numPoints & 0x00FF);
		}
		else
		{
			/* Fill number of bytes */
			gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.regByteCount;
		}
	}
	
	return lcl_mbtxIdx;	/* Return index value to update buffer in next function */
}

/*****************************************************************************
* Function name	: U8 Response_MB_Exception_Frame(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Creates modbus exception frame.
*               :
* Notes			: NA
* Global Variables Affected :  gb_mbTcp_tx_buf[] ---> Updates buffer.
*****************************************************************************/
U8 Response_MB_Exception_Frame(void)
{
	/* Initialize local index value */
	U8 lcl_mbtxIdx = 0;
	lcl_mbtxIdx = Fill_Header();	/* Get index value after filling the header values */
	gb_mbTcp_tx_buf[lcl_mbtxIdx++] = mbTransmit.exceptionCode;	/* Fill exception code */
	
	return lcl_mbtxIdx;	/* Return index value to update buffer in next function */
}

/*****************************************************************************
* Function name	: U8 Response_MB_Transmission_Frame(void)
* Returns		: nothing.
* Arguments    	: None.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Creates modbus transmission frame.
*               :
* Notes			: NA
* Global Variables Affected :  gb_mbTcp_tx_buf[] ---> Updates buffer.
*****************************************************************************/
U8 Response_MB_Transmission_Frame(void)
{
	/* Initialize local index value */
	U8 lcl_mbtxIdx = 0;
	
	lcl_mbtxIdx = Fill_Header();	/* Get index value after filling the header values */
	
	if (mbTransmit.funCode != PRESET_MULTIPLE_REG)
	{
		/*
		* Other than preset multiple holding registers fill the transmit data buffer with 
		* transmit register buffer values
		*/
		memcpy(&gb_mbTcp_tx_buf[lcl_mbtxIdx], mbTransmit.regDataBuf, mbTransmit.regByteCount);
		lcl_mbtxIdx = (lcl_mbtxIdx + mbTransmit.regByteCount);
	}
	
	return lcl_mbtxIdx;	/* Return index value to update buffer in next function */
}

/*****************************************************************************
* Function name	: void Update_mbFrame_length(U8 idxLength)
* Returns		: nothing.
* Arguments    	: U8 idxLength ---> Receives the transmission buffer index.
* Created by	: Ranjitkumar Ainapure.
* Date created	: 25/11/2023.
*
* Description	: Updates the modbus transmission frame length.
*               :
* Notes			: NA
* Global Variables Affected :  gb_mbTcp_tx_buf[] ---> Updates buffer.
							   gb_mbTCP_txbuf_len ---> frame length.
*****************************************************************************/
void Update_mbFrame_length(U8 idxLength)
{
	gb_mbTcp_tx_buf[4] = ((idxLength - mbHeaderLength) & 0xFF00) >> 8;
	gb_mbTcp_tx_buf[5] = ((idxLength - mbHeaderLength) & 0x00FF);
	gb_mbTCP_txbuf_len = idxLength;
}
/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: mb_tcp_server.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 09/11/2023.
* Module
* Description	: Header file for mb_tcp_server.c
				  Defines constants and macros for mb_tcp_server.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 09/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef MB_TCP_SERVER_H_
#define MB_TCP_SERVER_H_

#include "tcp_server.h"

/***** Macros related to MODBUS Registers *****/
#define READ_COIL_STATUS	(0x01)
#define READ_HOLDING_REG	(0x03)
#define READ_INPUT_REG		(0x04)
#define PRESET_SINGLE_REG	(0x06)
#define PRESET_MULTIPLE_REG	(0x10)

/***** General modbus related macros *****/
#define SIZE_DATA_BUF		(255)
#define MAX_REG_BYTES		(32)	/* Application requires this much */
#define MAX_IP_REG_BYTES	(112)	/* Application requires this much of bytes for IP registers */

/************ Application related Macros *****/
#define minHoldRegAdd		((3001) - (1))
#define maxHoldRegAdd		((3016) - (1))

#define mnIpRgAd_DrState	((1) - (1))
#define mxIpRgAd_DrState	((16) - (1))
#define mnIpRgAd_GrpState	((1001) - (1))
#define mxIpRgAd_GrpState	((1008) - (1))
#define mnIpRgAd_DrAlarm	((2001) - (1))
#define mxIpRgAd_DrAlarm	((2016) - (1))
#define mnIpRgAd_DrOprt		((4001) - (1))
#define mxIpRgAd_DrOprt		((4016) - (1))
#define mbHeaderLength		((7) - (1))

#define ipRgGrpIdx			(32)
#define ipRgAlrmIdx			(48)
#define ipRgOprtIdx			(80)

/**************** General Macros related to the Driver **************/
#define FLAG_SET	(1)
#define FLAG_RST	(0)

/***** Define contsant or enum variables for modbus exception codes *****/
enum
{
	EX_ILLIGAL_FUN = 0x01,		/* Illegal functionality */ 
	EX_ILLIGAL_DATA_ADD,		/* Illegal register address */
	EX_ILLIGAL_DATA_VAL,		/* Illegal data address/value */
	EX_SERVER_DEV_FAIL,			/* Server device failed */
	EX_SERVER_ACK,				/* Server acknowledgment */
	EX_SERVER_DEV_BUSY,			/* Server device is busy */
	EX_SERVER_NOT_ACK,			/* Server not acknowledged */
	EX_MEM_PARITY_ERR,			/* Memory parity error */
	EX_GATEWAY_PROBLEM1 = 0x0A,	/* Gateway Problem */
	EX_GATEWAY_PROBLEM2,		/* Gateway Problem */
	EXTENDED_EX_RESP = 0xFF		/* Extended Exception Response */
};

/***** Declaration of structure variable Modbus frame related *****/
typedef struct  
{
	U8 preset_mReg_f : 1;
	U8 exception_f;		/* Exception flag is set when error occured */
	U8 exceptionCode;	/* Exception code to be sent */
	
	U8 unitId;			/* Unit ID (1 Byte Value) */
	U8 funCode;			/* Function Code (1 Byte Value) */
	U8 regByteCount;	/* Register byte counts (1 Byte Value) */
	
	U8 regDataBuf[SIZE_DATA_BUF];		/* Register data buffer. */
	U8 appHoldingBuf[MAX_REG_BYTES];	/* App Holding Register data buffer. */
	U8 appIpRegBuf[MAX_IP_REG_BYTES];	/* App Input Register data buffer. */
	
	U16 txId;			/* Transaction ID (2 Bytes Value) */
	U16 protID;			/* Protocol Identifier (2 Bytes Value) */
	U16 frameLen;		/* Header Length (2 Bytes Value) */
	U16 regAddress;		/* Register Address (2 Bytes Value) */
	U16 numPoints;		/* Number of Points (2 Bytes Value) */
}MB_TCP_IP;

/***** Extern Structure variables *****/
extern MB_TCP_IP mbRecieve;
extern MB_TCP_IP mbTransmit;
extern MB_TCP_IP mbTemp;

/***** Function Prototypes *****/
void Validate_Modbus_Frame(void);
#endif /* MB_TCP_SERVER_H_ */
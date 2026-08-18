/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: user_uart.c
* Created By	: Mr Harshal Belan.
* Copied By		: Ranjitkumar Ainapure.
* Copied Date	: 18/08/2023.
*
* Module Description :	Calculate/Check CRC of Array.
*
*
* Controller	: 	ATSAM4S8B
*					512 KB		Flash.
*					128 KB		SRAM.
*
* REVISION HISTORY
* Version : 1.0
* Revision Date: 18/08/2023.
* Changes : NA.
*****************************************************************************/
#include "CRC.h"
#include "user_uart.h"
/*****************************************************************************
* Function name     : char CRC_Calculate_Check(unsigned char*Frame_Ptr, char Operation_Type)
* Returns           : 1 for data validity, 0 for data corruption
* Arguments         : Frame_Ptr = Holds the base address of the passed Array
*                   : Data Limit = Number of data bytes from SOF to one less than CRC LSB position
*                   : Op_Type = Specifies operation to be performed i.e. Calc or check CRC
* Created by        : Harshal Belan
* Date created      : 01/11/12
* Description       : Calculate/Check CRC of Array Passed by reference
* Global Variables Affected: Temp_CRC[],
* Notes                                                :
*****************************************************************************/
unsigned char CRC_Calculate_Check(unsigned char*Frame_Ptr, int Data_Limit, unsigned char Op_Type)
{
	unsigned char lcl_BitNo;

    unsigned int lcl_crc0 = 0xFFFF;
    unsigned int lcl_Actul_crc;
	unsigned int lcl_Index;

    /* CRC calculation Engine */
    for(lcl_Index=0;lcl_Index<Data_Limit;lcl_Index++)
    {
        lcl_crc0 = lcl_crc0 ^ (*(Frame_Ptr+lcl_Index));

        for(lcl_BitNo=1;lcl_BitNo <= 8;lcl_BitNo++)
        {
            if(lcl_crc0 & 0x0001)
            {
                lcl_crc0  = lcl_crc0>>1;
                lcl_crc0  = lcl_crc0 ^ 0xA001;
            }
            else
                lcl_crc0 = lcl_crc0 >> 1;
        }
    }
	
    /* If CRC Calculation Request */
    if(Op_Type == CRC_CALC)
    {
//        *(Frame_Ptr + lcl_Index++)= (lcl_crc0&0x00FF);                     //low byte of crc
//        *(Frame_Ptr + lcl_Index)  = (lcl_crc0&0xFF00)>>8;                 //high byte of crc

        gb_Temp_CRC[0]= (lcl_crc0&0x00FF);
        gb_Temp_CRC[1]= (lcl_crc0&0xFF00)>>8;
    }
    /* Else if CRC Chk Request */
    else if(Op_Type == CRC_CHECK)
    {
        lcl_Actul_crc  = (unsigned int)*(Frame_Ptr + lcl_Index++);           //high byte RECEIVED CRC
		lcl_Actul_crc |= (unsigned int)*(Frame_Ptr + lcl_Index)<<8;        //low  byte RECEIVED CRC
		
	    /* If Calc CRC matches Actual CRC*/
        if(lcl_crc0 == lcl_Actul_crc)
            return 1;
        /* else if Calc CRC Do not matches Actual CRC*/
        else
            return 0;
    }
    return 1;
}//End of function CRC_Check

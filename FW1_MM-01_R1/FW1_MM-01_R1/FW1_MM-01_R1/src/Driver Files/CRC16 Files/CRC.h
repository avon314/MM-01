/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: CRC.h
* Created By	: Mr Harshal Belan.
* Copied By		: Ranjitkumar Ainapure.
* Copied Date	: 18/08/2023.
*
* Module Description : Defines constants and macros and header file for CRC.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 18/08/2023.
* Changes		: NO.
*****************************************************************************/
unsigned char CRC_Calculate_Check(unsigned char*Frame_Ptr, int Data_Limit, unsigned char Op_Type);

#ifndef CRC_CALC
#define CRC_CALC (1)                  //macro used for CRC calculation
#endif
#ifndef CRC_CHECK
#define CRC_CHECK (0)                 //macro used for CRC calculation
#endif
unsigned char gb_Temp_CRC[2];       //var used for CRC calculation

/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: json_extraction.h
* Created By	: Shirisha  Ankam.
* Created Date	: 30/07/2025.
* Module
* Description	:
*
* Controller	: 	ATSAM4E16CA-AUR
*					1024 KB		Flash.
*                   128 KB		RAM.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 30/07/2025.
* Changes		: NA.
*****************************************************************************/

#ifndef FROZEN_EXTRACTION_H_
#define FROZEN_EXTRACTION_H_
#include "frozen.h"


#define DEBUG_JSON_EXT          (1)


typedef struct
{
	U32 regAdds;
	U8 data_arr[512];
	U16 data_len;
}GEN_MEM;
extern GEN_MEM gen;

typedef void (*FieldProcessor)(struct json_token token);

typedef struct {
	const char *key;
	FieldProcessor process_func;
} FieldEntry;

uint8_t hex_to_value(char c);
void Take_Action(unsigned char* atBuffer);

void Extract_JsonData_Using_Frozen_Library(const char *json);
void Extract_DI_Data_From_JSON(struct json_token token);
void Extract_IP_Data_From_JSON(struct json_token token);
void Extract_OP_Data_From_JSON(struct json_token token);
void Extract_RC_Data_From_JSON(struct json_token token);
void Extract_CT_Data_From_JSON(struct json_token token);
void Extract_MP_Data_From_JSON(struct json_token token);
void Extract_WR_Data_From_JSON(struct json_token token);
void Extract_MS_Data_From_JSON(struct json_token token);
void Extract_SI_Data_From_JSON(struct json_token token);
void Extract_AM_Data_From_JSON(struct json_token token);
void Extract_BT_Data_From_JSON(struct json_token token);
void Create_JSON_Frame(const char* dev_mac_id, const char* ipAddress, U8 model_type, U8 dev_type,
const uint8_t* lastDtTime, int dp,
const uint8_t* logEntries, U8 numEntries, char* jsonBuffer, U16 bufferSize) ;
#endif /* FROZEN_EXTRACTION_H_ */
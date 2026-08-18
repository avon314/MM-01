/*****************************************************************************
*                       Copyright (c)
*              Avon Building Solutions Pvt Ltd
*                   All RIGTHS RESERVED.
*
* Module Name	: tcp_server.h
* Created By	: Ranjitkumar Ainapure.
* Created Date	: 08/11/2023.
* Module
* Description	: Header file for tcp_server.c
				  Defines constants and macros for tcp_server.c.
*
* REVISION HISTORY
* Version		: 1.0
* Revision Date	: 08/11/2023.
* Changes		: NO.
*****************************************************************************/
#ifndef TCP_SERVER_H_
#define TCP_SERVER_H_

/***** Macro definitions *****/
#define RCV_NUM_OF_CHR		(255)
#define TX_NUM_OF_CHR		(255)
/** The priorities at which various tasks will get created. */
#define mainTCP_TASK_PRIORITY         (tskIDLE_PRIORITY)
/** The stack sizes allocated to the DSP stack: (256 * 4) = 1024 bytes. */
#define mainTCP_TASK_STACK_SIZE       (256)
#define SERVER_DATA_SIZE		(620)	// 1500
/***** Function Prototypes / Declarations *****/
void Create_TCP_Server_Task(void);
void Heap_Memory_Is(void);
U8 Inspect_Received_Frame(U8 *rcv_frame, U16 rcv_len);
void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
int hex_to_int(const char *hex_str);
void Mydns_found_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg);
void perform_dns_lookup_and_post(void);
/***** Global Function Prototypes *****/
extern U8 gb_mbTcp_rcv_buf[RCV_NUM_OF_CHR];
extern U16 gb_mbTCP_rcvbuf_len;
extern U8 gb_mbTCP_rcvd_f;

extern U8 gb_mbTcp_tx_buf[TX_NUM_OF_CHR];
extern U8 gb_mbTCP_txbuf_len;
extern U8 gb_mbTCP_tx_f;

#endif /* TCP_SERVER_H_ */
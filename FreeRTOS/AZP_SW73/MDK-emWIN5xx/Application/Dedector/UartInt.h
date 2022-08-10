#ifndef UART_INT_H
#define UART_INT_H

#include "AppCommon.h"
#include "UMDShared.h"

#define UART_INT_USED_PORT_NUM		COMMUNICATION_USART_PORT_NUM		/* Which UART will b used for uart interrupt */
#if (UART_INT_USED_PORT_NUM == 0)
	#define	UART_INT_HW_PORT			UART_0
	#define _UART_IRQ			UART0_IRQn
	#define _UART_IRQHander		UART0_IRQHandler
#elif (UART_INT_USED_PORT_NUM == 1)
	#define UART_INT_HW_PORT			UART_1
	#define _UART_IRQ			UART1_IRQn
	#define _UART_IRQHander		UART1_IRQHandler
#elif (UART_INT_USED_PORT_NUM == 2)
	#define UART_INT_HW_PORT			UART_2
	#define _UART_IRQ			UART2_IRQn
	#define _UART_IRQHander		UART2_IRQHandler
#elif (UART_INT_USED_PORT_NUM == 3)
	#define UART_INT_HW_PORT			UART_3
	#define _UART_IRQ			UART3_IRQn
	#define _UART_IRQHander		UART3_IRQHandler
#elif (UART_INT_USED_PORT_NUM == 4)
	#define UART_INT_HW_PORT			UART_4
	#define _UART_IRQ			UART4_IRQn
	#define _UART_IRQHander		UART4_IRQHandler
#endif

typedef void (*comm_timeout_fail_func) (void *msgp);
extern void UartInt_init(void);
extern uint32_t UARTSend(uint8_t *txbuf, uint16_t timeout, comm_timeout_fail_func ffunc);
extern void ReTXLastMsg(void);
extern void StopCommTimeout(void);
	
extern UmdPkt_Type last_send_msg;
extern UmdPkt_Type last_recv_msg;

extern volatile uint_fast8_t uart_pkt_rcv_active;
extern volatile uint_fast16_t uart_pkt_rcv_msec;
extern void handle_uart_rcv_timeout(void);


#endif

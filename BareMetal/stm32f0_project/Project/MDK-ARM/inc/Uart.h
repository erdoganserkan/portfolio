#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "Common.h"
#include "HDSerial.h"

/* TERMINAL - UART1 */
#define USART1TX_PIN                    	  GPIO_Pin_2
#define USART1TX_GPIO_PORT            	    GPIOA
#define USART1RX_PIN                    	  GPIO_Pin_3
#define USART1RX_GPIO_PORT            	    GPIOA
typedef enum 
{
  COM1 = 0,
  COM2 = 1
} COM_TypeDef;   

#define COMn                             1
#define EVAL_COM1                        USART1
#define EVAL_COM1_CLK                    RCC_APB2Periph_USART1

#define EVAL_COM1_TX_PIN                 GPIO_Pin_2
#define EVAL_COM1_TX_GPIO_PORT           GPIOA
#define EVAL_COM1_TX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_TX_SOURCE              GPIO_PinSource2
#define EVAL_COM1_TX_AF                  GPIO_AF_1

#define EVAL_COM1_RX_PIN                 GPIO_Pin_3
#define EVAL_COM1_RX_GPIO_PORT           GPIOA
#define EVAL_COM1_RX_GPIO_CLK            RCC_AHBPeriph_GPIOA
#define EVAL_COM1_RX_SOURCE              GPIO_PinSource3
#define EVAL_COM1_RX_AF                  GPIO_AF_1
   
#define EVAL_COM1_IRQn                   USART1_IRQn
#define EVAL_COM1_RCC    									RCC_APB2Periph_USART1


#define TERMINAL_RX_LENGTH			(128)
#define TERMINAL_TX_LENGTH			TERMINAL_RX_LENGTH
#define TERMINAL_MAX_MSG_LENGTH	(64)

typedef enum
{
	IDLE_STATE	= 0,
	BUSY_STATE,
	COMPLETE_STATE,
	ERR_STATE,
	
	TRANSMISSION_STATE_COUNT
} TransmissionState_Type;

/* Functions */
extern void Terminal_ISR(void);
extern void TerminalUsartInit(void);
extern void TerminalProcessMsg(void); 
extern void TerminalSendMsg(uint8_t *new_msg);
extern uint8_t Terminal_IsMsgPending(void);
extern void my_usart_init(void);
extern void Terminal_OnRXTimeout(void);
extern void prep_send_msg(uint8_t cmd_param, uint8_t dlen_param, uint8_t *dptr);
extern void DoLinuxMCUStatusProcessing(void);

/* Variables */
extern volatile uint8_t UsartRXState;
extern volatile uint8_t UsartTXState;
extern volatile uint8_t UsartRXTimeout;
extern volatile uint16_t UsartRXTimeoutCnt;

#endif

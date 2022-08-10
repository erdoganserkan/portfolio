#include <stdio.h>
#include <string.h>
#include "stm32f0xx.h"
#include "SysInit.h"
#include "MiddleWare.h"
#include "Main.h"
#include "AdcRead.h"
#include "Uart.h"

#define GET_RXBUFFER_DATA(relative_pos, result) {\
	uint16_t TempU16 = (uint16_t)LocalRXTail + (uint16_t)relative_pos;\
	if(TempU16 >= TERMINAL_RX_LENGTH)\
		TempU16 -= TERMINAL_RX_LENGTH;\
	result = RXBuffer[TempU16];\
}

#define WAIT_TX_IDLE()  \
{\
	volatile uint8_t LocalTxState;\
	do \
	{\
		ATOMIC_START(EVAL_COM1_IRQn);\
		LocalTxState = UsartTXState;\
		ATOMIC_END(EVAL_COM1_IRQn);\
	}	while(LocalTxState != IDLE_STATE);\
}

#define START_TRANSFER()	\
{\
	WAIT_TX_IDLE();\
	ATOMIC_START(EVAL_COM1_IRQn);\
	UsartTXState = BUSY_STATE;\
	USART_ITConfig(EVAL_COM1, USART_IT_TXE, ENABLE);\
	ATOMIC_END(EVAL_COM1_IRQn);\
}	

#define STOP_TRANSFER()		\
{\
	USART_ITConfig(EVAL_COM1, USART_IT_TXE, DISABLE);\
	UsartTXState = IDLE_STATE;\
}

port_pin_type const modem_controls[MODEM_COUNT] = {
	{M1RESET_GPIO_PORT, M1RESET_PIN}, {M2RESET_GPIO_PORT, M2RESET_PIN}, {M3RESET_GPIO_PORT, M3RESET_PIN},  
	{M4RESET_GPIO_PORT, M4RESET_PIN}, {M5RESET_GPIO_PORT, M5RESET_PIN}, {M6RESET_GPIO_PORT, M6RESET_PIN}
};

/* Variables */
static uint8_t RXBuffer[TERMINAL_RX_LENGTH] = {0};
static volatile uint8_t RXTail = 0;							/* Receive Processing Counter */
static volatile uint8_t RXHead = 0;							/* Receive Writing Counter */
static volatile uint8_t RXNextPktIndx = 0;					/* Next Packet starting address in RXBuffer[] */
static volatile uint8_t RPTargetCounter = 0;		/* Next Packet address to be processed by main() */
volatile uint8_t UsartRXState = IDLE_STATE;				/* Usart has complated the receiving of a new message */
volatile uint8_t UsartRXTimeout = FALSE;
volatile uint16_t UsartRXTimeoutCnt = 0;

static uint8_t TXBuffer[TERMINAL_TX_LENGTH] = {0};
static volatile uint8_t TXTail = 0;							/* Transmit Processing Counter */
static volatile uint8_t TXHead = 0;							/* Transmit Writing Counter */			
volatile uint8_t UsartTXState = IDLE_STATE;			/* Usart HW ready for sending new data */

/* Functions */
static void STM_EVAL_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct);

void Terminal_OnRXTimeout(void)
{
	USART_ITConfig(EVAL_COM1, USART_IT_RXNE, DISABLE);
	RXTail = 0;							/* Receive Processing Counter */
	RXHead = 0;							/* Receive Writing Counter */
	RXNextPktIndx = 0;					/* Next Packet starting address in RXBuffer[] */
	RPTargetCounter = 0;		/* Next Packet address to be processed by main() */
	UsartRXState = IDLE_STATE;				/* Usart has complated the receiving of a new message */
	UsartRXTimeout = FALSE;
	UsartRXTimeoutCnt = 0;
	uint8_t ReadByte = USART_ReceiveData(EVAL_COM1);
	USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
}	
	
uint8_t Terminal_IsMsgPending(void)
{	
	uint8_t localUS;
	
	ATOMIC_START(EVAL_COM1_IRQn);	
	localUS = UsartRXState;
	ATOMIC_END(EVAL_COM1_IRQn);	
	if(localUS == COMPLETE_STATE)
		return TRUE;
	else
		return FALSE;
}

void Terminal_ISR(void)
{
	uint16_t TempU16;
  if(USART_GetITStatus(EVAL_COM1, USART_IT_RXNE) != RESET) {
		uint8_t ReadByte;
		/* Clear the COM Receive Completed pending bit */
		ReadByte = USART_ReceiveData(EVAL_COM1);
		//USART_RequestCmd(EVAL_COM1, USART_Request_RXFRQ, ENABLE);
		if(RXHead == RXNextPktIndx) {
			/* LENGTH byte received */
			#if(0 == STM_FIXED_LENGTH_PACKETS)
				TempU16 = (uint16_t)RXNextPktIndx + (uint16_t)ReadByte; // Add packet length //
			#else
				TempU16 = (uint16_t)RXNextPktIndx + (uint16_t)sizeof(stm_comm_t); // Add packet length //
			#endif			
			if(TempU16 >= TERMINAL_RX_LENGTH)
					TempU16 -= TERMINAL_RX_LENGTH;
			RXNextPktIndx = (uint8_t)TempU16; 
			//:TODO: Start packet receive timeout 
			UsartRXState = BUSY_STATE;
		}
		// Store new data into circular receive buffer // 
		RXBuffer[RXHead++] = ReadByte;
		if(RXHead == TERMINAL_RX_LENGTH)
			RXHead = 0;

		// Check if we have received the last byte of active packet //
		if(RXHead == RXNextPktIndx)
		{
			/* "RXHead" has reached to the START OF NEXT PACKET */
			RPTargetCounter = RXHead;
			/* End of user message received, invoke main process to process it */
			UsartRXState = COMPLETE_STATE;
			//:TODO: Stop packet receive timeout timer 
		}
  }

  if(USART_GetITStatus(EVAL_COM1, USART_IT_TXE) != RESET) {   
    /* Write one byte to the transmit data register */
    USART_SendData(EVAL_COM1, TXBuffer[TXTail++]);
		if(TXTail == TERMINAL_TX_LENGTH)
			TXTail = 0;
		/* All desired characters has been send ??? */
    if(TXTail == TXHead)
    {
      /* Disable the EVAL_COM1 Transmit interrupt */
			STOP_TRANSFER();
    }
  }
}


void TerminalSendMsg(uint8_t *new_msg)
{
	volatile uint16_t indx;
	// Truncate message if longer than maximum // 
	if(sizeof(stm_comm_t) > TERMINAL_MAX_MSG_LENGTH) {
		while(1);
	}
	// Waait active trasmit to end, if there is any // 
	WAIT_TX_IDLE();
	// Store new message into tx buffer from last byte // 
	for(indx=0 ; (indx<sizeof(stm_comm_t)) ; indx++) {
		TXBuffer[TXHead++] = *new_msg++;
		if(TXHead == TERMINAL_TX_LENGTH)
			TXHead = 0;
	}
	// start sending message over uart // 
	START_TRANSFER();
}


void prep_send_msg(uint8_t cmd_param, uint8_t dlen_param, uint8_t *dptr)
{
	static stm_comm_t pkt = {
		.start_buf = "PMUF",
		.stop_buf = "PMUL"
	};
	volatile uint8_t indx;
	uint8_t *dst_ptr;

	memset(pkt.data, 0, sizeof(pkt.data));
	dst_ptr = pkt.data;
	pkt.cmd = cmd_param;
	pkt.len = dlen_param;
	for(indx=0 ; indx<dlen_param ; indx++)
		*dst_ptr++ = *dptr++;
	TerminalSendMsg((uint8_t *)&pkt);
}
	

void TerminalProcessMsg(void)
{
	const char start_sync[STM_PID_LENGTH] = "PMUF";
	const char stop_sync[STM_PID_LENGTH] = "PMUL";
	volatile uint8_t indx;
	uint8_t tempU8;
	uint8_t LocalRXTail = RXTail;
	uint8_t ActiveCmd, FirstData;
	uint8_t LocalRPTargetCounter;
	uint16_t TempU16;

	do
	{
		ATOMIC_START(EVAL_COM1_IRQn);					// ATOMIC START // 
		LocalRPTargetCounter = RPTargetCounter;
		ATOMIC_END(EVAL_COM1_IRQn);						// ATOMIC STOP // 

		// Get & Compare START-SYSNC //
		for(indx=0 ; indx<STM_PID_LENGTH ; indx++) {
			GET_RXBUFFER_DATA((SYNC_START_INDX+indx), tempU8);
			if(tempU8 != start_sync[indx])
				goto Skip_Pkt_Processing;
		}
		
		// Get & Compare STOP-SYSNC //
		for(indx=0 ; indx<STM_PID_LENGTH ; indx++) {
			GET_RXBUFFER_DATA((SYNC_STOP_INDX+indx), tempU8);
			if(tempU8 != stop_sync[indx])
				goto Skip_Pkt_Processing;
		}

		// Get the command //
		GET_RXBUFFER_DATA(CMD_INDX, ActiveCmd);
		GET_RXBUFFER_DATA(DATA_START_INDX, FirstData);

		// process the command //    
		switch(ActiveCmd) {
			case CMD_SEND_BATTEMP:
				prep_send_msg(ActiveCmd+1, 1, (uint8_t *)&BatTemp);
				break;
			case CMD_SEND_BATPERCENT:
				prep_send_msg(ActiveCmd+1, 1, (uint8_t *)&BatteryCapacity);
				break;
			case CMD_SEND_BATSTATE:
			#if(0)
			{
				// Read PG, if it is HIGH battery is NOT CHARGING //
				// If PG==LOW & STAT1==HIGH battery is CHARGING //
				// If PG==LOW & STAT1==HIGH battery charge COMPLETED //
				uint8_t state;
				if(1 == GPIO_ReadInputDataBit(BATPG_GPIO_PORT, BATPG_PIN))
					state = BAT_NOT_CHARGING;
				else {
					if(1 == GPIO_ReadInputDataBit(BATSTAT1_GPIO_PORT, BATSTAT1_PIN))
						state = BAT_NOT_CHARGING;
					else if(1 == GPIO_ReadInputDataBit(BATSTAT2_GPIO_PORT, BATSTAT2_PIN))
						state = BAT_CHARGE_COMPLETE;
					else
						state = BAT_CHARGE_ERR;
				}	
				prep_send_msg(ActiveCmd+1, 1, &state);
			}
			#endif
			break;
			case CMD_SEND_UNIC_ID:
				prep_send_msg(ActiveCmd+1, 12, (uint8_t *)get_uniqueIDptr());
				break;
			case CMD_SET_MODEM_STATE:
			{
				BitAction pin_state;
				uint8_t indx, state;
				indx = FirstData;			// 1st data byte is modem index //
				GET_RXBUFFER_DATA(DATA_START_INDX+1, state);	// 2nd data byte is modem state // 
				indx--;	// OMAP starts from "1" but "0" is the first modem index for us // 
				pin_state = (STATE_ENABLE == state)?(Bit_RESET):(Bit_SET);
				#if(MAIN_HW_TYPE == HW_3GBONDING_MB)
					if(2 < indx)	{// Internal Modems(indx = 0,1,2) are ACTIVE LOW but external modems(indx = 3,4,5) are ACTIVE HIGH // 
						pin_state = (STATE_ENABLE == state)?(Bit_SET):(Bit_RESET);
					}
				#endif
				GPIO_WriteBit(modem_controls[indx].port, modem_controls[indx].pin, pin_state);
				DelayMS(100);
				#if(RESET_MODEM_BEFORE_ENABLE == TRUE)
					if(STATE_ENABLE == state) {
						GPIO_WriteBit(RESETMODULES_GPIO_PORT, RESETMODULES_PIN, Bit_SET);	// Set Modems to RESET State // 
				f		DelayMS(100);
						GPIO_WriteBit(RESETMODULES_GPIO_PORT, RESETMODULES_PIN, Bit_RESET);	// Set MODEMs to SET state // 
						DelayMS(100);
					}
				#endif
				{	// send response packet // 
					uint8_t ret[3];
					ret[0] = OP_COMPLETED;
					ret[1] = ++indx;	// OMAP modem start index is one more than us // 
					ret[2] = state;
					prep_send_msg(ActiveCmd+1, 3, ret);
				}
			}
			break;
			#if(MAIN_HW_TYPE == HW_3GBONDING_MB)
				case CMD_SET_WIFI_STATE:
				{
					BitAction pin_state = (STATE_ENABLE == FirstData)?(Bit_SET):(Bit_RESET);
					GPIO_WriteBit(WIFI_POWER_GPIO_PORT, WIFI_POWER_PIN, pin_state);	// Enable WIFI power, it is ACTIVE HIGH // 
					{	// send response packet // 
						uint8_t ret[2];
						ret[0] = OP_COMPLETED;
						ret[1] = FirstData;
						prep_send_msg(ActiveCmd+1, 2, ret);
					}
				}
				break;
			#endif
			case CMD_RESET_MODEM_HUB:
				GPIO_WriteBit(MODEMHUBRESET_GPIO_PORT, MODEMHUBRESET_PIN, Bit_RESET);
				DelayMS(100);
				GPIO_WriteBit(MODEMHUBRESET_GPIO_PORT, MODEMHUBRESET_PIN, Bit_SET);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_RESET_OTG_HUB:
				GPIO_WriteBit(RESETOTGHUB_GPIO_PORT, RESETOTGHUB_PIN, Bit_RESET);
				DelayMS(100);
				GPIO_WriteBit(RESETOTGHUB_GPIO_PORT, RESETOTGHUB_PIN, Bit_SET);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_RESET_USBETH:
				#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
					GPIO_WriteBit(RESETUSBETH_GPIO_PORT, RESETUSBETH_PIN, Bit_RESET);
					DelayMS(100);
					GPIO_WriteBit(RESETUSBETH_GPIO_PORT, RESETUSBETH_PIN, Bit_SET);
				#endif
				{
					uint8_t ret[2];
					#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
						ret[0] = OP_COMPLETED;
					#elif(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
						ret[0] = OP_FAILED;	// There is NO USB->ETH on HW // 
					#endif
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				} 
				break;
			case CMD_SET_HB_STATE:
				if(STATE_ENABLE == FirstData) 
					SetHBState(TRUE);
				else 
					SetHBState(FALSE);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_POWER_CYCLE:
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 1, ret);
					NVIC_SystemReset();	// Omap is DEAD i can not go on living without him :)) // 
				}
				break;
			case CMD_SET_LCD_POWER_STATE:
				GPIO_WriteBit(LCDPOWER_GPIO_PORT, LCDPOWER_PIN, (STATE_ENABLE == FirstData)?Bit_SET:Bit_RESET);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_RESET_ENCODER:
				//__disable_irq();
				//USART_ITConfig(EVAL_COM1, USART_IT_RXNE, DISABLE);
				NVIC_DisableIRQ(EVAL_COM1_IRQn);
				GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_RESET_PIN_STATE);
				//POffEncoder();
				DelayMS(100);	// This function needs SYS_IRQ interrupt to be enabled // 
				//POnEncoder();
				GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_WORKING_PIN_STATE);
				NVIC_EnableIRQ(EVAL_COM1_IRQn);
			
				//__enable_irq();
				//USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
				// NVIC_EnableIRQ(EVAL_COM1_IRQn);
				if(1) {
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_SET_ENCODER_STATE: {
				uint8_t state = FirstData;
				if(STATE_ENABLE == state) {
					GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_RESET_PIN_STATE);
					POnEncoder();
				} else {
					GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_WORKING_PIN_STATE);
					POffEncoder();
				}
				DelayMS(100);	// This function needs SYS_IRQ interrupt to be enabled // 
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
			}
			break;
			case CMD_SET_FPGA_STATE:
				GPIO_WriteBit(SUSPENDFPGA_GPIO_PORT, SUSPENDFPGA_PIN, (STATE_ENABLE == FirstData)?Bit_RESET:Bit_SET);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_SET_USBETH_STATE:
				#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
					GPIO_WriteBit(USBETH3v3EN_GPIO_PORT, USBETH3v3EN_PIN, (STATE_ENABLE == FirstData) ? Bit_SET : Bit_RESET);
					GPIO_WriteBit(RESETUSBETH_GPIO_PORT, RESETUSBETH_PIN, (STATE_ENABLE == FirstData) ? Bit_SET : Bit_RESET);
				#endif
				{
						uint8_t ret[2];
						#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
							ret[0] = OP_COMPLETED;
						#elif(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
							ret[0] = OP_FAILED;	// There is NO USB->ETH on HW // 
						#endif
						ret[1] = FirstData;
						prep_send_msg(ActiveCmd+1, 2, ret);
					}
					break;
			case CMD_SET_FAN_DUTY:
			case CMD_SET_BACKLIGHT:
				set_pwm_duty((ActiveCmd==CMD_SET_BACKLIGHT)?1:2, FirstData);
				{
					uint8_t ret[2];
					ret[0] = OP_COMPLETED;
					ret[1] = FirstData;
					prep_send_msg(ActiveCmd+1, 2, ret);
				}
				break;
			case CMD_CHECK_LINUX_MCU:
				LinuxMCUCheck = TRUE;	// There is no response packet for this command // 
				// Send New CMD and Increment CMD Counter // 
				LinuxMCUCheckCmdCnt++;
				prep_send_msg(CMD_IS_LINUX_MCU_OK, 0, NULL);
				break;
			case RSP_LINUX_MCU_OK: {
				LinuxMCUCheckRspCnt++;	// Linux MCU give response to us // 
			}
			break;
			case CMD_SEND_SOFT_VERSION: {
				uint8_t version = HD_SERIAL_MCU_VERSION;
				prep_send_msg(ActiveCmd+1, 1, &version);
			} 
			break;
			case RSP_SEND_SOFT_VERSION: {
				linux_mcu_version = *((uint16_t *)&FirstData);
				//TODO: Maybe REVERSING required because of endianness // 
			}
			break;
			case CMD_SET_SDIPATCH_MUTE_STATE:
			 	GPIO_WriteBit(SDIPATCH_MUTE_PORT, SDIPATCH_MUTE_PIN, (STATE_ENABLE == FirstData) ? SDIPATCH_MUTE_STATE : SDIPATCH_UNMUTE_STATE);
				{
						uint8_t ret[2];
						#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
							ret[0] = OP_COMPLETED;
						#elif(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
							ret[0] = OP_FAILED;	// There is NO SDI-PATCH on HW // 
						#endif
						ret[1] = FirstData;
						prep_send_msg(ActiveCmd+1, 2, ret);
					}
				break;
			case CMD_POWER_OFF:
			{
				volatile uint8_t indx;
				// Power off modems, encoder //
				for(indx=0 ; indx<MODEM_COUNT ; indx++)
					GPIO_WriteBit(modem_controls[indx].port, modem_controls[indx].pin, Bit_SET);
				POffEncoder();				
				// Disable Omap-module, Power off main power source //
				GPIO_WriteBit(OMAP_WRON_GPIO_PORT, OMAP_WRON_PIN, Bit_SET);
				GPIO_WriteBit(OMAP_WARM_GPIO_PORT, OMAP_WARM_PIN, Bit_SET);
				GPIO_WriteBit(ENC3v3MAIN_GPIO_PORT, ENC3v3MAIN_PIN, Bit_SET);	// Disable MAin POWER // 
				// Enter into endless loop with deep sleep // 
				while(1)
					PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFE);
			}
			break;
			default:
				//while(STALE_ON_ERROR);	
				Terminal_OnRXTimeout();
				break;
		}
Skip_Pkt_Processing:		
		// Make LocalRXTail is showing start of next packet //
#if(0)
		TempU16 = (uint16_t)LocalRXTail + (uint16_t)RXBuffer[(uint16_t)LocalRXTail + (uint16_t)PKT_LEN_INDX];
#else
		TempU16 = (uint16_t)LocalRXTail + sizeof(stm_comm_t);
#endif
		if(TempU16 >= TERMINAL_RX_LENGTH)   
			TempU16 -= TERMINAL_RX_LENGTH;
		LocalRXTail = (uint8_t)TempU16;
	} while(LocalRPTargetCounter != LocalRXTail);
	// Set global RX processing index //
	RXTail = LocalRXTail;
	// Set RX state to IDLE again // 
	NVIC_DisableIRQ(EVAL_COM1_IRQn);					// ATOMIC START // 
	UsartRXState = IDLE_STATE;
	NVIC_EnableIRQ(EVAL_COM1_IRQn);						// ATOMIC STOP // 
}

USART_TypeDef* COM_USART[COMn] = {EVAL_COM1}; 
GPIO_TypeDef* COM_TX_PORT[COMn] = {EVAL_COM1_TX_GPIO_PORT};
GPIO_TypeDef* COM_RX_PORT[COMn] = {EVAL_COM1_RX_GPIO_PORT};
const uint32_t COM_USART_CLK[COMn] = {EVAL_COM1_CLK};
const uint32_t COM_TX_PORT_CLK[COMn] = {EVAL_COM1_TX_GPIO_CLK};
const uint32_t COM_RX_PORT_CLK[COMn] = {EVAL_COM1_RX_GPIO_CLK};
const uint16_t COM_TX_PIN[COMn] = {EVAL_COM1_TX_PIN};
const uint16_t COM_RX_PIN[COMn] = {EVAL_COM1_RX_PIN};
const uint16_t COM_TX_PIN_SOURCE[COMn] = {EVAL_COM1_TX_SOURCE};
const uint16_t COM_RX_PIN_SOURCE[COMn] = {EVAL_COM1_RX_SOURCE};
const uint16_t COM_TX_AF[COMn] = {EVAL_COM1_TX_AF};
const uint16_t COM_RX_AF[COMn] = {EVAL_COM1_RX_AF};

#if(0)
void TerminalUsartInit(void)		// uart hw & sw initialization to communicate with omap // 
{
	// pa2:uart tx, pa3:uart rx // 
	// 115200, 1n8, no parity // 
	USART_InitTypeDef USART_InitStructure;

  /* USART resources configuration (Clock, GPIO pins and USART registers) ----*/
  /* USART configured as follow:
        - BaudRate = 115200 baud  
        - Word Length = 8 Bits
        - One Stop Bit
        - No parity
        - Hardware flow control disabled (RTS and CTS signals)
        - Receive and transmit enabled
  */
	USART_DeInit(COM_USART[COM1]);
	USART_StructInit(&USART_InitStructure);
	
  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  STM_EVAL_COMInit(COM1, &USART_InitStructure);

	/* Configure interrupts */
	USART_ClearITPendingBit(EVAL_COM1, USART_FLAG_TC);
	USART_ReceiveData(COM_USART[COM1]);
	USART_ITConfig(COM_USART[COM1], USART_IT_TXE, ENABLE);
	USART_ITConfig(COM_USART[COM1], USART_IT_RXNE, ENABLE);
  NVIC_SetPriority (EVAL_COM1_IRQn, (1<<__NVIC_PRIO_BITS) - 1);  /* set Priority for Cortex-M0 System Interrupts */
	NVIC_EnableIRQ(EVAL_COM1_IRQn);				
}
#endif

/**
  * @brief  Configures COM port.
  * @param  COM: Specifies the COM port to be configured.
  *          This parameter can be one of following parameters:    
  *            @arg COM1
  * @param  USART_InitStruct: pointer to a USART_InitTypeDef structure that
  *   contains the configuration information for the specified USART peripheral.
  * @retval None
  */
static void STM_EVAL_COMInit(COM_TypeDef COM, USART_InitTypeDef* USART_InitStruct)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* Enable GPIO clock */
  RCC_AHBPeriphClockCmd(COM_TX_PORT_CLK[COM] | COM_RX_PORT_CLK[COM], ENABLE);

  /* Enable USART clock */
  RCC_APB2PeriphClockCmd(COM_USART_CLK[COM], ENABLE); 

  /* Connect PXx to USARTx_Tx */
  GPIO_PinAFConfig(COM_TX_PORT[COM], COM_TX_PIN_SOURCE[COM], COM_TX_AF[COM]);

  /* Connect PXx to USARTx_Rx */
  GPIO_PinAFConfig(COM_RX_PORT[COM], COM_RX_PIN_SOURCE[COM], COM_RX_AF[COM]);
  
  /* Configure USART Tx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = COM_TX_PIN[COM];
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(COM_TX_PORT[COM], &GPIO_InitStructure);
    
  /* Configure USART Rx as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = COM_RX_PIN[COM];
  GPIO_Init(COM_RX_PORT[COM], &GPIO_InitStructure);

  /* USART configuration */
  USART_Init(COM_USART[COM], USART_InitStruct);
    
  /* Enable USART */
  USART_Cmd(COM_USART[COM], ENABLE);
}


void my_usart_init(void)
{
	USART_InitTypeDef USART_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  RCC_AHBPeriphClockCmd(EVAL_COM1_TX_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(EVAL_COM1_RCC,ENABLE);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_1);
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_1);

  //Configure USART1 pins:  Rx and Tx ----------------------------
  GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_2 | GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  USART_InitStructure.USART_BaudRate = 115200;
  USART_InitStructure.USART_WordLength = USART_WordLength_8b;
  USART_InitStructure.USART_StopBits = USART_StopBits_1;
  USART_InitStructure.USART_Parity = USART_Parity_No;
  USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
  USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
  USART_Init(EVAL_COM1, &USART_InitStructure);

	/* Configure interrupts */
	USART_DirectionModeCmd(EVAL_COM1, USART_Mode_Tx, ENABLE);
	USART_DirectionModeCmd(EVAL_COM1, USART_Mode_Rx, ENABLE);

	USART_Cmd(EVAL_COM1,ENABLE);

	USART_ITConfig(EVAL_COM1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(EVAL_COM1, USART_IT_TXE, DISABLE);
	NVIC_EnableIRQ(EVAL_COM1_IRQn);				
}

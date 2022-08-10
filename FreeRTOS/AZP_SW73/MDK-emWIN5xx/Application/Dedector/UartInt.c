#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_uart.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <timers.h>
#include "UMDShared.h"
#include "AppCommon.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "ErrorLog.h"
#include "UartInt.h"
	

/* buffer size definition */
#define UART_RING_BUFSIZE 1024

/* Buf mask */
#define __BUF_MASK (UART_RING_BUFSIZE-1)
/* Check buf is full or not */
#define __BUF_IS_FULL(head, tail) ((tail&__BUF_MASK)==((head+1)&__BUF_MASK))
/* Check buf will be full in next receiving or not */
#define __BUF_WILL_FULL(head, tail) ((tail&__BUF_MASK)==((head+2)&__BUF_MASK))
/* Check buf is empty */
#define __BUF_IS_EMPTY(head, tail) ((head&__BUF_MASK)==(tail&__BUF_MASK))
/* Reset buf */
#define __BUF_RESET(bufidx)	(bufidx=0)
#define __BUF_INCR(bufidx)	(bufidx=(bufidx+1)&__BUF_MASK)
#define __BUF_SIZE(head,tail)	((head>tail)?(head-tail):(head + (UART_RING_BUFSIZE - tail)))

/************************** PRIVATE TYPES *************************/
/** @brief UART Ring buffer structure */
typedef struct
{
    __IO uint32_t tx_head;                /*!< UART Tx ring buffer head index */
    __IO uint32_t tx_tail;                /*!< UART Tx ring buffer tail index */
    __IO uint32_t rx_head;                /*!< UART Rx ring buffer head index */
    __IO uint32_t rx_tail;                /*!< UART Rx ring buffer tail index */
    __IO uint8_t  tx[UART_RING_BUFSIZE];  /*!< UART Tx data ring buffer */
    __IO uint8_t  rx[UART_RING_BUFSIZE];  /*!< UART Rx data ring buffer */
} UART_RING_BUFFER_T;

// UART Ring buffer
static UART_RING_BUFFER_T rb;
// Current Tx Interrupt enable state
static __IO FlagStatus TxIntStat;
uint8_t TimeoutState = FALSE;	// Timeout on TX is disabled @ startup // 
uint8_t TimeoutCnt = 0;
TimerHandle_t CommFRTmr;			// Expected response packet not received from detector // 
comm_timeout_fail_func fail_func = NULL;
// Shard resources // 
UmdPkt_Type last_send_msg;
UmdPkt_Type last_recv_msg;

volatile uint_fast8_t uart_pkt_rcv_active = FALSE;
volatile uint_fast16_t uart_pkt_rcv_msec = 0;
void handle_uart_rcv_timeout(void);

// --- GAUGE RECEIVE SPECIFIC ACTION ----//
uint8_t gauge_cnt = 0;
uint8_t gauge_max = 0;

/************************** PRIVATE FUNCTIONS *************************/
/* Interrupt service routines */
void _UART_IRQHander(void);
static void UART_IntErr(uint8_t bLSErrType);
static void UART_IntTransmit(void);
static void UART_IntReceive(void);
static uint32_t UARTGetFromBuffer(UART_ID_Type UartID, uint8_t *rxbuf, uint32_t buflen);
static void vTimerCallback( TimerHandle_t pxTimer );

extern xQueueHandle	GUI_Task_Queue;	// Defined in GUIDEMO_Start.c // 

void handle_uart_rcv_timeout(void) {
	if(TRUE == uart_pkt_rcv_active) {
		uart_pkt_rcv_msec += (1000/configTICK_RATE_HZ);
		if(uart_pkt_rcv_msec > UMD_PKT_TIMEOUT_DEFAULT_MS) {
			UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_RBR, DISABLE);
			uart_pkt_rcv_active = FALSE;
			__BUF_RESET(rb.rx_head);
			__BUF_RESET(rb.rx_tail);
			UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_RBR, ENABLE);
		}
	}
}

/*----------------- INTERRUPT SERVICE ROUTINES --------------------------*/
/*********************************************************************//**
 * @brief		UART0 interrupt handler sub-routine
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void _UART_IRQHander(void)
{
	uint32_t intsrc, tmp, tmp1;

	/* Determine the interrupt source */
	intsrc = UART_GetIntId(UART_INT_HW_PORT);
	tmp = intsrc & UART_IIR_INTID_MASK;

	// Receive Line Status
	if (tmp == UART_IIR_INTID_RLS){
		// Check line status
		tmp1 = UART_GetLineStatus(UART_INT_HW_PORT);
		// Mask out the Receive Ready and Transmit Holding empty status
		tmp1 &= (UART_LSR_OE | UART_LSR_PE | UART_LSR_FE \
				| UART_LSR_BI | UART_LSR_RXFE);
		// If any error exist
		if (tmp1) {
				UART_IntErr(tmp1);
		}
	}

	// Receive Data Available or Character time-out
	if ((tmp == UART_IIR_INTID_RDA) || (tmp == UART_IIR_INTID_CTI)) {
			UART_IntReceive();
	}

	// Transmit Holding Empty
	if (tmp == UART_IIR_INTID_THRE) {
			UART_IntTransmit();
	}
}

/********************************************************************//**
 * @brief 		UART receive function (ring buffer used)
 * @param[in]	None
 * @return 		None
 *********************************************************************/
static void UART_IntReceive(void)
{
	uint8_t tmpc;
	uint32_t rLen;
	uint8_t buf_size, buf_indx;
	uint8_t msg_buff[UMD_FIXED_PACKET_LENGTH];

	while(1){
		// Call UART read function in UART driver
		rLen = UART_Receive(UART_INT_HW_PORT, &tmpc, 1, NONE_BLOCKING);
		// If data received
		if(rLen) {
			buf_indx = (__BUF_SIZE(rb.rx_head,rb.rx_tail)) % sizeof(msg_buff);
			switch(buf_indx) {
				case 0:	// start of packet, start packet receive timeout software timer // 
					uart_pkt_rcv_active = TRUE;
					uart_pkt_rcv_msec = 0;
					break;
				case (sizeof(msg_buff)-1):	// packet completed, stop packet receive tmeout software timer // 
					uart_pkt_rcv_active = FALSE;
					break;
				default:
					break;
			}
			/* Check if buffer is more space
			 * If no more space, remaining character will be trimmed out
			 */
			if (!__BUF_IS_FULL(rb.rx_head,rb.rx_tail)){
				rb.rx[rb.rx_head] = tmpc;
				__BUF_INCR(rb.rx_head);
			}
		}
		else {
			// no more data
			break;
		}
	}
	// Check if there is minimum one complete message in the buffer //
	buf_size = __BUF_SIZE(rb.rx_head,rb.rx_tail);
	while( UMD_FIXED_PACKET_LENGTH <= buf_size) {
		portBASE_TYPE temp;
		UARTGetFromBuffer(UART_INT_HW_PORT, msg_buff, sizeof(msg_buff));	// get message from ring buffer //
		if((CMD_UMD_FIRST <= msg_buff[0]) && (CMD_UMD_LAST >= msg_buff[0]))	 {// Check CMd validy // 
			#if(0)
			extern uint8_t SearchState; 	// defined in GUIDEMO_Start.c // 
			if((SEARCH_STARTED != SearchState) && \
				((IND_GET_GAUGE == msg_buff[0]) || (IND_GET_TARGET_ID == msg_buff[0]) || (IND_GET_TARGET_NUM == msg_buff[0]))) 
			{
				// silently IGNORE packet // 
			} else 
			#endif
			{
			#if(0)
				if((SEARCH_STARTED == SearchState) && (IND_GET_GAUGE == msg_buff[0])) {
					UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
					uint8_t TGauge = msg_ptr->data.gauge;
					if(UMD_GAUGE_MAX < TGauge)
						TGauge = UMD_GAUGE_MAX;	// Truncation to maximum value for safety // 
					if(TGauge > gauge_max)
						gauge_max = TGauge;
						if(3 == (++gauge_cnt)) {
							gauge_cnt = 0;
							TGauge = gauge_max;
							gauge_max = 0;
						}
					}
			#endif
				memcpy(&last_recv_msg, msg_buff, UMD_FIXED_PACKET_LENGTH);
				if(NULL != GUI_Task_Queue) {
					xQueueSendFromISR(GUI_Task_Queue, (const void *)msg_buff, &temp); // forward message to gui task message queue by copying //
				}
				else {
					#if(DEBUG_ON_LCD == APP_DEBUG_OUTPUT)
						ERRM("MSG(0x%02X) GUI MSG QUEUE is NOT READY\n", msg_buff[0]);
					#endif
				}
			}
		} else {
			// Silently IGNORE CORRUPTED PACKET // 
			// DEBUGM("UNKNOWN CMD\n");	// We are in INTERRUPT here, debug messages can not work // 
		}
		buf_size -= UMD_FIXED_PACKET_LENGTH;	// Decrement ring buffer pending bytes count // 
	}
}

/********************************************************************//**
 * @brief 		UART transmit function (ring buffer used)
 * @param[in]	None
 * @return 		None
 *********************************************************************/
static void UART_IntTransmit(void)
{
    // Disable THRE interrupt
    UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_THRE, DISABLE);

	/* Wait for FIFO buffer empty, transfer UART_TX_FIFO_SIZE bytes
	 * of data or break whenever ring buffers are empty */
	/* Wait until THR empty */
	while (UART_CheckBusy(UART_INT_HW_PORT) == SET);

	while (!__BUF_IS_EMPTY(rb.tx_head,rb.tx_tail)) {
		/* Move a piece of data into the transmit FIFO */
		if (UART_Send(UART_INT_HW_PORT, (uint8_t *)&rb.tx[rb.tx_tail], 1, NONE_BLOCKING)){
			/* Update transmit ring FIFO tail pointer */
			__BUF_INCR(rb.tx_tail);
		} else {
			break;
		}
	}

	/* If there is no more data to send, disable the transmit
		 interrupt - else enable it or keep it enabled */
	if (__BUF_IS_EMPTY(rb.tx_head, rb.tx_tail)) {
    	UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_THRE, DISABLE);
    	// Reset Tx Interrupt state
    	TxIntStat = RESET;
    }
    else{
      	// Set Tx Interrupt state
			TxIntStat = SET;
    	UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_THRE, ENABLE);
    }
}


/*********************************************************************//**
 * @brief		UART Line Status Error
 * @param[in]	bLSErrType	UART Line Status Error Type
 * @return		None
 **********************************************************************/
static void UART_IntErr(uint8_t bLSErrType)
{
#if(0)
		while(1);
#else
	if(1) {
		sErrStore temp =  {
			.err_type = ERR_UART_PERIPHERAL,
			.ErrCore.uart_err = bLSErrType
		};
		push_err(&temp);
	}
#endif
}


/*-------------------------PRIVATE FUNCTIONS------------------------------*/
/*********************************************************************//**
 * @brief		UART transmit function for interrupt mode (using ring buffers)
 * @param[in]	UARTPort	Selected UART peripheral used to send data,
 * 				should be UART0
 * @param[out]	txbuf Pointer to Transmit buffer
 * @param[in]	buflen Length of Transmit buffer
 * @return 		Number of bytes actually sent to the ring buffer
 **********************************************************************/
uint32_t UARTSend(uint8_t *txbuf, uint16_t timeoutMS, comm_timeout_fail_func ffunc)
{
    uint8_t *data = (uint8_t *) txbuf;
    uint32_t bytes = 0;
		uint8_t buflen = UMD_FIXED_PACKET_LENGTH;
	
		if((pdFALSE != xTimerIsTimerActive(CommFRTmr)) && \
			(0 != memcmp(txbuf, &last_send_msg, UMD_FIXED_PACKET_LENGTH)))
			return 0;	// dont send anything different to dedector while waiting for an answer //
		if((0 != timeoutMS) && (NULL != ffunc)) { 
			// Store message to be send if timeout occures // 
			memcpy(&last_send_msg, txbuf, UMD_FIXED_PACKET_LENGTH);
			// Set new timeout as period of timeout timer // 
			if(pdFAIL == xTimerChangePeriod(CommFRTmr, timeoutMS*(configTICK_RATE_HZ/1000), \
				COMM_RSP_TIMEOUT_FR_TMR_CMD_BLOCK_MS*(configTICK_RATE_HZ/1000))) {
				while(TODO_ON_ERR);	// Timer CMD can not be send before maximum wait time passed // 
			}
			// Set function pointer to be called if timeout maximum count reaches @ fail case // 
			fail_func = ffunc;
			// start timeout software timer //
			xTimerStart(CommFRTmr, COMM_RSP_TIMEOUT_FR_TMR_CMD_BLOCK_MS*(configTICK_RATE_HZ/1000));
		}
		
	/* Temporarily lock out UART transmit interrupts during this
	   read so the UART transmit interrupt won't cause problems
	   with the index values */
    UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_THRE, DISABLE);

	/* Loop until transmit run buffer is full or until n_bytes
	   expires */
	while ((buflen > 0) && (!__BUF_IS_FULL(rb.tx_head, rb.tx_tail))) {
		/* Write data from buffer into ring buffer */
		rb.tx[rb.tx_head] = *data;
		data++;

		/* Increment head pointer */
		__BUF_INCR(rb.tx_head);

		/* Increment data count and decrement buffer size count */
		bytes++;
		buflen--;
	}

	/*
	 * Check if current Tx interrupt enable is reset,
	 * that means the Tx interrupt must be re-enabled
	 * due to call UART_IntTransmit() function to trigger
	 * this interrupt type
	 */
	if (TxIntStat == RESET) {
		UART_IntTransmit();
	}
	/*
	 * Otherwise, re-enables Tx Interrupt
	 */
	else {
		UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_THRE, ENABLE);
	}

    return bytes;
}

/*********************************************************************//**
 * @brief		UART read function for interrupt mode (using ring buffers)
 * @param[in]	UARTPort	Selected UART peripheral used to send data,
 * 				should be UART0
 * @param[out]	rxbuf Pointer to Received buffer
 * @param[in]	buflen Length of Received buffer
 * @return 		Number of bytes actually read from the ring buffer
 **********************************************************************/
static uint32_t UARTGetFromBuffer(UART_ID_Type UartID, uint8_t *rxbuf, uint32_t buflen)
{
    uint8_t *data = (uint8_t *) rxbuf;
    uint32_t bytes = 0;

	/* Temporarily lock out UART receive interrupts during this
	   read so the UART receive interrupt won't cause problems
	   with the index values */
	UART_IntConfig(UartID, UART_INTCFG_RBR, DISABLE);

	/* Loop until receive buffer ring is empty or
		until max_bytes expires */
	while ((buflen > 0) && (!(__BUF_IS_EMPTY(rb.rx_head, rb.rx_tail)))) {
		/* Read data from ring buffer into user buffer */
		*data = rb.rx[rb.rx_tail];
		data++;
		/* Update tail pointer */
		__BUF_INCR(rb.rx_tail);
		/* Increment data count and decrement buffer size count */
		bytes++;
		buflen--;
	}

	/* Re-enable UART interrupts */
	UART_IntConfig(UartID, UART_INTCFG_RBR, ENABLE);
	return bytes;
}

void UartInt_init(void)
{
	// UART Configuration structure variable
	UART_CFG_Type UARTConfigStruct;
	// UART FIFO configuration Struct variable
	UART_FIFO_CFG_Type UARTFIFOConfigStruct;

	Uart_PortPin_Init(UART_INT_HW_PORT);

	/* Initialize UART Configuration parameter structure to default state:
	 * Baudrate = 115200 bps
	 * 8 data bit
	 * 1 Stop bit
	 * None parity
	 */
	UART_ConfigStructInit(&UARTConfigStruct);
	UARTConfigStruct.Baud_rate = UMD_UART_BAUDRATE;

	// Initialize UART0 peripheral with given to corresponding parameter
	UART_Init(UART_INT_HW_PORT, &UARTConfigStruct);


	/* Initialize FIFOConfigStruct to default state:
	 * 				- FIFO_DMAMode = DISABLE
	 * 				- FIFO_Level = UART_FIFO_TRGLEV0
	 * 				- FIFO_ResetRxBuf = ENABLE
	 * 				- FIFO_ResetTxBuf = ENABLE
	 * 				- FIFO_State = ENABLE
	 */
	UART_FIFOConfigStructInit(&UARTFIFOConfigStruct);
	//UARTFIFOConfigStruct.FIFO_Level = UART_FIFO_TRGLEV1;
	// Initialize FIFO for UART0 peripheral
	UART_FIFOConfig(UART_INT_HW_PORT, &UARTFIFOConfigStruct);

	// Enable UART Transmit
	UART_TxCmd(UART_INT_HW_PORT, ENABLE);

    /* Enable UART Rx interrupt */
	UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_RBR, ENABLE);
	/* Enable UART line status interrupt */
	UART_IntConfig(UART_INT_HW_PORT, UART_INTCFG_RLS, ENABLE);
	/*
	 * Do not enable transmit interrupt here, since it is handled by
	 * UART_Send() function, just to reset Tx Interrupt state for the
	 * first time
	 */
	TxIntStat = RESET;

	// Reset ring buf head and tail idx
	__BUF_RESET(rb.rx_head);
	__BUF_RESET(rb.rx_tail);
	__BUF_RESET(rb.tx_head);
	__BUF_RESET(rb.tx_tail);
	memset(&last_send_msg,0xFF,UMD_FIXED_PACKET_LENGTH);
	memset(&last_recv_msg, 0xFF, UMD_FIXED_PACKET_LENGTH);

	/* Do FreeRTOS related initialization */
	CommFRTmr = xTimerCreate("CommFRTmr", UMD_CMD_TIMEOUT_DEFAULT_MS * (configTICK_RATE_HZ/1000), \
		pdTRUE, (void *)COMM_RSP_TIMEOUT_FR_TMR_INDX, vTimerCallback);
	if(NULL == CommFRTmr) {	// communication Responce timeout timer creation failed //
		while(TODO_ON_ERR);
	}
	
	// IRQ Priority has been set in BSP_Init_IRQ_Priorities() function // 
	/* preemption = 1, sub-priority = 1 */
	// NVIC_SetPriority(_UART_IRQ, 1);
	/* Enable Interrupt for UARTx channel */
  NVIC_EnableIRQ(_UART_IRQ);
}

void StopCommTimeout(void)
{
	// Stop periodic software timer until next time // 
	xTimerStop(CommFRTmr, COMM_RSP_TIMEOUT_FR_TMR_CMD_BLOCK_MS*(configTICK_RATE_HZ/1000));
	// Clear Resources // 
	TimeoutCnt = 0;
}

static void vTimerCallback( TimerHandle_t pxTimer )
{
	long timerID;
	if(NULL == pxTimer) {
		while(STALLE_ON_ERR);
	}
	// Eger bu callback' e birden cok timer gonderrirsen asagıdaki fonk ile timer ID' sini 
	timerID = ( long ) pvTimerGetTimerID( pxTimer );
	switch(timerID) {
		case COMM_RSP_TIMEOUT_FR_TMR_INDX:
			// Expected RSP packet NOT RECEIVED in maximum time after last CMD packet send //
			if(CMD_RESPONSE_TIMEOUT_MAX_COUNT == (++TimeoutCnt)) {
				StopCommTimeout();
				// Call pre-registered fail case function // 
				if(NULL != fail_func) 
					fail_func(&last_send_msg);
			}
			else	// We have still time to try, resend last registered message again to slave // 
				UARTSend((uint8_t *)&last_send_msg, 0, NULL);
			break;
		default:
			while(TODO_ON_ERR);
			break;
	}
}

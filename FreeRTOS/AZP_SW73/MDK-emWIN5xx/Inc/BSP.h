/*********************************************************************
*                SEGGER MICROCONTROLLER GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 2003-2010     SEGGER Microcontroller GmbH & Co KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File    : BSP.h
Purpose : BSP (Board support package)
--------  END-OF-HEADER  ---------------------------------------------
*/

#ifndef BSP_H                           /* avoid multiple inclusion */
#define BSP_H

#include <stdint.h>
#include "SEGGER.h"

/*********************************************************************
*
*       Defines, non-configurable
*
**********************************************************************
*/

#define KEY_SCAN_MS        25
#define MIN_KEYPRESS_MS    100
#define MIN_KEY_WAIT_MS    250	
#define PKEY_STACK_COUNT	 10

typedef enum
{
	KEY_UP				= 0,
	KEY_PLUS			= KEY_UP,	
	KEY_RIGHT			= KEY_UP,
	KEY_FIRST			= KEY_UP,
	KEY_AZP_DISC		= KEY_UP,
	
	KEY_DOWN  			= 1,
	KEY_MINUS			= KEY_DOWN,	
	KEY_LEFT			= KEY_DOWN,
	KEY_AZP_GROUND		= KEY_DOWN,
	
	KEY_OTO 			= 2,
	KEY_AZP_FAST		= KEY_OTO,
	KEY_A5P_OTO			= KEY_OTO,
		
	KEY_OK  			= 3,
	KEY_CONFIRM			= KEY_OK,	
	KEY_AZP_ENTER		= KEY_OK,
	
	KEY_ESC				= 4,
	KEY_MENU			= KEY_ESC,	
	KEY_LAST 			= KEY_ESC,
	KEY_AZP_MENU		= KEY_ESC,
	
	KEY_COUNT,
	
	NO_KEY_PRESSED		= 0xFF
} eKeyType;

#define KEY_EVENT_OFFSET_CH		'a'
typedef enum
{
	KEY_UP_EVENT	= KEY_EVENT_OFFSET_CH + KEY_UP,
	KEY_RIGHT_EVENT = KEY_EVENT_OFFSET_CH + KEY_RIGHT,
	KEY_PLUS_EVENT	= KEY_EVENT_OFFSET_CH + KEY_PLUS,
	KEY_AZP_DISC_EVENT	= KEY_EVENT_OFFSET_CH + KEY_AZP_DISC,

	KEY_DOWN_EVENT	= KEY_EVENT_OFFSET_CH + KEY_DOWN,
	KEY_LEFT_EVENT	= KEY_EVENT_OFFSET_CH + KEY_LEFT,
	KEY_MINUS_EVENT	= KEY_EVENT_OFFSET_CH + KEY_MINUS,
	KEY_AZP_GROUND_EVENT = KEY_EVENT_OFFSET_CH + KEY_AZP_GROUND,

	KEY_OTO_EVENT	= KEY_EVENT_OFFSET_CH + KEY_OTO,
	KEY_AZP_FAST_EVENT = KEY_EVENT_OFFSET_CH + KEY_AZP_FAST,
	KEY_A5P_OTO_EVENT = KEY_EVENT_OFFSET_CH + KEY_A5P_OTO,
	
	//KEY_DEPTH_EVENT	= KEY_EVENT_OFFSET_CH + KEY_DEPTH,

	KEY_OK_EVENT		= KEY_EVENT_OFFSET_CH + KEY_OK,
	KEY_CONFIRM_EVENT	= KEY_EVENT_OFFSET_CH + KEY_CONFIRM,
	KEY_AZP_ENTER_EVENT	= KEY_EVENT_OFFSET_CH + KEY_AZP_ENTER,
	
	KEY_ESC_EVENT	= KEY_EVENT_OFFSET_CH + KEY_ESC,
	KEY_MENU_EVENT	= KEY_EVENT_OFFSET_CH + KEY_MENU,
	KEY_AZP_MENU_EVENT = KEY_EVENT_OFFSET_CH + KEY_AZP_MENU,
	
	KEY_EVENT_COUNT	= KEY_COUNT
} eKeyEventType;	// Key Events to send emWin // 

extern eKeyType PullPkey(void);	// read next pressed key from stack // 
extern void KS_init(void);
extern uint8_t get_kp(void);

extern inline void set_lcd_bcklight_reduce_state(uint8_t new_state);
	
#define ADC_DIVIDER_HIGH_RES			(10000)
#define ADC_DIVIDER_LOW_RES				(4700)
#define ADC_VREF									(3300)
#define ADC_BITS_MAX							(12)
#define ADC_MAX_SPEED_HZ					(400000)
#define JACK_DETECTION_THRESHOLD_MV			(3000)	// TODO: Measure or calculate this, and test // 

extern uint32_t BSP_GET_RAW_ADC(uint8_t adc_ch);
extern void BSP_ADCInit(uint8_t adc_ch_bits);
extern void BSP_ADCStart (void);
extern void BSP_ADCStop (void);
extern volatile uint8_t ADC_done;

static inline uint32_t __RAW_ADC_TO_MV(uint32_t adc_raw_val) {
	uint32_t pin_val_MV = (adc_raw_val * ADC_VREF) / (0x1U << ADC_BITS_MAX);
	return pin_val_MV;
}

#define MIN_PWM_FREQ_HZ			250		/* 250 Hz */
#define DEFAULT_PWM_FREQ_HZ		10000	/* 10KHz */
#define MAX_PWM_FREQ_HZ			0xFFFF	/* 65.535 KHz */
typedef enum {
	BSP_PWM_FS1	= 0,		// Field Scanner Kanal1 PWM Ayari //
	BSP_PWM_LCD,				// LCD Backlight PWM Ayari // 
	
	BSP_PWM_COUNT
} ePWMCHs;
extern const uint8_t pwm_ch[BSP_PWM_COUNT];
extern void BSP_PWMSet(uint8_t pwm_hw, ePWMCHs ch, uint8_t pwm_duty);
extern void BSP_PWM_stop(uint8_t pwm_hw, ePWMCHs pwm_ch);
extern void BSP_PWM_start(uint8_t pwm_hw, ePWMCHs pwm_ch);
extern void BSP_set_pwm_freq_hz(uint8_t pwm_hw, uint32_t freq_hz);


/* In order to avoid warnings for undefined parameters */
#ifndef BSP_USE_PARA
  #if defined(NC30) || defined(NC308)
    #define BSP_USE_PARA(para)
  #else
    #define BSP_USE_PARA(para) para=para;
  #endif
#endif

/*********************************************************************
*
*       Functions
*
**********************************************************************
*/

/*********************************************************************
*
*       General
*/
void     BSP_Init       (void);
void     BSP_SetLED     (int Index);
void     BSP_ClrLED     (int Index);
void     BSP_ToggleLED  (int Index);
unsigned BSP_GetKeyStat (void);
void 	BSP_DelayMS		(U32 ms);
void 	BSP_InitIRQs(void);

/*********************************************************************
*
*       GUI
*/
void BSP_GUI_Init(void);

/*********************************************************************
*
*       USB
*/
void BSP_USB_Attach         (void);
void BSP_USB_InstallISR     (void (*pfISR)(void));
void BSP_USB_InstallISR_Ex  (int ISRIndex, void (*pfISR)(void), int Prio);
void BSP_USB_ISR_Handler    (void);
void BSP_USB_Init           (void);

/*********************************************************************
*
*       USBH
*/
void BSP_USBH_InstallISR    (void (*pfISR)(void));
void BSP_USBH_Init          (void);

/*********************************************************************
*
*       ETH
*
*  Functions for ethernet controllers (as far as present)
*/
void   BSP_ETH_Init           (unsigned Unit);
void   BSP_ETH_InstallISR     (void (*pfISR)(void));
void   BSP_ETH_InstallISR_Ex  (int ISRIndex, void (*pfISR)(void), int Prio);
void   BSP_ETH_ISR_Handler    (void);
U32    BSP_ETH_GetTransferMem (U32 * pPAddr, U32 * pVAddr);

/*********************************************************************
*
*       CACHE
*/
void BSP_CACHE_CleanInvalidateRange (void * p, unsigned NumBytes);
void BSP_CACHE_CleanRange           (void * p, unsigned NumBytes);
void BSP_CACHE_InvalidateRange      (void * p, unsigned NumBytes);

/*********************************************************************
*
*       UART
*/
void BSP_UART_SetReadCallback(unsigned Unit, void (*pfOnRx) (unsigned Unit, unsigned char Data));
void BSP_UART_SetWriteCallback(unsigned Unit, void (*pfOnTx) (unsigned Unit));
void BSP_UART_Write1(U8 Data);
void BSP_UART_SetBaudrate(unsigned Baudrate);
void BSP_UART_Init(void);

/*********************************************************************
*
*       SD  (used by file system)
*/

/***************************************************
*
*       BSP_SD_GetTransferMem
*
*  Function description
*     Delivers a memory area to be used by the SD-Card controller as transfer.
*     This function delivers the physical address and the virtual address of the tranfer memory.
*     The transfer area needs to be:
*     - Word aligned
*     - Uncached
*     - Have identical virtual and physical addresses
*     - The virtual address of the transfer area must be non-cacheable.
*     Additional requirements are that the memory used is fast enough to not block DMA transfers for too long.
*     In most systems, IRAM is used instead of external SDRAM, since the SDRAM can have relatively long latencies, primarily due to refresh cycles.
*     The size of the memory are is also returned (via pointer). It needs to be at least 512 bytes. In general, bigger values allow higher
*     performance since it allows transfer of multiple sectors without break.
*/
U32 BSP_SD_GetTransferMem(U32 * pPAddr, U32 * pVAddr);

#endif                                  /* avoid multiple inclusion */

/*************************** End of file ****************************/



#ifndef APP_COMMON_H
#define APP_COMMON_H

#include <stdint.h>
#include "MyMacro.h"

typedef enum
{
	LANG_TR = 0,	// Turkish // 
	LANG_MIN	= LANG_TR,
	LANG_EN,	// English //
	LANG_AR,	// Arabic //
	LANG_GE,	// German // 
	LANG_SP,	// Spanish //
	LANG_PR,	// Persian // 
	LANG_RS,	// Russian //
	LANG_FR,	// French //
	LANG_MAX	= LANG_FR,
	
	LANG_COUNT
} eLanguages;	// Language Enumaration Defitinition // 

typedef enum
{
	ST_SENS	= 0,			// Hassasiyet seviyesi
	ST_VOL,								// Ses seviyesi
	ST_BRIGHT,						// Ekran Parlakl1k Seviyesi 
	ST_GROUND_ID,				// Toprak Ayar1 ID degeri
	ST_COIL_T,							// Bobin Tipi
	ST_LANG,							// Cihaz Dili
	ST_INVERT,			// Gunes Modu Durumu //
	ST_FERROs,			// Degersiz Durumu //
	ST_BAT,
	ST_CRC,
	
	ST_COUNT
} eSettingsType;	// Cihazdaki saklanacak ayarlarin listesi // 

typedef __packed union {
	__packed struct {	// Dont change the order of components,
		uint16_t Sensitivity;
		uint16_t Volume;
		uint16_t Brightness;
		uint16_t Mineralization;
		uint16_t CoilType;
		uint16_t Language;
		uint16_t HighContrast;	// TRUE if HIGH CONTRAST(B&W) ENABLED, FALSE if disabled(NORMAL) //  
		uint16_t FerrosState;	// TRUE if ferros enabled, FALSE if disabled // 	
		uint16_t Battery;
		uint16_t Crc8;
	}str;
	__packed uint16_t Settings[ST_COUNT];
} uAppStore;

typedef enum
{
	GB_SCREEN 					= 0,	// Groud Balance //
	STD_SEARCH					= 1,	// Standard Search // 
	MA_SEARCH	 					= 2,	// Metal Analysing Search //
	MIN_SEARCH					= 3,	// Mineralized Area Search // 
	OTO_SEARCH					= 4,	// Otomatical Search // 
	DEPTH_CALC					= 5,	// Target Depth Calculation // 
	SYSTEM_SETTINGs 		= 6,
	DEVICE_REAL_SCREEN_COUNT,	// Screen Count excluding Radial Manu // 
	RM_SCREEN	= DEVICE_REAL_SCREEN_COUNT,
	AT_SCREEN,
	ATRADAR_SCREEN,
	DEVICE_ALL_SCREEN_COUNT,	// Screen Count including Radial Menu // 
} eRMPageType;

//---------------------------------------------------//
//------- DMA Channels of Application ---------------//
//---------------------------------------------------//
#define MCI_DMA_ENABLED          			(1)

#define WAVE_DAC_DMA_CHANNEL				(0)	// Highest Priority DMA channel is ZERO, Lowest is EIGHT // 
#define APLAY_DAC_DMA_CHANNEL				(1)
#define MCI_DMA_WRITE_CHANNEL            	(2)
#define MCI_DMA_READ_CHANNEL             	(3)

// Hardware platform for this software projects // 
#define	UMD_DETECTOR_BOARD		0		/* HW is real umd metal detctor */
#define	POWERMCU_DEV_BOARD		1		/* PowerMCU developmöent board */
	
// Check GLCD.h for more details about LCD types // 
#define LCD_4_3_TFT		    0   /* 4.3 inch TFT LCD */
#define LCD_4_3_TFT_UMD		1		/* UMD PROJECT TFT : PowerTrip : PH480272T-006-I13Q */
#define LCD_4_3_TFT_PRO		2	/* 4.3 inch HD TFT LCD */
#define LCD_5_0_TFT				3   /* 5 inch TFT LCD */
#define LCD_7_0_TFT				4   /* 7 inch TFT LCD : Used in ANTSIS Project*/

// Application resources sources to load into SD-RAM //
#define RES_LOAD_SDMMC				0x0
#define RES_LOAD_SPI_FLASH		0x1
#define RES_LOAD_NOR_FLASH		0x2
#define RES_LOAD_NAND_FLASH		0x3

// application Spi-Flash types definition //
#define SPI_FLASH_W25Q128FV				0		/* 128Mb (16MB) */
#define SPI_FLASH_SST25VF016B			1		/* 16Mb (2MB) */
#define SPI_FLASH_S25FL512S				2		/* 512Mb (64MB) */ 

// Application DEBUG Messages Output Options //
#define DEBUG_ON_LCD	0x0
#define DEBUG_ON_UART	0x1
#define DEBUG_ON_NULL	0x2

typedef struct
{
	uint8_t indx;
	uint16_t max;
	uint8_t min;
	uint8_t step;
} sSettingType;

// Application Critical Definitions //
#define RENDEZVOUS_WAIT_MS					250	/* Maximum wait time for task rendezvous */
#define STALLE_ON_ERR						1
#define TODO_ON_ERR							0		// Decide what to do later on this error type // 
#define APP_LOG_LEVEL						DEBUG_LEVEL
#define INTRO_STATE							FALSE
#define INTRO_KEY_LOCK						FALSE
#define KEY_PRESS_INTRO_SKIP			FALSE		// We need time for resource initialization on SDRAM during INTRO play // 
#define CLOCK_GENERATION_STATE				TRUE
#define USED_LCD            				LCD_4_3_TFT_UMD  
#define USED_HW								UMD_DETECTOR_BOARD
#define RES_LOAD_SOURCE						RES_LOAD_SPI_FLASH
#define ACTIVE_SPI_FLASH					SPI_FLASH_S25FL512S 		/* 64MB SPI FLASH */
//#define ACTIVE_SPI_FLASH					SPI_FLASH_W25Q128FV 	/* 16MB SPI FLASH */
#define SPI_FLASH_CHECK_CRC32				FALSE
#define APP_DEBUG_OUTPUT					DEBUG_ON_LCD
#define JACK_DETECT_STATE					TRUE 
 
// Audio  output device definitions //
#define AUDIO_IC_UDA1380		0x0
#define AUDIO_IC_LM49450		0x1
#define AUDIO_IC_LPCDAC			0x2
#if(UMD_DETECTOR_BOARD == USED_HW)
	#warning "UMD DETECTOR BOARD is USED as HW"
	//#define USED_AUDIO_DEVICE					AUDIO_IC_LM49450
	#define USED_AUDIO_DEVICE					AUDIO_IC_LPCDAC
#elif(POWERMCU_DEV_BOARD == USED_HW)
	#warning "POWERMCU DEV BOARD is USED as HW"
	#define USED_AUDIO_DEVICE					AUDIO_IC_UDA1380
#endif

// APP DEBUG OUTPUT SELECTION //
#if(DEBUG_ON_UART == APP_DEBUG_OUTPUT)
	#define FATALM	FATALL
	#define ALERTM 	ALERTL
	#define ERRM	ERRL
	#define INFOM	INFOL
	#define WARNM	WARNL
	#define DEBUGM	DEBUGL
	#define TRACEM	TRACEL
#elif(DEBUG_ON_LCD == APP_DEBUG_OUTPUT)
	#define FATALM	FATALGC
	#define ALERTM	ALERTGC
	#define ERRM	ERRGC
	#define INFOM	INFOGC
	#define WARNM	WARNGC
	#define DEBUGM	DEBUGGC
	#define TRACEM	TRACEGC
#else
	#err ""
#endif


// 1. FONT LOADING AREA (768KB)
#define RUNTIME_FONT_LOADING_AREA_START		((uint32_t)SDRAM_BASE_ADDR)
#define RUNTIME_FONT_LOADING_AREA_KB	(768U + 4U)	// (768KB + 4KB) area is reserved for application runtime fonts loading //

// 2. AUDIO TRACs LOADING SCRATCPAD AREA (1MB)
#define RUNTIME_AUDIO_TRACs_SDRAM_START		(RUNTIME_FONT_LOADING_AREA_START + (RUNTIME_FONT_LOADING_AREA_KB*1024U))
#define RUNTIME_AUDIO_TRACs_SDRAM_AREA_KB	(1U*1024U + 4U)	// (1MB + 4KB) area reserved for runtime audio tracs loading // 

// 3. PICs LOADING SCRATCPAD AREA (512KB)
#define RUNTIME_PICs_SDRAM_START		(RUNTIME_AUDIO_TRACs_SDRAM_START + (RUNTIME_AUDIO_TRACs_SDRAM_AREA_KB*1024U))
#if(0)
	#define RUNTIME_PICs_SDRAM_AREA_KB	(512U + 4U)	// (512KB + 4KB) reserved for runtime picture load  from 
																														// non-volatile memory // 
#else
	#define RUNTIME_PICs_SDRAM_AREA_KB	(2000U)	// Increased for AT demo full-screen images // 
#endif

// 4. FRAME BUFFER LOCATION 
/* Bu address frame buffer' larin bulundugu adres oluyor.  */
#define LCD_VRAM_BASE_ADDR 	    ((uint32_t)RUNTIME_PICs_SDRAM_START + (RUNTIME_PICs_SDRAM_AREA_KB*1024U))
#define FRAME_BUFFER_LENGTH			(((uint32_t)FRAME_SIZE * (uint32_t)NUM_BUFFERS))

// 5. EMWIN RESERVED AREA 
/* 256KB guvenlik payi olmazsa HardwareFault hatas1 aliyorum. Simdilik dursun:)) */
/* Bu address emWin internal usage adresinin baslangici */
#define LCD_GUI_RAM_BASE  \
	(uint32_t)(LCD_VRAM_BASE_ADDR + (256U*1024U) + FRAME_BUFFER_LENGTH) 
// Define the available number of bytes available for the GUI
#if(0)
f	#define GUI_NUMBYTES  (((1024U * 1024U) * 28U) + (400U * 1024U)) //  MByte
#else
	#if(USED_LCD == LCD_7_0_TFT)
f		#define GUI_NUMBYTES  (((1024U * 1024U) * 22U) + (400U * 1024U)) //  MByte (Reduce for AT-Demo full-screen pics)
	#else
		#define GUI_NUMBYTES  (((1024U * 1024U) * 26U) + (400U * 1024U)) //  MByte (Reduce for AT-Demo full-screen pics)
	#endif
#endif

// 6. FreeRTOS HUGE HEAP //
// #define configHEAP_ON_SDRAM  // Check FreeRTOSConfig.h file for same definition // 
#ifdef configHEAP_ON_SDRAM	
f	#define FreeRTOS_HEAP_START		(LCD_GUI_RAM_BASE + GUI_NUMBYTES + (256U*1024U))
	#define FreeRTOS_HUGE_HEAP_SIZE		(2U*1024U*1024U)
#else
	#define FreeRTOS_HEAP_START		(LCD_GUI_RAM_BASE + GUI_NUMBYTES)
	#define FreeRTOS_HUGE_HEAP_SIZE		0
#endif

#define SDRAM_MAX_ADDR_	(FreeRTOS_HEAP_START + FreeRTOS_HUGE_HEAP_SIZE)

#define INTRO_RAW_PICs_AREA_SIZE		(16UL * 1024UL * 1024UL)	
	// 16MB is used at the end of GUI RAM for INTRO RLE8 coded PICs storage // 
	// Each RLE8 picture size is max 50KB, @ 20fps there is 275pics (excluding gap pictures) and so totally 50*1024*275 bytes 
	// == 13.5MB SD-RAM size is required to store them. 
#define INTRO_RAW_PICs_AREA_START		((SDRAM_BASE_ADDR + SDRAM_SIZE) - INTRO_RAW_PICs_AREA_SIZE)

#define BATTERY_LEVEL_MAX		100
#define BATTERY_LEVEL_MIN		0
#define BRIGHTNESS_MAX			100
#define BRIGHTNESS_MIN			10

// Application Software Timers //
#define ID_TIMER_SB_UPDATE  				1
#define ID_TIMER_SCREEN_ANIM				2
#define ID_TIMER_OTOANIMATION  				ID_TIMER_SCREEN_ANIM
#define ID_TIMER_STDANIMATION  				ID_TIMER_SCREEN_ANIM
#define ID_TIMER_DEPTH_ANIMATION  			ID_TIMER_SCREEN_ANIM
#define ID_TIMER_GB_ANIM					ID_TIMER_SCREEN_ANIM
#define ID_TIMER_RM_ANIM					ID_TIMER_SCREEN_ANIM
#define ID_TIMER_SYS_ANIMATION				ID_TIMER_SCREEN_ANIM
#define ID_TIMER_MA_ANIM					ID_TIMER_SCREEN_ANIM

#define ID_TIMER_GB_REAL					3
#define ID_TIMER_GB_POPUP					4
#define ID_TIMER_BATTERY_POPUP				5
#define ID_TIMER_LANG_POPUP					6
#define ID_TIMER_SB_BAT_WARN				7
#define ID_TIMER_DEPTH_GUI					8
#define ID_TIMER_AT_RADAR					9
#define ID_TIMER_INTRO						10

/****************************************/
#define RM_INITIAL_ANIM		FALSE					/* :TODO: Dialing telephone animation with menu icons */
/****************************************/
#define EMWIN_GUI_TMR_NUM								(0)					/* Used for BSP main wait_ms function gor GUI */
#define TOUCH_SCREEN_TIMER							(LPC_TIM1)			/* Used for ADC reading animation for Search screen Gauge simulation in BSP.c */
#define TOUCH_SCREEN_TIMER_INT					(TIMER1_IRQn)
#define TOUCH_SCREEN_IRQ_HANDLER				TIMER1_IRQHandler
#define WAVE_AMPLITUDE_UPDATE_TMR				(LPC_TIM2)
#define WAVE_AMPLITUDE_UPDATE_TMR_INT			(TIMER2_IRQn)
#define WAVE_AMPLITUDE_UPDATE_TMR_IRQ_HANDLER	TIMER2_IRQHandler
#define CLOCK_GENERATION_LPC_TIMER				(LPC_TIM3)		/* Used for clock generation */
#define CLOCK_GENERATION_TMR_INT				(TIMER3_IRQn)	

//------------------------------------------------------------------------//
//--- APPLICATION INTERRUPT PRIORITIEs (LOWER NUMBER HIGHER PRIORITY) ----//
//------------------------------------------------------------------------//
#define APP_INTERRRUPT_PRIORITY_GROUPING	(4)

// Priority Group 0 (Highest) //
#define EXTI_IRQ_GROUP_PRIO					(0)	// EXTI : Input clock sync // 
#define EXTI_IRQ_SUB_PRIO					(0) // 0 // 
// Priority Group 1 (cortex-System IRQs) //
#define CORTEXM3_SYS_IRQ_GROUP_PRIO			(1)
#define CORTEXM3_SYS_IRQ_SUB_PRIO			(1)

// Priotiy Group 2 (Peripherals' Driver IRQs) // 
#define LCD_IRQ_GROUP_PRIO					(2)	// LCD data display // 
#define LCD_IRQ_SUB_PRIO					(0)	// 0 // 
#define DMA_IRQ_GROUP_PRIO					(2)	// DMA IRQ //
#define DMA_IRQ_SUB_PRIO					(0)	// 0 // 
#define I2S_IRQ_GROUP_PRIO					(2)	// AUDIO CODEC data send // 
#define I2S_IRQ_SUB_PRIO					(1) // 1 // 
#define I2C_IRQ_GROUP_PRIO					(2)	// AUDIO CODEC configuration // 
#define I2C_IRQ_SUB_PRIO					(2) // 2 // 
// Priotiy Group 3 (Application Specific IRQs) // 
#define UART1_IRQ_GROUP_PRIO				(3)	// UART character send & receive from detector // 
#define UART1_IRQ_SUB_PRIO					(0)	// 0 // 
#define MCI_IRQ_GROUP_PRIO					(3)	// MCI IRQ: SD-CARD read //
#define MCI_IRQ_SUB_PRIO					(1)	// 1 // 
#define USB_IRQ_GROUP_PRIO					(3)	// USB IRQ //
#define USB_IRQ_SUB_PRIO					(1)	// 1 // 
#define TMR2_IRQ_GROUP_PRIO					(3)	// TIMER2 : DAC sineWave modulator update // 
#define TMR2_IRQ_SUB_PRIO					(2)	// 2 // 
#define TMR1_IRQ_GROUP_PRIO					(3)	// TIMER1 : Touch-Screen sample timing //
#define TMR1_IRQ_SUB_PRIO					(2)	// 2 // 
#define SYSTICK_IRQ_GROUP_PRIO				(3)	// SYSTICK // 
#define SYSTICK_IRQ_SUB_PRIO				(3)	// 3 // 
#define ADC_IRQ_GROUP_PRIO					(3)	// ADC // 
#define ADC_IRQ_SUB_PRIO					(3)	// 3 // 


/****************************************/
/********* ADC CH DEFINITIONs ***********/
/****************************************/
#define POT_READ_ADC_CH			(ADC_CHANNEL_4)
#define BATTERY_READ_ADC_CH		(ADC_CHANNEL_1)
#define JACK_DETECT_ADC_CH		(ADC_CHANNEL_2)
#define ADC_CH_COUNT			(8)

/****************************************/
/*********** TASK DEFINITIONs ***********/
/****************************************/
//------------------------------------------------------------------------//
//----- APPLICATION TASK PRIORITIEs (BIGGER NUMBER HIGHER PRIORITY) ------//
//------------------------------------------------------------------------//
// GUI (Emwin) Task // 
// Durduk yere HWFault hatalar1 al1rsan bu genelde bu taskin stack' ini arttir. // 
#define GUI_TASK_PRIORITY				(tskIDLE_PRIORITY + 1)			// 1 //
#define GUI_TASK_STACK_SIZE				(1024*10)
#define GUI_TASK_MESSAGE_QUEUE_SIZE		(10)		// GUI task message queue maximum message count //  
#define GUI_TASK_FLAG					(0x1 << UMD_TASK_GUI)

// Seraial Port (PC Communication) Reading & Writing Task // 
#define SERIAL_TASK_PRIORITY			  	( tskIDLE_PRIORITY + 1 )		// 1 // 
#define SERIAL_TASK_STACK_SIZE				( 512 )
#define SERIAL_TASK_MESSAGE_QUEUE_SIZE		10
#define SERIAL_TASK_STATE					TRUE
#if(TRUE == SERIAL_TASK_STATE)
	#define SERIAL_TASK_FLAG				(0x1<<UMD_TASK_SERIAL)
#else
	#define SERIAL_TASK_FLAG				0
#endif

// Led blinking Task // 
#define LED_TASK_PRIORITY				( tskIDLE_PRIORITY + 1 )		// 1 // 
#define LED_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )
#define LED_TASK_STATE				((POWERMCU_DEV_BOARD == USED_HW)?(TRUE):(FALSE))
#if(TRUE == TS_TASK_STATE)
	#define LED_TASK_FLAG						(0x1<<UMD_TASK_LED)
#else
	#define LED_TASK_FLAG						0
#endif

// Keyboard Scan Poling Task //
#define KS_TASK_PRIORITY			  ( tskIDLE_PRIORITY + 1 + 1 )	// 2 // 
#define KS_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )
#define KS_TASK_FLAG							(0x1<<UMD_TASK_KS)

// Touch Screen Event Polling Task // 
#define TS_TASK_PRIORITY			  ( tskIDLE_PRIORITY + 1 + 1 )	// 2 // 
#define TS_TASK_STACK_SIZE			( configMINIMAL_STACK_SIZE )
#define TS_TASK_STATE						(((POWERMCU_DEV_BOARD == USED_HW) && ((USED_LCD == LCD_7_0_TFT) || (USED_LCD == LCD_4_3_TFT)))) ?  \
																		TRUE : FALSE
#if(TRUE == TS_TASK_STATE)
	#define TS_TASK_FLAG						(0x1<<UMD_TASK_TS)
#else
	#define TS_TASK_FLAG						0
#endif

// Serial port (Dedector Communication) Reading & Writing Task // 
#define COMM_TASK_PRIORITY			(tskIDLE_PRIORITY + 1 + 1 + 1)		// 3 // 
#define COMM_TASK_STACK_SIZE		(256)
#define COMM_TASK_FLAG					(0x1 << UMD_TASK_COMM)

#define MAX_TASK_PRIORITY				(COMM_TASK_PRIORITY + 1)

// SOFTWARE TIMREs //
#define COMM_TIMEOUT_FR_TMR_INDX	0
#define COMM_TIMEOUT_FR_TMR_CMD_BLOCK_MS	100

// Because of "configUSE_16_BIT_TICKS" is defiend as 0, UMD_TASK_COUNT can be maximum 24 // 
#define	UMD_TASK_GUI					(0)		/* GUI Management Task */ 
#define UMD_TASK_FIRST				UMD_TASK_GUI
#define	UMD_TASK_KS						(1)		/* Keypasd Scanning Task */ 
#define UMD_TASK_TS						(2)		/* Touch Screen Scanning Task */ 
#define	UMD_TASK_LED					(3)		/* led task, for debugging */
#define	UMD_TASK_SERIAL				(4)		/* Serial Debug framework task : Serial.c */
#define	UMD_TASK_COMM					(5)		/* Communication with detector task */
#define UMD_TASK_LAST					UMD_TASK_COMM
#define UMD_TASK_COUNT				(UMD_TASK_LAST + 1)
	
#define ALL_TASK_BITs	(GUI_TASK_FLAG | SERIAL_TASK_FLAG | LED_TASK_FLAG | KS_TASK_FLAG | TS_TASK_FLAG | COMM_TASK_FLAG)

// Common Servicing Function // 
extern void Print_Mem_Data(uint8_t force);
extern uint32_t get_block_crc32(uint8_t *block, uint32_t size);
extern void App_waitMS(uint32_t wMS);
extern void init_TPs(uint8_t tp_num);
extern uint32_t get_crc32(uint8_t val, uint8_t first);

// Shared //
typedef struct {	
	int16_t xsize;
	int16_t ysize;
	int16_t Factor1000;
	char name[16];
} sFactorInfoType;

// APPLICATION UART USAGE DEFINITIONs //
// UART0 is used by debug_framework; defined in debug_framework.h // 
// UART1 is used for dedector communication // 
#define COMMUNICATION_USART_PORT_NUM		1	// Used for detector communication // 	
#define USED_UART_DEBUG_PORT						0	// 0 is USB SERIAL converter @ CN6 // 

// SET TEST POINTs and SPECIAL FUNCTION GPIOs RELATED RESOURCEs //
#define TP5_PORT_NUM	(0)
#define TP5_PIN_NUM		(22)
#define TP4_PORT_NUM	(2)
#define TP4_PIN_NUM		(12)
#define MUTE_SPK_PORT_NUM	(0)
#define MUTE_SPK_PIN_NUM		(1)

#define SET_TP(port_num, pin_num, new_state) (GPIO_OutputValue(port_num, (0x1<<pin_num), new_state))
#define READ_TP(port_num, pin_num) ((GPIO_ReadValue(port_num) & (0x1<<pin_num))?(1):(0))

#endif

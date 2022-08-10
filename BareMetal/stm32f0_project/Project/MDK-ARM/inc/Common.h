#ifndef COMMON_H
#define COMMON_H

#include "Version.h"
#include "stm32f0xx.h"

#ifndef FALSE
	#define FALSE	0
#endif
#ifndef TRUE 
	#define TRUE	(!FALSE)
#endif

typedef struct {
	GPIO_TypeDef *port;
	uint16_t pin;
} port_pin_type;

#define REVERSE32(u32ptr) {\
	uint8_t *u8ptr = (uint8_t *)u32ptr;\
	/* change 0th and 3rd bytes content */\
	uint8_t tempU8 = u8ptr[0];\
	u8ptr[0] = u8ptr[3];\
	u8ptr[3] = tempU8;\
	/* change 1st and 2nd bytes content */ \
	tempU8 = u8ptr[1];\
	u8ptr[1] = u8ptr[2];\
	u8ptr[2] = tempU8;\
}
#define SLOG(fmt, args...) 			\
{\
}

#define ATOMIC_START(IRQn)	NVIC_DisableIRQ(IRQn)
#define ATOMIC_END(IRQn)	NVIC_EnableIRQ(IRQn)

// CONFIGURATION //
#define BERK_VOLTAGE_DROP_COMPENSATION 		FALSE
#define CABLE_LOSS_COMPANSATION						FALSE
#define SYSTICK_HZ		1000
#define SYSTICK_MS		(1000/SYSTICK_HZ)
#define STALE_ON_ERROR	(1)
#define CODE_DEBUG			TRUE		/* If Debugging with Keil is ACTIVE set this to TRUE */
#define DONT_RESET_OMAP			FALSE		/* If Debugging with Keil is ACTIVE set this to TRUE */
#define ENC_VBUS_CONTROL		FALSE
#define FPGA_RELEASE_DELAY_MS			3000	// Must be smaller than 65536 (16bit) // 
#define INITIAL_DELAY_MS					(13500)
#define ENC_INITIAL_RESET_CYCLE		3
#define RESET_MODEM_BEFORE_ENABLE	FALSE

#define HW_FUJITSU_HD_MB				0x0
#define HW_3GBONDING_MB					0x1
#define MAIN_HW_TYPE 						HW_FUJITSU_HD_MB

#if(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
// STAT1, STAT2, PG, TP20 Related usage definitions // 
	#define STAT1_AS_BUTTON_INPUT			FALSE
	#define STAT1_AS_FJEEPROM_CS			TRUE
	#define STAT2_AS_FJEEPROM_MISO		TRUE
	#define PG_AS_FJEEPROM_MOSI				TRUE
	#define TP20_AS_FJEEPROM_SCK			TRUE
#else
	#define STAT1_AS_BUTTON_INPUT			FALSE
	#define STAT1_AS_FJEEPROM_CS			FALSE
	#define STAT2_AS_FJEEPROM_MISO		FALSE
	#define PG_AS_FJEEPROM_MOSI				FALSE
	#define TP20_AS_FJEEPROM_SCK			FALSE
#endif

#endif

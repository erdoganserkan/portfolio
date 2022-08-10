#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include <stdint.h>

extern void DelayMS(uint16_t MSDelay);
extern uint32_t const *get_uniqueIDptr(void);
extern void pwm_init(void);	// SysFAN & LcdBacklight PWM control initialization // 
extern void set_pwm_duty(uint8_t ch, uint8_t duty);
extern void ReCycleOmap(void);
extern void freq_in_init(void);
extern void onewire_init(void);
extern void SetHBState(uint8_t new_state);

/* HB DETECTION */
#if(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
	#define HB_GPIO_PIN      			GPIO_Pin_6
	#define HB_GPIO_PORT          GPIOA
	#define HB_GPIO_CLK           RCC_AHBPeriph_GPIOA
	#define HB_EXTI_LINE          EXTI_Line6
	#define HB_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOA
	#define HB_EXTI_PIN_SOURCE    EXTI_PinSource6
	#define HB_EXTI_IRQn          EXTI4_15_IRQn 
#elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
	#define HB_GPIO_PIN      			GPIO_Pin_1
	#define HB_GPIO_PORT          GPIOB
	#define HB_GPIO_CLK           RCC_AHBPeriph_GPIOB
	#define HB_EXTI_LINE          EXTI_Line1
	#define HB_EXTI_PORT_SOURCE   EXTI_PortSourceGPIOB
	#define HB_EXTI_PIN_SOURCE    EXTI_PinSource1
	#define HB_EXTI_IRQn          EXTI0_1_IRQn 
#endif
#define HB_PROCESSING_INTERVAL_S		(5)
#define MIN_HB_COUNTER_VAL					(2)	// Counting Both of Rising & Falling edges during //
																					// HB_PROCESSING_INTERVAL_S seconds // 


void init_HB_detection(void);
void DoHBProcessing(void);
extern volatile uint8_t HBDetection;
extern volatile uint16_t HBProcessingCntr;
extern volatile uint8_t HBCounter;

extern volatile uint8_t LinuxMCUCheck;	// OMAP will enable this //
extern volatile uint16_t LinuxMCUCheckTimeCnt;	// Time Counting Variable //
extern volatile uint8_t LinuxMCUCheckRspCnt;
extern volatile uint8_t LinuxMCUCheckCmdCnt;

extern uint16_t linux_mcu_version;

#endif

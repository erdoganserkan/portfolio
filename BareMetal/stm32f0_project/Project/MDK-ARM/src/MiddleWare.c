#include <stdio.h>
#include <stdint.h>
#include "HDSerial.h"
#include "Common.h"
#include "stm32f0xx.h"
#include "stm32f0xx_adc.h"
#include "stm32f0xx_dma.h"
#include "SysInit.h"
#include "Uart.h"
#include "Main.h"
#include "MiddleWare.h"

/*** HeartBeat Detection ****/
volatile uint8_t HBDetection = FALSE;	// OMAP will enable this //
volatile uint8_t HBCounter = 0;		// Rising & Falling Edges Counter // 
volatile uint16_t HBProcessingCntr = 0;	// Time Counting Variable //
/*** LINUX MCU CHECK ***/
volatile uint8_t LinuxMCUCheck = FALSE;	// OMAP will enable this //
volatile uint16_t LinuxMCUCheckTimeCnt = 0;	// Time Counting Variable //
volatile uint8_t LinuxMCUCheckCmdCnt = 0; // Command Send counter // 
volatile uint8_t LinuxMCUCheckRspCnt = 0;

/*** PWM Generation ***/
static void PWM_gpio_config(void);
static TIM_OCInitTypeDef  TIM_OCInitStructure;
static uint16_t TimerPeriod = 0;

uint16_t linux_mcu_version = 0xFFFF;

// This function verified and working // 
void ReCycleOmap(void)
{
	GPIO_WriteBit(OMAP_WARM_GPIO_PORT, OMAP_WARM_PIN, Bit_SET);
	GPIO_WriteBit(OMAP_WRON_GPIO_PORT, OMAP_WRON_PIN, Bit_SET);
	DelayMS(250);
	GPIO_WriteBit(OMAP_WARM_GPIO_PORT, OMAP_WARM_PIN, Bit_RESET);
	GPIO_WriteBit(OMAP_WRON_GPIO_PORT, OMAP_WRON_PIN, Bit_RESET);
}

void pwm_init(void)	// SysFAN & LcdBacklight PWM control initialization // 
{
	// PB4, LCD PWM, 50KHz, TIM3_CH1 //
	// PB5, SYSFAN PWM, 1KHz, TIM3_CH2 // 

	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	uint16_t Channel1Pulse = 0, Channel2Pulse = 0;
	
  PWM_gpio_config();
  
  /* TIM3 Configuration ---------------------------------------------------
   Generate PWM signals with 2 different duty cycles:
   TIM3 input clock (TIM1CLK) is set to APB2 clock (PCLK2)    
    => TIM1CLK = PCLK2 = SystemCoreClock
   TIM1CLK = SystemCoreClock, Prescaler = 0, TIM1 counter clock = SystemCoreClock
   SystemCoreClock is set to 48 MHz for STM32F0xx devices
   
   The objective is to generate 4 PWM signal at 17.57 KHz:
     - TIM1_Period = (SystemCoreClock / 17570) - 1
   The channel 1 and channel 1N duty cycle is set to 50%
   The channel 2 and channel 2N duty cycle is set to 37.5%
   The Timer pulse is calculated as follows:
     - ChannelxPulse = DutyCycle * (TIM1_Period - 1) / 100
   
   Note: 
    SystemCoreClock variable holds HCLK frequency and is defined in system_stm32f0xx.c file.
    Each time the core clock (HCLK) changes, user had to call SystemCoreClockUpdate()
    function to update SystemCoreClock variable value. Otherwise, any configuration
    based on this variable will be incorrect. 
  ----------------------------------------------------------------------- */
  /* Compute the value to be set in ARR regiter to generate signal frequency at 17.57 Khz */
  TimerPeriod = (SystemCoreClock / DEF_LCD_BACKLIGHT_FREQ_HZ ) - 1;
  /* Compute CCR1 value to generate a duty cycle at DEFAULT for channel 1 */
  Channel1Pulse = (uint16_t) (((uint32_t) (100-100) * (TimerPeriod - 1)) / 100);
  /* Compute CCR2 value to generate a duty cycle at DEFAULT  for channel 2 */
  Channel2Pulse = (uint16_t) (((uint32_t) (100-100) * (TimerPeriod - 1)) / 100);

  /* TIM3 clock enable */
  RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3 , ENABLE);
  
  /* Time Base configuration */
  TIM_TimeBaseStructure.TIM_Prescaler = 0;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
  TIM_TimeBaseStructure.TIM_Period = TimerPeriod;
  TIM_TimeBaseStructure.TIM_ClockDivision = 0;
  TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;

  TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

  /* Channel 1, 2, 3 and 4 Configuration in PWM mode */
  TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
  TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
  TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Enable;
  TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
  TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;
  TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Set;
  TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCIdleState_Reset;

  TIM_OCInitStructure.TIM_Pulse = Channel1Pulse;
  TIM_OC1Init(TIM3, &TIM_OCInitStructure);	 // LCD BACKLIGHT PWM // 

  TIM_OCInitStructure.TIM_Pulse = Channel2Pulse;
  TIM_OC2Init(TIM3, &TIM_OCInitStructure);	// FAN PWM //

  /* TIM3 counter enable */
  TIM_Cmd(TIM3, ENABLE);

  /* TIM1 Main Output Enable */
  TIM_CtrlPWMOutputs(TIM3, ENABLE);
	
	DelayMS(100);
	set_pwm_duty(1, DEF_LCD_BACKLIGHT_DUTY);
	set_pwm_duty(2, DEF_FAN_PWM_DUTY);
}

void set_pwm_duty(uint8_t ch, uint8_t duty)
{
  if(ch == 1) {
		/* Compute CCR1 value to generate a duty cycle at DEFAULT for channel 1 */
		TIM_OCInitStructure.TIM_Pulse = (uint16_t) (((uint32_t) (100-duty) * (TimerPeriod - 1)) / 100);
		TIM_OC1Init(TIM3, &TIM_OCInitStructure);	 // LCD BACKLIGHT PWM // 
	}

  if(ch == 2) {
		/* Compute CCR2 value to generate a duty cycle at DEFAULT for channel 2 */
		TIM_OCInitStructure.TIM_Pulse = (uint16_t) (((uint32_t) (100-duty) * (TimerPeriod - 1)) / 100);
		TIM_OC2Init(TIM3, &TIM_OCInitStructure);	 // LCD BACKLIGHT PWM // 
	}
}

/**
  * @brief  Configure the TIM3 Pins.
  * @param  None
  * @retval None
  */
static void PWM_gpio_config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

  /* GPIOA Clocks enable */
  RCC_AHBPeriphClockCmd( RCC_AHBPeriph_GPIOB, ENABLE);
  
  /* GPIOB Configuration: TIM3 Channel 1, 2 as alternate function push-pull */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP ;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4, GPIO_AF_1);
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_1);
}

// Delay milisecond is based on SYSTIC IRQ and so be sure that SYSTICK IRQ is working before calling this function // 
void DelayMS(uint16_t MSDelay)
{
	extern volatile uint32_t systick_cnt;
	uint32_t local_tick = systick_cnt;
	
	local_tick += (MSDelay/SYSTICK_MS);
	while(systick_cnt != local_tick);
}

void onewire_init(void)
{
	// temperature sensor reaging init // 
}

void freq_in_init(void)
{
	// PB9 is used for TIM17_CH1 frequency measurement // 

}

uint32_t const *get_uniqueIDptr(void) 
{
	// uniqueID is 96bits // 
	return (uint32_t const *)0x1FFFF7AC;	// Address got from datasheet, DONT CHANGE //
}

// Set GPIO for EXTI interrupt, Enable EXTILine4-15 interrupt // 
void init_HB_detection(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  EXTI_InitTypeDef EXTI_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure;

  /* Enable the HBPin Clock */
  RCC_AHBPeriphClockCmd(HB_GPIO_CLK, ENABLE);
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_SYSCFG, ENABLE);

  /* Configure Button pin as input */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = HB_GPIO_PIN;
  GPIO_Init(HB_GPIO_PORT, &GPIO_InitStructure);

  {
    /* Connect Button EXTI Line to Button GPIO Pin */
    SYSCFG_EXTILineConfig(HB_EXTI_PORT_SOURCE, HB_EXTI_PIN_SOURCE);

    /* Configure Button EXTI line */
    EXTI_InitStructure.EXTI_Line = HB_EXTI_LINE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising_Falling;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    /* Enable and set Button EXTI Interrupt to the lowest priority */
    NVIC_InitStructure.NVIC_IRQChannel = HB_EXTI_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPriority = 0x03;
    NVIC_InitStructure.NVIC_IRQChannelCmd = DISABLE;

    NVIC_Init(&NVIC_InitStructure); 
  }
}

void SetHBState(uint8_t new_state)
{
	if(TRUE == new_state) {
		NVIC_DisableIRQ(HB_EXTI_IRQn);
		HBDetection = TRUE;	
		HBCounter = HBProcessingCntr = 0;
		NVIC_EnableIRQ(HB_EXTI_IRQn);
	}
	else {
		NVIC_DisableIRQ(HB_EXTI_IRQn);
		HBDetection = FALSE;
		HBCounter = HBProcessingCntr = 0;
	}
}

// Called every time "HB_PROCESSING_INTERVAL_S" seconds passed // 
void DoHBProcessing(void)
{
	if((TRUE == HBDetection) && ((HB_PROCESSING_INTERVAL_S*SYSTICK_HZ) <= HBProcessingCntr)) {
		HBProcessingCntr = 0;
		if(HBCounter >= MIN_HB_COUNTER_VAL) {
			// No Problem, OMAP is functional, Reset Counter //
			ATOMIC_START(HB_EXTI_IRQn);	// ATOMIC START //
			HBCounter = 0;
			ATOMIC_END(HB_EXTI_IRQn);		// ATOMIC END //
		}
		else {
			POffEncoder();
			set_pwm_duty(1, 0);
			set_pwm_duty(2, 0);
			GPIO_WriteBit(ENC3v3MAIN_GPIO_PORT, ENC3v3MAIN_PIN, Bit_SET);	// CUT OMAP and others power //
			GPIO_WriteBit(LCDPOWER_GPIO_PORT, LCDPOWER_PIN, Bit_SET);	// Disable LCD Board's Power // 
			GPIO_WriteBit(RESETMODULES_GPIO_PORT, RESETMODULES_PIN, Bit_SET);	// SET Modules to RESET state // 
			DelayMS(1000);	// Wait for a while for mainboard stabilization // 
			// OMAP is NOT functional, RECYCLE OMAP module & ME // 
			NVIC_SystemReset();
			//HBCounter = 0;
		}
	}
}

void DoLinuxMCUStatusProcessing(void) 
{
	if((TRUE == LinuxMCUCheck) && (((LINUX_MCU_CHECK_INTERVAL_MS*SYSTICK_HZ)/1000) == LinuxMCUCheckTimeCnt)) {
		LinuxMCUCheckTimeCnt = 0;
		// Send New CMD and Increment CMD Counter // 
		prep_send_msg(CMD_IS_LINUX_MCU_OK, 0, NULL);
		if(LINUX_MCU_CHECK_COUNT == (LinuxMCUCheckCmdCnt++)) {	// Chck if maximum CMD count reached // 
			if((LinuxMCUCheckRspCnt < LinuxMCUCheckCmdCnt) && ((LinuxMCUCheckCmdCnt - LinuxMCUCheckRspCnt) > (LINUX_MCU_CHECK_COUNT/4))) {
				// OMAP MCU didn' t give us response, it hass crashed and so RESET DEVICE // 
				NVIC_SystemReset();
			}
			else {	// OMAP responses are ok, clear resources for next cycle // 
				LinuxMCUCheck = FALSE;
				LinuxMCUCheckTimeCnt = LinuxMCUCheckRspCnt = LinuxMCUCheckCmdCnt = 0;
			}
		}
	}
}

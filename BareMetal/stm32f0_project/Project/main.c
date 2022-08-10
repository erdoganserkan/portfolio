/**
  ******************************************************************************
  * @file    main.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    23-March-2012
  * @brief   Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT 2012 STMicroelectronics</center></h2>
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  */ 

/* Includes ------------------------------------------------------------------*/
#include <stdio.h>
#include "Common.h"
#include "MiddleWare.h"
#include "SysInit.h"
#include "Uart.h"
#include "AdcRead.h"
#include "main.h"

/** @addtogroup STM32F0-Discovery_Demo
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t TimingDelay;
static uint8_t EncoderState = TRUE;
uint8_t BlinkSpeed = 0;
static uint8_t IsButtonPressed(void);
static void SetAllModemsPower(uint8_t new_state);
static void ACodec_Reset_Cycle(void);
static void CheckProgramFJEeprom(void);
extern void program_fj_eeprom(void);

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program. 	
  * @param  None
  * @retval None
  */
int main(void)
{
	sysinit();
	pwm_init();			// PWM for system fan control (FAN must be connected to mainboard)
	DelayMS(INITIAL_DELAY_MS);	// Wait until OMAP inits USB signals before powering on encoder // 
	ReCycle_Encoder();	// MB86M02x encoder is released here, it is ENABLED // 
	//CheckProgramFJEeprom();
	DelayMS(500);

	SetAllModemsPower(TRUE);
	#if(MAIN_HW_TYPE == HW_3GBONDING_MB)
		GPIO_WriteBit(WIFI_POWER_GPIO_PORT, WIFI_POWER_PIN, Bit_SET);	// Enable WIFI power, it is ACTIVE HIGH // 
	#endif
	
	init_HB_detection();	// omap application healt detection 
	//onewire_init();	// External temperature sensor communication 
	//freq_in_init();	// Frequency input for system fan
		
	Terminal_OnRXTimeout();		// Clear
	#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
		ACodec_Reset_Cycle();
	#endif
	while(1) {
		// Check UART messages from omap // 
		if(TRUE == Terminal_IsMsgPending())
			TerminalProcessMsg();
		if(TRUE == UsartRXTimeout) {
			Terminal_OnRXTimeout();
		}
		// Do Heart-Beat Related Processing // 	
		DoHBProcessing();
		// Do OMAP Checking Status // 
		DoLinuxMCUStatusProcessing();
		// Do ADC Related Processing // 
		//update_adc();
		#if(FALSE == CODE_DEBUG)
			// Enter to STANDBY and wait for INT //
			PWR_EnterSTANDBYMode();
		#endif
		
		{
			static int a=0;
			if(1 == a) {
				SetHBState(TRUE);
				a = 0;
			}
		}
	}

}

#if((TRUE == TP20_AS_FJEEPROM_SCK) && (TRUE == PG_AS_FJEEPROM_MOSI) && \
	(TRUE == STAT2_AS_FJEEPROM_MISO) && (TRUE == STAT1_AS_FJEEPROM_CS) && 0)
void CheckProgramFJEeprom(void)
{
	program_fj_eeprom();
}
#endif

#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
static void ACodec_Reset_Cycle(void) 
{
	volatile uint8_t indx;
	for(indx=0 ; indx<5 ; indx++) {
		GPIO_WriteBit(ACODEC_RESET_GPIO_PORT, ACODEC_RESET_PIN, Bit_RESET);	// reset state //
		DelayMS(100);
		GPIO_WriteBit(ACODEC_RESET_GPIO_PORT, ACODEC_RESET_PIN, Bit_SET);	// normal state //
		DelayMS(100);
	}
}
#endif

static void SetAllModemsPower(uint8_t new_state) 
{
	volatile uint8_t indx;
	extern port_pin_type const modem_controls[MODEM_COUNT];
	BitAction pin_state = (TRUE == new_state)?(Bit_RESET):(Bit_SET);
	
	for(indx=0;indx<MODEM_COUNT;indx++) {
		#if(MAIN_HW_TYPE == HW_3GBONDING_MB)
			if(2 < indx)	{// Internal Modems(indx = 0,1,2) are ACTIVE LOW but external modems(indx = 3,4,5) are ACTIVE HIGH // 
				pin_state = (TRUE == new_state)?(Bit_SET):(Bit_RESET);
			}
		#endif
		GPIO_WriteBit(modem_controls[indx].port, modem_controls[indx].pin, pin_state);	
		DelayMS(250);
	}
}
	
	
	
#if(STAT1_AS_BUTTON_INPUT	== TRUE)
f
static uint8_t IsButtonPressed(void)
{
	uint8_t prs_cnt = 0;
	volatile uint8_t indx = 0;
	for(indx=5 ; 0 != indx ; indx--) {	
		if(0 == GPIO_ReadInputDataBit(BUTTON_READ_PORT, BUTTON_READ_PIN))
			prs_cnt++;
		else { 
			if(0 != prs_cnt)
				prs_cnt--;
		}
		DelayMS(5);
	}
	if(5 == prs_cnt)
		return TRUE;
	return FALSE;
}
#else
	static uint8_t IsButtonPressed(void) {return FALSE;}
#endif


void POffEncoder(void)
{
	// Bu sure all voltages are powered down // 
#if(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
	GPIO_WriteBit(ENC3v3EN_GPIO_PORT, ENC3v3EN_PIN, Bit_RESET);				// Active HIGH, Disable 3.3V_ENC // 
#elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
	GPIO_WriteBit(ENC5vEN_GPIO_PORT, ENC5vEN_PIN, Bit_SET);				// Active HIGH, Disable 3.3V_ENC // 
#endif
	GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_RESET_PIN_STATE);
	DelayMS(750);
	EncoderState = FALSE;
}

void POnEncoder(void)
{
	uint8_t indx;
	// Start FJ SEQUENCE // 
#if(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
	GPIO_WriteBit(ENC3v3EN_GPIO_PORT, ENC3v3EN_PIN, Bit_SET);	// Enable 3.3V_ENC // 
#elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
	GPIO_WriteBit(ENC5vEN_GPIO_PORT, ENC5vEN_PIN, Bit_SET);	// Enable 3.3V_ENC // 
#endif
	DelayMS(100);
	
#if(1)
	for(indx=0 ; indx<ENC_INITIAL_RESET_CYCLE ; indx++) {
		GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_RESET_PIN_STATE);				// Execute chip reset // 
		DelayMS(250);
		GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_WORKING_PIN_STATE);				// Release chip reset // 
		DelayMS(250);
	}
#endif
	DelayMS(250);
	EncoderState = TRUE;
}

void ReCycle_Encoder(void)
{
	//GPIO_WriteBit(SUSPENDFPGA_GPIO_PORT, SUSPENDFPGA_PIN, Bit_SET);	// SET FPGA to SUSPENDED State //
	//DelayMS(500);
	POffEncoder();
	DelayMS(100);
	POnEncoder();
	//DelayMS(500);
	//GPIO_WriteBit(SUSPENDFPGA_GPIO_PORT, SUSPENDFPGA_PIN, Bit_RESET);	// SET FPGA to NORMAL State //
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif

// MHD-949: Adc birim testi fonksiyonu // 
void birim_test_adc(void)
{
	extern __IO uint16_t BatLevelMV;
	Init_ADC();
	update_adc();
	printf("Battery Voltage (%u mV); Battery Capacity (%%%u)\n", BatLevelMV, BatteryCapacity);
}

/* MHD-949: Uart birim testi */
void birim_test_uart(void)
{
	while(1)
	{
		// Check UART messages from omap // 
		uint8_t localUS;
		ATOMIC_START(EVAL_COM1_IRQn);	
		localUS = UsartRXState;
		ATOMIC_END(EVAL_COM1_IRQn);	
		if(localUS == BUSY_STATE) 
			TerminalProcessMsg();
	}
}

/* MHD-949: Modem power kontrol birim testi */
void birim_test_modem (void)
{
	extern port_pin_type const modem_controls[MODEM_COUNT];
	volatile uint8_t indx;
	for(indx=0 ; indx<MODEM_COUNT ; indx++)
	{
		GPIO_WriteBit(modem_controls[indx].port, modem_controls[indx].pin, Bit_SET);	/* Enable modem power */
		printf("modem(%u) POWER ENABLED\n", indx);
		DelayMS(2000);
		GPIO_WriteBit(modem_controls[indx].port, modem_controls[indx].pin, Bit_RESET);	/* Enable modem power */
		printf("modem(%u) POWER DISABLED\n", indx);
		DelayMS(2000);
	}
}

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

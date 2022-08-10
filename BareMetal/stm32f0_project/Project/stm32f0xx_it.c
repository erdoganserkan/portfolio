/**
  ******************************************************************************
  * @file    stm32f0xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    23-March-2012
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
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
#include "stm32f0xx_it.h"
#include "Common.h"
#include "SysInit.h"
#include "Uart.h"
#include "main.h"
#include "MiddleWare.h"

/** @addtogroup STM32F0-Discovery_Demo
  * @{
  */

/** @addtogroup STM32F0XX_IT
  * @brief Interrupts driver modules
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/* External Variables ---------------------------------------------------------*/
volatile uint32_t systick_cnt = 0;
extern volatile uint16_t FPGA_ReleaseMS;
/******************************************************************************/
/*            Cortex-M0 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
	systick_cnt++;
	if(TRUE == HBDetection) {
		HBProcessingCntr++;
	}
	if(TRUE == LinuxMCUCheck)
		LinuxMCUCheckTimeCnt++;
	
#if(1)
	if(BUSY_STATE == UsartRXState) {
		if(MESSAGE_SEND_TIMEOUT_MS == (UsartRXTimeoutCnt += (1000/SYSTICK_HZ))) {
			UsartRXTimeout = TRUE;
		}
	}
	else {
		UsartRXTimeoutCnt = 0;
	}
	
	#endif
}

/******************************************************************************/
/*                 STM32F0xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f0xx.s).                                               */
/******************************************************************************/

/**
  * @brief  This function handles EXT4-15 interrupt request.
  * @param  None
  * @retval None
  */
#if(MAIN_HW_TYPE == HW_FUJITSU_HD_MB)
__irq void EXTI4_15_IRQHandler(void)
#elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
__irq void EXTI0_1_IRQHandler(void)
#endif
{
	// OMAP set HBPin, RE/FE detected //
	if(SET == EXTI_GetITStatus(HB_EXTI_LINE)) {
		EXTI_ClearITPendingBit(HB_EXTI_LINE);
		++HBCounter;
	}
}

/**
  * @brief  This function handles USART1_IRQHandler interrupt request.
  * @param  None
  * @retval None
  */
__irq void USART1_IRQHandler(void) 
{
	// Last ch sent to omap, or new ch received from omap // 
		Terminal_ISR();
}



/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

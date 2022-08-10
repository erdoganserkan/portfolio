#include "stm32f0xx.h"
#include "Uart.h"
#include "MiddleWare.h"
#include "AdcRead.h"
#include "SysInit.h"

static void init_GPIO(void);

void sysinit(void)
{
  RCC_ClocksTypeDef RCC_Clocks;
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / SYSTICK_HZ);
	init_GPIO();

	DelayMS(1000);
	// Enable 3.3V_Main, This will enable 5v_Main too //
	// And so LCD power will be anabled // 
	GPIO_WriteBit(ENC3v3MAIN_GPIO_PORT, ENC3v3MAIN_PIN, Bit_RESET);	// OMAP will powered up here // 
	DelayMS(100);	// Wait for a while for mainboard stabilization // 
	GPIO_WriteBit(LCDPOWER_GPIO_PORT, LCDPOWER_PIN, Bit_RESET);	// Enable LCD Board's Power // 
	GPIO_WriteBit(RESETMODULES_GPIO_PORT, RESETMODULES_PIN, Bit_RESET);	// Modules are NORMAL state // 
	GPIO_WriteBit(MODEMHUBRESET_GPIO_PORT, MODEMHUBRESET_PIN, Bit_SET);	// Release MODEM HUB to normal operation // 
	#if(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
		GPIO_WriteBit(RESETOTGHUB_GPIO_PORT, RESETOTGHUB_PIN, Bit_SET);	// Release OTG HUB to normal operation // 	
	#endif	
	//init_HB_detection();
	//TerminalUsartInit();
	my_usart_init();
	//Init_ADC();
	#if(FALSE == DONT_RESET_OMAP)
		ReCycleOmap();	// OMAP System Enable pins are released here, it will start boot exactly here // 
	#endif
}

static void init_GPIO(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure;
  
	//---------- OUTPUT SETTINGs ------//
  /* Enable the GPIO Clocks */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOF, ENABLE);

  /* Configure the Encoder RESET pin */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Pin = ENCRESET_PIN;

  GPIO_Init(ENCRESET_GPIO_PORT, &GPIO_InitStructure);
  #if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
  	GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_DEFVAL);
  #elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
		GPIO_WriteBit(ENCRESET_GPIO_PORT, ENCRESET_PIN, ENCRESET_RESET_PIN_STATE);
  #endif

	/* Configure the M1 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M1RESET_PIN;
  GPIO_Init(M1RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M1RESET_GPIO_PORT, M1RESET_PIN, M1RESET_DEFVAL);

  /* Configure the M2 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M2RESET_PIN;
  GPIO_Init(M2RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M2RESET_GPIO_PORT, M2RESET_PIN, M2RESET_DEFVAL);

  /* Configure the M3 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M3RESET_PIN;
  GPIO_Init(M3RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M3RESET_GPIO_PORT, M3RESET_PIN, M3RESET_DEFVAL);

  /* Configure the M4 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M4RESET_PIN;
  GPIO_Init(M4RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M4RESET_GPIO_PORT, M4RESET_PIN, M4RESET_DEFVAL);

  /* Configure the M5 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M5RESET_PIN;
  GPIO_Init(M5RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M5RESET_GPIO_PORT, M5RESET_PIN, M5RESET_DEFVAL);

  /* Configure the M6 RESET pin */
  GPIO_InitStructure.GPIO_Pin = M6RESET_PIN;
  GPIO_Init(M6RESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(M6RESET_GPIO_PORT, M6RESET_PIN, M6RESET_DEFVAL);

  /* Configure the OMAP_WRON pin */
  GPIO_InitStructure.GPIO_Pin = OMAP_WRON_PIN;
  GPIO_Init(OMAP_WRON_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(OMAP_WRON_GPIO_PORT, OMAP_WRON_PIN, OMAP_WRON_DEFVAL);

#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
  /* Configure the ENC3v3EN pin */
  GPIO_InitStructure.GPIO_Pin = ENC3v3EN_PIN;
  GPIO_Init(ENC3v3EN_GPIO_PORT, &GPIO_InitStructure);
  GPIO_WriteBit(ENC3v3EN_GPIO_PORT, ENC3v3EN_PIN, ENC3v3EN_DEFVAL);
#elif(MAIN_HW_TYPE == HW_3GBONDING_MB)
	/* Configure the ENC5vEN pin */
	GPIO_InitStructure.GPIO_Pin = ENC5vEN_PIN;
	GPIO_Init(ENC5vEN_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(ENC5vEN_GPIO_PORT, ENC5vEN_PIN, ENC5vEN_DEFVAL);
#endif

  /* Configure the LCDPOWER pin */
  GPIO_InitStructure.GPIO_Pin = LCDPOWER_PIN;
  GPIO_Init(LCDPOWER_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(LCDPOWER_GPIO_PORT, LCDPOWER_PIN, LCDPOWER_DEFVAL);

  /* Configure the MODEMHUBRESET pin */
  GPIO_InitStructure.GPIO_Pin = MODEMHUBRESET_PIN;
  GPIO_Init(MODEMHUBRESET_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(MODEMHUBRESET_GPIO_PORT, MODEMHUBRESET_PIN, MODEMHUBRESET_DEFVAL);

  #if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
  	/* Configure the USBETH3v3EN pin */
  	GPIO_InitStructure.GPIO_Pin = USBETH3v3EN_PIN;
  	GPIO_Init(USBETH3v3EN_GPIO_PORT, &GPIO_InitStructure);
  	GPIO_WriteBit(USBETH3v3EN_GPIO_PORT, USBETH3v3EN_PIN, USBETH3v3EN_DEFVAL);
		
  	/* Configure the SDIPATCH MUTE Control pin */
  	GPIO_InitStructure.GPIO_Pin = SDIPATCH_MUTE_PIN;
  	GPIO_Init(SDIPATCH_MUTE_PORT, &GPIO_InitStructure);
  	GPIO_WriteBit(SDIPATCH_MUTE_PORT, SDIPATCH_MUTE_PIN, SDIPATCH_MUTE_DEFVAL);
		
  #endif
	/* Configure the OMAP_WARM pin */
  GPIO_InitStructure.GPIO_Pin = OMAP_WARM_PIN;
  GPIO_Init(OMAP_WARM_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(OMAP_WARM_GPIO_PORT, OMAP_WARM_PIN, OMAP_WARM_DEFVAL);

  /* Configure the ENC3v3MAIN pin */
  GPIO_InitStructure.GPIO_Pin = ENC3v3MAIN_PIN;
  GPIO_Init(ENC3v3MAIN_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(ENC3v3MAIN_GPIO_PORT, ENC3v3MAIN_PIN, ENC3v3MAIN_DEFVAL);

  #if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
  	/* Configure the RESETUSBETH pin */
	GPIO_InitStructure.GPIO_Pin = RESETUSBETH_PIN;
	GPIO_Init(RESETUSBETH_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(RESETUSBETH_GPIO_PORT, RESETUSBETH_PIN, RESETUSBETH_DEFVAL);
  #endif

  /* Configure the RESETOTGHUB pin */
  GPIO_InitStructure.GPIO_Pin = RESETOTGHUB_PIN;
  GPIO_Init(RESETOTGHUB_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(RESETOTGHUB_GPIO_PORT, RESETOTGHUB_PIN, RESETOTGHUB_DEFVAL);

  /* Configure the RESETMODULES pin */
  GPIO_InitStructure.GPIO_Pin = RESETMODULES_PIN;
  GPIO_Init(RESETMODULES_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(RESETMODULES_GPIO_PORT, RESETMODULES_PIN, RESETMODULES_DEFVAL);

  /* Configure the SUSPEND_FPGA pin */
  GPIO_InitStructure.GPIO_Pin = SUSPENDFPGA_PIN;
  GPIO_Init(SUSPENDFPGA_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(SUSPENDFPGA_GPIO_PORT, SUSPENDFPGA_PIN, SUSPENDFPGA_DEFVAL);

//----------------------------------------------------------------//
//----------------------------------------------------------------//
#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
//----------------------------------------------------------------//
//----------------------------------------------------------------//

	#if(1)	// FJ BOARD Number 5 specific modification // 
		/* Configure the SUSPEND_FPGA pin */
		GPIO_InitStructure.GPIO_Pin = ACODEC_RESET_PIN;
		GPIO_Init(ACODEC_RESET_GPIO_PORT, &GPIO_InitStructure);
		GPIO_WriteBit(ACODEC_RESET_GPIO_PORT, ACODEC_RESET_PIN, ACODEC_RESET_DEFVAL);
	#endif
	//---------- INPUT SETTINGs ------//
#if(FALSE == STAT2_AS_FJEEPROM_MISO)	
  /* Configure STAT2 pin as input for BUTTON state read */
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = BATSTAT2_PIN;
  GPIO_Init(BATSTAT2_GPIO_PORT, &GPIO_InitStructure);
#else
  /* Configure STAT2 pin as input for SPI MISO */
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;		// Highest speed request // 
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = FJEEPROM_MISO_PIN;
  GPIO_Init(FJEEPROM_MISO_PORT, &GPIO_InitStructure);
#endif

  /* Configure STAT1 pin as input */
	#if(STAT1_AS_BUTTON_INPUT == TRUE)
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
		GPIO_InitStructure.GPIO_Pin = BATSTAT1_PIN;
		GPIO_Init(BATSTAT1_GPIO_PORT, &GPIO_InitStructure);
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	// Enable internal pull-up to be sure that logical level is HIGH // 
		//GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	#elif(TRUE == STAT1_AS_FJEEPROM_CS)
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;					// Output pin //
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			// No pull-up or down // 
		GPIO_InitStructure.GPIO_Pin = FJEEPROM_CS_PIN;				// CS pin // 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;		// Hishest sped request // 
		GPIO_Init(FJEEPROM_CS_PORT, &GPIO_InitStructure);
		GPIO_WriteBit(FJEEPROM_CS_PORT, FJEEPROM_CS_PIN, FJEEPROM_CS_DEFVAL);		// CS pin is HIGH @startup // 
	#endif

	#if(TRUE == TP20_AS_FJEEPROM_SCK)
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			// No pull-up or down // 
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;					// Output pin //
		GPIO_InitStructure.GPIO_Pin = FJEEPROM_SCK_PIN;				// MOSI pin // 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;		// Hishest sped request // 
		GPIO_Init(FJEEPROM_SCK_PORT, &GPIO_InitStructure);
		GPIO_WriteBit(FJEEPROM_SCK_PORT, FJEEPROM_SCK_PIN, FJEEPROM_SCK_DEF_VAL);		// CS pin is LOW @startup // 
	#endif

	#if(TRUE == PG_AS_FJEEPROM_MOSI)
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;			// No pull-up or down // 
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;					// Output pin //
		GPIO_InitStructure.GPIO_Pin = FJEEPROM_MOSI_PIN;				// MOSI pin // 
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_Level_3;		// Hishest sped request // 
		GPIO_Init(FJEEPROM_MOSI_PORT, &GPIO_InitStructure);
		GPIO_WriteBit(FJEEPROM_MOSI_PORT, FJEEPROM_MOSI_PIN, FJEEPROM_MOSI_DEFVAL);		// CS pin is HIGH @startup // 
	#endif

  /* Configure I2CEXPINT pin as input */
  GPIO_InitStructure.GPIO_Pin = I2CEXPINT_PIN;
  GPIO_Init(I2CEXPINT_GPIO_PORT, &GPIO_InitStructure);

	//----------------------------------------------------------------//
	//----------------------------------------------------------------//
#elif(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
	//----------------------------------------------------------------//
	//----------------------------------------------------------------//
  
	/* Configure the LCD_BCKLGHT pin */
  GPIO_InitStructure.GPIO_Pin = LCD_BCKLGHT_PIN;
  GPIO_Init(LCD_BCKLGHT_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(LCD_BCKLGHT_GPIO_PORT, LCD_BCKLGHT_PIN, LCD_BCKLGHT_DEFVAL);

	/* Configure the WIFI POWER pin */
  GPIO_InitStructure.GPIO_Pin = WIFI_POWER_PIN;
  GPIO_Init(WIFI_POWER_GPIO_PORT, &GPIO_InitStructure);
	GPIO_WriteBit(WIFI_POWER_GPIO_PORT, WIFI_POWER_PIN, WIFI_POWER_DEFVAL);

	/* INPUT PINs */
	/* Configure FAN SENSE pin as input */

	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Pin = FAN_SENSE_PIN;
  GPIO_Init(FAN_SENSE_GPIO_PORT, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;	// Enable internal pull-up to be sure that logical level is HIGH // 
	GPIO_Init(FAN_SENSE_GPIO_PORT, &GPIO_InitStructure);

#endif
}

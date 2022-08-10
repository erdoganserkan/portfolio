#ifndef SYSINIT_H
#define SYSINIT_H

#include 	"Common.h"
#include "stm32f0xx.h"

//------------------------------------------------------------------------------//
//------------------------------------------------------------------------------//
#if(MAIN_HW_TYPE	== HW_FUJITSU_HD_MB)
//------------------------------------------------------------------------------//
//------------------------------------------------------------------------------//
	/* GPIO Definitions */
	// PA0 - FUJITSU ENCODER RESET - Active LOW // 
	#define ENCRESET_RESET_PIN_STATE								Bit_RESET
	#define ENCRESET_WORKING_PIN_STATE							Bit_SET

	#define ENCRESET_PIN                         GPIO_Pin_0
	#define ENCRESET_GPIO_PORT                   GPIOA
	#define ENCRESET_DEFVAL											 ENCRESET_RESET_PIN_STATE	// HOLD pin @ RESET(logic LOW) until power sequence completed // 

	// PA1 - BATTERY TEMPERATURE - ANALOG // 
	#if(ENC_VBUS_CONTROL == TRUE)
		#define VBUS_CONTROL_PIN                        GPIO_Pin_1
		#define VBUS_CONTROL_PORT                  			GPIOA
		#define VBUS_CONTROL_DEFVAL											Bit_RESET	// Active HIGH, Disabled by default //
	#endif

	#if(TP20_AS_FJEEPROM_SCK == TRUE)
		#define FJEEPROM_SCK_PORT				GPIOA
		#define FJEEPROM_SCK_PIN				GPIO_Pin_4
		#define FJEEPROM_SCK_DEF_VAL		Bit_RESET
	#endif
	
	
	// PA5 - BATTERY LEVEL - ANALOG // 
	#define BATLEVEL_PIN                   GPIO_Pin_5
	#define BATLEVEL_PORT                  GPIOA

	// PA12 - Modem2 Reset - Active HIGH // 
	#define M2RESET_PIN                         GPIO_Pin_12
	#define M2RESET_GPIO_PORT                   GPIOA
	#define M2RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PA11 - Modem3 Reset - Active HIGH // 
	#define M3RESET_PIN                         GPIO_Pin_11
	#define M3RESET_GPIO_PORT                   GPIOA
	#define M3RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PA10 - Modem4 Reset - Active HIGH // 
	#define M4RESET_PIN                         GPIO_Pin_10
	#define M4RESET_GPIO_PORT                   GPIOA
	#define M4RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PA9 - Modem5 Reset - Active HIGH // 
	#define M5RESET_PIN                         GPIO_Pin_9
	#define M5RESET_GPIO_PORT                   GPIOA
	#define M5RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PA8 - Modem6 Reset - Active HIGH // 
	#define M6RESET_PIN                         GPIO_Pin_8
	#define M6RESET_GPIO_PORT                   GPIOA
	#define M6RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PA15 - Bat Stat 2 - Input //
	#if(0)
		#define BATSTAT2_PIN                        GPIO_Pin_15
		#define BATSTAT2_GPIO_PORT                  GPIOA
	#elif(STAT2_AS_FJEEPROM_MISO == TRUE)
		#define FJEEPROM_MISO_PORT			GPIOA
		#define FJEEPROM_MISO_PIN				GPIO_Pin_15
	#endif
	// PC13 - Omap RESWRON - Active HIGH // 
	#define OMAP_WRON_PIN                       GPIO_Pin_13
	#define OMAP_WRON_GPIO_PORT                 GPIOC
	#if(TRUE == DONT_RESET_OMAP)
		#define OMAP_WRON_DEFVAL									  Bit_RESET	// ENABLE State // 
	#else
		#define OMAP_WRON_DEFVAL									  Bit_SET	// DISABLE State // 
	#endif
	// PC14 - +3.3V_ENC enable - Active HIGH // 
	#define ENC3v3EN_PIN                    	  GPIO_Pin_14
	#define ENC3v3EN_GPIO_PORT            	    GPIOC
	#define ENC3v3EN_DEFVAL						   			  Bit_RESET	// RESET State, 3.3V_ENC disabled by default // 

	// PC15 - LCDPOWER enable - Active HIGH // 
	#define LCDPOWER_PIN                    	  GPIO_Pin_15
	#define LCDPOWER_GPIO_PORT            	    GPIOC
	#define LCDPOWER_DEFVAL						   			  Bit_SET	// Set State, LCD POWER DISABLED by default // 

	// PF6 - Modem1 Reset - Active HIGH // 
	#define M1RESET_PIN                         GPIO_Pin_6
	#define M1RESET_GPIO_PORT                   GPIOF
	#define M1RESET_DEFVAL											Bit_SET	// RESET State : Disabled //

	// PF7 - Modem Hub RESET - Active HIGH // 
	#define MODEMHUBRESET_PIN                   GPIO_Pin_7
	#define MODEMHUBRESET_GPIO_PORT            	GPIOF
	#define MODEMHUBRESET_DEFVAL						   	Bit_RESET	// RESET State, ModemHUB is at RESET state @initialization 

	// PB1 - TP21 - SDI-PATH Board MUTE Control - Active LOW // 
	#define SDIPATCH_MUTE_PIN                  	  GPIO_Pin_1
	#define SDIPATCH_MUTE_PORT    	        	    GPIOB
	#define SDIPATCH_MUTE_STATE										Bit_RESET
	#define SDIPATCH_UNMUTE_STATE									Bit_SET
	#define SDIPATCH_MUTE_DEFVAL				   			  SDIPATCH_UNMUTE_STATE		// Disable MUTE, Sound output is OK // 

	// PB2 - +3.3V_USB ETH Power enable - Active HIGH // 
	#define USBETH3v3EN_PIN                    	  GPIO_Pin_2
	#define USBETH3v3EN_GPIO_PORT            	    GPIOB
	 #define USBETH3v3EN_DEFVAL				 	   			  Bit_RESET		// Disable USB->ETH Power // 
	//#define USBETH3v3EN_DEFVAL				 	   			  Bit_SET		// ENABLE USB->ETH Power // 

	// PB3 - Bat Stat 1 - Input //
	#define BATSTAT1_PIN                        GPIO_Pin_3
	#define BATSTAT1_GPIO_PORT                  GPIOB
	#if(STAT1_AS_BUTTON_INPUT	== TRUE)
		#define BUTTON_READ_PIN		BATSTAT1_PIN
		#define BUTTON_READ_PORT	BATSTAT1_GPIO_PORT
	#elif(STAT1_AS_FJEEPROM_CS == TRUE)
		#define FJEEPROM_CS_PORT			GPIOB
		#define FJEEPROM_CS_PIN				GPIO_Pin_3
		#define FJEEPROM_CS_DEFVAL		Bit_SET
	#endif

	#if(0)
		// PB6 - Bat PG - Input //
		#define BATPG_PIN                        GPIO_Pin_6
		#define BATPG_GPIO_PORT                  GPIOB
	#elif(PG_AS_FJEEPROM_MOSI == TRUE)
		#define FJEEPROM_MOSI_PIN				GPIO_Pin_6
		#define FJEEPROM_MOSI_PORT			GPIOB
		#define FJEEPROM_MOSI_DEFVAL		Bit_RESET
	#endif

	#define ACODEC_RESET_PIN                        GPIO_Pin_4
	#define ACODEC_RESET_GPIO_PORT                  GPIOA
	#define ACODEC_RESET_DEFVAL									Bit_SET	// Active LOW, disabled by default // 

	// PB7 - Omap RESWARM - Active HIGH // 
	#define OMAP_WARM_PIN                       GPIO_Pin_7
	#define OMAP_WARM_GPIO_PORT                 GPIOB
	#if(TRUE == DONT_RESET_OMAP)
		#define OMAP_WARM_DEFVAL									  Bit_RESET	// ENABLE State // 
	#else
		#define OMAP_WARM_DEFVAL									  Bit_SET	// DISABLE State // 
	#endif

	// PB8 - +3.3V_MAIN Power enable - Active HIGH // 
	#define ENC3v3MAIN_PIN                    	  GPIO_Pin_8
	#define ENC3v3MAIN_GPIO_PORT            	    GPIOB
	#define ENC3v3MAIN_DEFVAL						   			  Bit_SET	// SET State, 3v3 MAIN is D by Default //

	// PB12 - USB TO ETHERNET CHIP RESET - Active LOW //
	#define RESETUSBETH_PIN                    	  GPIO_Pin_12
	#define RESETUSBETH_GPIO_PORT            	    GPIOB
	#define RESETUSBETH_DEFVAL				 	   			  Bit_RESET		// RESET USB -> ETH : Disabled // 
	//#define RESETUSBETH_DEFVAL				 	   			  Bit_SET		// SET USB -> ETH : ENABLED // 

	// PB13 - OTG HUB CHIP RESET - Active LOW //
	#define RESETOTGHUB_PIN                    	  GPIO_Pin_13
	#define RESETOTGHUB_GPIO_PORT            	    GPIOB
	#define RESETOTGHUB_DEFVAL				 	   			  Bit_SET	// SET State, OTG HUB is set ENABLED by Default //

	// PB14 - RESET MODEM MODULES - Active HIGH //
	#define RESETMODULES_PIN                    	  GPIO_Pin_14
	#define RESETMODULES_GPIO_PORT            	    GPIOB
	#define RESETMODULES_DEFVAL					 	   			  Bit_SET	// Set State, MODEM MODULES are RESET OPERATION by Default //

	// PB15 - I2C_EXPANDER INT - INPUT //
	#define I2CEXPINT_PIN                    	  GPIO_Pin_15
	#define I2CEXPINT_GPIO_PORT            	    GPIOB

	// PF0 - SUSPEND FPGA - Active HIGH //
	#define SUSPENDFPGA_PIN                    	  GPIO_Pin_0
	#define SUSPENDFPGA_GPIO_PORT            	    GPIOF
	#define SUSPENDFPGA_DEFVAL				 	   			  Bit_RESET	// RESet State, FPGA NORMAL OPERATION by Default //
	//#define SUSPENDFPGA_DEFVAL				 	   			  Bit_SET	// SET State, FPGA SUSPENDED until we release @later stages //

//------------------------------------------------------------------------------//
//------------------------------------------------------------------------------//
#elif(MAIN_HW_TYPE	==	HW_3GBONDING_MB)
//------------------------------------------------------------------------------//
//------------------------------------------------------------------------------//

	/* GPIO Definitions */
	// PA0 - NA 
	// PA1 - NA
	// PA2 - USART1-TX
	// PA3 - USART1-RX
	// PA4 - TP2
	// PA5 - NA
	// PA6 - Disable WIFI
	// PA7 - NA
	// PA8 - M4_ENABLE (ACTIVE HIGH)
	// PA9 - M6_ENABLE (ACTIVE HIGH)
	// PA10 - RESET_3G HUB (ACTIVE LOW)
	// PA11 - +5V_ENC_ENABLE (ACTIVE LOW)
	// PA12 - SUSPEND FPGA (ACTIVE HIGH)
	// PA13 - NA
	// PA14 - NA
	// PA15 - BAT STAT2 
			
	// PF0 - TP_OSC
	// PF1 - NA
	// PF6 - ENC_RESET (ACTIVE_LOW)
	// PF7 - RESET_OTG_HUB (ACTIVE_LOW)
	
	// PA6 - WIFI_POWER_ENABLE (ACTIVE HIGH) // 
	#define WIFI_POWER_PIN                        GPIO_Pin_6
	#define WIFI_POWER_GPIO_PORT	                GPIOA
	#define WIFI_POWER_DEFVAL			  							Bit_SET	// Reset State : ENABLED // 

	// PA8 - M4 RESET & ENABLE (ACTIVE HIGH) // 
	#define M4RESET_PIN                         GPIO_Pin_8
	#define M4RESET_GPIO_PORT                   GPIOA
	#define M4RESET_DEFVAL											Bit_RESET	// Reset State : Disabled // 

	// PA9 - M6 RESET & ENABLE (ACTIVE HIGH) // 
	#define M6RESET_PIN                         GPIO_Pin_9
	#define M6RESET_GPIO_PORT                   GPIOA
	#define M6RESET_DEFVAL											Bit_RESET	// Reset State : Disabled // 

	// PA10 - Modem Hub RESET - Active LOW // 
	#define MODEMHUBRESET_PIN                   GPIO_Pin_10
	#define MODEMHUBRESET_GPIO_PORT            	GPIOA
	#define MODEMHUBRESET_DEFVAL						   	Bit_RESET	// RESET State, ModemHUB is at RESET state @initialization 

	// PA12 - SUSPEND FPGA - Active HIGH //
	#define SUSPENDFPGA_PIN                    	  GPIO_Pin_12
	#define SUSPENDFPGA_GPIO_PORT            	    GPIOA
	#define SUSPENDFPGA_DEFVAL				 	   			  Bit_RESET	// RESet State, FPGA NORMAL OPERATION by Default //
	//#define SUSPENDFPGA_DEFVAL				 	   			  Bit_SET	// SET State, FPGA SUSPENDED until we release @later stages //

	/*******************************************************************************/
	/*************************			PORTB		****************************************/
	/*******************************************************************************/
	// PB0 - VIN
	// PB1 - OMAP_HB
	// PB2 - OMAP_RESPON
	// PB3 - TP6
	// PB4 - LCD_PWM
	// PB5 - FAN PWM
	// PB6 - NA
	// PB7 - LCD_BCKLIGHT_DISABLE
	// PB8 - NA
	// PB9 - FAN SENSE
	// PB10 - OMAP_RESPARM
	// PB11 - M1_ENABLE (ACTIVE LOW, INTERNAL MODEM)
	// PB12 - M2_ENABLE (ACTIVE LOW, INTERNAL MODEM)
	// PB13 - M3_ENABLE (ACTIVE LOW, INTERNAL MODEM)
	// PB14 - RESET_3G_MODULES
	// PB15 - M5_ENABLE (ACTIVE HIGH)

	// PB2 - Omap RESWRON - Active HIGH // 
	#define OMAP_WRON_PIN                       GPIO_Pin_2
	#define OMAP_WRON_GPIO_PORT                 GPIOB
	#if(TRUE == DONT_RESET_OMAP)
		#define OMAP_WRON_DEFVAL									  Bit_RESET	// ENABLE State // 
	#else
		#define OMAP_WRON_DEFVAL									  Bit_SET	// DISABLE State // 
	#endif
	
	// PB7 - LCD_BCKLGHT enable - Active HIGH // 
	#define LCD_BCKLGHT_PIN                    	  GPIO_Pin_7
	#define LCD_BCKLGHT_GPIO_PORT            	    GPIOB
	#define LCD_BCKLGHT_DEFVAL					   			  Bit_RESET	// RESet State, LCD POWER DISABLED by default // 

	// PB9 - FAN_SENSE - INPUT // 
	#define FAN_SENSE_PIN                    	  GPIO_Pin_9
	#define FAN_SENSE_GPIO_PORT            	    GPIOB

	// PB10 - Omap RESWARM - Active HIGH // 
	#define OMAP_WARM_PIN                       GPIO_Pin_10
	#define OMAP_WARM_GPIO_PORT                 GPIOB
	#if(TRUE == DONT_RESET_OMAP)
		#define OMAP_WARM_DEFVAL									  Bit_RESET	// ENABLE State // 
	#else
		#define OMAP_WARM_DEFVAL									  Bit_SET	// DISABLE State // 
	#endif

	// PB11 - M1 RESET & ENABLE (ACTIVE LOW) // 
	#define M1RESET_PIN                         GPIO_Pin_11
	#define M1RESET_GPIO_PORT                   GPIOB
	#define M1RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PB12 - M2 RESET & ENABLE (ACTIVE LOW) // 
	#define M2RESET_PIN                         GPIO_Pin_12
	#define M2RESET_GPIO_PORT                   GPIOB
	#define M2RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PB13 - M3 RESET & ENABLE (ACTIVE LOW) // 
	#define M3RESET_PIN                         GPIO_Pin_13
	#define M3RESET_GPIO_PORT                   GPIOB
	#define M3RESET_DEFVAL											Bit_SET	// Reset State : Disabled // 

	// PB14 - RESET MODEM MODULES - Active HIGH //
	#define RESETMODULES_PIN                    	  GPIO_Pin_14
	#define RESETMODULES_GPIO_PORT            	    GPIOB
	#define RESETMODULES_DEFVAL					 	   			  Bit_SET	// Set State, MODEM MODULES are RESET OPERATION by Default //

	// PB15 - M5 RESET & ENABLE (ACTIVE HIGH) // 
	#define M5RESET_PIN                         GPIO_Pin_15
	#define M5RESET_GPIO_PORT                   GPIOB
	#define M5RESET_DEFVAL											Bit_RESET	// Reset State : Disabled // 

	/*******************************************************************************/
	/*************************			PORTC		****************************************/
	/*******************************************************************************/
	// PC13 - LCD_POFF	(ACTIVE_HIGH)
	// PC14 - LCD_5V_EN	(ACTIVE_HIGH)
	// PC15 - 3v3_MAIN_EN	(ACTIVE_LOW)
	
	// PC13 - LCDPOWER enable - Active HIGH // 
	#define LCDPOWER_PIN                    	  GPIO_Pin_13
	#define LCDPOWER_GPIO_PORT            	    GPIOC
	#define LCDPOWER_DEFVAL						   			  Bit_SET	// Set State, LCD POWER DISABLED by default // 

	// PC14 - ENC_5V Power enable - Active LOW // 
	#define ENC5vEN_PIN                    	  GPIO_Pin_14
	#define ENC5vEN_GPIO_PORT            	    GPIOC
	#define ENC5vEN_DEFVAL						   			  Bit_SET	// SET State, 5V ENCODER is DISABLED by Default //

	// PC15 - +3.3V_MAIN Power enable - Active LOW // 
	#define ENC3v3MAIN_PIN                    	  GPIO_Pin_15
	#define ENC3v3MAIN_GPIO_PORT            	    GPIOC
	#define ENC3v3MAIN_DEFVAL						   			  Bit_SET	// SET State, 3v3 MAIN is DISABLED by Default //

	/*******************************************************************************/
	/*************************			PORTF		****************************************/
	/*******************************************************************************/

	// PF6 - ENC_XRESET - Active LOW // 
	#define ENCRESET_PIN                    	  GPIO_Pin_6
	#define ENCRESET_GPIO_PORT            	    GPIOF
	#define ENCRESET_RESET_PIN_STATE	   			  Bit_SET	// RESET State, ENCODER is disabled by default // 
	#define ENCRESET_WORKING_PIN_STATE					Bit_RESET

	// PF7 - OTG HUB CHIP RESET - Active LOW //
	#define RESETOTGHUB_PIN                    	  GPIO_Pin_7
	#define RESETOTGHUB_GPIO_PORT            	    GPIOF
	#define RESETOTGHUB_DEFVAL				 	   			  Bit_RESET	// SET State, OTG HUB is set DISABLED by Default //

	
	
#else
	#error "UNDEFINED MAIN HW TYPE"
#endif
//-------------------------------------------//
//----- SPECIAL FUNTIONs INITIALIZATION -----//
//-------------------------------------------//

void init_onewire(void);	// one wire communication initialization // 
void init_freq_input(void);	// Fansense freq. measurement initialization //
void sysinit(void);
void AudioADC_SetSoftMode(void);


#endif

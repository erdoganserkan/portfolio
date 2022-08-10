 /**********************************************************************
* $Id$		I2s_Audio.c		2011-06-02
*//**
* @file		I2s_Audio.c
* @brief	This example describes how to use I2S to play a short demo
*			audio data on LPC177x_8x
* @version	1.0
* @date		02. June. 2011
* @author	NXP MCU SW Application Team
*
* Copyright(C) 2011, NXP Semiconductor
* All rights reserved.
*
***********************************************************************
* Software that is described herein is for illustrative purposes only
* which provides customers with programming information regarding the
* products. This software is supplied "AS IS" without any warranties.
* NXP Semiconductors assumes no responsibility or liability for the
* use of the software, conveys no license or title under any patent,
* copyright, or mask work right to the product. NXP Semiconductors
* reserves the right to make changes in the software without
* notification. NXP Semiconductors also make no representation or
* warranty that such application will be suitable for the specified
* use without further testing or modification.
* Permission to use, copy, modify, and distribute this software and its
* documentation is hereby granted, under NXP Semiconductors'
* relevant copyright in the software, without fee, provided that it
* is used in conjunction with NXP Semiconductors microcontrollers.  This
* copyright, permission, and disclaimer notice must appear in all copies of
* this code.
**********************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_i2s.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_dac.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_timer.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <event_groups.h>
#include <task.h>
#include <GLCD.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include <BSP.h>
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "UartInt.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "AppSettings.h"
#include "Dac.h"
#include "ACodec.h"
#include "APlay.h"


#define IS_RUNTIME_AUDIO_RES_EXIST(audio_indx) \
	(((NULL != SBResources[AUDIO_TRACs]) && (0 != SBResources[AUDIO_TRACs][audio_indx].hMemHandle) && (0 != SBResources[AUDIO_TRACs][audio_indx].size))?(TRUE):(FALSE)) 

#define USE_MCUSRAM_AUDIO_BUFFER	FALSE	// FALSE: Play i2s sound directly from ext-sdram //
											// TRUE: Play i2s sound by copying data from ext-sdram to int-sram // 

/************************** PRIVATE DEFINITIONS *************************/
/** Max buffer length */
#define AUDIO_BUFFER_SIZE			(1048 * 10)	// ýnternal sram temporary audio buffer // 

/************************** PRIVATE VARIABLES ***********************/
//extern unsigned char audio[];
#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
	static uint8_t tx_buffer[AUDIO_BUFFER_SIZE];
#endif
static uint32_t data_offset, buffer_offset,remain_data;
static uint32_t tx_offset = 0;

/************************** PRIVATE FUNCTIONS *************************/
void __I2S_Callback(void);
void I2S_IRQHandler(void);
static void __Buffer_Init(void);
static uint8_t __LPCDAC_Aplay_Init(void);

static volatile uint8_t __first_dma_isr = FALSE;	// Used for initial DMA->DAC transfer bug // 
volatile uint8_t aplay_state = AUDIO_TASK_NOINIT;
static uint8_t active_sound_indx = 0;
static uint8_t *active_sound_adr = NULL;
#if(AUDIO_IC_LPCDAC != USED_AUDIO_DEVICE)
	static uint32_t active_sound_size = 0;
	static uint32_t total_dma_transfer_count = 0;
#else
	static uint32_t total_dma_transfer_count = 0;
#endif
static uint32_t dmaDACCh_TermianalCnt=0, dmaDACCh_ErrorCnt=0;
static GPDMA_Channel_CFG_Type GPDMACfg;
static GPDMA_LLI_Type DMA_LLI_Struct;
static uint16_t AppVolume;


uint8_t is_audio_playing(void)
{
	return (AUDIO_TASK_PLAYING == aplay_state) ? TRUE : FALSE;
}

uint8_t APLAY_DAC_DMA_Handler(void)
{
	// check GPDMA interrupt on channel 0
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, APLAY_DAC_DMA_CHANNEL)) {
		//check interrupt status on channel 0
		// Check counter terminal status
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, APLAY_DAC_DMA_CHANNEL)) {
			// Clear terminate counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, APLAY_DAC_DMA_CHANNEL);
			dmaDACCh_TermianalCnt++;
			// DMA transfer completed for current audio file // 	
			if(AUDIO_TASK_PLAYING == aplay_state) {
				// 1- Update DMA source pointer in audio file data on SDRAM 
				// 2- Update DMA transfer size if necessary 
				// 3- If audio file play completed, Disable DMA Terminal Count INT 
				if(FALSE == __first_dma_isr) {
					total_dma_transfer_count -= GPDMACfg.TransferSize;
					active_sound_adr += (GPDMACfg.TransferSize*2);	// 16Bit wide transfers done and so address increment is twice of transfers // 
				} else {
					__first_dma_isr = FALSE;
					GPDMACfg.TransferSize = (total_dma_transfer_count >= APLAY_DMA_SIZE) ? (APLAY_DMA_SIZE): (total_dma_transfer_count);
				}	
				GPDMACfg.SrcMemAddr = DMA_LLI_Struct.SrcAddr = (uint32_t)active_sound_adr;	

				if(0 != total_dma_transfer_count) {	// There is still data to SEND // 
					if(total_dma_transfer_count < GPDMACfg.TransferSize) {
						GPDMACfg.TransferSize = total_dma_transfer_count;
					}

					DMA_LLI_Struct.Control= GPDMACfg.TransferSize
												| (1<<18) // source width 16 bit ; DAC is 10bit with lefth justified in 16bit register //
												| (1<<21) // dest. width 16 bit ; audio files all have 16bit unsigned int samples // 
												| (1<<26) // source increment //
												| GPDMA_DMACCxControl_I;	// Enable terminal count INT
					GPDMA_Setup(&GPDMACfg);

				} else {	// There is NO DATA to send // 
					// DMA_LLI_Struct.Control &= (~GPDMA_DMACCxControl_I); // DISABLE terminal count INT, AGAIN 
					// GPDMA_Setup(&GPDMACfg);
					GPDMA_ChannelCmd(APLAY_DAC_DMA_CHANNEL, DISABLE);
					NVIC_DisableIRQ(DMA_IRQn);
					aplay_state = AUDIO_TASK_IDLE;
				}
			}
		}
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, APLAY_DAC_DMA_CHANNEL)) {
			// Clear error counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, APLAY_DAC_DMA_CHANNEL);
			dmaDACCh_ErrorCnt++;
		}

		return 1;
	}
	return  0;
}

inline uint8_t isDacAudioActive(void) {
	return (AUDIO_TASK_PLAYING == aplay_state)?(TRUE):(FALSE);
}
void start_dac_audio(uint8_t audio_indx, uint8_t blocking) 
{
	uint32_t start_time_ms = GUI_X_GetTime();
	volatile uint16_t indx;

	// If audio is MUTED return immediately // 
	if((APP_IS_DETECTOR == APP_GetValue(ST_DEV_TYPE)) && (VOLUME_MIN == APP_GetValue(ST_VOL)))
		return;
	else if((APP_IS_FIELD_SCANNER == APP_GetValue(ST_DEV_TYPE)) && (VOLUME_MIN == APP_GetValue(ST_AT_VOL)))
		return;

	// wait until previous audio playing completes // 
	while((AUDIO_TASK_PLAYING == aplay_state) && (APLAY_WAIT_FOR_PREV_MAX_MS > (GUI_X_GetTime() - start_time_ms))){
		App_waitMS(10);
	}	
	
	if(IS_RUNTIME_AUDIO_RES_EXIST(audio_indx)) {
		active_sound_indx = audio_indx;
		active_sound_adr = SBResources[AUDIO_TRACs][audio_indx].SDRAM_adr;
		total_dma_transfer_count = (SBResources[AUDIO_TRACs][audio_indx].size)/2;	// DMA transfers are 16bit wide, and totally there are half of file size // 			

		// 1- Set DMA source to the location of audio file in SDRAM 
		// 2- Set DMA transfer size to minimum value to recover DMA->DAC first transfer bug //  
		// 3- Start DMA & DAC 
		DMA_LLI_Struct.SrcAddr = GPDMACfg.SrcMemAddr = (uint32_t)active_sound_adr;
		__first_dma_isr = TRUE;
		GPDMACfg.TransferSize = 1;
		DMA_LLI_Struct.Control= GPDMACfg.TransferSize
								| (1<<18) // source width 16 bit ; DAC is 10bit with lefth justified in 16bit register //
								| (1<<21) // dest. width 16 bit ; audio files all have 16bit unsigned int samples // 
								| (1<<26) // source increment //
								| GPDMA_DMACCxControl_I;	// Enable terminal count INT

		GPDMA_Setup(&GPDMACfg);
		aplay_state = AUDIO_TASK_PLAYING;
		GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, APLAY_DAC_DMA_CHANNEL);	// Clear previous false interrupts // 
		GPDMA_ChannelCmd(APLAY_DAC_DMA_CHANNEL, ENABLE);
		// 4- Enable DMA INT 
		NVIC_EnableIRQ(DMA_IRQn);

		if(TRUE == blocking) {
			while(AUDIO_TASK_PLAYING == aplay_state)	// Wait until audio play completed // 
				App_waitMS(10);
		}
	}
}

void stop_dac_audio(void)
{
	// 1- Stop DMA channel 
	GPDMA_ChannelCmd(APLAY_DAC_DMA_CHANNEL, DISABLE);
	// 2- Disable DMA Interrupt 
	NVIC_DisableIRQ(DMA_IRQn);
	// 3- Clear resources 
	aplay_state = AUDIO_TASK_IDLE;
}

uint8_t audio_get_state(void)
{
	return aplay_state;
}

static uint8_t __LPCDAC_Aplay_Init(void)
{
	uint8_t ret = FALSE;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;

	// Set DAC & DMA with new settings (16bit transfer, all file in one transfer)
	
	init_TPs(4);
	AppVolume = APP_GetValue(ST_VOL);
		
	//Prepare DMA link list item structure
	DMA_LLI_Struct.SrcAddr= (uint32_t)(SBResources[AUDIO_TRACs][0].SDRAM_adr);
	DMA_LLI_Struct.DstAddr= (uint32_t)(&(LPC_DAC->CR));		// 16 Bit transfer to MSB of DAC // 
	DMA_LLI_Struct.NextLLI= (uint32_t)&DMA_LLI_Struct;
	DMA_LLI_Struct.Control= APLAY_DMA_SIZE
								| (1<<18) // source width 16 bit ; DAC is 10bit with lefth justified in 16bit register //
								| (1<<21) // dest. width 16 bit ; audio files all have 16bit unsigned int samples // 
								| (1<<26) // source increment //
								| GPDMA_DMACCxControl_I;	// Enable terminal count INT
	/* GPDMA block section -------------------------------------------- */
	/* Initialize GPDMA controller */
	GPDMA_Init();
	
	// Setup GPDMA channel --------------------------------
	// channel 0
	GPDMACfg.ChannelNum = APLAY_DAC_DMA_CHANNEL;
	
	// Source memory ; this will be changed when each audiio file playing request // 
	GPDMACfg.SrcMemAddr = (uint32_t)SBResources[AUDIO_TRACs][0].SDRAM_adr;
	
	// Destination memory - unused
	GPDMACfg.DstMemAddr = 0;
	
	// Transfer size
	GPDMACfg.TransferSize = APLAY_DMA_SIZE;

	// Transfer width - unused
	GPDMACfg.TransferWidth = 0;

	// Transfer type
	GPDMACfg.TransferType = GPDMA_TRANSFERTYPE_M2P;

	// Source connection - unused
	GPDMACfg.SrcConn = 0;

	// Destination connection
	GPDMACfg.DstConn = GPDMA_CONN_DAC;

	// Linker List Item - unused
	GPDMACfg.DMALLI = (uint32_t)&DMA_LLI_Struct;
	
	// Setup channel with given parameter
	GPDMA_Setup(&GPDMACfg);
	/* Disble GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);
	
	DAC_Init(0);
	
	// set time out for DAC, sine Wave frequency // 
	DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);

	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_ConfigDAConverterControl(0, &DAC_ConverterConfigStruct);
	
	return ret;
}   

void audio_play_init(void)
{
	#if(AUDIO_IC_LPCDAC != USED_AUDIO_DEVICE)
		/* 		This code will set I2S in MASTER mode, 44100Hz, Mono,16bit
		* 		This example is used to test I2S transmit mode. There is an audio
		* 		data in audiodata.c in mono 32000Hz 16 bit, it will be send out to
		* 		I2S port. User must plug Ext I2S DAC IC to hear sound.
		*/
		I2S_MODEConf_Type I2S_ClkConfig;
		I2S_CFG_Type I2S_ConfigStruct;
		volatile uint32_t indx;

	    /* Initialize I2S peripheral ------------------------------------*/
		/* Pin configuration:
		 * Assign:	- P0.7 as I2STX_CLK
		 * 			- P0.8 as I2STX_WS
		 * 			- P0.9 as I2STX_SDA
		 *          - P1.16 as I2SMCLK
		 */
		#if((UMD_DETECTOR_BOARD == USED_HW) || (POWERMCU_DEV_BOARD == USED_HW))
			PINSEL_ConfigPin(0,7,1);
			PINSEL_ConfigPin(0,8,1);
			PINSEL_ConfigPin(0,9,1);
			PINSEL_ConfigPin(1,16,2);
		#endif

		I2S_Init(LPC_I2S);

		/* setup:
		 * 		- wordwidth: 16 bits
		 * 		- mono mode
		 * 		- master mode for I2S_TX
		 * 		- Frequency = 44.1 kHz
		 */

		/* Audio Config*/
		I2S_ConfigStruct.wordwidth = I2S_WORDWIDTH_16;
		I2S_ConfigStruct.mono = I2S_MONO;
		I2S_ConfigStruct.stop = I2S_STOP_ENABLE;
		I2S_ConfigStruct.reset = I2S_RESET_ENABLE;
		I2S_ConfigStruct.ws_sel = I2S_MASTER_MODE;
		I2S_ConfigStruct.mute = I2S_MUTE_DISABLE;
		I2S_Config(LPC_I2S,I2S_TX_MODE,&I2S_ConfigStruct);

		/* Clock Mode Config*/
		I2S_ClkConfig.clksel = I2S_CLKSEL_FRDCLK;
		I2S_ClkConfig.fpin = I2S_4PIN_DISABLE;
		I2S_ClkConfig.mcena = I2S_MCLK_ENABLE;
		I2S_ModeConfig(LPC_I2S,&I2S_ClkConfig,I2S_TX_MODE);

		I2S_FreqConfig(LPC_I2S, AUDIO_SAMPLE_FREQ_HZ, I2S_TX_MODE);

		I2S_Stop(LPC_I2S, I2S_TX_MODE);

		/* TX FIFO depth is 4 */
		I2S_IRQConfig(LPC_I2S,I2S_TX_MODE,4);
		I2S_IRQCmd(LPC_I2S,I2S_TX_MODE,ENABLE);

		for(indx = 0; indx <0x1000000; indx++);
	#endif

	#if(AUDIO_IC_UDA1380 == USED_AUDIO_DEVICE)
		Uda1380_Init(AUDIO_CODEC_I2C_FREQ_HZ, AUDIO_SAMPLE_FREQ_HZ);
	#endif
	#if(AUDIO_IC_LM49450 == USED_AUDIO_DEVICE)
		LM49450_Init(AUDIO_CODEC_I2C_FREQ_HZ, AUDIO_SAMPLE_FREQ_HZ);
	#endif
	#if(AUDIO_IC_LPCDAC == USED_AUDIO_DEVICE)
		__LPCDAC_Aplay_Init();
	#endif
	
	aplay_state = AUDIO_TASK_IDLE;
}

#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
	/*********************************************************************//**
	 * @brief		I2S callback function, will be call when I2S send a half of
					buffer complete
	 * @param[in]	None
	 * @return 		None
	 **********************************************************************/
	static void __I2S_Callback(void)
	{
		if(remain_data >=AUDIO_BUFFER_SIZE/2) {
			if(buffer_offset == AUDIO_BUFFER_SIZE) {
				// copy audio data into remain half of tx_buffer
				memcpy(tx_buffer + AUDIO_BUFFER_SIZE/2, active_sound_adr + data_offset, AUDIO_BUFFER_SIZE/2);
				buffer_offset = 0;
			} else {
				// copy audio data into remain half of tx_buffer
				memcpy(tx_buffer, active_sound_adr + data_offset, AUDIO_BUFFER_SIZE/2);
			}
			data_offset += AUDIO_BUFFER_SIZE/2;
			remain_data -= AUDIO_BUFFER_SIZE/2;
		}
		else { // reach the last copy
			if(buffer_offset == AUDIO_BUFFER_SIZE) {
				// copy audio data into remain half of tx_buffer
				memcpy(tx_buffer + AUDIO_BUFFER_SIZE/2, active_sound_adr + data_offset, remain_data);
				buffer_offset = 0;
			} else {
				// copy audio data into remain half of tx_buffer
				memcpy(tx_buffer, active_sound_adr + data_offset, remain_data);
			}
		}
	}
#endif

#if(AUDIO_IC_LPCDAC != USED_AUDIO_DEVICE)
/*----------------- INTERRUPT SERVICE ROUTINES --------------------------*/
/*********************************************************************//**
 * @brief		I2S IRQ Handler, call to send data to transmit buffer
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void I2S_IRQHandler()
{
	uint32_t txlevel,i;
	txlevel = I2S_GetLevel(LPC_I2S,I2S_TX_MODE);
	if(txlevel <= 4) {
		for(i=0;i<8-txlevel;i++) {
			#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
				LPC_I2S->TXFIFO = *(uint32_t *)(tx_buffer + buffer_offset);
			#else
				LPC_I2S->TXFIFO = *(uint32_t *)(active_sound_adr + tx_offset);
			#endif
			tx_offset +=4;
			#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
				buffer_offset +=4;
				//call __I2S_Callback() function when reach each half of tx_buffer
				if((buffer_offset == AUDIO_BUFFER_SIZE/2)||(buffer_offset == AUDIO_BUFFER_SIZE))
					__I2S_Callback();
			#endif	
			if(tx_offset >= active_sound_size) { //stop audio playing if it is reached to the end of buffer
				stop_i2s_audio();
			}
		}
	}
}

#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
	/*********************************************************************//**
	 * @brief		Initialize transmit buffer
	 * @param[in]	none
	 * @return 		None
	 **********************************************************************/
	static void __Buffer_Init(void)
	{
		memcpy(tx_buffer, active_sound_adr, AUDIO_BUFFER_SIZE);
		buffer_offset = 0;
		data_offset = AUDIO_BUFFER_SIZE;
		remain_data = active_sound_size - AUDIO_BUFFER_SIZE;
	}
#endif

void stop_i2s_audio(void) 
{
	NVIC_DisableIRQ(I2S_IRQn);
	aplay_state = AUDIO_TASK_IDLE;
	I2S_Stop(LPC_I2S, I2S_TX_MODE);
	tx_offset = 0;
}

void start_i2s_audio(uint8_t audio_indx, uint8_t blocking) 
{	// Start audio playing only if it is initializaed into SDRAM // 
	if(IS_RUNTIME_AUDIO_RES_EXIST(audio_indx)) {
		active_sound_indx = audio_indx;
		active_sound_adr = SBResources[AUDIO_TRACs][audio_indx].SDRAM_adr;
		active_sound_size = SBResources[AUDIO_TRACs][audio_indx].size;
		#if(TRUE == USE_MCUSRAM_AUDIO_BUFFER)
			__Buffer_Init();
		#endif
		I2S_Start(LPC_I2S);
		aplay_state = AUDIO_TASK_PLAYING;
		NVIC_EnableIRQ(I2S_IRQn);
		if(TRUE == blocking) {
			while(AUDIO_TASK_PLAYING == aplay_state)	// Wait until audio play completed // 
				App_waitMS(1);
		}
	}
}
#endif

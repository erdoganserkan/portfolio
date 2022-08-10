/**********************************************************************
* $Id$		Dac_SineWave.c	2011-06-02
*//**
* @file		Dac_SineWave.c
* @brief	This example describes how to use DAC to generate a sine wave
* 			using DMA to transfer data
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
#include <math.h>
#include "lpc_types.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_dac.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_timer.h"

/* Example group ----------------------------------------------------------- */
/** @defgroup DAC_SineWave	DAC SineWave
 * @ingroup DAC_Examples
 * @{
 */

#define _ONE_POSITIVE_HALF		(0)
#define _DMA_USING						(0)


/************************** PRIVATE MACROS *************************/
#define SINEWAVE_DEF_AMPLITUDE			220

//Thi minimum value is as amplitude to make all the signed not to be less than 0 (zero)
#define SINEWAVE_DC_OFFSET				SINEWAVE_DEF_AMPLITUDE

#if _ONE_POSITIVE_HALF
	#define NUM_SINE_SAMPLE	30
	#define SINE_FREQ_IN_HZ	100
#else
	#define NUM_SINE_SAMPLE	60
	#define SINE_FREQ_IN_HZ	1200
#endif

#define MODULATOR_WAVE_SAMPLES		(16)
#ifndef M_PI
	#define M_PI	3.141592653589
#endif

/** DMA size of transfer */
#define DMA_SIZE		NUM_SINE_SAMPLE
#define WAVE_AMPLITUDE_UPDATE_TMR				(LPC_TIM2)
#define WAVE_AMPLITUDE_UPDATE_TMR_INT		(TIMER2_IRQn)

static void __WAVE_Update_Amp(uint32_t new_amp, uint8_t lut_indx);

typedef enum 
{
	DAC_SINE_WAVE	= 0,	// Modulator for GOLD(Special NonFerro) targets // 
	DAC_TRIANGLE_WAVE,		// Modulator for METAL (Ferro, NonFerro) targets // 
	DAC_SAWTOOTH_WAVE,		// Modulator for 	MINERAL and UNKNOWN targets // 	
	DAC_SQUARE_WAVE,		// Modulator for CAVITY targets // 
	
	DAC_MODULATOR_TYPEs_COUNT
} eSineModulatorType;

/************************** PRIVATE VARIABLES *************************/
GPDMA_Channel_CFG_Type GPDMACfg;

uint32_t amp_sine[MODULATOR_WAVE_SAMPLES];
uint32_t amp_square[MODULATOR_WAVE_SAMPLES];
uint32_t amp_triangle[MODULATOR_WAVE_SAMPLES];
uint32_t amp_saw[MODULATOR_WAVE_SAMPLES];
uint8_t amp_indx = 0;
uint8_t amp_type = DAC_TRIANGLE_WAVE;
uint8_t active_lut = 0;
const uint32_t *amp_tables[DAC_MODULATOR_TYPEs_COUNT] = 
{
	amp_sine,
	amp_triangle,
	amp_saw,
	amp_square
};

#define WAVE_AMPLITUDE_UPDATE_TMR_IRQ_HANDLER	TIMER2_IRQHandler
static DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;
static GPDMA_LLI_Type DMA_LLI_Struct;
static uint32_t dac_sine_lut[NUM_SINE_SAMPLE], dac_sine_lut2[NUM_SINE_SAMPLE];
static uint32_t sin_0_to_90_16_samples[16] = 
{
	0,		358,	785,	1135,
	1543,	1933,	2243,	2592,
	2912,	3155,	3414,	3602,
	3791,	3939,	4028,	4096
};
static TIM_TIMERCFG_Type TIM_ConfigStruct;
static TIM_MATCHCFG_Type TIM_MatchConfigStruct;

void WAVE_AMPLITUDE_UPDATE_TMR_IRQ_HANDLER(void)
{
	if (TIM_GetIntStatus(WAVE_AMPLITUDE_UPDATE_TMR, TIM_MR0_INT)== SET) {	
		TIM_ClearIntPending(WAVE_AMPLITUDE_UPDATE_TMR, TIM_MR0_INT);
		// Apply new aplitude to base sine wave // 
		active_lut ^= 1;
		__WAVE_Update_Amp(amp_tables[amp_type][amp_indx], active_lut);
		// Invoke DMA to use new updated dac lut // 
		DMA_LLI_Struct.SrcAddr= (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));
		GPDMACfg.SrcMemAddr = (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));
		//GPDMA_ChannelCmd(0, DISABLE);	// Dont disable DMA, this makes DAC output out of control // 
		GPDMA_Setup(&GPDMACfg);
		GPDMA_ChannelCmd(0, ENABLE);
		if(++amp_indx == MODULATOR_WAVE_SAMPLES)
			amp_indx = 0;		
	}
}


void WAVE_init_modulator_luts(void)
{
	volatile uint32_t cnt;
	// Calculate sine values //
	for(cnt = 0; cnt < MODULATOR_WAVE_SAMPLES; cnt++) {
		double angle = (((180*cnt)/MODULATOR_WAVE_SAMPLES)*M_PI)/180.0;
		double res = sin(angle);
		amp_sine[cnt] = res * 512;	
	}
	// Calculate saw values //
	for(cnt = 0; cnt < (MODULATOR_WAVE_SAMPLES-1); cnt++) {
		double slope = 512.0/(MODULATOR_WAVE_SAMPLES-1);
		amp_saw[cnt] = slope * cnt;
	}
	amp_saw[MODULATOR_WAVE_SAMPLES-1] = 512;
	
	// Calculate triangle values //
	for(cnt = 0; cnt <= MODULATOR_WAVE_SAMPLES/2; cnt++) {
		double slope = 512.0/(MODULATOR_WAVE_SAMPLES/2);
		amp_triangle[cnt] = amp_triangle[MODULATOR_WAVE_SAMPLES-cnt] = slope * cnt;
	}
	// Calculate triangle values //
	for(cnt = 0; cnt < MODULATOR_WAVE_SAMPLES; cnt++) {
		if(cnt <= (MODULATOR_WAVE_SAMPLES/2))
			amp_square[cnt] = 512;
		else
			amp_square[cnt] = 0;
	}
}

void WAVE_Update_Modulator_Freq(uint32_t new_freq_hz) 
{
	new_freq_hz *= MODULATOR_WAVE_SAMPLES;
	TIM_MatchConfigStruct.MatchValue   = (1000000UL / new_freq_hz);
	TIM_ConfigMatch(WAVE_AMPLITUDE_UPDATE_TMR, &TIM_MatchConfigStruct);
}


void WAVE_Update_Tmr_Init(uint32_t freq_hz)
{
	// Initialize timer 0, prescale count time of 100uS
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;

	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	//Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	//Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = FALSE;
	//Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct.ExtMatchOutputType =TIM_EXTMATCH_NOTHING;
	
	freq_hz *= MODULATOR_WAVE_SAMPLES;
	// Set Match value //
	TIM_MatchConfigStruct.MatchValue   = (1000000UL / freq_hz);

	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_Init(WAVE_AMPLITUDE_UPDATE_TMR, TIM_TIMER_MODE, &TIM_ConfigStruct);
	TIM_ConfigMatch(WAVE_AMPLITUDE_UPDATE_TMR, &TIM_MatchConfigStruct);

	// IRQ Priority has been set in BSP_Init_IRQ_Priorities() function // 
	/* preemption = 1, sub-priority = 1 */
	// NVIC_SetPriority(WAVE_AMPLITUDE_UPDATE_TMR_INT, 1);
}

void WAVE_Update_Start(void)
{
	NVIC_ClearPendingIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
	/* Enable interrupt for selected timer */
	NVIC_EnableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
	// Start timer
	TIM_Cmd(WAVE_AMPLITUDE_UPDATE_TMR, ENABLE);
}

void WAVE_Update_Stop(void) 
{
	// To start timer
	TIM_Cmd(WAVE_AMPLITUDE_UPDATE_TMR, DISABLE);
	/* Enable interrupt for selected timer */
	NVIC_DisableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
}

static void __WAVE_Update_Amp(uint32_t new_amp, uint8_t lut_indx) 
{
	uint32_t *lut_p = (0 == lut_indx) ? (dac_sine_lut):(dac_sine_lut2);
	volatile uint32_t cnt;
	for(cnt = 0; cnt < NUM_SINE_SAMPLE; cnt++) {
		if(cnt <= 15) {
			lut_p[cnt] = SINEWAVE_DC_OFFSET + ((new_amp * sin_0_to_90_16_samples[cnt])>>12);
		}
		else if(cnt <= 30) {
			lut_p[cnt] =  SINEWAVE_DC_OFFSET + ((new_amp * sin_0_to_90_16_samples[30-cnt])>>12);
		}
		else if(cnt <= 45) {
			lut_p[cnt] = SINEWAVE_DC_OFFSET - ((new_amp * sin_0_to_90_16_samples[cnt-30])>>12);
		}
		else {
			lut_p[cnt] = SINEWAVE_DC_OFFSET - ((new_amp * sin_0_to_90_16_samples[60-cnt])>>12);
		}

		//To make sure the output value is not over 10-bit width of DAC
		if(lut_p[cnt] > 0x3FF)
			lut_p[cnt] = 0x3FF;

		#if _DMA_USING
			//Shift the value before DMA to DAC component in case of using DMA
			lut_p[cnt] = (lut_p[cnt] << 6);
		#endif
	}
}

/*-------------------------MAIN FUNCTION------------------------------*/
/*********************************************************************//**
 * @brief		c_entry: Main DAC program body
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void c_entry(void)
{
	uint32_t cnt, max;

#if(0)
retry:
	max = 515;
	for(cnt = 0; cnt < 16; cnt++) {
		double angle = (((90*cnt)/16)*M_PI)/180.0;
		double res = sin(angle);
		sin_0_to_90_16_samples[cnt] = res * max;
	}
	goto retry;
#endif

	WAVE_init_modulator_luts();	
	
	// Clear all value
	for(cnt = 0; cnt < NUM_SINE_SAMPLE; cnt++) {
		dac_sine_lut[cnt] = dac_sine_lut2[cnt] = 0;
	}

	//Prepare DAC sine look up table
	__WAVE_Update_Amp(SINEWAVE_DEF_AMPLITUDE, active_lut);

#if _DMA_USING
	//Prepare DMA link list item structure
	DMA_LLI_Struct.SrcAddr= (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));

	DMA_LLI_Struct.DstAddr= (uint32_t)&(LPC_DAC->CR);

	DMA_LLI_Struct.NextLLI= (uint32_t)&DMA_LLI_Struct;

	DMA_LLI_Struct.Control= DMA_SIZE
								| (2<<18) //source width 32 bit
								| (2<<21) //dest. width 32 bit
								| (1<<26); //source increment

	/* GPDMA block section -------------------------------------------- */
	/* Initialize GPDMA controller */
	GPDMA_Init();

	// Setup GPDMA channel --------------------------------
	// channel 0
	GPDMACfg.ChannelNum = 0;

	// Source memory
	GPDMACfg.SrcMemAddr = (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));

	// Destination memory - unused
	GPDMACfg.DstMemAddr = 0;

	// Transfer size
	GPDMACfg.TransferSize = DMA_SIZE;

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
#endif

	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = RESET;

	DAC_Init(0);

	// set time out for DAC
	// clk = sine_freq * number_of_samples_per_sine_cycle * sample_freq 	(pulses of clock)
	// => sample_freq = clk / (sine_freq * number_of_samples_per_sine_cycle)

	cnt = CLKPWR_GetCLK(CLKPWR_CLKTYPE_PER);
	cnt = cnt/(SINE_FREQ_IN_HZ * NUM_SINE_SAMPLE);
	DAC_SetDMATimeOut(0, cnt);

#if _DMA_USING
	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
#endif
	DAC_ConfigDAConverterControl(0, &DAC_ConverterConfigStruct);

#if _DMA_USING
	// Enable GPDMA channel 0
	GPDMA_ChannelCmd(0, ENABLE);
#else
	cnt = 0;

	while(1) {
		DAC_UpdateValue(0, dac_sine_lut[cnt]);

		while(!DAC_IsIntRequested(0));

		cnt ++;

		if(cnt == NUM_SINE_SAMPLE)
			cnt = 0;
	}
#endif

	WAVE_Update_Tmr_Init(50);
	WAVE_Update_Start();
	while(1);

}
/* With ARM and GHS toolsets, the entry point is main() - this will
   allow the linker to generate wrapper code to setup stacks, allocate
   heap area, and initialize and copy code and data segments. For GNU
   toolsets, the entry point is through __start() in the crt0_gnu.asm
   file, and that startup code will setup stacks and data */
int main(void)
{
	c_entry();
	return 0;
}

void __low_level_init() {}
/**
 * @}
*/

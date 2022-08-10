#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "lpc_types.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_uart.h"
#include "lpc177x_8x_dac.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_timer.h"
#include <FreeRTOS.h>
#include <queue.h>
#include <timers.h>
#include "UMDShared.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "UartInt.h"
#include "APlay.h"
#include "Dac.h"

#define _DMA_USING							(1)

/************************** PRIVATE MACROS *************************/
#define NUM_SINE_SAMPLE					(60)
//#define DEF_SINE_FREQ_IN_HZ			(60)

/** DMA size of transfer */
#define DMA_SIZE		NUM_SINE_SAMPLE
#define MODULATOR_WAVE_SAMPLES		(16)	/* Sine wave modulators: square, sine, triangle, saw signal samples */

#define DAC_WAVE_START_FREQ_HZ	300
typedef struct {
	uint16_t gaugeF;
	uint16_t freq;
} dac_gauge_freq_t;

volatile uint8_t dac_state;
static uint16_t active_dma_freq = 0;
static uint16_t AppVolume = DEFAULT_VOLUME;
static uint8_t active_search_mode = AUTOMATIC_SEARCH_TYPE;
static volatile uint16_t Gauge_PathF[PATH_MEMBER_COUNT] = {UMD_GAUGE_MIN*GAUGE_FRACTUATION};
static volatile uint8_t path_cnt = 0;
static uint32_t dmaDACCh_TermianalCnt=0, dmaDACCh_ErrorCnt=0;
static uint32_t active_dac_dma_update_freq_hz = 0;

static uint16_t permitted_max;
static 	uint16_t current_amp = 0;

static uint16_t const freqFTID[TARGET_ID_COUNT] = {
	300/*no_target*/, 
	350/*cavity*/, 
	515/*ferros*/, 
	600/*non-ferros*/,
	700/*gold*/,
	450/*mineral*/
};
static uint16_t const freqRTID[RTID_COUNT] = {
	300/*no-target*/,
	350/*cavity*/,
	650/*metal*/,
	450/*mineral*/
};

	
// SEARCH-MODE specific gauge level frequency constant (flash storage) definitions // 
// WARNING! This table must have at least two members for UMD_GAUGE_MIN and UMD_GAUGE_MAX // 
static const dac_gauge_freq_t dac_gauge_freq_lut_AUTO[] = {
	{UMD_GAUGE_MIN*GAUGE_FRACTUATION,300},
	{1*GAUGE_FRACTUATION,320},
	{2*GAUGE_FRACTUATION,325},
	{3*GAUGE_FRACTUATION,327},
	{4*GAUGE_FRACTUATION,330},
	{5*GAUGE_FRACTUATION,335},
	{UMD_GAUGE_MAX*GAUGE_FRACTUATION, 800}
};
static const dac_gauge_freq_t dac_gauge_freq_lut_OTHERs[] = {
	{UMD_GAUGE_MIN*GAUGE_FRACTUATION,300},
	{1*GAUGE_FRACTUATION,320},
	{2*GAUGE_FRACTUATION,325},
	{3*GAUGE_FRACTUATION,327},
	{4*GAUGE_FRACTUATION,330},
	{5*GAUGE_FRACTUATION,335},
	{UMD_GAUGE_MAX*GAUGE_FRACTUATION, 600}
};

// Common RAM tables for Gauge level frequencies // 
static dac_gauge_freq_t dac_gauge_freq_lut[] = {
	{UMD_GAUGE_MIN*GAUGE_FRACTUATION,300},
	{1*GAUGE_FRACTUATION,320},
	{2*GAUGE_FRACTUATION,325},
	{3*GAUGE_FRACTUATION,327},
	{4*GAUGE_FRACTUATION,330},
	{5*GAUGE_FRACTUATION,335},
	{UMD_GAUGE_MAX*GAUGE_FRACTUATION, 800}
};

/************************** PRIVATE VARIABLES *************************/
uint16_t dac_sine_lut[NUM_SINE_SAMPLE];
static uint16_t dac_sine_lut2[NUM_SINE_SAMPLE];

static uint32_t amp_sine[MODULATOR_WAVE_SAMPLES];
static uint32_t amp_square[MODULATOR_WAVE_SAMPLES];
static uint32_t amp_triangle[MODULATOR_WAVE_SAMPLES];
static uint32_t amp_saw[MODULATOR_WAVE_SAMPLES];
static uint8_t amp_indx = 0;
static uint8_t amp_type = DAC_SINE_WAVE;
static volatile uint8_t active_lut = 0;
static volatile uint8_t change_lut = FALSE;
#if(12 == BASE_SINE_PEAK_BITs)
	const uint16_t sin_0_to_90_16_samples[MODULATOR_WAVE_SAMPLES] = {
		0,		358,	785,	1135,
		1543,	1933,	2243,	2592,
		2912,	3155,	3414,	3602,
		3791,	3939,	4028,	4096
	};
#else
	#error ""
#endif
static const uint32_t *amp_tables[DAC_MODULATOR_TYPEs_COUNT] = {
	amp_sine,
	amp_triangle,
	amp_saw,
	amp_square
};

// MCU Peripheral Config Structures, Defined global for reusing // 
static GPDMA_Channel_CFG_Type GPDMACfg;
static GPDMA_LLI_Type DMA_LLI_Struct;
static TIM_TIMERCFG_Type TIM_ConfigStruct;
static TIM_MATCHCFG_Type TIM_MatchConfigStruct;

// Module Function Prototypes // 
void __WAVE_Update_Amp(uint32_t new_amp);
static void __WAVE_Modulator_Stop(void);
static void __WAVE_Modulator_Start(uint32_t freq_hz);
static void __WAVE_Modulator_Tmr_Init(uint32_t freq_hz);
static void __WAVE_Modulator_luts_init(void);

static uint16_t __WAVE_Calc_Amplitude_Gauge(uint16_t GaugeF);
static uint16_t __WAVE_Calc_Freq_Gauge(uint16_t GaugeF);


//uint32_t DAC_Amplitude = DAC_DEFAULT_AMP;
//volatile static uint_fast8_t DAC_Amp_Changed = 1;

inline uint8_t is_dac_playing(void) {
	return (AUDIO_TASK_PLAYING == dac_state) ? TRUE : FALSE;
}

uint8_t WAVE_DAC_DMA_Handler(void)
{
	static uint8_t flag = 0;
	// check GPDMA interrupt on channel of DAC // 
	if (GPDMA_IntGetStatus(GPDMA_STAT_INT, WAVE_DAC_DMA_CHANNEL)) {
		//check interrupt status on channel of DAC // 
		// Check counter terminal status
		if(GPDMA_IntGetStatus(GPDMA_STAT_INTTC, WAVE_DAC_DMA_CHANNEL)) {
			// Clear terminate counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTTC, WAVE_DAC_DMA_CHANNEL);
			dmaDACCh_TermianalCnt++;

			if(TRUE == change_lut) {
				change_lut = FALSE;

				uint16_t current_gaugeF = Gauge_PathF[path_cnt];
				// Get new base-sine signal amplitude //
				// Apply new aplitude to base sine wave // 
				#if(TRUE == DAC_MODULATOR_DISABLED)	
					{
						active_lut ^= 1;
						__WAVE_Update_Amp(__WAVE_Calc_Amplitude_Gauge(current_gaugeF));	
						// Invoke DMA to use new updated dac lut // 
						DMA_LLI_Struct.Control &= (~GPDMA_DMACCxControl_I);	// DISABLE terminal count INT, AGAIN 
						DMA_LLI_Struct.SrcAddr = GPDMACfg.SrcMemAddr = (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));
						GPDMA_Setup(&GPDMACfg);
					}
					{
						extern uint8_t active_page;	// GUIDEMO_Start.c // 
						// Set NEW Dac update freq (Wave Freq) // 
						if((AZP_ALL_METAL_SCR != active_page) && (AZP_DISC_SCR != active_page) && (AZP_FAST_SCR != active_page) && \
							(A5P_DISC_SCR != active_page) && (A5P_ALL_METAL_SCR != active_page) && (A5P_AUTO_SCR != active_page))
							DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * __WAVE_Calc_Freq_Gauge(current_gaugeF));
						if((PATH_MEMBER_COUNT -1) != path_cnt)
							path_cnt++;
					}
				#else
					active_lut ^= 1;
					// Base-sine signal amplitude is calcualted with Modulators (sine, square etc.) //
					__WAVE_Update_Amp(amp_tables[amp_type][amp_indx]);	
					if(++amp_indx == MODULATOR_WAVE_SAMPLES)
						amp_indx = 0;		
					// Invoke DMA to use new updated dac lut // 
					DMA_LLI_Struct.Control &= (~GPDMA_DMACCxControl_I); // DISABLE terminal count INT, AGAIN 
					DMA_LLI_Struct.SrcAddr = GPDMACfg.SrcMemAddr = (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));
					GPDMA_Setup(&GPDMACfg);
					//NOTE: If modulator is enabled, base-sine signal frequemcy is not changed according to GAUGEs // 
					//:TODO: What to do according to GAUGEs ??? // 
				#endif

				/* Disble GPDMA interrupt AGAIN */
				NVIC_DisableIRQ(DMA_IRQn);
			}
		}
		if (GPDMA_IntGetStatus(GPDMA_STAT_INTERR, WAVE_DAC_DMA_CHANNEL)) {
			// Clear error counter Interrupt pending
			GPDMA_ClearIntPending (GPDMA_STATCLR_INTERR, WAVE_DAC_DMA_CHANNEL);

			dmaDACCh_ErrorCnt++;
		}

		return 1;	//Invoke caller about that ISR is handled // 
	}
	return  0;	// Invoek caller about that ISR is irrevelant // 
}

/*********************************************************************//**
 * @brief		Modulator timer interrupt, this will trigger DMA interrupt 
 * 				for new sine-base amp and freq calcualtions 
 * @param[in]	None
 * @return 		None
 **********************************************************************/
void WAVE_AMPLITUDE_UPDATE_TMR_IRQ_HANDLER(void)
{
	if (TIM_GetIntStatus(WAVE_AMPLITUDE_UPDATE_TMR, TIM_MR0_INT)== SET) {
		TIM_ClearIntPending(WAVE_AMPLITUDE_UPDATE_TMR, TIM_MR0_INT);

		{
			change_lut = TRUE;
			// Enable DMA Terminal Count INT // 
			DMA_LLI_Struct.Control |= GPDMA_DMACCxControl_I;	// Enable terminal count INT
			GPDMA_Setup(&GPDMACfg);
			NVIC_EnableIRQ(DMA_IRQn);
		}
	}
}

/* Calculate sine-wave amplitude of DAC output according to gauge value */
static uint16_t __WAVE_Calc_Amplitude_Gauge(uint16_t GaugeF)
{
	// If detector is muted return minimum allowed dac amplitude // 
	if(AppVolume == VOLUME_MIN) {
		//return current_amp = (MODULATOR_MAX_VAL*10U)/100U;
		return current_amp = 0;
	}
	
	// Apply the effect of gauge level received from detector // 
	if(AUTOMATIC_SEARCH_TYPE != active_search_mode) {	
		if(GaugeF <= (5U*GAUGE_FRACTUATION))
			current_amp = ((MODULATOR_MAX_VAL * 5)/10) + \
				(((uint32_t)MODULATOR_MAX_VAL * 2U * (uint32_t)GaugeF)/(50U*(uint32_t)GAUGE_FRACTUATION)); 
		else if(((50U*GAUGE_FRACTUATION) >= GaugeF) && ((5U*GAUGE_FRACTUATION) < GaugeF))
			current_amp = (((MODULATOR_MAX_VAL * 7)/10) + \
				((GaugeF - (5*GAUGE_FRACTUATION))*((MODULATOR_MAX_VAL * 3))) / (45U*10U*GAUGE_FRACTUATION));
		else
			current_amp = MODULATOR_MAX_VAL;
	} else {
		if((UMD_GAUGE_MIN*GAUGE_FRACTUATION) != GaugeF) {
			extern uint8_t active_page; // GUIDEMO_Start.c // 
			if((AZP_ALL_METAL_SCR != active_page) && (AZP_DISC_SCR != active_page) && (AZP_FAST_SCR != active_page) && \
				(A5P_ALL_METAL_SCR != active_page) && (A5P_DISC_SCR != active_page) && (A5P_AUTO_SCR != active_page))
				current_amp = permitted_max;
			else {
					if(550 < active_dma_freq) {
						current_amp = (permitted_max/20UL) + (19UL*GaugeF*permitted_max)/(20UL*GAUGE_FRACTUATION*UMD_GAUGE_MAX);
						current_amp /= 3;
					} else {
						current_amp = (permitted_max/10UL) + (9UL*GaugeF*permitted_max)/(10UL*GAUGE_FRACTUATION*UMD_GAUGE_MAX);
						current_amp *= 2;
					}
					// patch for mineral targets // 
					if((active_dma_freq == freqFTID[TARGET_FERROs] || (active_dma_freq == freqFTID[TARGET_CAVITY]) || \
						(active_dma_freq == freqFTID[TARGET_MINERAL])) && (current_amp > AZP_FERROs_DAC_AMP_LIMIT)) {
							current_amp = AZP_FERROs_DAC_AMP_LIMIT;
					}
			}
		} else {
			//current_amp = (MODULATOR_MAX_VAL)/200U;
			current_amp = 1;
		}
	}

	return current_amp;
}

// Calculate corresponding base dac sine signal frequency for given gauge // 
uint16_t __WAVE_Calc_Freq_Gauge(uint16_t GaugeF)
{
	volatile uint16_t gx;
	uint16_t wave_freq_hz = 0xFFFF;
	uint16_t next_indx=UMD_GAUGE_MIN, prev_indx=UMD_GAUGE_MAX;
	for(gx=0 ; gx<=UMD_GAUGE_MAX ; gx++) {
		if(dac_gauge_freq_lut[gx].gaugeF == GaugeF) {	// If Gauge is FOUND on lut, use it directly // 
			wave_freq_hz = dac_gauge_freq_lut[gx].freq;
			break;
		}
		if(GaugeF > dac_gauge_freq_lut[gx].gaugeF)
			prev_indx = gx;
		if(GaugeF < dac_gauge_freq_lut[gx].gaugeF) {
			next_indx = gx;
			break;
		}
		if(UMD_GAUGE_MAX == dac_gauge_freq_lut[gx].gaugeF) // Maximum Gauge reached but we can not FOUND // 
			break;
	}
	if(0xFFFF == wave_freq_hz) {	// Gauge directly NOT MATCHED, CALCULATINO NEEDED // 
		uint32_t gauge_diff = dac_gauge_freq_lut[next_indx].gaugeF - dac_gauge_freq_lut[prev_indx].gaugeF;
		uint32_t freq_diff = dac_gauge_freq_lut[next_indx].freq - dac_gauge_freq_lut[prev_indx].freq;
		wave_freq_hz = dac_gauge_freq_lut[prev_indx].freq + \
			((GaugeF - dac_gauge_freq_lut[prev_indx].gaugeF)*freq_diff)/gauge_diff;
	}

	return wave_freq_hz;
}

// Parameter is Fractuated Gauge // 
void WAVE_Update_FreqAmp_Gauge(uint16_t GaugeF)
{
	volatile uint8_t indx=0;
	uint16_t last_gaugeF = Gauge_PathF[path_cnt];
	
	// 1- Disable Modulator interrupt // 
	/* Disable interrupt */
	NVIC_DisableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
	// 2- Calculate and set all path freq and amplitudes in to array 
	// 3- Clear path counter  
	#if(1)
	if((last_gaugeF + GAUGE_FRACTUATION) <= GaugeF) {
		Gauge_PathF[PATH_MEMBER_COUNT-1] = GaugeF;
		for(indx=0 ; indx<(PATH_MEMBER_COUNT-1) ; indx++) 
			Gauge_PathF[indx] = last_gaugeF + (((GaugeF - last_gaugeF)*(indx+1)) / (PATH_MEMBER_COUNT+1));	
		path_cnt = 0;
	}
	else if((GaugeF + GAUGE_FRACTUATION) <= last_gaugeF) {
		Gauge_PathF[PATH_MEMBER_COUNT-1] = GaugeF;
		for(indx=0 ; indx<(PATH_MEMBER_COUNT-1); indx++) 
			Gauge_PathF[indx]= last_gaugeF - (((last_gaugeF - GaugeF)*(indx+1)) / (PATH_MEMBER_COUNT+1));	
		path_cnt = 0;
	}
	else {
		for(indx=0 ; indx<PATH_MEMBER_COUNT ; indx++) 
			Gauge_PathF[indx]= last_gaugeF = GaugeF;	
		path_cnt = PATH_MEMBER_COUNT - 1;
	}
	#else
		for(indx=0 ; indx<PATH_MEMBER_COUNT ; indx++) {
			Gauge_PathF[indx]= GaugeF; 
		}
		path_cnt = PATH_MEMBER_COUNT - 1;
	#endif
	// 4- Re-Enable Modulator Interrupt 
	NVIC_EnableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
}

void WAVE_Modulator_Update_Freq(uint32_t new_freq_hz) 
{
	#if(FALSE == DAC_MODULATOR_DISABLED)
	f	new_freq_hz *= MODULATOR_WAVE_SAMPLES;
	#endif
	TIM_MatchConfigStruct.MatchValue   = (1000000UL / new_freq_hz);
	TIM_ConfigMatch(WAVE_AMPLITUDE_UPDATE_TMR, &TIM_MatchConfigStruct);
}

void WAVE_Madulator_Set_Type(eSineModulatorType new_type)
{
	NVIC_DisableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);	// Disable Modulator timer IRQ temporaryly // 
	amp_type = new_type;	// Change modulator type // 
	amp_indx = 0;	// Start modulator amplitudes from stratch // 
	NVIC_EnableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);	// ReEnable modulator timer IRQ // 
}

// Each time if there is a need to do changes on base-sine amp and freq, this ISR occurs // 
static void __WAVE_Modulator_Start(uint32_t freq_hz)
{
	// Set new freq for modulator update timer // 
	WAVE_Modulator_Update_Freq(freq_hz);
	/* Clear & Enable interrupt for selected timer */
	NVIC_ClearPendingIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
	NVIC_EnableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
	// Start timer
	TIM_Cmd(WAVE_AMPLITUDE_UPDATE_TMR, ENABLE);
}

static void __WAVE_Modulator_Stop(void) 
{
	// Stop timer
	TIM_Cmd(WAVE_AMPLITUDE_UPDATE_TMR, DISABLE);
	/* Disable interrupt */
	NVIC_DisableIRQ(WAVE_AMPLITUDE_UPDATE_TMR_INT);
}

void __WAVE_Update_Amp(uint32_t new_amp) 
{
	uint16_t *lut_p = (0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2);
	volatile uint32_t cnt;
	for(cnt = 0; cnt < NUM_SINE_SAMPLE; cnt++) {
		if(cnt <= 15) {
			lut_p[cnt] = SINEWAVE_DC_OFFSET + ((new_amp * sin_0_to_90_16_samples[cnt])>>BASE_SINE_PEAK_BITs);
		}
		else if(cnt <= 30) {
			lut_p[cnt] =  SINEWAVE_DC_OFFSET + ((new_amp * sin_0_to_90_16_samples[30-cnt])>>BASE_SINE_PEAK_BITs);
		}
		else if(cnt <= 45) {
			lut_p[cnt] = SINEWAVE_DC_OFFSET - ((new_amp * sin_0_to_90_16_samples[cnt-30])>>BASE_SINE_PEAK_BITs);
		}
		else {
			lut_p[cnt] = SINEWAVE_DC_OFFSET - ((new_amp * sin_0_to_90_16_samples[60-cnt])>>BASE_SINE_PEAK_BITs);
		}

		//To make sure the output value is not over 10-bit width of DAC
		lut_p[cnt] &= 0x3FF;

		#if _DMA_USING
			//Shift the value before DMA to DAC component in case of using DMA
			lut_p[cnt] = (lut_p[cnt] << 6);
		#endif
	}
}
   
void DAC_DMA_Update_Freq(uint32_t freq_hz) 
{
	active_dac_dma_update_freq_hz = freq_hz;
	uint32_t cnt;
	// Change DMA timeout for freq
	cnt = CLKPWR_GetCLK(CLKPWR_CLKTYPE_PER);
	// set time out for DAC
	// clk = sine_freq * number_of_samples_per_sine_cycle * sample_freq 	(pulses of clock)
	// => sample_freq = clk / (sine_freq * number_of_samples_per_sine_cycle)
	cnt = cnt/freq_hz;
	DAC_SetDMATimeOut(DAC_OUT_CHANNEL, cnt);
}


uint32_t get_DAC_DMA_Update_Freq(void) {
	return active_dac_dma_update_freq_hz;
}

void DAC_test(void)
{
	volatile uint16_t Gauge;
	srand(SysTick->VAL);
	
	uint16_t freqs[101];
	uint16_t amps[101];
	for(Gauge=0 ; Gauge<=100 ; Gauge++) {
		freqs[Gauge] = __WAVE_Calc_Freq_Gauge(Gauge*GAUGE_FRACTUATION);
		amps[Gauge] = __WAVE_Calc_Amplitude_Gauge(Gauge*GAUGE_FRACTUATION);
	}
	WAVE_Generator_init(STD_SEARCH_TYPE);
	WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
	while(1) {
		WAVE_Update_FreqAmp_Gauge((rand()%UMD_GAUGE_MAX)*GAUGE_FRACTUATION);
		path_cnt = rand()%5;
	}
}

void WAVE_Generator_init(uint8_t search_mode)
{
	uint32_t cnt, max;
	DAC_CONVERTER_CFG_Type DAC_ConverterConfigStruct;

	while(TRUE == is_audio_playing());	// wait until active audio file playing is completed // 
	
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
		
	init_TPs(4);
	AppVolume = APP_GetValue(ST_VOL);
	{
		// Apply the effect of volume setting, it is truncation of dac output amplitude // 
		if(AppVolume <= APP_VOL_DAC_AMPLITUDE_START_LEVEL)
			permitted_max = (((uint32_t)APP_VOL_DAC_AMPLITUDE_START_LEVEL * MODULATOR_MAX_VAL)) / VOLUME_MAX;
		else
			permitted_max = ((MODULATOR_MAX_VAL * (uint32_t)AppVolume) / VOLUME_MAX);
	}
	active_search_mode = search_mode;
	change_lut = FALSE;
	active_lut = 0;
	// Update maximum Wave frequency according to search mode // 
	if(AUTOMATIC_SEARCH_TYPE == search_mode) 
		memcpy(dac_gauge_freq_lut, dac_gauge_freq_lut_AUTO, sizeof(dac_gauge_freq_lut));
	else
		memcpy(dac_gauge_freq_lut, dac_gauge_freq_lut_OTHERs, sizeof(dac_gauge_freq_lut));

	#if(TRUE != DAC_MODULATOR_DISABLED)
		__WAVE_Modulator_luts_init();	// sine-Wave modulator waves lookup tables initialization // 
	#endif
	
	//Prepare DAC sine look up table
	//__WAVE_Update_Amp(DAC_DEFAULT_AMP);
	__WAVE_Update_Amp(__WAVE_Calc_Amplitude_Gauge(UMD_GAUGE_MIN*GAUGE_FRACTUATION));
	if(0 == active_lut)
		memcpy(dac_sine_lut2, dac_sine_lut, sizeof(dac_sine_lut2));
	else
		memcpy(dac_sine_lut, dac_sine_lut2, sizeof(dac_sine_lut2));

	//Prepare DMA link list item structure
	DMA_LLI_Struct.SrcAddr= (uint32_t)((0 == active_lut) ? (dac_sine_lut):(dac_sine_lut2));
	DMA_LLI_Struct.DstAddr= (uint32_t)&(LPC_DAC->CR);
	DMA_LLI_Struct.NextLLI= (uint32_t)&DMA_LLI_Struct;
	DMA_LLI_Struct.Control= DMA_SIZE
								| (1<<18) //source width 16 bit
								| (1<<21) //dest. width 16 bit
								| (1<<26); //source increment 
	/* GPDMA block section -------------------------------------------- */
	/* Initialize GPDMA controller */
	GPDMA_Init();

	// Setup GPDMA channel --------------------------------
	// channel 0
	GPDMACfg.ChannelNum = WAVE_DAC_DMA_CHANNEL;

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
	/* Disble GPDMA interrupt */
	NVIC_DisableIRQ(DMA_IRQn);

	DAC_Init(0);

	// set time out for DAC, sine Wave frequency // 
	DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * (uint32_t)dac_gauge_freq_lut[0].freq);

	DAC_ConverterConfigStruct.CNT_ENA = SET;
	DAC_ConverterConfigStruct.DMA_ENA = SET;
	DAC_ConfigDAConverterControl(0, &DAC_ConverterConfigStruct);

	__WAVE_Modulator_Tmr_Init(MODULATOR_DEF_FREQ_HZ);
}

void WAVE_quick_start(void)
{
	// Restart DAC 
	GPDMA_ChannelCmd(WAVE_DAC_DMA_CHANNEL, ENABLE);
	// Start timer
	TIM_Cmd(WAVE_AMPLITUDE_UPDATE_TMR, ENABLE);
}

void WAVE_Generator_start(uint8_t Gauge, uint32_t modulator_freq_hz, uint16_t wave_amp) 
{
	// Set sine wave amplitude // 
	//__WAVE_Update_Amp(wave_amp);
	__WAVE_Update_Amp(__WAVE_Calc_Amplitude_Gauge(Gauge*GAUGE_FRACTUATION));
	// Restart DAC 
	GPDMA_ChannelCmd(WAVE_DAC_DMA_CHANNEL, ENABLE);
	// Set Wave Freq // 
	DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * __WAVE_Calc_Freq_Gauge(Gauge));
	// Start WAVE Modulator timer // 
	if(0 != modulator_freq_hz)
		__WAVE_Modulator_Start(modulator_freq_hz);
	dac_state = AUDIO_TASK_PLAYING;
}

void WAVE_SetFreq_RTID(uint8_t target_id) {
	DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * freqRTID[target_id]);	
	active_dma_freq = freqRTID[target_id];
}

void WAVE_SetFreq_FTID(uint8_t target_id) {
	DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * freqFTID[target_id]);	
	active_dma_freq = freqFTID[target_id];
}

void WAVE_Generator_stop(uint8_t reset_DAC_FreqAmp, uint8_t disable_DMA, uint8_t stop_Modulator)
{
	dac_state = AUDIO_TASK_IDLE;
	// Stop DAC update with DMA 
	if(TRUE == disable_DMA) {
		volatile uint8_t indx;
		GPDMA_ChannelCmd(WAVE_DAC_DMA_CHANNEL, DISABLE);
	}
	// Stop Modulator Timer // 
	if(TRUE == stop_Modulator)
		__WAVE_Modulator_Stop();

	// Set safe value for dac output //
	if(TRUE == reset_DAC_FreqAmp) {
		volatile uint8_t indx;
		// Reset WAVE Frequency to initial value // 
		for(indx=0 ; indx<sizeof(Gauge_PathF)/sizeof(Gauge_PathF[0]); indx++)
			Gauge_PathF[indx] = UMD_GAUGE_MIN*GAUGE_FRACTUATION;
		path_cnt = 0;
		DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * __WAVE_Calc_Freq_Gauge(UMD_GAUGE_MIN*GAUGE_FRACTUATION));
		__WAVE_Update_Amp(__WAVE_Calc_Amplitude_Gauge(UMD_GAUGE_MIN*GAUGE_FRACTUATION));
	}
}

 void reset_DAC_freq(void){
	DAC_DMA_Update_Freq((uint32_t)NUM_SINE_SAMPLE * __WAVE_Calc_Freq_Gauge(UMD_GAUGE_MIN*GAUGE_FRACTUATION));
}

#if(TRUE != DAC_MODULATOR_DISABLED)
static void __WAVE_Modulator_luts_init(void)
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
		double slope = (double)512/(MODULATOR_WAVE_SAMPLES-1);
		amp_saw[cnt] = slope * cnt;
	}
	amp_saw[MODULATOR_WAVE_SAMPLES-1] = 512;
	
	// Calculate triangle values //
	for(cnt = 0; cnt <= MODULATOR_WAVE_SAMPLES/2; cnt++) {
		double slope = (double)512/(MODULATOR_WAVE_SAMPLES/2);
		amp_triangle[cnt] = amp_triangle[MODULATOR_WAVE_SAMPLES-cnt] = slope * cnt;
	}
	// Calculate square values //
	for(cnt = 0; cnt < MODULATOR_WAVE_SAMPLES; cnt++) {
		if(cnt <= (MODULATOR_WAVE_SAMPLES/2))
			amp_square[cnt] = 512;
		else
			amp_square[cnt] = 0;
	}
}
#endif

static void __WAVE_Modulator_Tmr_Init(uint32_t freq_hz)
{
	// Initialize timer 0, prescale count time of 100uS
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_USVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;
	TIM_Init(WAVE_AMPLITUDE_UPDATE_TMR, TIM_TIMER_MODE, &TIM_ConfigStruct);

	// use channel 0, MR0
	TIM_MatchConfigStruct.MatchChannel = 0;
	// Enable interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct.IntOnMatch   = TRUE;
	//Enable reset on MR0: TIMER will reset if MR0 matches it
	TIM_MatchConfigStruct.ResetOnMatch = TRUE;
	//Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct.StopOnMatch  = FALSE;
	//Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct.ExtMatchOutputType = TIM_EXTMATCH_NOTHING;
	WAVE_Modulator_Update_Freq(freq_hz);

	// IRQ Priority has been set in BSP_Init_IRQ_Priorities() function // 
	/* preemption = 1, sub-priority = 1 */
	// NVIC_SetPriority(WAVE_AMPLITUDE_UPDATE_TMR_INT, 1);
}


#ifndef DAC_H
#define DAC_H

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include "LPC177x_8x.h"
#include "lpc177x_8x_dac.h"
#include "lpc177x_8x_gpdma.h"
#include "lpc177x_8x_clkpwr.h"


#define DAC_OUT_CHANNEL				(0)
#define APP_VOL_DAC_AMPLITUDE_START_LEVEL		(10)	// The output is hearable from this level of DAC output Vpp // 
#define APP_VOL_DAC_JACK_DETECT_LEVEL			(1)
#define DAC_DEFAULT_AMP				(((uint32_t)MODULATOR_MAX_VAL*(uint32_t)APP_VOL_DAC_AMPLITUDE_START_LEVEL)/(uint32_t)VOLUME_MAX)
#define DAC_JACK_DETECT_AMP			(((uint32_t)MODULATOR_MAX_VAL*(uint32_t)100)/(uint32_t)VOLUME_MAX)
#define DAC_MODULATOR_DISABLED		TRUE	/* If modulator is disabled simple sinus signal is generated */
											/* Base-Sine amplitude and frequemcy is calculated from smooth-dac change mechanism */
#define PATH_MEMBER_COUNT				10
#define MODULATOR_DEF_FREQ_HZ		(33)
#define MODULATOR_IRQ_FOR_SMOOT_DAC	FALSE
#define GAUGE_FRACTUATION		10
#define BASE_SINE_PEAK_BITs		12
#define LPC_DAC_BITs			10
#define SINEWAVE_DC_OFFSET				((0x1<<LPC_DAC_BITs)/2)		/* Half of DAC output maximum value */
#define MODULATOR_MAX_VAL				(((0x1<<LPC_DAC_BITs)/2)-1)
#define AZP_FERROs_DAC_AMP_LIMIT		((6*MODULATOR_MAX_VAL)/7)

typedef enum 
{
	DAC_SINE_WAVE	= 0,	// Modulator for GOLD(Special NonFerro) targets // 
	DAC_TRIANGLE_WAVE,		// Modulator for METAL (Ferro, NonFerro) targets // 
	DAC_SAWTOOTH_WAVE,		// Modulator for 	MINERAL and UNKNOWN targets // 	
	DAC_SQUARE_WAVE,		// Modulator for CAVITY targets // 
	
	DAC_MODULATOR_TYPEs_COUNT
} eSineModulatorType;

typedef enum {
	SIMPLE_MULTIPLER_MODULATOR	= 0,
	WAVEFORM_MODULATOR,
	
	DACMODUALTORs_COUNT
} eDAC_MODULATOR_TYPEs;

extern uint8_t is_dac_playing(void);
extern void WAVE_quick_start(void);
extern void WAVE_Modulator_Update_Freq(	uint32_t new_modulator_freq_hz);
extern uint16_t WAVE_Update_Freq_Gauge(uint32_t freq_hz);
extern void WAVE_Madulator_Set_Type(eSineModulatorType type);
extern void WAVE_Generator_init(uint8_t search_type);
extern void WAVE_Generator_start(uint8_t Gauge, uint32_t modulator_freq_hz, uint16_t wave_amp);
void WAVE_Generator_stop(uint8_t reset_DAC_FreqAmp, uint8_t disable_DMA, uint8_t stop_Modulator);	
void WAVE_Update_FreqAmp_Gauge(uint16_t Gauge);
inline void reset_DAC_freq(void);
void DAC_test(void);
uint8_t WAVE_DAC_DMA_Handler(void);
void DAC_DMA_Update_Freq(uint32_t freq_hz);
uint32_t get_DAC_DMA_Update_Freq(void);
void WAVE_SetFreq_FTID(uint8_t target_id);
void WAVE_SetFreq_RTID(uint8_t target_id);


#endif

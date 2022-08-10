/*
 * adv7181c.h
 *
 *  Created on: Jan 17, 2014
 *      Author: serkan
 */

#ifndef ADV7181C_H_
#define ADV7181C_H_

#include <stdio.h>
#include <stdint.h>

#define ADV7181C_ADR					(0x42>>1)
#define ANALOG_VIDVALID_CNT_MAX			(25)

/*
 * ##CP 1080i##
:1080i/50Hz YPrPb In 20Bit 422 HS/VS Encoder:
42 05 01 ; Prim_Mode =001b COMP
42 06 0C ; VID_STD for 1080i
42 C3 46 ; ADC1 to Ain4, ADC0 to Ain6,
42 C4 B5 ; ADC2 to Ain5 and enables manual override of mux
42 1D 47 ; Enable 28MHz Crystal
42 3A 21 ; set latch clock settings to 010b, Power Down ADC3
42 3B 81 ; Enable Internal Bias
42 3C 5D ; PLL_QPUMP to 101b
42 6B 81 ; 422 20bit OUT
42 C9 00 ; SDR
42 73 CF ; Enable Manual Gain and set CH_A gain
42 74 A3 ; Set CH_A and CH_B Gain - 0FAh
42 75 E8 ; Set CH_B and CH_C Gain
42 76 FA ; Set CH_C gain
42 7B 1C ; TURN OFF EAV & SAV CODES Set BLANK_RGB_SEL
42 7C 93 ; set HS position/ HS Polarity - positive
42 7D D4 ; set HS position
42 7E 2D ; set HS position
42 85 19 ; Turn off SSPD and force SOY. For Eval Board.
42 86 0B ; Enable stdi_line_count_mode
42 87 EA ; Reprogram pll_div_ratio for 1080i/50Hz = 2640
42 88 50
42 8F 03 ; Reprogram freerun_line_length also to 1018 for 28.636MHz xtal
42 90 FA
42 B7 1B ; use internal VS width for fixing VS vibration
42 BF 06 ; Blue Screen Free Run Colour
42 C0 40 ; default color
42 C1 F0 ; default color
42 C2 80 ; Default color
42 C5 01 ; CP_CLAMP_AVG_FACTOR[1-0] = 00b
42 F4 3F ; Max Drive Strength
42 0E 80 ; ADI Recommended Setting
42 52 46 ; ADI Recommended Setting
42 54 00 ; ADI Recommended Setting
42 57 01 ; ADI Recommended Setting
42 F6 3B ; ADI Recommended Setting
42 0E 00 ; ADI Recommended Setting
56 17 02 ; Software Reset
56 00 1E ; Power up dacs and PLL
56 01 10 ; ED/HD SDR
56 02 21 ; Rev.3 861B timing mode
56 30 70 ; 1080u@25Hz AV codes disabled
56 31 01 ; H54x enabled, Pixel Data Valid
56 33 6C ; PrPb SSAF, Sync filter enabled, 422 enabled - CbCr swap
56 39 20 ; ED/HD 861B Timing
End
 *
 */

typedef enum	// Directly register(Status1, 0x10) values; DONT EDIT
{
	NTSM_MJ		= 0,
	NTSC_443 	= 1,
	PAL_M		= 2,
	PAL_60		= 3,
	PAL_BGHID	= 4,
	SECAM		= 5,
	PAL_CN		= 6,
	SECAM_525	= 7,

	ADV7181C_CVBS_TYPEs_COUNT
} eADV7181C_CVBS_TYPEs;

typedef struct
{
	uint8_t cvbs_type;	// eADV7181C_CVBS_TYPEs //
	uint8_t inpok;		// input valid or NOT //
} sADV7181C_VidInfo;

extern sADV7181C_VidInfo *ADV7181C_get_input_info(uint16_t tryCnt, uint16_t waitMS);

#endif /* ADV7181C_H_ */

/*
 * adv7611.h
 *
 *  Created on: Jan 13, 2014
 *      Author: serkan
 */

#ifndef ADV7611_H_
#define ADV7611_H_

#include "IF.h"
#include "libadv.h"

/*************************************************/
/********** GENERAL CONTROL **********************/
/*************************************************/
#define HDMI_NON_FAST_SWITCHING		TRUE
#define ADV7611_POWER_DOWN_MODE		0
#define ADV7611_INITIAL_RESET 		TRUE

/*************************************************/
/****** I2C ADDRESSES & BUS **********************/
/*************************************************/
#if(ALSB_PULL_UP == TRUE)
	#define IO_MAP_ADR				(0x9A>>1)
#else
#define IO_MAP_ADR				(0x98>>1)
#endif
#define CEC_MAP_ADR				(0x80>>1)
#define INFOFRAME_MAP_ADR		(0x7C>>1)
#define DPLL_MAP_ADR			(0x4C>>1)
#define KVS_MAP_ADR				(0x64>>1)
#define EDID_MAP_ADR			(0x6C>>1)
#define HDMI_MAP_ADR			(0x68>>1)
#define CP_MAP_ADR				(0x44>>1)

// Check state for timing values validity //
	// Both must be "1" //
#define DE_REGEN_FILTER_LOCKED_MAP	0x68
#define DE_REGEN_FILTER_LOCKED_ADR	0x07
#define DE_REGEN_FILTER_LOCKED_BIT	5

#define DE_REGEN_LCK_RAW_MAP		0x98
#define DE_REGEN_LCK_RAW_ADR		0x6A
#define DE_REGEN_LCK_RAW_BIT		0

/*****************************************/
/********* HORIZONTAL TIMING DETAILS *******/
/*****************************************/
// HSYNC:: Input Video Details //
#define TOTAL_LINE_WIDTH_MAP		0x68
#define TOTAL_LINE_WIDTH_ADRL		0x1F
#define TOTAL_LINE_WIDTH_ADRH		0x1E
#define TOTAL_LINE_WIDTH_BITs		14
// Line Width //
#define LINE_WIDTH_MAP		0x68
#define LINE_WIDTH_ADRL		0x08
#define LINE_WIDTH_ADRH		0x07
#define LINE_WIDTH_BITs		13
// Front Porch //
#define HSYNC_FRONT_PORCH_MAP	0x68
#define HSYNC_FRONT_PORCH_ADRL	0x21
#define HSYNC_FRONT_PORCH_ADRH	0x20
#define HSYNC_FRONT_PORCH_BITS	13
// Back Porch //
#define HSYNC_BACK_PORCH_MAP	0x68
#define HSYNC_BACK_PORCH_ADRL	0x25
#define HSYNC_BACK_PORCH_ADRH	0x24
#define HSYNC_BACK_PORCH_BITS	13
// Pulse Width //
#define HSYNC_PULSE_WIDTH_MAP	0x68
#define HSYNC_PULSE_WIDTH_ADRL	0x23
#define HSYNC_PULSE_WIDTH_ADRH	0x22
#define HSYNC_PULSE_WIDTH_BITS	13

// Signal Polarity Definitions //
#define DVI_HSYNC_POLARITY_MAP		0x68
#define DVI_HSYNC_POLARITY_ADR		0x05
#define DVI_HSYNC_POLARITY_BIT		5

#define ACTIVE_LOW_POLARITY			(0)
#define ACTIVE_HIGH_POLARITY		(1)

typedef enum {
	CP_NORMAL_OP	= 0,
	CP_FREE_RUN,

	CP_STATUS_COUNT
} eadv7611_CP_status_t;

typedef struct
{
	uint8_t de_regen_locked;	// TRUE if locked //
	uint8_t de_regen_raw;		// TRUE if locked //
	uint8_t polarity;			//	Active HIGH or LOW //
	uint16_t tot_line_width;		// Total ilne length
	uint16_t active_line_width;		// Actve line length
	uint16_t front_porch;			// Front porch
	uint16_t back_porch;			// back porch
	uint16_t pulse_witdh;			// horizontal pulse width
} adv7611_hor_timing_type;

/*****************************************/
/********* VERTICAL TIMING DETAILS *******/
/*****************************************/
#define VERT_FILTER_LOCKED_MAP		0x68
#define VERT_FILTER_LOCKED_ADR		0x07
#define VERT_FILTER_LOCKED_BIT		7

#define V_LOCKED_RAW_MAP			0x68
#define V_LOCKED_RAW_ADR			0x6A
#define V_LOCKED_RAW_BIT			1

// VSYNC polarity //
#define DVI_VSYNC_POLARITY_MAP		0x68
#define DVI_VSYNC_POLARITY_ADR		0x05
#define DVI_VSYNC_POLARITY_BIT		4

// FIELD0 Total Height //
#define FIELD0_TOTAL_HEIGHT_MAP		0x68
#define FIELD0_TOTAL_HEIGHT_ADRL	0x27
#define FIELD0_TOTAL_HEIGHT_ADRH	0x26
#define FIELD0_TOTAL_HEIGHT_BITS	14

// FIELD0 Height //
#define FIELD0_HEIGHT_MAP			0x68
#define FIELD0_HEIGHT_ADRL			0x0A
#define FIELD0_HEIGHT_ADRH			0x09
#define FIELD0_HEIGHT_BITS			13

//:TODO: FIELD0 front_porch, back_porch, pulse_width,
// FIELD1 Details; Field1 is valid only if "HDMI_INTERLACED" is 1 //
#define FIELD1_TOTAL_HEIGHT_MAP		0x68
#define FIELD1_TOTAL_HEIGHT_ADRL	0x29
#define FIELD1_TOTAL_HEIGHT_ADRH	0x28
#define FIELD1_TOTAL_HEIGHT_BITS	14

#define FIELD1_HEIGHT_MAP			0x68
#define FIELD1_HEIGHT_ADRL			0x0C
#define FIELD1_HEIGHT_ADRH			0x0B
#define FIELD1_HEIGHT_BITS			13
//:TODO: FIELD1 front_porch, back_porch, pulse_width,

typedef struct
{
	uint8_t v_locked_raw;	// TRUE if locked //
	uint8_t vert_filter_locked;		// TRUE if locked //
	uint8_t polarity;			//	Active HIGH or LOW //
	uint16_t tot_height;
	uint16_t active_height;
	uint16_t front_porch;
	uint16_t back_porch;
	uint16_t pulse_witdh;
} adv7611_ver_timing_type;

/*****************************************/
/********* COMMON TIMING DETAILS *********/
/*****************************************/
typedef struct
{
	uint8_t vidstd;				// VID_STD value in device register //
	uint8_t PR;					// Pixel Repeation timing for horizontal //
	uint8_t interlaced;			// interlaced flag //
	uint16_t horres;			// horizontal resolution //
	uint16_t verres;			// vertical resolution, active vertical lines //
	uint8_t  v_freq;			// v_freq setting //
} vidstd_type;

typedef struct
{
	uint8_t interlaced;
	uint8_t video_valid;
	uint8_t cp_status;				// CP module status "FREE_RUN / NORMAL OPERATION" //
	adv7611_ver_timing_type ver_timing;
	adv7611_hor_timing_type hor_timing;
} adv7611_inp_vid_type;
// If bit is 1, it is INTERLACED and FIELD1 datas are valid //
#define HDMI_INTERLACED_MAP			0x68
#define HDMI_INTERLACED_ADR			0x0B
#define HDMI_INTERLACED_BIT			5

// Calculate total horizontal blanking and set this bit if required //
#define NEW_VS_PARAM_MAP			0x68
#define NEW_VS_PARAM_ADR			0x4C
#define NEW_VS_PARAM_BIT			2

/*****************************************/
/********* AUDIO TIMING DETAILS *********/
/*****************************************/
#define AUDIO_PLL_LOCKED_MAP		0x68
#define AUDIO_PLL_LOCKED_ADR		0x04
#define AUDIO_PLL_LOCKED_BIT		0

// Default I2S mode, 24bit, Right Justified, 256fs
// READ-ONLY //
// 0: layout1(2 Channel), 1:Layout0(8 Channel)
#define AUDIO_CH_MD_RAW_MAP			0x68
#define AUDIO_CH_MD_RAW_ADR			0x65
#define AUDIO_CH_MD_RAW_BIT			4

// READ-ONLY //
// 0: Stereo Aduio, 1: Multi-Channel //
#define AUDIO_CHANNEL_MODE_MAP		0x68
#define AUDIO_CHANNEL_MODE_ADR		0x07
#define AUDIO_CHANNEL_MODE_BIT		6

// MUTE ADUIO //
// 0:Audio Normal Operation, 1:Mute Audio //
#define MUTE_AUDIO_MAP				0x68
#define MUTE_AUDIO_ADR				0x1A
#define MUTE_AUDIO_BIT				4

// Internal Audio Mute State; READ-ONLY //
// 0:Audio Not-Muted, 1:Audio Muted //
#define INTERNAL_MUTE_RAW_MAP		0x68
#define INTERNAL_MUTE_RAW_ADR		0x65
#define INTERNAL_MUTE_RAW_BIT		6

// Receşved Frame Audio Mute State //
// 0:AV_MUTE not-set, 1:AV_MUTE set //
#define AV_MUTE_MAP			0x68
#define AV_MUTE_ADR			0x04
#define AV_MUTE_BIT			6

// Sampling Frequency //
#define SAMPLING_FREQ_MAP		0x68
#define SAMPLING_FREQ_ADR		0x39
#define SAMPLING_FREQ_BITS		4

// DPP Bypass //
#define DPP_BYPASS_EN_MAP		0x44
#define DPP_BYPASS_EN_ADR		0xBD
#define DPP_BYPASS_EN_BIT		4

typedef enum
{
	RGB_LIMITED	= 0,
	RGB_FULL,
	YUV_601,
	YUV_709,
	XVYCC_601,
	XVYCC_709,
	YUV_601_FULL,
	YUV_709_FULL,
	sYCC_601,
	Adobe_YCC_601,
	Adobe_RGB,

	HDMI_COLORSPACE_COUNT
} HDMI_COLORSPACE_TYPE;
#define HDMI_COLORSPACE_MAP		0x68
#define HDMI_COLORSPACE_ADR		0x53
#define HDMI_COLORSPACE_BITS	4

// AV Code Insertion //
#define AVCODE_INSERT_EN_MAP	0x98
#define AVCODE_INSERT_EN_ADR	0x05
#define AVCODE_INSERT_EN_BIT	2

#define VIDVALID_CNT_MAX			25
#define ADV7611_RES_DETECT_MAX_TRY      ((VIDVALID_CNT_MAX*10)/2)
#define ADV7611_RES_DETECT_STEP_INTERVAL_MS     50
#define VFREQ_INDX					(4)
#define VFREQ_MASK					(0x7)
typedef enum
{
	VFREQ_60	= 0,
	VFREQ_50	= 1,
	VFREQ_30	= 2,
	VFREQ_25	= 3,
	VFREQ_24	= 4,

	VFREQ_COUNT
} evFreq;

#define EDID_FILE_NAME "/root/SiI9135A_EDID_IMAGE.bin"
#define EDID_MEM_SIZE		(256)

// Shared Objects //

// Shared Functions //
extern uint8_t ADV7611_get_input_res(uint8_t max_try, uint8_t delayMS);

#endif /* ADV7611_H_ */

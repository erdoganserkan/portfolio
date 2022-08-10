/*
 * libADV.h
 *
 *  Created on: Jan 17, 2014
 *      Author: serkan
 */

#ifndef LIBADV_H_
#define LIBADV_H_

#include <stdint.h>
#include <adv7181c.h>

#ifndef FALSE
	#define FALSE	0
#endif
#ifndef TRUE
	#define TRUE	(!FALSE)
#endif

#define LIBC2ADV_LOG_FILE	"/root/libc2adv"	// to use regular file as log output device //
//#define LIBC2ADV_LOG_FILE	""	// To use sdtout as log output device //

#define ALSB_PULL_UP	FALSE	/* ALSB pin of ADV7611 pulled up by 10k, check hardware !!! */
#define ArraySize(x)		(sizeof(x)/sizeof(x[0]))

// FREE RUN COLOR DEFs //
// Received from "http://stnsoft.com/DVD/color_pick.html"
// Change the order of the second and third colors //
#define FR_YELLOW_CHA	0xBF
#define FR_YELLOW_CHB	0xA0
#define FR_YELLOW_CHC	0x1F

#define FR_PURPLE_CHA	0x3C
#define FR_PURPLE_CHB	0xE8
#define FR_PURPLE_CHC	0xE8

#define FR_BLACK_CHA	0x10
#define FR_BLACK_CHB	0x78
#define FR_BLACK_CHC	0x78

#define FR_GREEN_CHA	0x68
#define FR_GREEN_CHB	0x18
#define FR_GREEN_CHC	0x28

#define FR_WHITE_CHA	0xEB
#define FR_WHITE_CHB	0x88
#define FR_WHITE_CHC	0x88

typedef enum {
	FR_WHEN_NO_TMDS_CLK	= 0,
	FR_WHEN_NO_TMDS_and_MISMATCH,

	FR_MODEs_COUNT
} eFR_MODEs;

typedef enum {
	FR_FROM_INPUT_FORMAT	= 0,
	FR_FROM_DESIRED_TEST_PATTERN,

	FR_SRC_COUNT
} eFR_SRCs;

typedef enum
{
	ADV7181C_CVBS_PAL 				= 0,
	ADV7181C_CVBS_NTSC				= 1,
	ADV7181C_SDP_YC					= 2,
	ADV7181C_SDP_YPrPb				= 3,
	ADV7181C_FAST_BLANK				= 4,
	ADV7181C_CP_YPrPb_525i			= 5,
	ADV7181C_CP_YPrPb_625i			= 6,
	ADV7181C_525p_60Hz_20bit_422_SAVEAV		= 7,
	ADV7181C_625p_50Hz_20bit_422_SAVEAV		= 8,
	ADV7181C_720p_50Hz_20bit_422_SAVEAV		= 9,
	ADV7181C_720p_60Hz_20bit_422_SAVEAV		= 10,
	ADV7181C_1080i_50Hz_20bit_422_SAVEAV	= 11,
	ADV7181C_1080i_60Hz_20bit_422_SAVEAV	= 12,
	ADV7181C_AUTO_RESOLUTION	= 13,

	ADV7181C_MODE_COUNT
} eADV7181C_MODEs;

typedef enum
{
	HDMI_576i 				= 0,
	HDMI_480i 				= 1,
	HDMI_SD_RES_MAX         = HDMI_480i,
	HDMI_576p				= 2,
	HDMI_480p				= 3,
	HDMI_720p50				= 4,
	HDMI_720p60				= 5,
	HDMI_1080i50			= 6,
	HDMI_1080i60			= 7,
	HDMI_1080p50			= 8,
	HDMI_1080p60			= 9,
	HDMI_AUTO_RESOLUTION	= 10,	/* HDMI input resolution will be received from input registers */
	HDMI_INPUT_NOT_ACTIVE	= 11,	/* There is NO active video @ HDMI input */
	HDMI_INPUT_ERR			= 12,	/* Read/Write error occured when accessing the chip */
	HDMI_RES_NON_STANDARD	= 13,	/* HDMI input resolution is NOT STARDARD */

	HDMI_RES_COUNT
} eADV7611_MODEs;
#define DEFAULT_HDMI_RES	HDMI_720p50

#define	ANALOG_AUDIO_DUAL_MONO		0
#define ANALOG_AUDIO_MONO_LEFT		1
#define ANALOG_AUDIO_MONO_RIGHT		2
#define ANALOG_AUDIO_STEREO			3
#define FJF_AUDIO_MODEs_COUNT		4
// Only Applicable for SDI & ANALOG streams, Ignored for SDIEMB & HDMI streams //

extern int8_t is_hdmi_video_active(void);
extern int8_t is_analog_video_active(void);	// Return Val: TRUE if video input is valid, FALSE o.w. //
extern int8_t libc2adv_init(const int log_level);
extern int8_t hdmi_init(uint8_t modeHDMI, uint8_t *test_pattern_state_ptr);
extern int8_t analog_video_init(uint8_t mode);
extern uint8_t analog_sound_init(uint16_t waitMS, uint16_t MaxTryCnt, uint8_t audio_mode);
extern uint8_t is_hdmi_res_changed(void);	// TRUE if input RESOLUTION changed since last stream started, FALSE otherwise //
												// MODEO master application should poll this function periodically //
int8_t set_free_run_video(eFR_MODEs fr_mode, eFR_SRCs fr_src, uint8_t fr_enable, uint8_t force, uint8_t fr_pattern, \
	uint8_t chA, uint8_t chB, uint8_t chC);

#endif /* LIBADV_H_ */

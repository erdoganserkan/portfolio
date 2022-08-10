#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <log.h>
#include <libi2c.h>
#include "IF.h"
#include "adv7611.h"
#include "../inc/libadv_common.h"

// Variables //
static adv7611_inp_vid_type vid_timing;
extern mj_log_type libADV_log_instance;
static uint8_t VidValidCnt = 0;
static uint8_t VFreqRegVal = 0xFF;
static uint8_t last_hdmi_format = HDMI_INPUT_NOT_ACTIVE;
static uint8_t active_test_pattern = DEFAULT_HDMI_RES;
static char const *hdmi_format_strs[HDMI_RES_COUNT] = {	// Sequence MSU BE SAME with "eADV7611_MODEs" type definition //
	"HDMI_576i",
	"HDMI_480i",
	"HDMI_576p",
	"HDMI_480p",
	"HDMI_720p50",
	"HDMI_720p60",
	"HDMI_1080i50",
	"HDMI_1080i60",
	"HDMI_1080p50",
	"HDMI_1080p60",
	"AUTO_RESOLUTION",
	"NOT_ACTIVE",
	"INPUT_ERR",
	"NON_STANDARD"
};

static uint8_t __set_fr_colors(uint8_t chA, uint8_t chB, uint8_t chC);
static IF_t *__get_newAVI_infoframe(void);
static int8_t __hdmi_update_vid_timings(void);
static int16_t __ADV7611_initEDID(void);
static int16_t __hdmi_set_power(uint8_t state);	// param : TRUE if power-ON, FALSE for power-OFF //
static int8_t __ADV7611_setFR_test(uint8_t vid_std, uint8_t op_format_sel);
static int8_t __hdmi_check_audio_regs(void);
static uint8_t __hdmi_set_test_pattern(eADV7611_MODEs new_pattern);

static const vidstd_type vidstd[] =
{
//	vidstd, PR, interlaced, hor, ver, v_freq //
/*0*/	{0xA, 0, FALSE, 720, 480, 0},		// PR:0, progressive, 720x480p @ 60Hz //
/*1*/	{0x13, 0, FALSE, 1280, 720, 0},		// PR:0, progressive, 1280x720p @ 60Hz //
/*2*/	{0x14, 0, TRUE, 1920, 1080, 0},		// PR:0, interlaced, 1920x1080i @ 60Hz //
/*3*/	{0x0, 1, TRUE, 720, 480, 0},		// PR:1, interlaced, 720(1440)x480i @ 60Hz //
/*4*/	{0x0, 3, TRUE, 720, 480, 0},		// PR:3, interlaced, 720(2880)x480i @ 60Hz //
/*5*/	{0xA, 1, FALSE, 720, 480, 0},		// PR:1, progressive, 720(1440)x480p @ 60Hz //
/*6*/	{0x1E, 0, FALSE, 1920, 1080, 0},	// PR:0, progressive, 1920x1080p @ 60Hz //
/*7*/	{0xB, 0, FALSE, 720, 576, 0},		// PR:0, progressive, 720x576p @ 60Hz //
/*8*/	{0xA3, 0, FALSE, 1280, 720, 0x1},	// PR:0, progressive, 1280x720p @ 50Hz //
/*9*/	{0x14, 0, FALSE, 1920, 1080, 0x1},	// PR:0, interlaced, 1920x1080i @ 50Hz //
/*10*/	{0x1, 1, TRUE, 720, 576, 0},		// PR:1, interlaced, 720(1440)x576i @ 60Hz //
/*11*/	{0x1, 3, TRUE, 720, 480, 0},		// PR:3, interlaced, 720(2880)x480i @ 60Hz //
/*12*/	{0xA, 1, FALSE, 720, 576, 0},		// PR:1, progressive, 720(1440)x576p @ 60Hz //
/*13*/	{0x1E, 0, FALSE, 1920, 1080, 0x1},	// PR:1, v_freq=1, 1920x1080p @ 50Hz //
/*14*/	{0x1E, 0, FALSE, 1920, 1080, 0x4},	// PR:0, v_freq=0x4, 1920x1080p @ 24Hz //
/*15*/	{0x1E, 0, FALSE, 1920, 1080, 0x3},	// PR:0, v_freq=0x4, 1920x1080p @ 25Hz //
/*16*/	{0xA, 3, FALSE, 720, 480, 0},		// PR:0, progressive, 2880x480p @ 60Hz //
/*17*/	{0xA, 3, FALSE, 720, 576, 0}		// PR:0, progressive, 720(2880)x576p @ 60Hz //
};

// Functions //
int8_t set_free_run_video(eFR_MODEs fr_mode, eFR_SRCs fr_src, uint8_t fr_enable, uint8_t force, uint8_t fr_pattern, \
	uint8_t chA, uint8_t chB, uint8_t chC)
{
	int16_t ret;

	fr_mode %= FR_MODEs_COUNT;
	fr_src %= FR_SRC_COUNT;

	const char *modes[] = {"FR_WHEN_NO_TMDS_CLK", "FR_WHEN_NO_TMDS_and_MISMATCH"};
	const char *srcs[] = {"FR_FROM_INPUT_FORMAT", "FR_FROM_DESIRED_TEST_PATTERN"};
	TRACEL(libADV_log_instance, "%s()-> CALLED with fr_mode(%s), fr_src(%s), fr_enable(%s), force(%s), fr_pattern(%s), color(0x%02X,0x%02X,0x%02X)\n", \
		__FUNCTION__, modes[fr_mode], srcs[fr_src], (TRUE == fr_enable)?("ENABLE"):("DISABLE"), (TRUE == force)?("TRUE"):("FALSE"), \
				hdmi_format_strs[fr_pattern]);

	__set_fr_colors(chA, chB, chC);

	__hdmi_set_test_pattern(fr_pattern);	// FREE-RUN will output from TEST-PATTERN ALL TIMEs //
	if(FR_FROM_INPUT_FORMAT == fr_src) {	// FR output will be determined from buffered input format values //
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC9, (0x1<<0), set_free_run_video_FAILED);
	} else if(FR_FROM_DESIRED_TEST_PATTERN == fr_src) {
		TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC9, (0x1<<0), set_free_run_video_FAILED);
	} else {
		ERRL(libADV_log_instance, "UNEXPECTED FR SOURCE(%u)\n", fr_src);
	}

	// Set FR mode //
	DEBUGL(libADV_log_instance, "requested fr_mode(%u)\n", fr_mode);
	if(FR_WHEN_NO_TMDS_and_MISMATCH == fr_mode)	{
		TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBA, (0x1<<1), set_free_run_video_FAILED);
	}
	else if(FR_WHEN_NO_TMDS_CLK == fr_mode) {
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBA, (0x1<<1), set_free_run_video_FAILED);
	} else {
		ERRL(libADV_log_instance, "UNEXPECTED FR MODE(%u)\n", fr_mode);
	}

	// Set FR enable & disable //
	TRACEL(libADV_log_instance, "requested fr_enable(%u)\n", fr_enable);
	fr_enable &= 0x01;
	if(TRUE == fr_enable)	{	// ENABLE FREE-RUN MODE //
		TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBA, (0x1<<0), set_free_run_video_FAILED);
	}
	else {	// DISABLE FREE-RUN MODE //
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBA, (0x1<<0), set_free_run_video_FAILED);
	}

	// Handle Force to FREERUN request //
	if(TRUE == force) {
		TRACEL(libADV_log_instance, "FR FORCE requested\n");
		TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBF, (0x1<<0), set_free_run_video_FAILED);
	}
	else {
		TRACEL(libADV_log_instance, "FR FORCE NOT requested\n");
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBF, (0x1<<0), set_free_run_video_FAILED);
	}

	return 0;

set_free_run_video_FAILED:
	ERRL(libADV_log_instance, "set_free_run_video() FAILED\n");
	return -1;
}

static uint8_t __set_fr_colors(uint8_t chA, uint8_t chB, uint8_t chC)
{
	TRACEL(libADV_log_instance, "%s()-> CALLED\n", __FUNCTION__);
	// Set channel color values //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC0, chA, set_FR_colors_FAILED);	// Y
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC1, chB, set_FR_colors_FAILED);	// Pb
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC2, chC, set_FR_colors_FAILED);	// Pr

	// Set override default color bit "CP map, 0xBF:2" //
	TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBF, (0x1<<2), set_FR_colors_FAILED);
	// Enable DEFAULT Colors when FREE-RUN //
	TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBF, (0x1<<1), set_FR_colors_FAILED);

	TRACEL(libADV_log_instance, "%s()-> SUCCESS\n", __FUNCTION__);
	return TRUE;

set_FR_colors_FAILED:
	ERRL(libADV_log_instance, "%s()-> FAILED\n", __FUNCTION__);
	return FALSE;
}

// returns TRUE if video is active, FALSE o.w. //
int8_t is_hdmi_video_active(void)
{
	if((TRUE == vid_timing.hor_timing.de_regen_locked) && (TRUE == vid_timing.hor_timing.de_regen_raw) &&
	#if(0)
		// VERTICAL filters are not stable to check and so disabled //
		(TRUE == vid_timing.ver_timing.v_locked_raw) && (TRUE == vid_timing.ver_timing.vert_filter_locked))
	#else
		// IF CP status is NORMAL operation this points that input is connected and there is valid signal at HDMI input //
		(CP_NORMAL_OP == vid_timing.cp_status))
	#endif
	{
		INFOL(libADV_log_instance, "HDMI INPUT LOCKED waiting for stabilization\n");
		INFOL(libADV_log_instance, "VID VALID COUNTER(%u) OK\n", VidValidCnt);
		if((VIDVALID_CNT_MAX != VidValidCnt) && (VIDVALID_CNT_MAX != (++VidValidCnt)))
			return vid_timing.video_valid = FALSE;	//
		else {
			return vid_timing.video_valid = TRUE;
		}
	}
	else {
		INFOL(libADV_log_instance, "HDMI INPUT NOT LOCKED\n");
		vid_timing.video_valid = FALSE;
		VidValidCnt = 0;
	}

	return vid_timing.video_valid = FALSE;
}

static int8_t __ADV7611_setVFreq(uint8_t vFreqLocal)
{
	switch(vFreqLocal) {
		case 60:
			VFreqRegVal = 0;
			break;
		case 50:
			VFreqRegVal = 1;
			break;
		case 30:
			VFreqRegVal = 2;
			break;
		case 25:
			VFreqRegVal = 3;
			break;
		case 24:
			VFreqRegVal = 4;
			break;
		default:
			ERRL(libADV_log_instance, "UNEXPECTED PARAM(%u)\n", vFreqLocal);
			break;
	}

	{	// Update V_FREQ //
		int16_t regval;
		// Read value of register (IOMAP, adr:0x01) //
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, regval, IO_MAP_ADR, 0x1, ADV7611_setVFreq_FAILED);
		regval &= (~(VFREQ_MASK<<VFREQ_INDX));
		#if(0)
			regval |= ((vidstd[matched].v_freq)<<VFREQ_INDX);	// Set frequency in the table //
		#else
			regval |= (VFreqRegVal<<VFREQ_INDX);	// Set desired frequency //
		#endif
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x01, regval, ADV7611_setVFreq_FAILED);
	}
	return TRUE;

ADV7611_setVFreq_FAILED:
	ERRL(libADV_log_instance, "ADV7611_setVFreq() FAILED\n");
	return FALSE;
}

static uint8_t __get_format_from_regs(void)
{
	uint8_t retval = HDMI_INPUT_NOT_ACTIVE;
	// determine HDMI input resolution by using VIDEO DETAILs from REGISTERs (NOT CERTAIN) //
	if((vid_timing.ver_timing.active_height > 470) && (vid_timing.ver_timing.active_height < 490)) { // horizontal is 480 lines //
		if(FALSE == vid_timing.interlaced)
			retval = HDMI_480p;	// 480p resolution detected //
		else
			retval = HDMI_480i;	// 480i resolution detected //
	}
	else if((vid_timing.ver_timing.active_height > 560) && (vid_timing.ver_timing.active_height < 590)) { // horizontal is 576 lines //
		if(FALSE == vid_timing.interlaced)
			retval = HDMI_576p;	// 576p resolution detected //
		else
			retval = HDMI_576i;	// 576i resolution detected //
	}
	else if((vid_timing.ver_timing.active_height > 700) && (vid_timing.ver_timing.active_height < 740)) { // horizontal is 720 lines //
		retval = HDMI_720p60;	// 720p50/720p60 resolution detected //
	}
	else if((vid_timing.ver_timing.active_height > 1060) && (vid_timing.ver_timing.active_height < 1100)) { // horizontal is 1080 lines //
		if(FALSE == vid_timing.interlaced)
			retval = HDMI_1080p50;	// 1080p50/1080p60 resolution detected //
		else
			retval = HDMI_1080i50;	// 1080i50/1080i60 resolution detected //
	}
	else {
		retval = HDMI_RES_NON_STANDARD;
	}

	DEBUGL(libADV_log_instance, "HDMI input format from REGISTERs (%u)\n", retval);

	return retval;
}

/* Read input format details from AVI INFO FRAME */
static uint8_t __get_format_from_IF(uint8_t max_try, uint32_t delayUS)
{
	volatile uint8_t indx = 0, retval = HDMI_INPUT_NOT_ACTIVE;
	HDMIrx_VidFrm_t format = eHDMIrx_VidFrm_None;

	// Determine HDMI input format by using AVI INFO FRAME DATA (for EXACT RESULTs) //
	for(indx=0 ; indx<max_try ; indx++)
	{
		IF_t *infop = __get_newAVI_infoframe();
		if(NULL != infop) {
			format = HDMIrx_getVideoFormat(infop);
			DEBUGL(libADV_log_instance, "DETECTED HDMIrx_VidFrm_t(%u)\n", format);
		} else {
			usleep(delayUS);
			continue;
		}
		IF_Delete(infop);	// Delete created info-frame header and data //
		break;
	}
	if(indx == max_try) {	// MAX AVI-INFO FRAME reading trial count reached //
		ERRL(libADV_log_instance, "AVI INFO FRAME read FAILED\n");
		retval = 0xFF;	// This method FAILED //
	}
	else {	// INFO-FRAME read and now analyse it //
		switch(format) {
			case eHDMIrx_VidFrm_576i50:
				retval = HDMI_576i;	// 576p resolution detected //
				break;
			case eHDMIrx_VidFrm_576p50:
				retval = HDMI_576p;	// 576p resolution detected //
				break;
			case eHDMIrx_VidFrm_480i60:
				retval = HDMI_480i;	// 480i resolution detected //
				break;
			case eHDMIrx_VidFrm_480p60:
				retval = HDMI_480p;	// 480p resolution detected //
				break;
			case eHDMIrx_VidFrm_720p50:
				retval = HDMI_720p50;	// 720p50 resolution detected //
				break;
			case eHDMIrx_VidFrm_720p60:
				retval = HDMI_720p60;	// 720p60 resolution detected //
				break;
			case eHDMIrx_VidFrm_1080i50:
				retval = HDMI_1080i50;	// 1080i50 resolution detected //
				break;
			case eHDMIrx_VidFrm_1080p50:
				retval = HDMI_1080p50;
				break;
			case eHDMIrx_VidFrm_1080i60:
				retval = HDMI_1080i60;	// 1080i60 resolution detected //
				break;
			case eHDMIrx_VidFrm_1080p60:
				retval = HDMI_1080p60;
				break;
			case eHDMIrx_VidFrm_UnKnown:
				retval = HDMI_RES_NON_STANDARD;
				break;
			case eHDMIrx_VidFrm_None:
			default:
				retval = HDMI_INPUT_NOT_ACTIVE;
				break;
		}
	}

	DEBUGL(libADV_log_instance, "HDMI input format from AVI INFO FRAME (%u)\n", retval);
	return retval;
}

uint8_t ADV7611_get_input_res(uint8_t max_try, uint8_t delayMS)
{
	volatile uint8_t indx;

	uint32_t delayUS = (uint32_t)delayMS * 1000UL;
	int8_t lock_state = 1;	// NOT ACTIVE INPUT is DEFAULT VALUE //
	for(indx=0 ; indx<max_try ; indx++) {	// Do iteration until timeout //
		lock_state = __hdmi_update_vid_timings();
		if((0 == lock_state) || (0 > lock_state))	// Wait until error(lock_state < 0) or input valid(lock_state == 0) sensed //
			break;
		usleep(delayUS);
	}
	if(0 == lock_state) {	// HDMI input LOCKED //
#if(0)
		last_hdmi_format = __get_format_from_regs();
#else
		uint8_t temp_format = __get_format_from_IF(max_try, delayUS);
		if(0xFF == temp_format) {
			last_hdmi_format = __get_format_from_regs();
		} else last_hdmi_format = temp_format;
#endif
	}
	else if(0 > lock_state) {
		ERRL(libADV_log_instance, "%s()-->> FAILED\n", __FUNCTION__);
		last_hdmi_format = HDMI_INPUT_ERR;
	}
	else {	// "ret > 0", hdmi is NOT LOCKED, input is NOT VALID //
		last_hdmi_format = HDMI_INPUT_NOT_ACTIVE;
	}

	return last_hdmi_format;
}

// Return Value:
	// "-1" if error //
	// "1" if video is not-valid //
	// "0" if video is valid //
static int8_t __hdmi_update_vid_timings(void)
{
	int16_t ret;
	uint8_t changed = FALSE;
	int32_t oldval, valL;
	uint8_t log_val = 0;

	#if(1)
	// Check CP core status //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, CP_MAP_ADR, 0xFF, update_vid_timings_FAIL);
	if(ret & (0x01<<4)) {
		INFOL(libADV_log_instance, "CP core is FREE_RUN\n");
		vid_timing.cp_status = CP_FREE_RUN;
	} else {
		INFOL(libADV_log_instance, "CP_core normal operation\n");
		vid_timing.cp_status = CP_NORMAL_OP;
	}
	#endif

	// Update video lock bit states //
	// Read DE_REGEN_FILTER_LOCKED; HDMI MAP, 0x07[5] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x07, update_vid_timings_FAIL);
	if(ret & ((uint8_t)0x1<<5)) {
		vid_timing.hor_timing.de_regen_locked = TRUE;
		log_val |= (0x1<<0);
	} else
		vid_timing.hor_timing.de_regen_locked = FALSE;

	#if(0)
		// This bit is NOT stable according to input status, it is NOT set even input is OK //
			// end so disabled //
		// Read VERT_FILTER_LOCKED; HDMI MAP, 0x07[7] //
		if(ret & ((uint8_t)0x1<<7)) {
			vid_timing.ver_timing.vert_filter_locked = TRUE;
			log_val |= (0x1<<1);
		} else
			vid_timing.ver_timing.vert_filter_locked = FALSE;
	#endif

	// Read DE_REGEN_LCD_RAW; IO MAP, 0x6A[0] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, IO_MAP_ADR, 0x6A, update_vid_timings_FAIL);
	if(ret & ((uint8_t)0x1<<0)) {
		vid_timing.hor_timing.de_regen_raw = TRUE;
		log_val |= (0x1<<2);
	} else
		vid_timing.hor_timing.de_regen_raw = FALSE;
	#if(0)
		// This bit is NOT stable according to input status, it is NOT set even input is OK //
			// end so disabled //
		// Read V_LOCKED_RAW; IO MAP, 0x6A[1] //
		if(ret & ((uint8_t)0x1<<1)) {
			vid_timing.ver_timing.v_locked_raw = TRUE;
			log_val |= (0x1<<3);
		} else
			vid_timing.ver_timing.v_locked_raw = FALSE;
	#endif

	DEBUGL(libADV_log_instance, "log_val = 0x%02X\n", log_val);
	// Return if there is NO LOCK //
	if(FALSE == is_hdmi_video_active())
		return 1;

	oldval = vid_timing.hor_timing.tot_line_width;
	// Read active vertical & horizontal values and interlaced/progressive state //
	// Read total line width(horizontal); total 14bits length, HDMI MAP 0x08[7:0] & 0x07[4:0] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x1F, update_vid_timings_FAIL);
	valL = ret & 0x00FF;
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x1E, update_vid_timings_FAIL);
	ret &= 0x3F;	// Only first 6bits are valid //
	vid_timing.hor_timing.tot_line_width = ((int32_t)ret*256) + valL;
	INFOL(libADV_log_instance, "total_line_length(%d)\n", vid_timing.hor_timing.tot_line_width);
	if(oldval != vid_timing.hor_timing.tot_line_width){
		INFOL(libADV_log_instance, "total_line_length CHANGED\n");
		changed = TRUE;
	}
	else {
		TRACEL(libADV_log_instance, "total_line_length NOT-CHANGED\n");
	}

	oldval = vid_timing.hor_timing.active_line_width;
	// Read active vertical & horizontal values and interlaced/progressive state //
	// Read linewidth(horizontal); total 13bits length, HDMI MAP 0x08[7:0] & 0x07[4:0] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x08, update_vid_timings_FAIL);
	valL = ret & 0xFF;
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x07, update_vid_timings_FAIL);
	ret &= 0x1F;	// Only first 5bits are valid //
	vid_timing.hor_timing.active_line_width = ((int32_t)ret*256) + valL;
	INFOL(libADV_log_instance, "active_line_length(%d)\n", vid_timing.hor_timing.active_line_width);
	if(oldval != vid_timing.hor_timing.active_line_width){
		INFOL(libADV_log_instance, "active_line_length CHANGED\n");
		changed = TRUE;
	}
	else {
		TRACEL(libADV_log_instance, "active_line_length NOT-CHANGED\n");
	}

	oldval = vid_timing.interlaced;
	// Read Interlaced / Progressive State //
	// Read V_LOCKED_RAW; IHDMI MAP, 0x0B[5] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x0B, update_vid_timings_FAIL);
	if(ret & ((uint8_t)0x1<<5))
		vid_timing.interlaced = TRUE;
	else
		vid_timing.interlaced = FALSE;
	INFOL(libADV_log_instance, "Video is %s\n", (TRUE == vid_timing.interlaced)?("INTERLACED"):("PROGRESSIVE"));
	if(oldval != vid_timing.interlaced) {
		INFOL(libADV_log_instance, "interlaced/progressive CHANGED\n");
		changed = TRUE;
	}
	else {
		TRACEL(libADV_log_instance, "interlaced/progressive NOT-CHANGED\n");
	}

	oldval = vid_timing.ver_timing.active_height;
	// Read Field0-Height; total 13bits length, HDMI MAP 0x0A[7:0] & 0x09[4:0] //
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x0A, update_vid_timings_FAIL);
	valL = ret & 0xFF;
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x09, update_vid_timings_FAIL);
	ret &= 0x1F;	// Only first 5bits are valid //
	vid_timing.ver_timing.active_height = ((int32_t)ret*256) + valL;
	INFOL(libADV_log_instance, "Field0's height(%d)\n", vid_timing.ver_timing.active_height);
	// Additional processing for interlaced input //
	if(TRUE == vid_timing.interlaced) {
		int32_t Field1;
		// Add odd & even (Field0 & Field1) heights to get total height //
		// Read Field1-Height; total 13bits length, HDMI MAP 0x0C[7:0] & 0x0B[4:0] //
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x0C, update_vid_timings_FAIL);
		valL = ret & 0xFF;
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x0B, update_vid_timings_FAIL);
		ret &= 0x1F;	// Only first 5bits are valid //
		Field1 = ((int32_t)ret*256) + valL;
		INFOL(libADV_log_instance, "Field1's height(%d)\n", Field1);

		vid_timing.ver_timing.active_height += Field1;
	}
	if(oldval != vid_timing.ver_timing.active_height) {
		INFOL(libADV_log_instance, "active_height CHANGED\n");
		changed = TRUE;
	}
	else {
		TRACEL(libADV_log_instance, "active_height NOT-CHANGED\n");
	}

	if(changed == TRUE) {
		volatile uint8_t indx;
		uint8_t matched = 0xFF;
		for(indx=0 ; indx<ArraySize(vidstd) ; indx++) {
			// Compare active horizontal line width //
			//uint32_t TotLineWithd = (vidstd[indx].PR + 1) * vidstd[indx].horres;
			uint32_t TotLineWithd = vidstd[indx].horres;
			DEBUGL(libADV_log_instance, "Comparing ActiveLineWidth(%u) with (%u) @ indx(%u)\n", \
				vid_timing.hor_timing.active_line_width, TotLineWithd, indx);
			if(vid_timing.hor_timing.active_line_width == TotLineWithd)
				matched = indx;
			else {
				matched = 0xFF;
				continue;
			}
			// Compare Interlaced / Progressive //
			DEBUGL(libADV_log_instance, "Comparing Inter/Progress(%u) with (%u) @ indx(%u)\n", \
				vid_timing.interlaced, vidstd[indx].interlaced, indx);
			if(vid_timing.interlaced == vidstd[indx].interlaced)
				matched = indx;
			else {
				matched = 0xFF;
				continue;
			}

			// Compare active vertical height //
			DEBUGL(libADV_log_instance, "Comparing VerticalHeight(%u) with (%u) @ indx(%u)\n", \
					vid_timing.ver_timing.active_height, vidstd[indx].verres, indx);
			if(vid_timing.ver_timing.active_height == vidstd[indx].verres) {
				INFOL(libADV_log_instance, "MATCHED @ indx(%u), VID_STD(0x%02X), V_FREQ(0x%02X)\n", indx, vidstd[indx].vidstd, vidstd[indx].v_freq);
				matched = indx;
				break;
			}
			else
				matched = 0xFF;
		}
#if(0)
		if(0xFF != matched) {
			int16_t regval = 0;
			// 1- Update VID_STD (IOMAP, Adr:0x0)//
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0, vidstd[matched].vidstd, update_vid_timings_FAIL);
			// 2- Update V_FREQ //
			ADV7611_setVFreq(50);	// Default Value : 50 Hz //
			// 3- //
		} else {
			ON_FAILURE(libADV_log_instance, "VID_STD & V_FREQ setting NOT MATCHED\n",update_vid_timings_FAIL);
		}
#endif
	}

	return 0;	// Video Input locked and parameters updated //

update_vid_timings_FAIL:
	ERRL(libADV_log_instance, "libADV7611_update_vid_timings() FAILED\n");
	return -1;
}

// Eğer adv7611'i free run force moduna alacaksan öncelikle bu fonksiyonu çağır yoksa burdaki yazma işlmeleri fail oluyor. //
static int16_t __ADV7611_initEDID(void)
{
#define EDID_CMP_ADRs	9
	int16_t ret = 0;
	uint8_t *edid_ram = NULL;
	volatile uint16_t indx;
	FILE *edid_file_handle = NULL;
	uint8_t edid_cmp_adr[EDID_CMP_ADRs] = {0x23, 0x55, 0x71, 0xa0, 0xbd, 0xc0, 0xd6, 0xe9, 0xFF};
	uint8_t write_required = FALSE;

	if(NULL == (edid_ram = calloc(1, EDID_MEM_SIZE+10))) {
		ERRL(libADV_log_instance, "calloc for %u bytes FAILED (%s)\n", EDID_MEM_SIZE, strerror(errno));
		goto ADV7611_initEDID_FAILED;
	}

	edid_file_handle = fopen(EDID_FILE_NAME, "rb");
	if(NULL == edid_file_handle) {
		ERRL(libADV_log_instance, "Edid Image File(%s) Open FAILED (%s)\n", EDID_FILE_NAME, strerror(errno));
		goto ADV7611_initEDID_FAILED;
	}
	if(EDID_MEM_SIZE != fread(edid_ram, 1, EDID_MEM_SIZE, edid_file_handle)) {
		ERRL(libADV_log_instance, "fread for EDIDFile(%s) FAILED (%s)\n", EDID_FILE_NAME, strerror(errno));
		goto ADV7611_initEDID_FAILED;
	}

	if(0) {
		for(indx=0 ;; indx++) {
			printf("0x%02X ", edid_ram[indx]);
			if(((indx%16) == 15) && (0 != indx))
				printf("\n");
			if(0xFF == indx)
				break;
		}
	}

	// Check if edid is valid or not //
	if(1) {
		int16_t read_val = 0;
		volatile uint8_t addr;
		for(indx=0;indx<EDID_CMP_ADRs;indx++) {
			addr = edid_cmp_adr[indx];
			TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, read_val, EDID_MAP_ADR, edid_cmp_adr[indx], ADV7611_initEDID_FAILED);
			DEBUGL(libADV_log_instance, "COMPARING adr(0x%02X), org_val(0x%02X), read_val(0x%02X)\n", addr, edid_ram[addr], read_val);
			if(read_val != edid_ram[addr]) {
				DEBUGL(libADV_log_instance, "NOT SAME, WRITE REQUIRED\n");
				write_required = TRUE;
				break;
			}
			else {
				DEBUGL(libADV_log_instance, "SAME SKIPPING\n");
			}
		}
	}
	else
		write_required = TRUE;

	// Do real edid memory fill operation if required //
	if(TRUE == write_required) {
		uint8_t ErrCnt = 0;
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x6C, 0xA3, ADV7611_initEDID_FAILED);	 // enable manual HPA
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x20, 0x70, ADV7611_initEDID_FAILED); // HPD low
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, KVS_MAP_ADR, 0x74, 0x00, ADV7611_initEDID_FAILED); // disable internal EDID
		for (indx=0; indx<EDID_MEM_SIZE; indx++) {
			while(0 != libc2i2c_write_reg8(libADV_log_instance, I2C_BUS_LIBADV, EDID_MAP_ADR, indx, edid_ram[indx])) {
				if(3 == ErrCnt++)
					goto ADV7611_initEDID_FAILED;
				usleep(50000);
			}
			ErrCnt = 0;
			usleep(10000);	// Süre bundan kısa olursa yazma problemleri çıkabiliyor.
		}
		usleep(1000000);	// Dummy wait from original MCU code //
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, KVS_MAP_ADR, 0x74, 0x01, ADV7611_initEDID_FAILED); // enable internal EDID
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x20, 0xF0, ADV7611_initEDID_FAILED); // HPD high
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x6C, 0xA2, ADV7611_initEDID_FAILED); // disable manual HPA
	}

	goto ADV7611_initEDID_COMPLETED;
ADV7611_initEDID_FAILED:
	ret = -1;
ADV7611_initEDID_COMPLETED:

	if(NULL != edid_ram)	// cleaning //
		free(edid_ram);
	if(0 != ret) {
		ERRL(libADV_log_instance, "%s()-> FAILED\n", __FUNCTION__);
	}
	else {
		DEBUGL(libADV_log_instance, "%s()-> SUCCESS\n", __FUNCTION__);
	}
	return ret;
}

static int16_t __hdmi_set_power(uint8_t state)
{
	if(state == TRUE) {
		// Do power-on sequence //
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x48, 0x01, set_power_failed);	// Ring Osc Enabled //
		TRY_CMASK(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x73, 0x01, set_power_failed);	// DDC Pads Enabled //
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0C, 0x42, set_power_failed);	// RX Section @ PowerDown State (?) //
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CEC_MAP_ADR, 0x2A, 0x3E, set_power_failed);	// CEC Remains @ Disabled State //
		hdmi_init(HDMI_AUTO_RESOLUTION, NULL);	// Do register initialization from scratch //
	}
	else {
		// Do power-down sequence //
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0C, 0x62, set_power_failed);
		#if(ADV7611_POWER_DOWN_MODE == 0)
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CEC_MAP_ADR, 0x2A, 0x3E, set_power_failed);	// POWER_DOWN 0, CEC Disabled //
		#elif(ADV7611_POWER_DOWN_MODE == 1)
			TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CEC_MAP_ADR, 0x2A, 0x3F, set_power_failed);	// POWER_DOWN 1, CEC Enabled //
		#else
			#error ""
		#endif
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x48, 0x01, set_power_failed);	// Ring Osc Disabled //
		TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x73, 0x01, set_power_failed);	// DDC Pads Disabled //
	}

	return 0;

set_power_failed:
	ERRL(libADV_log_instance, "libADV7611_set_power() FAILED\n");
	return -1;
}

// This function NOT working for now //
static int8_t __ADV7611_setFR_test(uint8_t vid_std, uint8_t op_format_sel)
{
	DEBUGL(libADV_log_instance, "%s()-> CALLED\n", __FUNCTION__);
	DEBUGL(libADV_log_instance, "vid_std(0x%02X), op_format_sel(0x%02X)\n", vid_std, op_format_sel);
	libc2i2c_write_reg8(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFF, 0x80);	// After reset ADV7611 not responding for a while //
													// and so it seems that write operation FAILED but we should ignore //
	usleep(500000);	// 500 ms sleep //

	// Set memory map addresses //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF4, 0x80, ADV7611_setFR_test_FAILED);	// CEC //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF5, 0x7C, ADV7611_setFR_test_FAILED);	// INFOFRAME //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF8, 0x4C, ADV7611_setFR_test_FAILED);	// DPLL //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF9, 0x64, ADV7611_setFR_test_FAILED);	// KVS //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFA, 0x6C, ADV7611_setFR_test_FAILED);	// EDID //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFB, 0x68, ADV7611_setFR_test_FAILED);	// HDMI //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFD, 0x44, ADV7611_setFR_test_FAILED);	// CP //

	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x00, vid_std, ADV7611_setFR_test_FAILED);	// 1080i@50Hz
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x01, 0x15, ADV7611_setFR_test_FAILED);	// Prim-mode HDMI-COMP
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x03, op_format_sel, ADV7611_setFR_test_FAILED);
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x04, 0x62, ADV7611_setFR_test_FAILED);	// 28.6MHz, P[23:16]:x, P[15:8]:Y, P[7:0]:Cb/Cr
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x05, 0x3D, ADV7611_setFR_test_FAILED);	// AV Codes ON & FIELD output @ FILED/DE pin, BLANK data when HYSNC & VSYNC //
													// Invert Cb & Cr order required //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x06, 0xAF, ADV7611_setFR_test_FAILED);	// Positive VS & FIELD, Negative HS & LLC polarity //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0b, 0x44, ADV7611_setFR_test_FAILED);	// PowerUP
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0c, 0x42, ADV7611_setFR_test_FAILED);	// Power UP
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x14, 0x7f, ADV7611_setFR_test_FAILED);	// Max drive strength
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x15, 0x80, ADV7611_setFR_test_FAILED);	// Disable Tristate
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x19, 0x83, ADV7611_setFR_test_FAILED);	// LLC DLL Phase
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x33, 0x40, ADV7611_setFR_test_FAILED);	// LLC DLL Enable

	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xba, 0x01, ADV7611_setFR_test_FAILED);	// Enable Free Run
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xbf, 0x17, ADV7611_setFR_test_FAILED);	// Force FreeRun & Manual Color
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xc0, FR_YELLOW_CHA, ADV7611_setFR_test_FAILED);	// Green Color
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xc1, FR_YELLOW_CHB, ADV7611_setFR_test_FAILED);	// Red Color
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xc2, FR_YELLOW_CHC, ADV7611_setFR_test_FAILED);	// Blue Color
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xc9, 0x2D, ADV7611_setFR_test_FAILED);	// Disable auto buffering

	DEBUGL(libADV_log_instance, "%s()-> SUCCESS\n\n", __FUNCTION__);
	return 0;

ADV7611_setFR_test_FAILED:
	ERRL(libADV_log_instance, "%s()-> FAILED\n\n", __FUNCTION__);
	return -1;
}

static int8_t __hdmi_check_audio_regs(void)
{
	int16_t ret = 0;
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, IO_MAP_ADR, 0x65, ADV7611_check_audio_regs);
	DEBUGL(libADV_log_instance, "IO_MAP(0x65) = 0x%02X\n", ret);
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x04, ADV7611_check_audio_regs);
	DEBUGL(libADV_log_instance, "HDMI_MAP(0x04) = 0x%02X\n", ret);
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x07, ADV7611_check_audio_regs);
	DEBUGL(libADV_log_instance, "HDMI_MAP(0x07) = 0x%02X\n", ret);

	return 0;

ADV7611_check_audio_regs:
	ERRL(libADV_log_instance, "%s()-> FAILED\n");
	return -1;
}

static uint8_t __hdmi_set_test_pattern(eADV7611_MODEs new_pattern)
{
	uint8_t vid_std_reg_val;
	uint8_t vfreq = 50;

	DEBUGL(libADV_log_instance, "New HDMI Patern(0x%02X) to set\n", new_pattern);
	switch(new_pattern) {	// Values received from "static const vidstd_type vidstd[]" table above //
		case HDMI_480i:
			vid_std_reg_val = 0x00;	/*:TODO: Testing required*/
			vfreq = 30;	/* :TODO: Doğruluğunu kontrol et */
			break;
		case HDMI_576i:
			vid_std_reg_val = 0x01;	/*:TODO: Testing required*/
			vfreq = 25;	/* :TODO: Verify this by testing */
			break;
		case HDMI_480p:
			vid_std_reg_val = 0x0A;	/*OK*/
			vfreq = 60;
			break;
		case HDMI_720p50:
			vfreq = 50;
			vid_std_reg_val = 0x13;
			break;
		case HDMI_720p60:
			vid_std_reg_val = 0x13;
			vfreq = 60;
			break;
		case HDMI_1080i50:
			vid_std_reg_val = 0x14;
			vfreq = 50;
			break;
		case HDMI_1080i60:
			vid_std_reg_val = 0x14;
			vfreq = 60;
			break;
		case HDMI_1080p50:
			vid_std_reg_val = 0x1E;
			vfreq = 50;
			break;
		case HDMI_1080p60:
			vid_std_reg_val = 0x1E;
			vfreq = 60;
			break;
		default:
			ERRL(libADV_log_instance, "UNEXPECTED HDMI Pattern (%u) to set\n", new_pattern);
		case HDMI_576p:
			vfreq = 50;
			vid_std_reg_val = 0x0B;	/*OK*/
			break;
	}
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0, vid_std_reg_val, hdmi_set_test_pattern_FAILED);
	__ADV7611_setVFreq(vfreq);

	active_test_pattern = new_pattern;

	return TRUE;

hdmi_set_test_pattern_FAILED:
	ERRL(libADV_log_instance, "I2C IO error @ hdmi_set_test_pattern(0x%02X)\n", new_pattern);
	return FALSE;
}

int8_t hdmi_init(uint8_t modeHDMI, uint8_t *test_pattern_state_ptr)
{
	int8_t retval = DEFAULT_HDMI_RES;

	//return ADV7611_setFR_test(0x14, 0x81);	// 20Bit SDR ITU-656, 1080i@50Hz //

#if(ADV7611_INITIAL_RESET == TRUE)
	// Reset I2C Registers of ADV7611 //
	libc2i2c_write_reg8(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFF, 0x80);	// After reset ADV7611 not responding for a while //
													// and so it seems that write operation FAILED but we should ignore //
	usleep(500000);	// 500 ms sleep //
#endif
	// Set memory map addresses //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF4, 0x80, ADV7611_init_FAIL);	// CEC //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF5, 0x7C, ADV7611_init_FAIL);	// INFOFRAME //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF8, 0x4C, ADV7611_init_FAIL);	// DPLL //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xF9, 0x64, ADV7611_init_FAIL);	// KVS //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFA, 0x6C, ADV7611_init_FAIL);	// EDID //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFB, 0x68, ADV7611_init_FAIL);	// HDMI //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0xFD, 0x44, ADV7611_init_FAIL);	// CP //

	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x00, 0x14, ADV7611_init_FAIL);	// Hardware Manual @ Page 23, PRIM_MODE = 0x06, V_FREQ = 0 //
#if(0)
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x01, 0x06, ADV7611_init_FAIL);	// Hardware Manual @ Page 23, PRIM_MODE = 0x06, V_FREQ = 0 //
#else
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x01, 0x15, ADV7611_init_FAIL);	// HDMI Component Mode(Video çözünürlükleri desteği) & 50Hz FreeMode Freq. //
															// Scriptlerde HDMI-graphics moduna ayarlanmış Çünkü o proje bilgisayar çözünürlükleri üzerine //
																// biz video çözünürlükleri ile uğraşıyoruz.
#endif
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x02, 0xF4, ADV7611_init_FAIL);	// Input ColorSpace from HDMI-Block,  Auto CSC, YCrCb out, Set op_656 bit //
#if(1)
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x03, 0x81, ADV7611_init_FAIL);	// 20 bit ITU-656 SDR mode //
#else
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x03, 0x91, ADV7611_init_FAIL);	// 20 bit SDR 4:2:2 mode 4 //
#endif
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x04, 0x62, ADV7611_init_FAIL);	// 28.6MHz, P[23:16]:x, P[15:8]:Y, P[7:0]:Cb/Cr
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x05, 0x3C, ADV7611_init_FAIL);	// AV Codes ON & FIELD output @ FILED/DE pin, BLANK data when HYSNC & VSYNC //
																// Invert Cb & Cr order required //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x06, 0xAF, ADV7611_init_FAIL);	// Positive VS & FIELD, Negative HS & LLC polarity //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0B, 0x44, ADV7611_init_FAIL);	// Power up part //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x0C, 0x42, ADV7611_init_FAIL);	// Power up part //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x15, 0xA0, ADV7611_init_FAIL);	// Enable All Pins but DEFAULTs set //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x19, 0x8A, ADV7611_init_FAIL);	// LLC DLL phase //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x14, 0x7F, ADV7611_init_FAIL);	// Max Drive Strength //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x33, 0x40, ADV7611_init_FAIL);	// LLC DLL enable //
	// CP MAP SETTINGs //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xBA, 0x01, ADV7611_init_FAIL);	// ENABLE HDMI FreeRun, FreeRun if no TMDS clock @ HDMI port //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0xC9, 0x2C, ADV7611_init_FAIL);	// Not swap in DDR mode, free run params from last received HDMI //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, CP_MAP_ADR, 0x6C, 0x00, ADV7611_init_FAIL);	// ADI recommended setting //
	// CP:0x7B ve 0x7C registerlarında ile F,V,S ve SAV, EAV ile ilgili kritik ayarlar var //
	// KVS MAP SETTINGs //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, KVS_MAP_ADR, 0x40, 0x81, ADV7611_init_FAIL);	// Disable HDCP 1.1 features //
	// HDMI MAP SETTINGs //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x9B, 0x03, ADV7611_init_FAIL);	// ADI recommended setting //
#if(HDMI_NON_FAST_SWITCHING == TRUE)
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC1, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC2, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC3, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC4, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC5, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC6, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC7, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC8, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xC9, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xCA, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xCB, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0xCC, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
#endif
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x0, 0x0, ADV7611_init_FAIL);	// Set HDMI Input Port A //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x83, 0xFE, ADV7611_init_FAIL);	// SEnable clock terminator for port A //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x6F, 0x0C, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x85, 0x1F, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x87, 0x70, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x8D, 0x04, ADV7611_init_FAIL);	// LFG for above 480/576p //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x8E, 0x1E, ADV7611_init_FAIL);	// HFG for above 480/576p //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x1A, 0x8A, ADV7611_init_FAIL);	// unmute audio //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x57, 0xDA, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x58, 0x01, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x4C, 0x44, ADV7611_init_FAIL);	// ADI recommended setting //
	TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, HDMI_MAP_ADR, 0x75, 0x10, ADV7611_init_FAIL);	// DDC drive strength //

	// Init EDID Memory //
	if(0 != __ADV7611_initEDID())		// Bu fonksiyonu eğer free run FORCE etmeden sonra cagirirsan yazma işlemi yapılamıyor //
		goto ADV7611_init_FAIL;

	// Clear TEST PATTERN related setting to default HDMI input required values //
	set_free_run_video(FR_WHEN_NO_TMDS_CLK, FR_FROM_INPUT_FORMAT, TRUE, FALSE, DEFAULT_HDMI_RES, \
			FR_YELLOW_CHA, FR_YELLOW_CHB, FR_YELLOW_CHC);

	// Check HDCP status of INPUT //
	{
		int16_t ret;
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, HDMI_MAP_ADR, 0x05, ADV7611_init_FAIL);
		if(ret & (0x1<<6)) {
			ERRL(libADV_log_instance, "------- HDCP Encrypted CONTENT DETECTED :: ATTENTION, THERE IS NO HDCP SUPPORT -------\n");
		} else {
			INFOL(libADV_log_instance, "NO HDCP CONTENT DETECTED\n");
		}

	}

	// DETECT HDMI resolution if auto resolution desired //
	if(HDMI_AUTO_RESOLUTION == modeHDMI) {
		volatile uint8_t indx;
		DEBUGL(libADV_log_instance, "HDMI AUTO RESOLUTION REQUEST detected\n");
		for(indx=0 ; indx<10 ; indx++)
			usleep(100000U);	// Wait for 1 seconds //
		retval = ADV7611_get_input_res(ADV7611_RES_DETECT_MAX_TRY, ADV7611_RES_DETECT_STEP_INTERVAL_MS);
		DEBUGL(libADV_log_instance, "___________DETECTED HDMI RESOLUTION(%d) => \"%s\"___________\n", retval, hdmi_format_strs[retval]);
		if(HDMI_AUTO_RESOLUTION < retval) {
			if(HDMI_INPUT_ERR == retval)	// If resolution sense FAILED //
				goto ADV7611_init_FAIL;
			else if	(HDMI_INPUT_NOT_ACTIVE == retval) { // HDMI_RES_NON_STANDARD, HDMI_AUTO_RESOLUTION or HDMI_INPUT_NOT_ACTIVE //
				retval = active_test_pattern;
				if(NULL != test_pattern_state_ptr) {
				    INFOL(libADV_log_instance, "ACTIVE TEST PATTERN detected for HDMI\n");
				    *test_pattern_state_ptr = TRUE;
				}
			}
			else {
				ERRL(libADV_log_instance, "--- NON STANDARD HDMI INPUT DETECTED ----\n");
				goto ADV7611_init_FAIL;
			}
		} else {
			if(HDMI_1080p60 == retval) {
				ERRL(libADV_log_instance, "---1080p60 input resolution is NOT SUPPORTED----\n");
				goto ADV7611_init_FAIL;
			}
			else if(HDMI_1080p50 == retval) {
				ERRL(libADV_log_instance, "------1080p50 input resolution is NOT SUPPORTED-----\n");
				goto ADV7611_init_FAIL;
			}
			else if((HDMI_576i == retval) || (HDMI_480i == retval)) {
				INFOL(libADV_log_instance, "SD input format detected, changing output pins structure\n");
				TRY_WRITE(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x03, 0x00, ADV7611_init_FAIL);	// 8 bit ITU-656 SDR mode //
				TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x19, (0x3<<6), ADV7611_init_FAIL);	// 8 bit ITU-656 SDR mode //
				TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x33, (0x1<<6), ADV7611_init_FAIL);	// 8 bit ITU-656 SDR mode //
			}
		}
		__hdmi_set_test_pattern(retval);
	}

	DEBUGL(libADV_log_instance, "ADV7611_init() COMPLETED SUCCESFULLY\n\n");
	return retval;

ADV7611_init_FAIL:
	ERRL(libADV_log_instance, "ADV7611_init() FAILED\n\n");
	return retval = -1;
}

IF_t *__get_newAVI_infoframe(void)
{
	volatile uint8_t indx;
	IF_t *info_frame_p = NULL;

	if(NULL == info_frame_p) {
		if(NULL == (info_frame_p = IF_New(IF_TYPE_AVI))) {
			ERRL(libADV_log_instance, "NEW AVI INFOFRAME creation FAILED\n");
			return NULL;
		}
	}
	UInt8 *Payload = IF_GetPayload(info_frame_p);

	int16_t ret;
	TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, IO_MAP_ADR, 0x79, __get_newAVI_infoframe_FAILED);
	if(!(ret & 0x1)) {
		ERRL(libADV_log_instance, "New AVI-Info Frame NOT RECEIVED\n");
		return NULL;	// New AVI Info-Frame not received //
	}
	// Clear NEW INFO-Frame Flag //
	TRY_SMASK(libADV_log_instance, I2C_BUS_LIBADV, IO_MAP_ADR, 0x7B, 0x01, __get_newAVI_infoframe_FAILED);
	// Read all bytes of Info-Frame //
	INFOL(libADV_log_instance, "Reading AVI INFO Frame\n");
	for(indx=0 ; indx<AVI_MAX_LEN; indx++) {
		TRY_READ(libADV_log_instance, I2C_BUS_LIBADV, ret, INFOFRAME_MAP_ADR, indx, __get_newAVI_infoframe_FAILED);
		Payload[indx] = ret;
		TRACEL(libADV_log_instance, "IF-DATA[%u] : 0x%02X\n", indx, Payload[indx]);
	}

	return info_frame_p;

__get_newAVI_infoframe_FAILED:
	ERRL(libADV_log_instance, "__get_newAVI_infoframe() FAILED \n");
	return NULL;
}

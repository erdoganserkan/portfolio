#ifndef SYS_LOADING_H
#define SYS_LOADING_H

#include <stdio.h>
#include <stdint.h>
#include "UMDShared.h"

#define SYS_LOAD_ICON_COUNT			8
#define SYS_LOAD_RECTANGLE_ROUND	10
#define SYS_LOAD_ANIM_MS			(300)
#define SYS_LOAD_TOTAL_STEPs		(SYS_LOAD_TOTAL_LENGHT_MS / SYS_LOAD_ANIM_MS)	// 40
#define SYS_LOAD_ICON_STEP			(SYS_LOAD_TOTAL_STEPs / SYS_LOAD_ICON_COUNT)	// 5

#define SYS_LOAD_LBAR_LEFT_X	10
#define SYS_LOAD_LBAR_LEFT_Y	10
#define SYS_LOAD_RBAR_LEFT_X	397
#define SYS_LOAD_RBAR_LEFT_Y	10
#define SYS_LOAD_BAR_SIZE_X		70
#define SYS_LOAD_BAR_SIZE_Y		250

#define SYS_LOAD_STR_LEFT_X		107
#define SYS_LOAD_STR_LEFT_Y		3
#define SYS_LOAD_STR_RIGHT_X	365
#define SYS_LOAD_STR_RIGHT_Y	63

// Use same pictures with AT_RADAR SCREEN // 
#define SYS_LOAD_MID_WIN_LEFT_X			86
#define SYS_LOAD_MID_WIN_LEFT_Y			62
#define SYS_LOAD_MID_WIN_SIZE_X			305
#define SYS_LOAD_MID_WIN_SIZE_Y			208

#define SYS_LOAD_RADAR_PICs_LEFT_X			(146 - SYS_LOAD_MID_WIN_LEFT_X)
#define SYS_LOAD_RADAR_PICs_LEFT_Y			(78 - SYS_LOAD_MID_WIN_LEFT_Y)
#define SYS_LOAD_RADAR_PICs_SIZE_X			186
#define SYS_LOAD_RADAR_PICs_SIZE_Y			176

#define SYS_LOAD_ICONs_SIZE_X		50
#define SYS_LOAD_ICONs_SIZE_Y		50

#define SYS_LOAD_ICON1_LEFT_X		(97 - 0)
#define SYS_LOAD_ICON1_LEFT_Y		(64 - 0)
#define SYS_LOAD_ICON5_LEFT_X		(328 - 0)
#define SYS_LOAD_ICON5_LEFT_Y		(64 - 0)

#define SYS_LOAD_ICON2_LEFT_X		(86 - 0)
#define SYS_LOAD_ICON2_LEFT_Y		(114 - 0)
#define SYS_LOAD_ICON6_LEFT_X		(342 - 0)
#define SYS_LOAD_ICON6_LEFT_Y		(114 - 0)

#define SYS_LOAD_ICON3_LEFT_X		(86 - 0)
#define SYS_LOAD_ICON3_LEFT_Y		(170 - 0)
#define SYS_LOAD_ICON7_LEFT_X		(344 - 0)
#define SYS_LOAD_ICON7_LEFT_Y		(170 - 0)

#define SYS_LOAD_ICON4_LEFT_X		(102 - 0)
#define SYS_LOAD_ICON4_LEFT_Y		(222 - 0)
#define SYS_LOAD_ICON8_LEFT_X		(327 - 0)
#define SYS_LOAD_ICON8_LEFT_Y		(222 - 0)

extern uint8_t SYS_Loading(void);

#endif

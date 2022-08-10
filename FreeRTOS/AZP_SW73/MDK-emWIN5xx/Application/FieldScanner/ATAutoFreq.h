#ifndef AT_AUTO_FREQ_H
#define AT_AUTO_FREQ_H

#include <stdio.h>
#include <stdint.h>
#include "AppCommon.h"

#define AT_AUTOF_RECTANGLE_ROUND 3

#define AT_FREQ_ICON_LEFT_X	66
#define AT_FREQ_ICON_LEFT_Y	70
#define AT_FREQ_ICON_SIZE_X	154
#define AT_FREQ_ICON_SIZE_Y	166

#define AT_AUTOF_REC_WINDOW_LEFT_X	246
#define AT_AUTOF_REC_WINDOW_LEFT_Y	24
#define AT_AUTOF_REC_WINDOW_SIZE_X	192
#define AT_AUTOF_REC_WINDOW_SIZE_Y	220

#define AT_FREQ_TYPE_ICON_SIZE_X		50
#define AT_FREQ_TYPE_ICON_SIZE_Y		26

#define AT_FREQ_TYPE_PART_SIZE_X		192
#define AT_FREQ_TYPE_PART_SIZE_Y		44

#define AT_AUTO_FREQ_TYPE1_LEFT_X	135
#define AT_AUTO_FREQ_TYPE1_LEFT_Y	9		/* Increment for other types below this one */
#define AT_AUTO_FREQ_PART_SIZE_X	AT_AUTOF_REC_WINDOW_SIZE_X
#define AT_AUTO_FREQ_PART_SIZE_Y	(AT_AUTOF_REC_WINDOW_SIZE_Y/AT_AUTO_FREQ_COUNT)

#define AT_AUTOF_TYPE1_LEFT_Y	(25 - 0)
#define AT_AUTOF_TYPE2_LEFT_Y	(71 - 0)
#define AT_AUTOF_TYPE3_LEFT_Y	(116 - 0)
#define AT_AUTOF_TYPE4_LEFT_Y	(158 - 0)
#define AT_AUTOF_TYPE5_LEFT_Y	(203 - 0)
#define AT_AUTOF_REC2_Y_DIFF	5

#define AT_AUTOF_TYPE_ICONS_LEFT_X	385

extern uint8_t AT_Auto_Freq(void);

#endif

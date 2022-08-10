#ifndef AT_MAN_FREQ_H
#define AT_MAN_FREQ_H

#include <stdio.h>
#include <stdint.h>
#include <BSP.h>

#define AT_MAN_FREQ_ICON_LEFT_X     66
#define AT_MAN_FREQ_ICON_LEFT_Y     71

#define AT_MAN_FREQ_STR_LEFT_X      79
#define AT_MAN_FREQ_STR_LEFT_Y      32

#define AT_MAN_FREQ_WIN_LEFT_X      263
#define AT_MAN_FREQ_WIN_LEFT_Y      34
#define AT_MAN_FREQ_WIN_SIZE_X      162
#define AT_MAN_FREQ_WIN_SIZE_Y      210

#define AT_MAN_FREQ_NUM1_LEFT_X		26
#define AT_MAN_FREQ_NUM1_LEFT_Y		52
#define AT_MAN_FREQ_NUM2_LEFT_X		110
#define AT_MAN_FREQ_NUM2_LEFT_Y		52

#define AT_MAN_FREQ_FLOAT_STR_LEFT_X	51
#define AT_MAN_FREQ_FLOAT_STR_LEFT_Y	142
#define AT_MAN_FREQ_FLOAT_STR_UNIT_LEFT_X	50
#define AT_MAN_FREQ_FLOAT_STR_UNIT_LEFT_Y	170

#define AT_MAN_FREQ_STEPs_HZ       	100
#define AT_MAN_FREQ_MAX             ((10000 > MAX_PWM_FREQ_HZ) ? MAX_PWM_FREQ_HZ : 10000)
#define AT_MAN_FREQ_MIN             ((300 < MIN_PWM_FREQ_HZ) ? MIN_PWM_FREQ_HZ : 300)

extern uint8_t AT_ManuelFreq(void);

#endif

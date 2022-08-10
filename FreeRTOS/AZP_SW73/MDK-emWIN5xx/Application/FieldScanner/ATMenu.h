#ifndef AT_MENU_H
#define AT_MENU_H

#include <stdio.h>
#include <stdint.h>
#include <AppCommon.h>

// AT screen voltage output enable port & pins // 
// P1.1 : 10V enable 
#define AT_P2P_10V_ENABLE_PORT		1
#define AT_P2P_10V_ENABLE_PIN		1
// P1.0 : 15V enable 
#define AT_P2P_15V_ENABLE_PORT		1
#define AT_P2P_15V_ENABLE_PIN		0
// P3.28 : 18V enable 
#define AT_P2P_18V_ENABLE_PORT		3
#define AT_P2P_18V_ENABLE_PIN		28

#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
	#define AT_VOLTAGE_OUTPUT_SHTDOWN_PORT	3
	#define AT_VOLTAGE_OUTPUT_SHTDOWN_PIN	17
#endif

#define  AT_MENU_ICON_COUNT	6

#define AT_SUB_MENU_PAGE_STR_LEFT_X	65
#define AT_SUB_MENU_PAGE_STR_LEFT_Y	25


//-----------------------//
#define ACTIVE_WIN_LEFT_X	68
#define ACTIVE_WIN_LEFT_Y	73
#define ACTIVE_SIZE_X		344
#define ACTIVE_SIZE_Y		155

#define ACTIVE_ICON_STR_LEFT_X	80
#define ACTIVE_ICON_STR_LEFT_Y	29
#define ACTIVE_ICON_STR_SIZE_X	320
#define ACTIVE_ICON_STR_SIZE_Y	40

// LANG ICON // 
#define ICON6_LEFT_X	70
#define ICON6_LEFT_Y	105
#define ICON6_SIZE_X	64
#define ICON6_SIZE_Y	62

// BRIGHT ICON // 
#define ICON5_LEFT_X	160
#define ICON5_LEFT_Y	74
#define ICON5_SIZE_X	55
#define ICON5_SIZE_Y	53

// VOL ICON // 
#define ICON4_LEFT_X	267
#define ICON4_LEFT_Y	74
#define ICON4_SIZE_X	55
#define ICON4_SIZE_Y	53

// DISTANCE ICON // 
#define ICON3_LEFT_X	347
#define ICON3_LEFT_Y	106
#define ICON3_SIZE_X	66
#define ICON3_SIZE_Y	66

// AUTO FREQ ICON // 
#define ICON2_LEFT_X	247
#define ICON2_LEFT_Y	143
#define ICON2_SIZE_X	83
#define ICON2_SIZE_Y	83

// MAN FREQ ICON // 
#define ICON1_LEFT_X	151
#define ICON1_LEFT_Y	145
#define ICON1_SIZE_X	83
#define ICON1_SIZE_Y	83



extern uint8_t AT_Menu(void);
extern void AT_enable_PWM(uint16_t at_pwm_freq, uint8_t pwm1_duty);
extern void AT_disable_PWM(void);
extern void AT_set_PWM(uint8_t pwm1_duty, uint8_t pwm2_duty);
extern void AT_set_voltage_p2p(uint8_t p2p_voltage_def);

#endif

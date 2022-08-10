#ifndef AT_SETTING_H
#define AT_SETTING_H

#include <stdio.h>
#include <stdint.h>

//-----------------------------------------//
//------------- AT LANGUAGE ---------------//
//-----------------------------------------//


extern uint8_t AT_Setting(void);
extern void AT_enable_PWM(uint16_t at_pwm_freq, uint8_t pwm1_duty);
extern void AT_disable_PWM(void);
extern void AT_set_PWM(uint8_t pwm1_duty, uint8_t pwm2_duty);
extern void AT_set_voltage_p2p(uint8_t p2p_voltage_def);

#endif

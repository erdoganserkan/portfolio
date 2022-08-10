#ifndef ANALOG_H
#define ANALOG_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define ANALOG_POWER_EN_PORT		(5)
#define ANALOG_POWER_EN_PIN			(4)

#define RADIAL_MENU_ENTERENCE_PORT		(2)
#define RADIAL_MENU_ENTERENCE_PIN		(25)

#define UART_TRANSMIT_FIRST_BYTE_PORT		(2)
#define UART_TRANSMIT_FIRST_BYTE_PIN		(25)

#define MAT0_PORT		(2) 
#define MAT0_PIN		(26)
#define MAT1_PORT		(2)
#define MAT1_PIN		(27)
#define MAT2_PORT		(2)
#define MAT2_PIN		(30)
#define MAT3_PORT		(2)
#define MAT3_PIN		(31)

// Used for Rising-Edge detection of REF_CLK // 
#define EXTI0_PORT	(2)
#define EXTI0_PIN		(10)
#define EXTI1_PORT	(2)
#define EXTI1_PIN		(11)

// Capture CH0 is used for REF_CLK frequency measurement // 
#define CAP0_PORT		(3)		
#define CAP0_PIN		(27)

#define C_CLK_TIMER_MAT		(0)	// P2.26 //
#define B_CLK_TIMER_MAT		(1)	// P2.27 //
#define A_CLK_TIMER_MAT		(3)	// P2.31 //
#define REF_CLK_TIMER_MAT	(2)	// P2.30 //

#define PHASE_to_MATCH_VAL(phase, match_val) { \
	if( 0 == phase) \
		match_val = 1; \
	else if( 180*CLOCK_GENERATE_PHASE_FRACTION == phase ) \
		match_val = (input_clock_half_count)-1; \
	else \
		match_val = ((((uint32_t)phase) * input_clock_half_count)/(180UL*CLOCK_GENERATE_PHASE_FRACTION)); \
}
#define FREQ_MEASUREMENT_COUNT	(18)
#define FREQ_MEASUREMENT_CORRECTION_FACTOR	(0.976)

extern void init_clock_generation(void);
extern void start_clock_generation(void);
extern void stop_clock_generation(void);	
extern void set_mat_value(uint8_t mat, uint16_t phase);
extern void measure_input_clock_freq(void);
extern void clock_gen_demo(void);
extern void Analog_init(void);
extern void Analog_Deinit(void);
extern void on_clock_cmd(uint8_t match, uint8_t *pkt);
extern void Process_Clk_gen_Msg(uint8_t *gui_msg);

#endif

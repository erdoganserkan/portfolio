#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "LPC177x_8x.h"
#include "lpc177x_8x_timer.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_exti.h"
#include "lpc177x_8x_clkpwr.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <queue.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include <GLCD.h>
#include <BSP.h>
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Strings.h"
#include "UartInt.h"
#include "Analog.h"

static TIM_TIMERCFG_Type TIM_ConfigStruct;
static TIM_MATCHCFG_Type TIM_MatchConfigStruct[4] = {0};
static volatile uint32_t input_clock_half_count = 0xFFFFFFFF;	// Negative values means frequency measurement failed // 
static uint32_t matches[4] = {0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF};

// init detector related clock generation resources // 
void init_clock_generation(void)
{
	// Set default input clock count //
	if(0xFFFFFFFF == input_clock_half_count) 
		input_clock_half_count = (CLKPWR_GetCLK(CLKPWR_CLKTYPE_PER)/DEFAULT_INPUT_CLK)/2;

	// Set pin functions as TIMER1 MATCH outputs agin // 	
	PINSEL_ConfigPin(MAT0_PORT, MAT0_PIN, 3);	// T3_MAT0	 // 
	PINSEL_ConfigPin(MAT1_PORT, MAT1_PIN, 3);	// T3_MAT1	 //
	PINSEL_ConfigPin(MAT2_PORT, MAT2_PIN, 3);	// T3_MAT2	 //
	PINSEL_ConfigPin(MAT3_PORT, MAT3_PIN, 3);	// T3_MAT3 //

	// Initialize timer, prescaler is minimum for maximum resolution; timer will count with PCLK // 
	TIM_ConfigStruct.PrescaleOption = TIM_PRESCALE_TICKVAL;
	TIM_ConfigStruct.PrescaleValue	= 1;
	//TIM_DeInit(CLOCK_GENERATION_LPC_TIMER);	// If using timer3 this line makes Hard-Fault // 
	TIM_Init(CLOCK_GENERATION_LPC_TIMER, TIM_TIMER_MODE, &TIM_ConfigStruct);

	// use channel 0, MR0
	TIM_MatchConfigStruct[0].MatchChannel = 0;
	// DISABLE interrupt when MR0 matches the value in TC register
	TIM_MatchConfigStruct[0].IntOnMatch   = FALSE;
	#if(0 == REF_CLK_TIMER_MAT)
		// ENABLE reset on MR0: TIMER will reset if MR0 matches it
		TIM_MatchConfigStruct[0].ResetOnMatch = TRUE;
		if(0xFFFFFFFF == matches[0]) {
			matches[0] = input_clock_half_count;
		}
	#else
		// DISABLE reset on MR0: TIMER will NOT reset if MR0 matches it
		TIM_MatchConfigStruct[0].ResetOnMatch = FALSE;
		if(0xFFFFFFFF == matches[0]) {
			PHASE_to_MATCH_VAL((0*CLOCK_GENERATE_PHASE_FRACTION), matches[0]);
		}
	#endif
	// DON'T Stop on MR0 if MR0 matches it
	TIM_MatchConfigStruct[0].StopOnMatch  = FALSE;
	// Toggle MR0.0 pin if MR0 matches it
	TIM_MatchConfigStruct[0].ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	// Set Match value //
	TIM_MatchConfigStruct[0].MatchValue = matches[0];
	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[0]));


	// use channel 1, MR1
	TIM_MatchConfigStruct[1].MatchChannel = 1;
	// DISABLE interrupt when MR1 matches the value in TC register
	TIM_MatchConfigStruct[1].IntOnMatch   = FALSE;
	#if(1 == REF_CLK_TIMER_MAT)
		// ENABLE reset on MR1: TIMER will reset if MR1 matches it
		TIM_MatchConfigStruct[1].ResetOnMatch = TRUE;
		if(0xFFFFFFFF == matches[1]) {
			matches[1] = input_clock_half_count;
		}
	#else
		// DISABLE reset on MR1: TIMER will NOT reset if MR1 matches it
		TIM_MatchConfigStruct[1].ResetOnMatch = FALSE;
		if(0xFFFFFFFF == matches[1]) {
			PHASE_to_MATCH_VAL((10*CLOCK_GENERATE_PHASE_FRACTION), matches[1]);
		}
	#endif
	// DON'T Stop on MR0 if MR1 matches it
	TIM_MatchConfigStruct[1].StopOnMatch  = FALSE;
	// Toggle MR1.0 pin if MR1 matches it
	TIM_MatchConfigStruct[1].ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	// Set Match value //
	TIM_MatchConfigStruct[1].MatchValue = matches[1];	
	// Set configuration for Tim_config and Tim_MatchConfig	// Init timer again for clock generation //
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[1]));

		
	// use channel 2, MR2
	TIM_MatchConfigStruct[2].MatchChannel = 2;
	// DISABLE interrupt when MR2 matches the value in TC register
	TIM_MatchConfigStruct[2].IntOnMatch   = FALSE;
	#if(2 == REF_CLK_TIMER_MAT)
		// ENABLE reset on MR0: TIMER will reset if MR2 matches it
		TIM_MatchConfigStruct[2].ResetOnMatch = TRUE;
		if(0xFFFFFFFF == matches[2]) {
			matches[2] = input_clock_half_count;
		}
	#else
		// DISABLE reset on MR0: TIMER will NOT reset if MR2 matches it
		TIM_MatchConfigStruct[2].ResetOnMatch = FALSE;
		if(0xFFFFFFFF == matches[2]) {
			PHASE_to_MATCH_VAL((20*CLOCK_GENERATE_PHASE_FRACTION), matches[2]);
		}
	#endif
	// DON'T Stop on MR2 if MR2 matches it
	TIM_MatchConfigStruct[2].StopOnMatch  = FALSE;
	//Toggle MR2.0 pin if MR2 matches it
	TIM_MatchConfigStruct[2].ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	// Set Match value //
	TIM_MatchConfigStruct[2].MatchValue = matches[2];
	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &TIM_MatchConfigStruct[2]);


	// use channel 3, MR3
	TIM_MatchConfigStruct[3].MatchChannel = 3;
	// DISABLE interrupt when MR3 matches the value in TC register
	TIM_MatchConfigStruct[3].IntOnMatch   = FALSE;
	#if(3 == REF_CLK_TIMER_MAT)
		// ENABLE reset on MR3: TIMER will reset if MR3 matches it
		TIM_MatchConfigStruct[3].ResetOnMatch = TRUE;
		if(0xFFFFFFFF == matches[3]) {
			matches[3] = input_clock_half_count;
		}
	#else
		// DISABLE reset on MR3: TIMER will NOT reset if MR3 matches it
		TIM_MatchConfigStruct[3].ResetOnMatch = FALSE;
		if(0xFFFFFFFF == matches[3]) {
			PHASE_to_MATCH_VAL((30*CLOCK_GENERATE_PHASE_FRACTION), matches[3]);
		}
	#endif
	// DON'T Stop on MR2 if MR3 matches it
	TIM_MatchConfigStruct[3].StopOnMatch  = FALSE;
	//Toggle MR3.0 pin if MR3 matches it
	TIM_MatchConfigStruct[3].ExtMatchOutputType = TIM_EXTMATCH_TOGGLE;
	// Set Match value //
	TIM_MatchConfigStruct[3].MatchValue = matches[3];
	// Set configuration for Tim_config and Tim_MatchConfig
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[3]));

	/* preemption = 1, sub-priority = 1 */
	// NVIC_SetPriority(CLOCK_GENERATION_TMR_INT, ((0x01<<3)|0x01));

	/* DISABLE interrupt for timer 1 */
	NVIC_DisableIRQ(CLOCK_GENERATION_TMR_INT);

	// STOP timer
	TIM_Cmd(CLOCK_GENERATION_LPC_TIMER, DISABLE);
	
}

	
// start analog clocks(ref, a, b, c) that detectors needs // 
void start_clock_generation(void)
{
	// Reset timer & leave it as stopped // 
	TIM_Cmd(CLOCK_GENERATION_LPC_TIMER, DISABLE);
	TIM_ResetCounter(CLOCK_GENERATION_LPC_TIMER);

	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[0]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[1]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[2]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[3]));

	TIM_ClearIntPending(CLOCK_GENERATION_LPC_TIMER, TIM_MR0_INT);
	TIM_ClearIntPending(CLOCK_GENERATION_LPC_TIMER, TIM_MR1_INT);
	TIM_ClearIntPending(CLOCK_GENERATION_LPC_TIMER, TIM_MR2_INT);
	TIM_ClearIntPending(CLOCK_GENERATION_LPC_TIMER, TIM_MR3_INT);

	// set REF_CLK pin's initial state as HIGH //
	CLOCK_GENERATION_LPC_TIMER->EMR |= ((0x1)<<REF_CLK_TIMER_MAT);	
	// set A, B and C clk pins' initial state as LOW //
	CLOCK_GENERATION_LPC_TIMER->EMR &= (~((~((0x1)<<REF_CLK_TIMER_MAT)) & 0xF));
	
	TIM_Cmd(CLOCK_GENERATION_LPC_TIMER, ENABLE);
}


// stop detector related clock generation // 
void stop_clock_generation(void)
{
	TIM_Cmd(CLOCK_GENERATION_LPC_TIMER, DISABLE);
	TIM_ResetCounter(CLOCK_GENERATION_LPC_TIMER);
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[0]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[1]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[2]));
	TIM_ConfigMatch(CLOCK_GENERATION_LPC_TIMER, &(TIM_MatchConfigStruct[3]));
}

void clock_gen_demo(void) {
	Analog_init();
	init_clock_generation();
	start_clock_generation();
	uint16_t phase0 = 0, phase1 = 0, phase2 = 0, lsb = 0;
	while(1) {
		volatile uint8_t newCH = TRUE;
		if(TRUE == newCH) {
			//newCH = FALSE;
			set_mat_value(CMD_SET_A_CLOCK_DELAY, phase0*CLOCK_GENERATE_PHASE_FRACTION + lsb);
			set_mat_value(CMD_SET_B_CLOCK_DELAY, phase1*CLOCK_GENERATE_PHASE_FRACTION + lsb);
			set_mat_value(CMD_SET_C_CLOCK_DELAY, phase2*CLOCK_GENERATE_PHASE_FRACTION + lsb);
			if(++lsb == 10) {
				lsb = 0;

				if(1 < (++phase0))
					phase0 = 0;
				if(1 < (++phase1))
					phase1 = 0;
				if(1 < (++phase2))
					phase2 = 0; 
			}
		}
		vTaskDelay(1000 * (configTICK_RATE_HZ/1000));
	}
}

void Analog_init(void) 
{
	// enable power to analog board // 
	PINSEL_ConfigPin(ANALOG_POWER_EN_PORT, ANALOG_POWER_EN_PIN, 0);	// T1_MAT0	: A_CLK : GPIO Config 
	GPIO_SetDir(ANALOG_POWER_EN_PORT, (1<<ANALOG_POWER_EN_PIN), 1);
	GPIO_SetValue(ANALOG_POWER_EN_PORT, (1<<ANALOG_POWER_EN_PIN));	
}

void Analog_Deinit(void) 
{
	// DISABLE power to analog board // 
	PINSEL_ConfigPin(ANALOG_POWER_EN_PORT, ANALOG_POWER_EN_PIN, 0);	// T1_MAT0	: A_CLK : GPIO Config 
	GPIO_SetDir(ANALOG_POWER_EN_PORT, (1<<ANALOG_POWER_EN_PIN), 1);
	GPIO_ClearValue(ANALOG_POWER_EN_PORT, (1<<ANALOG_POWER_EN_PIN));	
}

void set_mat_value(uint8_t mat, uint16_t phase)
{
	uint32_t match_val;
	DEBUGM("%s()-> mat(%u), phase(%u.%u)\n", __FUNCTION__, mat, phase/10, phase%10);
	if(mat > 3)
		mat = 3;
	//stop_clock_generation();	// There is NO problem when non-stop clock change and generate is done // 
																// It is not necessary to STOP clock generation before change timer MATCH values // 
	if(REF_CLK_TIMER_MAT == mat) {
		match_val = input_clock_half_count;
	} else {
		if((180*CLOCK_GENERATE_PHASE_FRACTION) < phase) 
			phase = 180*CLOCK_GENERATE_PHASE_FRACTION;
		PHASE_to_MATCH_VAL(phase, match_val);
	}
	DEBUGM("%s()-> match_timer_val(%u)\n", __FUNCTION__, match_val);
	TIM_MatchConfigStruct[mat].MatchValue = matches[mat] = match_val;
	TIM_UpdateMatchValue(CLOCK_GENERATION_LPC_TIMER, mat, match_val);
	start_clock_generation();
}

void on_clock_cmd(uint8_t match, uint8_t *pkt) {
	uint16_t phase = ((uint16_t)pkt[2])*CLOCK_GENERATE_PHASE_FRACTION + \
		(((uint16_t)pkt[3])%CLOCK_GENERATE_PHASE_FRACTION);
	// 1- Set new clock phase 
	set_mat_value(match, phase);
	// 2- Send response packet // 
	UmdPkt_Type send_msg;
	send_msg.cmd = pkt[0] + 1;
	send_msg.length = 3;
	send_msg.data.cmd_status = CMD_DONE;
	vTaskDelay(10 * (configTICK_RATE_HZ/1000));
	UARTSend((uint8_t *)&send_msg, 0, NULL);
}

void Process_Clk_gen_Msg(uint8_t *gui_msg) {
	switch(((UmdPkt_Type *)gui_msg)->cmd) {
		case CMD_SET_A_CLOCK_DELAY:
			on_clock_cmd(A_CLK_TIMER_MAT, gui_msg);
			break;
		case CMD_SET_B_CLOCK_DELAY: 
			on_clock_cmd(B_CLK_TIMER_MAT, gui_msg);
			break;
		case CMD_SET_C_CLOCK_DELAY: 
			on_clock_cmd(C_CLK_TIMER_MAT, gui_msg);
			break;
		case CMD_SET_REF_CLOCK_FREQ: {
				uint16_t desired_freq_hz = (((uint16_t)gui_msg[2])<<8) + ((uint16_t)gui_msg[3]);	// first MSB than LSB // 
				input_clock_half_count = (CLKPWR_GetCLK(CLKPWR_CLKTYPE_PER)/desired_freq_hz)/2;	// Set GLOBAL object // 
				DEBUGM("SET_REF_CLK : freq(%u Hz), clock count for half_period (%u cycles)\n", desired_freq_hz, input_clock_half_count);
				on_clock_cmd(REF_CLK_TIMER_MAT, gui_msg);
		}
		break;
		default:
			while(1);
			break;
	}
}


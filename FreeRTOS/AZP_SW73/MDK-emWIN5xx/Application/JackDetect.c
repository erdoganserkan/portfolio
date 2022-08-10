#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "LPC177x_8x.h"         // Device specific header file, contains CMSIS
#include "system_LPC177x_8x.h"  // Device specific header file, contains CMSIS
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_adc.h"
#include "lpc177x_8x_timer.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "WM.h"
#include "UMDShared.h"
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "AppFont.h"
#include "AppPics.h"
#include "RuntimeLoader.h"
#include "Popup.h"
#include "BSP.h"
#include "Battery.h"
#include "StatusBar.h"

extern EventGroupHandle_t xRendezvousBits;	// Used for Task Rendezvous //

static uint16_t jd_values[JD_FRAME_SAMPLES_COUNT];
static uint16_t jd_sample_indx = 0;
static uint8_t js_capture_state = JD_CAPTURE_IDLE;

static void __task_JackDetect(void *pvArg);

void JD_init(void) 
{	
	xTaskCreate( __task_JackDetect , "JDTask" , JD_TASK_STACK_SIZE , \
		NULL , JD_TASK_PRIORITY , NULL ); 
}

static void __task_JackDetect(void *pvArg)
{
	// Wait for GUI & Other tasks to be ready before entering to main processing loop // 
	if(NULL != xRendezvousBits)
		xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_COMM), ALL_TASK_BITs, portMAX_DELAY );
	else
		while(TODO_ON_ERR);

	//:TODO: Adc channel initialization
	// Start sample high freq audio tone 
	// Capture a frame and process it and set initial state of JACK DETECT 
	// Stop tone generation 

	while(1) {
		vTaskDelay(1 * (configTICK_RATE_HZ/1000));	// 1ms delay // 
		//:TODO: Check if adc frame capture completed
			// if completed process results and give desicion for jack state and update mute pin // 
	}
}

void start_jd_capture(void) 
{
	// Clear sample counter (shared with adc-interrupt-handler) // 
	// set state to "capture active" // 
	// Set for continuos capture mode start JD ADC CH capture 
	#if(0)
	// JACK STATE READING RELATED ADC PROCESSING // 
	uint16_t jack_det_ch_raw, jack_detect_mv;
	jack_det_ch_raw = BSP_GET_RAW_ADC(JACK_DETECT_ADC_CH);
	jack_detect_mv = __RAW_ADC_TO_MV(jack_det_ch_raw);
	
	// update jack detect results moving average // 
	jd_sum -= jd_samples[jd_indx];
	jd_sum += jack_detect_mv;
	jd_samples[jd_indx] = jack_detect_mv;
	if(++jd_indx == JACK_DETECT_AVG_DEPTH)
		jd_indx=0;
#if(8 == JACK_DETECT_AVG_DEPTH)
		jack_detect_mv = (jd_sum>>3);
#else
	#error ""
#endif
	
	if(((JACK_DETECTION_THRESHOLD_MV*11)/10) > jack_detect_mv) { 
		// volatage is high, sense pin is connected to 5V pull-up, UNMUTE spaeakers // 
		uint16_t Jack_State = HP_JACK_INSERTED;
		SB_setget_component(SB_JACK_STATE, TRUE, &Jack_State);
		SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, UNMUTE_SPEAKERS_PIN_STATE);
	} else if((((JACK_DETECTION_THRESHOLD_MV*11)/10) >= jack_detect_mv) && (((JACK_DETECTION_THRESHOLD_MV*9)/10) <= jack_detect_mv)) {
		// This is safe threshold area for oscillation, hold the previous position, dont do changes // 
	}
	else {	// volatage is LOW, sense pin is unconnected to 5v pull-up, MUTE speakers //					
		uint16_t Jack_State = HP_JACK_NOT_INSERTED;
		SB_setget_component(SB_JACK_STATE, TRUE, &Jack_State);
		SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, MUTE_SPEAKERS_PIN_STATE);
	}
	#endif
}


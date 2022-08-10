#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "lpc177x_8x.h"
#include "lpc177x_8x_uart.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <WM.h>
#include <GUIDEMO.h>
#include <GLCD.h>
#include "AppCommon.h"
#include "UMDShared.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "TSTask.h"

static void __task_TSTask(void *pvArg);
static xTaskHandle TSTaskHandle;
volatile uint8_t new_ts = FALSE, new_ts2 = FALSE;
volatile uint8_t min_ts_wait_cnt = MIN_GAP_TS_EVENT_MS;
volatile TSTaskPoint_t ts_point;

extern EventGroupHandle_t xRendezvousBits;	// Used for Task Rendezvous //

void TSTaskInit(void)
{
	xTaskCreate( __task_TSTask , "TSTask" , TS_TASK_STACK_SIZE , NULL , \
		TS_TASK_PRIORITY , &TSTaskHandle );
}

static void __task_TSTask(void *pvArg)
{
	struct WM_MESSAGE msg;	
	
	// Wait for GUI & Other tasks to be ready before entering to main processing loop // 
	if(NULL != xRendezvousBits)
		xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_TS), ALL_TASK_BITs, portMAX_DELAY );
	else
		while(TODO_ON_ERR);
	
	// Do real(hw based) touch screen calibration // 
	TouchPanel_Calibrate( GLCD_X_SIZE, GLCD_Y_SIZE);

	for(;;) {
		calibrate();	// Do touch screen event polling // 
		if(0 != min_ts_wait_cnt) {
			// We are waiting for minimum time pass, before ready for new touch event // 
			min_ts_wait_cnt--;
		} else {
			// Ready for new TS event; Check if there is new TS event //
			if(TRUE == new_ts2) {
				// Construct message and send it to focused widget // 
				msg.hWin = WM_GetFocussedWindow();
				msg.MsgId = GUI_USER_MSG_NEW_TS_TOUCH;
				msg.Data.v = ts_point.data_int;
				DEBUGM("New TS EVENT x=%u : y=%u\n", ts_point.points[0], ts_point.points[1]);
				WM_SendMessage(msg.hWin, &msg);			
				
				// Set ready for new ts event read // 
				min_ts_wait_cnt = MIN_GAP_TS_EVENT_MS;
				new_ts2 = new_ts = FALSE;
			}
		}
		// Do a minimum delay static wait // 
		vTaskDelay(1 * (configTICK_RATE_HZ/1000));	// 5ms OS delay // 
	}
}
int ts_calibration_demo(void)
{ 
	//TP_Init(); 
  //SDRAM_32M_16BIT_Init();
	//GLCD_Init();

	TouchPanel_Calibrate( GLCD_X_SIZE, GLCD_Y_SIZE);
	/* Infinite loop */
	while (1)	
	{
		calibrate();
	}
}


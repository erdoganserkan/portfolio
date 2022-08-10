#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "lpc177x_8x.h"
#include "lpc177x_8x_uart.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include "WM.h"
#include "GUIDEMO.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "debug_frmwrk.h"
#include <monitor.h>
#include "Serial.h"

volatile uint8_t newCH = FALSE;
static void __task_Serial_DbgFRMWRK(void *pvArg);
static xTaskHandle SERIALTaskHandle;

extern EventGroupHandle_t xRendezvousBits;	// Used for Task Rendezvous //

void SerialInit(void)
{
	xTaskCreate( __task_Serial_DbgFRMWRK , "SerialTask" , SERIAL_TASK_STACK_SIZE , NULL , \
		SERIAL_TASK_PRIORITY , &SERIALTaskHandle );
}

static void __task_Serial_DbgFRMWRK(void *pvArg)
{
	struct WM_MESSAGE msg;
	uint8_t CH;
	
	// Wait for GUI & Other tasks to be ready before entering to main processing loop // 
	if(NULL != xRendezvousBits)
		xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_SERIAL), ALL_TASK_BITs, portMAX_DELAY );
	else
		while(TODO_ON_ERR);
	
	// main_fatfs(NULL);
	
	for(;;) {
		if(TRUE == UARTGetCharInNonBlock(DEBUG_UART_PORT, &CH)) {					// Check for new CH without blocking // 
			newCH = TRUE;
		#if(TRUE == SERIAL_TASK_STATE)
			if('G' == CH) {
				msg.hWin = WM_GetFocussedWindow();
				msg.MsgId = GUI_USER_MSG_NEW_GAUGE;
				msg.Data.v = rand()%UMD_GAUGE_MAX;
				WM_SendMessage(msg.hWin, &msg);
			}
			else if('T' == CH) {
				msg.hWin = WM_GetFocussedWindow();
				msg.MsgId = GUI_USER_MSG_NEW_TARGET_ID;
				msg.Data.v = TARGET_CAVITY;
				WM_SendMessage(msg.hWin, &msg);
			}
			else if('E' == CH) {
				msg.hWin = WM_GetFocussedWindow();
				msg.MsgId = GUI_USER_MSG_NEW_GROUND_ID;
				msg.Data.v = GROUND_ID_MAX;
				WM_SendMessage(msg.hWin, &msg);
			}
		#endif	
		}
		else 
			vTaskDelay(5 * (configTICK_RATE_HZ/1000));	// 5ms OS delay // 
	}
}



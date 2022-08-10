#include "lpc_types.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_uart.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "task.h"
#include "UMDShared.h"
#include "AppCommon.h"
#include "debug_frmwrk.h"
#include "UartInt.h"
#include "Communication.h"

extern EventGroupHandle_t xRendezvousBits;	// Used for Task Rendezvous //
static void __task_Communication(void *pvArg);

void CommInit(void)
{
	UartInt_init();
	
	xTaskCreate( __task_Communication , "CommTask" , COMM_TASK_STACK_SIZE , \
		NULL , COMM_TASK_PRIORITY , NULL );	
}

static void __task_Communication(void *pvArg)
{
	//:TODO: Do sending & receiving queue initializations // 
	//:TODO: Enable uart interrupts // 
	// Wait for GUI & Other tasks to be ready before entering to main processing loop // 
	if(NULL != xRendezvousBits)
		xEventGroupSync( xRendezvousBits, (0x1<<UMD_TASK_COMM), ALL_TASK_BITs, portMAX_DELAY );
	else
		while(TODO_ON_ERR);

	while(1) {
		vTaskDelay(10 * (configTICK_RATE_HZ/1000));	// 10ms delay // 
		//:TODO://
			// RECEIVING MISSION //
				// Read HW and get complete messages from serial port //
				// Check message validity //
				// If message is valid than send to GUI task as a message // //
			// SENDING MISSION //
				// Check queue //
				// If there is any message, send them to detector // 
	}
}


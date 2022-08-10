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
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Strings.h"
#include "UartInt.h"
#include "Analog.h"
#include "DiffCalculator.h"

static uint_fast16_t ActiveDiffIndx;
static uint_fast8_t AverageActivated;
static int32_t FirstBuffer[DEF_DIFF_DEPTH];
static int32_t FirstBufferSum;
static int32_t FirstBufferAvg;
static int32_t SecondBuffer[DEF_DIFF_DEPTH];
static int32_t SecondBufferSum;
static int32_t SecondBufferAvg;

void DiffCalc_init(void) {
	memset(FirstBuffer,0,sizeof(FirstBuffer));
	memset(SecondBuffer,0,sizeof(SecondBuffer));

	FirstBufferSum = 0;
	FirstBufferAvg = 0;
	SecondBufferSum = 0;
	SecondBufferAvg = 0;
	ActiveDiffIndx = 0;
	AverageActivated = TRUE;
}

int32_t DiffCalc_getdiff(int32_t NewSample) 
{
	int32_t OldestI32;
	// update the average of first buffer, feed this buffer with new sample //
	OldestI32 = FirstBuffer[ActiveDiffIndx];
	FirstBufferSum -= OldestI32;								// Minus the oldest member from sum of buffer //
	FirstBuffer[ActiveDiffIndx] = NewSample;					// Add newest value to buffer //
	FirstBufferSum += NewSample;
	if(AverageActivated == TRUE)
		FirstBufferAvg = FirstBufferSum/(DEF_DIFF_DEPTH);

	// update the average of second buffer, feed this buffer with oldest sample of first buffer (buffer above) //
	SecondBufferSum -= SecondBuffer[ActiveDiffIndx];
	SecondBufferSum += OldestI32;
	SecondBuffer[ActiveDiffIndx] = OldestI32;
	if(AverageActivated == TRUE)
		SecondBufferAvg = SecondBufferSum/(DEF_DIFF_DEPTH);

	if((++ActiveDiffIndx) == DEF_DIFF_DEPTH)					// If buffer is overflowed return to start of buffer //
		ActiveDiffIndx = 0;

	if(AverageActivated == TRUE)
		return (FirstBufferAvg - SecondBufferAvg);
	else
		return (FirstBufferSum - SecondBufferSum);
}

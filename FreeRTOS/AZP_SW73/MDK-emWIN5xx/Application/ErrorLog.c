#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <event_groups.h>
#include <task.h>
#include "GLCD.h"
#include "GUIDEMO.h"
#include "DIALOG.h"
#include "BSP.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "AppSettings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "ErrorLog.h"


static uint8_t err_tail = 0, err_head = 0;
static sErrStore err_buf[ERR_RECORD_STACK_DEPTH];

void push_err(sErrStore *err_obj_ptr)	// Record new err // 
{
	err_buf[err_head++] = *err_obj_ptr;
	if(ERR_RECORD_STACK_DEPTH == err_head)
		err_head = 0;
}

sErrStore *pop_err(void)		// Get error from records if there is any // 
{
	sErrStore *ret = NULL;
	if(err_head != err_tail) {
		ret = &err_buf[err_tail];
		if(ERR_RECORD_STACK_DEPTH == (++err_tail))
			err_tail = 0;
	}
	return ret;
}

void LogErr(char *str_buf, const sErrStore *err_datap)	// Get Detailed data about "Error" and log it // 
{
	char sbuf[16];
	volatile uint8_t indx;
	if(NULL == str_buf) {
		ERRM("NULL_PARAMETER\n");
		return;
	}
	memset(sbuf, 0, sizeof(sbuf));
	sprintf(str_buf, "ERR_PKT : ");
	for(indx=0 ; indx<sizeof(sErrStore) ; indx++) {
		sprintf(sbuf, "%02X ", ((uint8_t *)err_datap)[indx]); strcat(str_buf, sbuf);
	}
	sprintf(sbuf, "\n"); strcat(str_buf, sbuf);
}

#ifndef TSTASK_H
#define TSTASK_H

#include "lpc177x_8x_uart.h"
#include <FreeRTOS.h>
#include <task.h>
#include "TouchPanel.h"

#define MIN_GAP_TS_EVENT_MS		(100)

typedef __packed union {
	uint16_t points[2];
	int data_int;
} TSTaskPoint_t;

extern volatile uint8_t new_ts, new_ts2;
extern volatile TSTaskPoint_t ts_point;

extern void TSTaskInit(void);
extern int ts_calibration_demo(void);

#endif

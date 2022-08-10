#ifndef SERIAL_H
#define SERIAL_H

#include <stdio.h>
#include <stdint.h>
#include "AppCommon.h"

// Debug Levels //
#define TRACE_LEVEL		0
#define DEBUG_LEVEL		1
#define WARN_LEVEL		2
#define INFO_LEVEL		3
#define ERR_LEVEL			4
#define ALERT_LEVEL		5
#define FATAL_LEVEL		6

// Main Log Definitions //
#if(APP_LOG_LEVEL	<= FATAL_LEVEL)
	#define FATALL(fmt, args...)       \
	do {\
		xprintf("FTL:" fmt, ##args); \
	} while(0);
#else
	#define FATALL(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= ALERT_LEVEL)
	#define ALERTL(fmt, args...)       \
	do {\
		xprintf("ALR:" fmt, ##args); \
	} while(0);
#else
	#define ALERTL(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= ERR_LEVEL)
	#define ERRL(fmt, args...)       \
	do {\
		xprintf("ERR:" fmt, ##args); \
	} while(0);
#else
	#define ERRL(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= INFO_LEVEL)
	#define INFOL(fmt, args...)       \
	do {\
		xprintf("INF:" fmt, ##args); \
	} while(0);
#else
	#define INFOL(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= WARN_LEVEL)
	#define WARNL(fmt, args...)       \
	do {\
		xprintf("WRN: " fmt, ##args); \
	} while(0);
#else
	#define WARNL(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= DEBUG_LEVEL)
	#define DEBUGL(fmt, args...)       \
	do {\
		xprintf("DBG: " fmt, ##args); \
	} while(0);
#else
	#define DEBUGL(fmt, args...)	do{}while(0);
#endif
		
#if(APP_LOG_LEVEL	<= TRACE_LEVEL)
	#define TRACEL(fmt, args...)       \
	do {\
		xprintf("TRC: " fmt, ##args); \
	} while(0);
#else
	#define TRACEL(fmt, args...)	do{}while(0);
#endif

extern void SerialInit(void);

#endif

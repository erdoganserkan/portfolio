#ifndef GUI_CONSOLE_H
#define GUI_CONSOLE_H

#include <stdio.h>
#include <stdint.h>
#include "AppCommon.h"
#include "GLCD.h"

// GUI CONSOLE RESOURCESS //
#define GUI_CONSOLE_CHAR_HEIGHT	(16)
#define CHAR_HEIGHT_GAP			(0)
#define GUI_CONSOLE_CHAR_WIDTH	(8)
#define CHAR_WIDTH_GAP			(0)
#define GUIC_LINE_MAX	(GLCD_Y_SIZE / (GUI_CONSOLE_CHAR_HEIGHT + CHAR_HEIGHT_GAP))
#define GUIC_COLUMN_MAX ((GLCD_X_SIZE / (GUI_CONSOLE_CHAR_WIDTH + CHAR_WIDTH_GAP))-1)

extern char gui_buf[(GUIC_COLUMN_MAX*3)/2];

#if(APP_LOG_LEVEL	<= FATAL_LEVEL)
	#define FATALGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "F:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Red);\
	} while(0);
#else
	#define FATALGC(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= ALERT_LEVEL)
	#define ALERTGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "A:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Purple);\
	} while(0);
#else
	#define ALERTGC(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= ERR_LEVEL)
	#define ERRGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "E:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Red);\
	} while(0);
#else
	#define ERRGC(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= INFO_LEVEL)
	#define INFOGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "I:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Olive);\
	} while(0);
#else
	#define INFOGC(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= WARN_LEVEL)
	#define WARNGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "W:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Yellow);\
	} while(0);
#else
	#define WARNGC(fmt, args...)	do{}while(0);
#endif

#if(APP_LOG_LEVEL	<= DEBUG_LEVEL)
	#define DEBUGGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "D:" fmt, ##args); \
		Send_GUI_Console(gui_buf, Blue);\
	} while(0);
#else
	#define DEBUGGC(fmt, args...)	do{}while(0);
#endif
		
#if(APP_LOG_LEVEL	<= TRACE_LEVEL)
	#define TRACEGC(fmt, args...)       \
	do {\
		sprintf(gui_buf, "T:" fmt, ##args); \
		Send_GUI_Console(gui_buf, LightGrey);\
	} while(0);
#else
	#define TRACEGC(fmt, args...)	do{}while(0);
#endif


extern void Send_GUI_Console(char *new_str, uint16_t clr);


#endif

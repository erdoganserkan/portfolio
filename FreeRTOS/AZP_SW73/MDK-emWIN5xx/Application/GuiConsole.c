#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_ssp.h"
#include "lpc177x_8x_mci.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <semphr.h>
#include "GLCD.h"
#include "WM.h"
#include "BSP.h"
#include "AppCommon.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "GuiConsole.h"

char gui_buf[(GUIC_COLUMN_MAX*3)/2];

void Send_GUI_Console(char *new_str, uint16_t clr) 
{
	static uint16_t line_num = 0;
	uint16_t current_line = line_num;
	
	if(NULL == new_str) {
		GLCD_Clear(0, Black);GLCD_Clear(1, Black);GLCD_Clear(2, Black);GLCD_Clear(3, Black);
		line_num = 0;
		return;
	}
	uint8_t slen = strlen(new_str);
	if(slen > GUIC_COLUMN_MAX) {
		new_str[GUIC_COLUMN_MAX] = '\0';
	}
	if(GUIC_LINE_MAX == line_num) {	// Screen move is required // 
		memmove((void *)LCD_VRAM_BASE_ADDR, (void *)(LCD_VRAM_BASE_ADDR + ((GUI_CONSOLE_CHAR_HEIGHT + CHAR_HEIGHT_GAP) * GLCD_X_SIZE * 2)), \
			(GLCD_X_SIZE * GLCD_Y_SIZE *2) - ((GUI_CONSOLE_CHAR_HEIGHT + CHAR_HEIGHT_GAP) * GLCD_X_SIZE * 2));
		current_line = GUIC_LINE_MAX -1;
	} else
		line_num++;
	GUI_Text(0, current_line*(GUI_CONSOLE_CHAR_HEIGHT + CHAR_HEIGHT_GAP), new_str, clr, Black);
	// Fill the rest of line with SPACE character // 
	if(slen < GUIC_COLUMN_MAX) {
		volatile uint32_t indx = slen;
		for(; indx<GUIC_COLUMN_MAX; indx++)
			PutChar(indx*(GUI_CONSOLE_CHAR_WIDTH + CHAR_WIDTH_GAP), current_line*(GUI_CONSOLE_CHAR_HEIGHT + CHAR_HEIGHT_GAP), 
				' ', clr, Black);
	}
}


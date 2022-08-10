#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include <BSP.h>
#include "AppCommon.h" 
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "SYSSettings.h"
#include "Analog.h"
#include "ATSetting.h"

#define AT_MENU_ANIM_MS	(250)
#define AT_SETTING_MAX_INNER_WINS		(5)

typedef struct {
	uint16_t x;
	uint16_t y;
} sPoint;

static uint8_t new_page;
static uint8_t ScreenExit;
static uint8_t active_icon = 0;	// Active Icon Position Number // 
static sPoint const win_up_points[AT_MENU_ICON_COUNT]= {
	{AT_MENU_POINTER_POS1_UPX, AT_MENU_POINTER_POS1_UPY},
	{AT_MENU_POINTER_POS2_UPX, AT_MENU_POINTER_POS2_UPY},
	{AT_MENU_POINTER_POS3_UPX, AT_MENU_POINTER_POS3_UPY},
	{AT_MENU_POINTER_POS4_UPX, AT_MENU_POINTER_POS4_UPY},
	{AT_MENU_POINTER_POS5_UPX, AT_MENU_POINTER_POS5_UPY}
};
static void _cbBk(WM_MESSAGE * pMsg);
static WM_CALLBACK *OldDesktopCallback;
static WM_HTIMER hTimerAnim;
static uint8_t icon_state;
static uint8_t prev_icon;
static uint8_t first_time;
static uint8_t ready4pkey = TRUE;
static WM_HWIN ATSetWins[AT_SETTING_MAX_INNER_WINS];

uint8_t AT_Setting(uint8_t new_page) 
{
	int xLCDSize;
	int yLCDSize;
	volatile uint8_t indx;

	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))	// AT_MENU_PICs Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	ScreenExit = FALSE;
	new_page = AT_SCREEN;
	OldDesktopCallback = NULL;
	WM_HTIMER hTimerAnim = 0;
	icon_state = 0;
	prev_icon = active_icon;
	first_time = TRUE;
	ready4pkey = TRUE;
	for(volatile uint8_t indx=0 ; indx<AT_SETTING_MAX_INNER_WINS ; indx)
		ATSetWins[AT_SETTING_MAX_INNER_WINS] = 0;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk);
	WM_InvalidateWindow(WM_HBKWIN);
	WM_SetFocus(WM_HBKWIN);	

	switch(new_page) {
		case AT_DISTANCE_SCR:
			// Locate DISTANCE icon and screen STR on desktop window //  
			// Create a rectange window on right-side for distance selection rectangles // 
			break;
		case AT_LANG_SCR:
			// Locate LANGUAGE icon and screen STR on desktop window //  
			// Create a rectange window on right-side for country flags and selection pointer //
			break;
		case AT_BRIGHT_SCR:
			// Locate BRIGHT icon and screen STR on desktop window //  
			// Create a window for LEVEL Number // 
			// Create a window for level BAR (Includes NULL & FULL parts) // 
			break;
		case AT_VOL_SCR:
			// Locate VOLUME icon and screen STR on desktop window //  
			// Create a window for LEVEL Number // 
			// Create a window for level BAR (Includes NULL & FULL parts) // 
			break;
		case AT_MAN_FREQ_SCR: 
			// Locate VOLUME icon and screen STR on desktop window //  
			// Create a window for two digit gradients, flaoting point number and unit string // 
			break;
		case AT_LOADING_SCR:
			// Locate screen string, big auto-freq-range icon, search-info string on Desktop window // 
			// Create a window for loading bar pictures // 
			break;
		default:
			while(STALLE_ON_ERR);
			break;
	}	
	//SB_init(SB_REDUCED_MODE_USE_RIGHT);

 
	// Animation loop
	while (FALSE == ScreenExit) {
		if(!GUI_Exec1())  {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}

	// Release back analog circuit related resources // 
	GPIO_ClearValue(FS_PWM_REG_SHTDWN_PORT, (1<<FS_PWM_REG_SHTDWN_PIN));  	// LOW:PowerON FS Regulator // 
  	AT_set_voltage_p2p(AT_VOLTAGE_P2P_18V);
	AT_disable_PWM();
	// Do Deletion of created objects & Release of Resources // 
	WM_DeleteTimer(hTimerAnim);
	GUI_ClearKeyBuffer();
	//SB_delete();
	
	return new_page;
}

static inline void __draw_string(void) {
	// Update active icon's text // 
	//:TODO:Recover back string area //  
	if(0 != SBResources[AT_MENU_PICs][AT_MENU_STR_BACK].hMemHandle) {
		GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_MENU_STR_BACK].hMemHandle, \
			((GLCD_X_SIZE-AT_MENU_STR_SIZEX)/2), AT_MENU_STR_UPY);
	}
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetColor(GUI_YELLOW);
	GUI_SetFont(APP_32B_FONT);
	GUI_DispStringHCenterAt(GetString(STR_ATM_SCAN_FREQ + active_icon), AT_MENU_STR_MIDDX, AT_MENU_STR_MIDDY);
}
	/*************************** End of file ****************************/

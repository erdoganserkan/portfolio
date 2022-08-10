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
#include "ATDistance.h"
#include "ATAutoFreq.h"
#include "ATManuelFreq.h"
#include "ATLoading.h"

#define AT_LOADING_ANIM_MS		200

typedef struct {
	uint8_t active_bar_indx;
	WM_HWIN hATLoadingBar;
	WM_CALLBACK *OldDesktopCallback;
	char temp_str[32];
	char temp_str2[32];
} AT_PARA;

static uint8_t new_page;
extern uint16_t const at_auto_freqs[AT_AUTO_FREQ_COUNT];
static 	AT_PARA* pPara = NULL;

static void _cbDraw_LoadingBars(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);

/*********************************************************************
*
*       _cbDraw_LoadingBars
*
*  Function description:
*    callback funtion for updating horizontal bars of loading screen
*/
static void _cbDraw_LoadingBars(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	AT_PARA      * pPara;
	WM_KEY_INFO * pInfo;
	static 	WM_HTIMER ATLoadTimer;

	hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

	switch (pMsg->MsgId) {
		case WM_CREATE:
			// Create animation timer //
 			ATLoadTimer = WM_CreateTimer(hWin, ID_TIMER_AT_LOADING, AT_LOADING_ANIM_MS, 0);
			break;	
		case WM_DELETE:
			WM_DeleteTimer(ATLoadTimer);
			ATLoadTimer = 0;
			break;
		case WM_TIMER: {
		  int Id = WM_GetTimerId(pMsg->Data.v);
		  switch (Id) {
			  case ID_TIMER_AT_LOADING:
			  	if(pPara->active_bar_indx < (AT_LOADING_BAR_COUNT-1))
			  	  pPara->active_bar_indx++;
				else
				    pPara->active_bar_indx = 0;
				WM_InvalidateWindow(hWin);
				WM_RestartTimer(pMsg->Data.v, AT_LOADING_ANIM_MS);
				break;
			  default:	  // Ignore silently // 
				  break;
			}
	  	}
		break;
		case WM_PAINT: 
			if(0 != SBResources[AT_LOADING_PICs][AT_LOADING_BAR1 + pPara->active_bar_indx].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_LOADING_PICs][AT_LOADING_BAR1 + pPara->active_bar_indx].hMemHandle, \
					AT_LOADING_BAR_WIN_LEFT_X, AT_LOADING_BAR_WIN_LEFT_Y);
			}			
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*********************************************************************
*
*       AT_Loading
*
*  Function description:
*    Creates and executes At loading screen //
*/
uint8_t AT_Loading(void) 
{
    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(AT_LOADING_PICs, 0xFF))
		while(TODO_ON_ERR);
		
	new_page = ATRADAR_SCREEN;
	pPara->OldDesktopCallback = NULL;
	pPara->active_bar_indx = 0;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);

	// Create a rectange window for loading bar pictures //
	pPara->hATLoadingBar = \
		WM_CreateWindowAsChild(AT_LOADING_BAR_WIN_LEFT_X, AT_LOADING_BAR_WIN_LEFT_Y, \
			AT_LOADING_BAR_SIZE_X, AT_LOADING_BAR_SIZE_Y, \
				WM_HBKWIN, WM_CF_SHOW, _cbDraw_LoadingBars, sizeof(pPara));
	WM_SetUserData(pPara->hATLoadingBar,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hATLoadingBar);
	
	SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
	// Animation loop for AT_LOADING_SCREEN_DURATION_MS	miliseconds // 
	uint32_t timer_ms_exit = GUI_X_GetTime() + AT_LOADING_SCREEN_DURATION_MS;
	while (timer_ms_exit >= GUI_X_GetTime()) {
		if(!GUI_Exec1())  {	
			vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}

	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
  	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_SetFocus(WM_HBKWIN);
	SB_delete();
	WM_DeleteWindow(pPara->hATLoadingBar);
	pPara->hATLoadingBar = 0;
	
	free(pPara);
	pPara=NULL;
	return new_page;
}

/*********************************************************************
*
*       _cbBk_Desktop
*
*  Function description:
*    Callback routine of desktop window
*/
static void _cbBk_Desktop(WM_MESSAGE * pMsg) {
	uint8_t the_res_indx = 0;
	WM_KEY_INFO * pInfo;

	switch (pMsg->MsgId) {
		case WM_PAINT: {
			uint16_t range_y_pos;
			
			// locate background image // 	
			if(0 != SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle, 0, 0);
			}
			// locate auto frequency target icon if auto-frequency is selected or man-freq icon // 
			if(AT_AUTO_FREQ_SELECTED == APP_GetValue(ST_AT_MAN_AUTO_SEL)) {
				uint8_t auto_freq_indx = APP_GetValue(ST_AT_AUTO_FREQ_INDX);
				// Locate auto frequemcy target type icon // 
				if(0 != SBResources[AT_LOADING_PICs][AT_LOADING_AUTOF_TYPE1 + auto_freq_indx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_LOADING_PICs][AT_LOADING_AUTOF_TYPE1 + auto_freq_indx].hMemHandle, \
						AT_LOADING_ICON_LEFT_X, AT_LOADING_ICON_LEFT_Y);
				}
				// Locate Auto Frequency Target Name // 
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_YELLOW);
				GUI_SetFont(APP_24B_FONT);
				GUI_DispStringAt(GetString(STR_AT_TYPE1_LONG_GOLD_STR + auto_freq_indx), \
					AT_LOADING_NEXT_SCR_STR_LEFT_X, AT_LOADING_NEXT_SCR_STR_LEFT_Y);
				
				// Locate Auto Frequency // 
				memset(pPara->temp_str, 0, sizeof(pPara->temp_str));
				sprintf(pPara->temp_str, "%u.%u %s", at_auto_freqs[auto_freq_indx]/1000, (at_auto_freqs[auto_freq_indx]%1000)/100, \
					GetString(STR_AT_MAN_FREQ_UNIT));
				GUI_DispStringAt(pPara->temp_str, AT_LOADING_NEXT_SCR_STR_LEFT_X, AT_LOADING_NEXT_SCR_STR_LEFT_Y + 20);

				range_y_pos = AT_LOADING_NEXT_SCR_STR_LEFT_Y + 40;
			} else {	
				// Locate AT loading manual freq icon // 
				uint16_t man_freq = APP_GetValue(ST_AT_MAN_FREQ);
				if(0 != SBResources[AT_LOADING_PICs][AT_LOADING_MANFREQ].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_LOADING_PICs][AT_LOADING_MANFREQ].hMemHandle, \
						AT_LOADING_ICON_LEFT_X, AT_LOADING_ICON_LEFT_Y);
				}
				// locate Manual Frequency //
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_YELLOW);
				GUI_SetFont(APP_24B_FONT);
				memset(pPara->temp_str, 0, sizeof(pPara->temp_str));
				sprintf(pPara->temp_str, "%u.%u %s", man_freq/1000, (man_freq%1000)/100, GetString(STR_AT_MAN_FREQ_UNIT));
				GUI_DispStringAt(pPara->temp_str, AT_LOADING_NEXT_SCR_STR_LEFT_X, AT_LOADING_NEXT_SCR_STR_LEFT_Y);

				range_y_pos = AT_LOADING_NEXT_SCR_STR_LEFT_Y + 20;
			}
			// locate target distance string // 
			{
				#if(0)
					const char *range_strs[AT_DISTANCE_RANGE_COUNT] = { AT_RANGE1_NUM_STR, AT_RANGE2_NUM_STR, AT_RANGE3_NUM_STR, AT_RANGE4_NUM_STR};
					memset(temp_str, 0, sizeof(temp_str));
					sprintf(temp_str, "%s %s", range_strs[APP_GetValue(ST_AT_DIST_RANGE)], GetString(STR_AT_DISTANCE_METER));
				#else	
					const char *range_strs[AT_DISTANCE_RANGE_COUNT] = { 
						GetString(STR_SHORT_RANGE), 
						GetString(STR_MID_RANGE),
						GetString(STR_LONG_RANGE),
						GetString(STR_MAXIMUM_RANGE),
					};
					memset(pPara->temp_str2, 0, sizeof(pPara->temp_str2));
					sprintf(pPara->temp_str2, "%s", range_strs[APP_GetValue(ST_AT_DIST_RANGE)]);
				#endif
				GUI_DispStringAt(pPara->temp_str2, AT_LOADING_NEXT_SCR_STR_LEFT_X, range_y_pos);
			}
			// locate at-loading screen string // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_32B_FONT);
			{
				GUI_RECT const grect = {AT_LOADING_STR_LEFT_X, AT_LOADING_STR_LEFT_Y, AT_LOADING_STR_RIGHT_X, AT_LOADING_STR_RIGHT_Y};
				GUI_DispStringInRectWrap(GetString(STR_AT_LOADING_STR), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_CENTER, \
					GUI_WRAPMODE_WORD);
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


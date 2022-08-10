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
#include "ATMenu.h"
#include "ATAutoFreq.h"
#include "ATDistance.h"

typedef struct {
    uint8_t ScreenExit;
	uint8_t ready4pkey;
	uint8_t active_distance_indx;
	WM_HWIN hDistanceWin;
	WM_CALLBACK *OldDesktopCallback;
	uint16_t const at_auto_freqs[AT_AUTO_FREQ_COUNT];
	char str_buf[32];
} AT_PARA;

static uint8_t new_page;

static void _cbDraw_Distance_Select(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);


/*********************************************************************
*
*       _cbDraw
*
*  Function description:
*    callback function for distance selection rectangles drawing window
*/
static void _cbDraw_Distance_Select(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	AT_PARA      * pPara;
	WM_KEY_INFO * pInfo;

	hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

	switch (pMsg->MsgId) {
		case WM_PAINT: { 
			volatile uint8_t indx;

			#if(0)
				const char *range_strs[AT_DISTANCE_RANGE_COUNT] = { AT_RANGE1_NUM_STR, AT_RANGE2_NUM_STR, AT_RANGE3_NUM_STR, AT_RANGE4_NUM_STR};
			#else	
				const char *range_strs[AT_DISTANCE_RANGE_COUNT] = { 
					GetString(STR_SHORT_RANGE), 
					GetString(STR_MID_RANGE),
					GetString(STR_LONG_RANGE),
					GetString(STR_MAXIMUM_RANGE),
				};
			#endif
			for(indx=0 ; indx<AT_DISTANCE_RANGE_COUNT ; indx++) {
				uint8_t pic_ofset = (indx == pPara->active_distance_indx)?1:0;	// If selected RED part, BLACK o.w. //
				// 1- locate RED and BLACK parts // 
				if(0 != SBResources[AT_DISTANCE_PICs][AT_DISTANCE_BLACK_PART + pic_ofset].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_DISTANCE_PICs][AT_DISTANCE_BLACK_PART + pic_ofset].hMemHandle, \
						AT_DISTANCE_RANGE_WIN_LEFT_X, AT_DISTANCE_RANGE_WIN_LEFT_Y + (indx*AT_DISTANCE_PART_SIZE_Y));
				}
				// 2- locate distance range strings //
				{
					memset(pPara->str_buf, 0, sizeof(pPara->str_buf));
					GUI_SetTextMode(GUI_TM_TRANS);
					GUI_SetColor(GUI_YELLOW);
					GUI_SetFont(APP_19B_FONT);
					#if(0)
				    	sprintf(pPara->str_buf, "%s %s", range_strs[indx], GetString(STR_AT_DISTANCE_METER));
					#else
						sprintf(pPara->str_buf, "%s", range_strs[indx]);
					#endif
					{
						GUI_RECT const grect = {0, (indx*AT_DISTANCE_PART_SIZE_Y), + AT_DISTANCE_PART_SIZE_X, \
							((indx+1)*AT_DISTANCE_PART_SIZE_Y)};
						GUI_DispStringInRectWrap(pPara->str_buf, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
							GUI_WRAPMODE_WORD);
					}
				}
			}
			
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("AT DISTANCE KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
			    uint8_t key_state = TRUE;
				switch (pInfo->Key) {
					case KEY_CONFIRM_EVENT:	// OK //
						// store selected distance level into flash memory // 
						APP_SetVal(ST_AT_DIST_RANGE, pPara->active_distance_indx,TRUE);
						// exit and return to ATMenu page
						pPara->ScreenExit = TRUE;
						new_page = AT_LOADING_SCR;
						break;
					case KEY_MENU_EVENT:	// ESC // 
						pPara->ScreenExit = TRUE;
						new_page = AT_MENU;
						break;
					case KEY_DOWN_EVENT:
						if((AT_DISTANCE_RANGE_COUNT-1) > pPara->active_distance_indx)
						    pPara->active_distance_indx++;
						else 
						    pPara->active_distance_indx = 0;
						WM_InvalidateWindow(hWin);
						break;
					case KEY_UP_EVENT:
						break;
						if(0 < pPara->active_distance_indx)
						    pPara->active_distance_indx--;
						else 
						    pPara->active_distance_indx = AT_DISTANCE_RANGE_COUNT-1;
						WM_InvalidateWindow(hWin);
					default:	
					    key_state = FALSE;
						break;
				}
				if(TRUE == key_state)
                    start_dac_audio(BUTTON_OK_SOUND, FALSE); // Dont wait until audio file complete //
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*********************************************************************
*
*       AT_Auto_Freq
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t AT_Distance(void)
{
	int xLCDSize;
	int yLCDSize;
	volatile uint8_t indx;
	AT_PARA			* pPara;

    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(AT_DISTANCE_PICs, 0xFF))
		while(TODO_ON_ERR);

	pPara->ScreenExit = FALSE;
	new_page = AT_MENU;
	pPara->OldDesktopCallback = NULL;
	pPara->active_distance_indx = APP_GetValue(ST_AT_DIST_RANGE);
	pPara->ready4pkey = TRUE;

	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);

	// Create a rectange window on right-side for auto-range selection rectangles // 
	pPara->hDistanceWin = \
		  WM_CreateWindowAsChild(AT_DISTANCE_RANGE_WIN_LEFT_X, AT_DISTANCE_RANGE_WIN_LEFT_Y, AT_DISTANCE_RANGE_WIN_SIZE_X, AT_DISTANCE_RANGE_WIN_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_Distance_Select, sizeof(pPara));
	WM_SetUserData(pPara->hDistanceWin,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hDistanceWin);
	
	SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
	// Animation loop
	while (FALSE == pPara->ScreenExit) {
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
	WM_DeleteWindow(pPara->hDistanceWin);
	
	free(pPara);
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
		case WM_PAINT:
			// locate background image // 	
			if(0 != SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle, 0, 0);
			}
			// locate distance icon //
			if(0 != SBResources[AT_DISTANCE_PICs][AT_DISTANCE_ICON].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_DISTANCE_PICs][AT_DISTANCE_ICON].hMemHandle, AT_DISTANCE_ICON_LEFT_X, AT_DISTANCE_ICON_LEFT_Y);
			}
			// locate screen string // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_24B_FONT);
			{
				GUI_RECT const grect = {AT_SUB_MENU_PAGE_STR_LEFT_X, AT_SUB_MENU_PAGE_STR_LEFT_Y, \
					AT_SUB_MENU_PAGE_STR_LEFT_X + 200, AT_SUB_MENU_PAGE_STR_LEFT_Y + 50};
				GUI_DispStringInRectWrap(GetString(STR_ATM_DISTANCE), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


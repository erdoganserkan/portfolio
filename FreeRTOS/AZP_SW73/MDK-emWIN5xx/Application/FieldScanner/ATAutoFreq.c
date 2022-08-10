#include <stdio.h>
#include <stdlib.h>
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
#include "APlay.h"
#include "ATMenu.h"
#include "ATAutoFreq.h"

typedef struct {
	uint8_t ready4pkey;
	uint8_t ScreenExit;
	uint8_t active_freq_indx;
	WM_CALLBACK *OldDesktopCallback;
	WM_HWIN hFreqSelect;
} AT_PARA;

extern uint16_t const at_auto_freqs[AT_AUTO_FREQ_COUNT];
static uint8_t new_page;

static void _cbDraw(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);


/*********************************************************************
*
*       _cbDraw
*
*  Function description:
*    callback funtion for frequency selection window 
*/
static void _cbDraw(WM_MESSAGE * pMsg) 
{
	WM_HWIN     hWin;
	AT_PARA      * pPara;
	WM_KEY_INFO * pInfo;

	hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

    switch (pMsg->MsgId) {
		case WM_PAINT: { 
			volatile uint8_t indx;
			// locate window background pic // 
			//if(0 != SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_WIN_BACK].hMemHandle) {
				//GUI_MEMDEV_WriteAt(SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_WIN_BACK].hMemHandle, 0, 0);
			//}
			//GUI_SetBkColor(GUI_BLACK);
			//GUI_Clear();
			
			for(indx=0 ; indx<AT_AUTO_FREQ_COUNT ; indx++) {
				uint16_t winpos_x = AT_AUTOF_REC_WINDOW_LEFT_X;
				uint16_t winpos_y = AT_AUTOF_REC_WINDOW_LEFT_Y + ((uint16_t)indx)*AT_AUTO_FREQ_PART_SIZE_Y;
				uint8_t active_part = (pPara->active_freq_indx == indx) ? (AT_AUTO_FREQ_RED_PART) : (AT_AUTO_FREQ_BLACK_PART);
				// 1- Locate PART Pictures (active is RED, others BLACK)
				if(0 != SBResources[AT_AUTO_FREQ_PICs][active_part].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_AUTO_FREQ_PICs][active_part].hMemHandle, winpos_x, winpos_y);
				}
				// 2- Locate Type Icons
				if(0 != SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_TYPE1 + indx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_TYPE1 + indx].hMemHandle, \
						AT_AUTOF_REC_WINDOW_LEFT_X + AT_AUTOF_REC_WINDOW_SIZE_X - AT_FREQ_TYPE_ICON_SIZE_X - 5, \
							AT_AUTOF_REC_WINDOW_LEFT_Y + ((AT_AUTO_FREQ_PART_SIZE_Y-AT_FREQ_TYPE_ICON_SIZE_Y)/2) + ((uint16_t)indx)*AT_AUTO_FREQ_PART_SIZE_Y);
				}
				// 3- Print type strings 
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_CYAN);
				GUI_SetFont(APP_19B_FONT);
				{
					winpos_x -= AT_AUTOF_REC_WINDOW_LEFT_X;
					winpos_y -= AT_AUTOF_REC_WINDOW_LEFT_Y;
					GUI_RECT const grect = {winpos_x, winpos_y, winpos_x + AT_AUTOF_REC_WINDOW_SIZE_X - AT_FREQ_TYPE_ICON_SIZE_X, \
						winpos_y + AT_FREQ_TYPE_PART_SIZE_Y};
					GUI_DispStringInRectWrap(GetString(STR_AT_AUTO_TYPE1_STR+indx), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
						GUI_WRAPMODE_WORD);
				}
			}
		}
		break;
		case WM_KEY:
			TRACEM("AT AUTO FREQ KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
			    uint8_t key_state = TRUE;
				switch (pInfo->Key) {
					case KEY_CONFIRM_EVENT:	// OK //
						// store auto frequency into flash memory and enable auto freq selection // 
						APP_SetVal(ST_AT_AUTO_FREQ_INDX, pPara->active_freq_indx,TRUE);
						APP_SetVal(ST_AT_MAN_AUTO_SEL, AT_AUTO_FREQ_SELECTED, TRUE);
						// exit and return to ATMenu page
						pPara->ScreenExit = TRUE;
						new_page = AT_LOADING_SCR;
						break;
					case KEY_MENU_EVENT:	// ESC // 
					    pPara->ScreenExit = TRUE;
					    new_page = AT_MENU;
						break;
					case KEY_DOWN_EVENT:
						if((AT_AUTO_FREQ_COUNT-1) > pPara->active_freq_indx)
						    pPara->active_freq_indx++;
						else 
						    pPara->active_freq_indx = 0;
						WM_InvalidateWindow(pPara->hFreqSelect);
						break;
					case KEY_UP_EVENT:
						if(0 < pPara->active_freq_indx)
						    pPara->active_freq_indx--;
						else 
						    pPara->active_freq_indx = AT_AUTO_FREQ_COUNT-1;
						WM_InvalidateWindow(pPara->hFreqSelect);
						break;
					default:	
					    key_state = FALSE;
						break;
				}
				if(TRUE == key_state)
						start_dac_audio(BUTTON_OK_SOUND, FALSE); // Dont wait until audio file complete //
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
*       AT_Auto_Freq
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t AT_Auto_Freq(void) 
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
	if(0 != InitGroupRes(AT_AUTO_FREQ_PICs, 0xFF))
		while(TODO_ON_ERR);

	pPara->ScreenExit = FALSE;
	new_page = AT_MENU;
	pPara->OldDesktopCallback = NULL;
	pPara->active_freq_indx = APP_GetValue(ST_AT_AUTO_FREQ_INDX);
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
	pPara->hFreqSelect = \
		  WM_CreateWindowAsChild(AT_AUTOF_REC_WINDOW_LEFT_X, AT_AUTOF_REC_WINDOW_LEFT_Y, AT_AUTOF_REC_WINDOW_SIZE_X, AT_AUTOF_REC_WINDOW_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw, sizeof(pPara));
	WM_SetUserData(pPara->hFreqSelect,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hFreqSelect);
	
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
	WM_DeleteWindow(pPara->hFreqSelect);
	pPara->hFreqSelect = 0;
	
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
			// locate auto frequency icon // 
			if(0 != SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_ICON].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_AUTO_FREQ_PICs][AT_AUTO_FREQ_ICON].hMemHandle, AT_FREQ_ICON_LEFT_X, AT_FREQ_ICON_LEFT_Y);
			}
			// locate screen string // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_24B_FONT);
			{
				GUI_RECT const grect = {AT_SUB_MENU_PAGE_STR_LEFT_X, AT_SUB_MENU_PAGE_STR_LEFT_Y, \
					AT_SUB_MENU_PAGE_STR_LEFT_X + 200, AT_SUB_MENU_PAGE_STR_LEFT_Y + 50};
				GUI_DispStringInRectWrap(GetString(STR_ATM_OTO_FREQ), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_TOP, \
					GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


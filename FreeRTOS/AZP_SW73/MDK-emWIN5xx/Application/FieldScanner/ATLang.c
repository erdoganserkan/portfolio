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
#include "ATLang.h"

#define AT_LANG_ANIM_MS	(250)

typedef struct {
	uint8_t ready4pkey;
	uint8_t icon_state;
    uint8_t ScreenExit;
    uint8_t active_icon; // Active Icon Position Number //
    WM_HWIN hLanSelectWin;
    WM_CALLBACK *OldDesktopCallback;
} AT_PARA;

typedef struct {
	uint16_t x;
	uint16_t y;
} sPoint;

static WM_HTIMER hTimerAnim;
static uint8_t new_page;
static sPoint const win_up_points[AT_LANG_COUNT]= {
	{AT_LANG_PTR_LANG1_LEFT_X, AT_LANG_PTR_LANG1_LEFT_Y},
    {AT_LANG_PTR_LANG2_LEFT_X, AT_LANG_PTR_LANG2_LEFT_Y},
    {AT_LANG_PTR_LANG3_LEFT_X, AT_LANG_PTR_LANG3_LEFT_Y},
    {AT_LANG_PTR_LANG4_LEFT_X, AT_LANG_PTR_LANG4_LEFT_Y},
    {AT_LANG_PTR_LANG5_LEFT_X, AT_LANG_PTR_LANG5_LEFT_Y},
    {AT_LANG_PTR_LANG6_LEFT_X, AT_LANG_PTR_LANG6_LEFT_Y},
};

static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void _cbBk_Lang(WM_MESSAGE * pMsg);

static void _cbBk_Lang(WM_MESSAGE * pMsg)
{
    WM_HWIN     hWin;
    AT_PARA      * pPara;
    WM_KEY_INFO * pInfo;

    hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

    switch (pMsg->MsgId) {
        case WM_PAINT: {
            volatile uint8_t indx;
            // Locate background picture //
            if(0 != SBResources[AT_LANG_PICs][AT_LANGs_WIN_BACK].hMemHandle) {
                GUI_MEMDEV_WriteAt(SBResources[AT_LANG_PICs][AT_LANGs_WIN_BACK].hMemHandle, \
					AT_LANG_ICON_WIN_LEFT_X, AT_LANG_ICON_WIN_LEFT_Y);
            }

            // do pointer animation //
            if(pPara->icon_state) {
                if(0 != SBResources[AT_LANG_PICs][AT_LANG_PTR].hMemHandle) {
                    GUI_MEMDEV_WriteAt(SBResources[AT_LANG_PICs][AT_LANG_PTR].hMemHandle, \
                        win_up_points[pPara->active_icon].x, win_up_points[pPara->active_icon].y);
                }
            }
        }
        break;
        case WM_CREATE:
            hTimerAnim = WM_CreateTimer(hWin, ID_TIMER_AT_LANG, AT_LANG_ANIM_MS, 0);
            break;
        case WM_DELETE:
            WM_DeleteTimer(hTimerAnim);
            break;
        case WM_SET_FOCUS:
            pMsg->Data.v = 0;
            break;
        case WM_TIMER: {
            int Id = WM_GetTimerId(pMsg->Data.v);
            switch (Id) {
                case ID_TIMER_AT_LANG: {
                    pPara->icon_state ^= 1;
                    WM_InvalidateWindow(hWin);
                    WM_RestartTimer(pMsg->Data.v, AT_LANG_ANIM_MS);
                }
                break;
                default:
                    break;
            }
        }
        break;
        case WM_KEY:
            TRACEM("AT LANGUAGE KEY Handler Working");
            if(TRUE != pPara->ready4pkey)
                break;  // DONT process key press events if GUI not ready //
            pInfo = (WM_KEY_INFO *)pMsg->Data.p;
            if (pInfo->PressedCnt) {
                uint8_t key_state = TRUE;
                switch (pInfo->Key) {
                    case KEY_CONFIRM_EVENT: // OK //
                        // store auto frequency into flash memory and enable auto freq selection //
                        APP_SetVal(ST_AT_LANG, pPara->active_icon, TRUE);
                        // exit and return to ATMenu page
                        pPara->ScreenExit = TRUE;
                        new_page = AT_MENU;
                        break;
                    case KEY_MENU_EVENT:    // ESC //
                        pPara->ScreenExit = TRUE;
                        new_page = AT_MENU;
                        break;
                    case KEY_LEFT_EVENT:
                        if(pPara->active_icon > 0)
                            pPara->active_icon--;
                        else
                            pPara->active_icon = AT_LANG_COUNT-1;
                        WM_InvalidateWindow(hWin);
                        break;
                    case KEY_RIGHT_EVENT:
                        if((AT_LANG_COUNT-1) > pPara->active_icon)
                            pPara->active_icon++;
                        else
                            pPara->active_icon = 0;
                        WM_InvalidateWindow(hWin);
                        break;
                    default:
                        key_state = FALSE;
                        break;
                }
                if(TRUE == key_state)
                    start_dac_audio(BUTTON_OK_SOUND, FALSE); // Dont wait until audio file complete //
            }
            break;
        case GUI_USER_LANG_CHANGED:
            TRACEM("GUI_USER_LANG_CHANGED event received\n");
            break;
        default:
            WM_DefaultProc(pMsg);
            break;
    }
}

/*********************************************************************
*
*       AT_Lang
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t AT_Lang(void)
{
	int xLCDSize;
	int yLCDSize;
	volatile uint8_t indx;
	AT_PARA *pPara;

    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))	
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(AT_LANG_PICs, 0xFF))	
		while(TODO_ON_ERR);

	pPara->ScreenExit = FALSE;
	new_page = AT_MENU;
	pPara->OldDesktopCallback = NULL;
	pPara->icon_state = 0;
	pPara->active_icon = APP_GetValue(ST_AT_LANG);
	hTimerAnim = 0;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	pPara->ready4pkey = TRUE;

	WM_InvalidateWindow(WM_HBKWIN);

    // Create a rectange window on right-side //
    pPara->hLanSelectWin = \
          WM_CreateWindowAsChild(AT_LANG_ICON_WIN_LEFT_X, AT_LANG_ICON_WIN_LEFT_Y, AT_LANG_ICON_WIN_SIZE_X, AT_LANG_ICON_WIN_SIZE_Y, \
              WM_HBKWIN, WM_CF_SHOW, _cbBk_Lang, sizeof(pPara));
    WM_SetUserData(pPara->hLanSelectWin,   &pPara, sizeof(pPara));
    WM_SetFocus(pPara->hLanSelectWin);

    SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
	// Animation loop
	while (FALSE == pPara->ScreenExit) {
		if(!GUI_Exec1())  {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
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
    WM_DeleteWindow(pPara->hLanSelectWin);
	
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
            // Locate language icon //
            if(0 != SBResources[AT_LANG_PICs][AT_LANG_ICON].hMemHandle) {
                GUI_MEMDEV_WriteAt(SBResources[AT_LANG_PICs][AT_LANG_ICON].hMemHandle, \
                    AT_LANG_ICON_LEFT_X, AT_LANG_ICON_LEFT_Y);
            }
		    // Locate screen string //
            GUI_SetTextMode(GUI_TM_TRANS);
            GUI_SetColor(GUI_YELLOW);
            GUI_SetFont(APP_32B_FONT);
			{
				GUI_RECT const grect = {AT_SUB_MENU_PAGE_STR_LEFT_X, AT_SUB_MENU_PAGE_STR_LEFT_Y, \
					AT_SUB_MENU_PAGE_STR_LEFT_X + 200, AT_SUB_MENU_PAGE_STR_LEFT_Y + 50};
				GUI_DispStringInRectWrap(GetString(STR_ATM_LANG), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/

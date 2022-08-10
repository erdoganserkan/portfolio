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
#include "ATBrightVol.h"

typedef struct {
	uint8_t ready4pkey;
	uint8_t ScreenExit;
	uint8_t bar_indx;
	uint16_t vol_backup;
	uint16_t bright_backup;
	WM_CALLBACK *OldDesktopCallback;
    char str_buf[32];
} AT_PARA;

extern uint8_t active_page; // Defined in GUIDEMO_Start.c module //
static uint8_t new_page;

static void _cbDraw_BarsAndNums(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);

/*********************************************************************
*
*       _cbDraw
*
*  Function description:
*    callback funtion for frequency selection window 
*/
static void _cbDraw_BarsAndNums(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	AT_PARA      * pPara;
	WM_KEY_INFO * pInfo;

	hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

    switch (pMsg->MsgId) {
		case WM_PAINT: {
		    GUI_RECT gRect = {0, 0, AT_BRIGHT_VOL_WIN_SIZE_X-AT_BRIGHT_VOL_BAR_SIZE_X, AT_BRIGHT_VOL_WIN_SIZE_Y};
			// Draw Digit Surrounding Rectangle Frame //
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			GUI_DrawGradientRoundedH(2, 2, AT_BRIGHT_VOL_WIN_SIZE_X-AT_BRIGHT_VOL_BAR_SIZE_X-2, AT_BRIGHT_VOL_WIN_SIZE_Y-2,\
				20, GUI_DARKRED, GUI_LIGHTBLUE);
			GUI_SetColor(GUI_YELLOW);
			GUI_DrawRoundedRect(0,0,AT_BRIGHT_VOL_WIN_SIZE_X-AT_BRIGHT_VOL_BAR_SIZE_X, AT_BRIGHT_VOL_WIN_SIZE_Y, 20);
			GUI_SetColor(GUI_BLACK);
			GUI_DrawGradientRoundedV(15, 15, AT_BRIGHT_VOL_WIN_SIZE_X-AT_BRIGHT_VOL_BAR_SIZE_X-15, AT_BRIGHT_VOL_WIN_SIZE_Y-15,\
				15, GUI_DARKYELLOW, GUI_LIGHTGREEN);
			GUI_DrawRoundedRect(14,14,AT_BRIGHT_VOL_WIN_SIZE_X-AT_BRIGHT_VOL_BAR_SIZE_X-14, AT_BRIGHT_VOL_WIN_SIZE_Y-14,15);
			// Draw Level (Brihtness/Volume) Digits in that Rectangle //
			GUI_SetTextMode(GUI_TM_TRANS);
			if(10 != pPara->bar_indx) 
				GUI_SetFont(&GUI_FontD48);
			else
				GUI_SetFont(&GUI_FontD32);
			sprintf(pPara->str_buf, "%u", pPara->bar_indx*10);
			GUI_DispStringInRectWrap(pPara->str_buf, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			// Locate Bar-Level Pictures //
			{
			    for(volatile uint16_t indx = 0 ;indx<=AT_BRIGHT_VOL_BAR_COUNT; indx++) {
			        uint16_t active_posy = AT_BRIGHT_VOL_BAR_LEFT_Y + (AT_BRIGHT_VOL_BAR_SIZE_Y*(AT_BRIGHT_VOL_BAR_COUNT - indx));
			        if(((indx <= pPara->bar_indx) || (AT_BRIGHT_VOL_BAR_COUNT == pPara->bar_indx)) && (0 != pPara->bar_indx)){
	                    if(0 != SBResources[AT_BRIGHT_VOL_PICs][AT_SET_FULL].hMemHandle) {
	                        GUI_MEMDEV_WriteAt(SBResources[AT_BRIGHT_VOL_PICs][AT_SET_FULL].hMemHandle, \
	                            (AT_BRIGHT_VOL_BAR_LEFT_X), active_posy);
	                    }
			        } else {
	                    if(0 != SBResources[AT_BRIGHT_VOL_PICs][AT_SET_NULL].hMemHandle) {
	                        GUI_MEMDEV_WriteAt(SBResources[AT_BRIGHT_VOL_PICs][AT_SET_NULL].hMemHandle, \
	                            (AT_BRIGHT_VOL_BAR_LEFT_X), active_posy);
	                    }
			        }
			    }
			}
		}
		break;
        case WM_KEY:
            TRACEM("AT AUTO FREQ KEY Handler Working");
            if(TRUE != pPara->ready4pkey)
                break;  // DONT process key press events if GUI not ready //
            pInfo = (WM_KEY_INFO *)pMsg->Data.p;
            if (pInfo->PressedCnt) {
                uint8_t key_state = TRUE;
                switch (pInfo->Key) {
                    case KEY_CONFIRM_EVENT: // OK //
                        if(AT_BRIGHT_SCR == active_page) {
                            APP_SetVal(ST_AT_BRIGTH, pPara->bar_indx * AT_BRIGHT_VOL_BAR_COUNT, TRUE);
                        } else {
							APP_SetVal(ST_AT_VOL, pPara->bar_indx * AT_BRIGHT_VOL_BAR_COUNT, TRUE);
							if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))	// Reinit all audio files with new Volume level //	
								while(TODO_ON_ERR);
							key_state = FALSE;	// Dont play button-ok sound // 
                        }
                        pPara->ScreenExit = TRUE;
                        new_page = AT_MENU;
                        break;
                    case KEY_MENU_EVENT:    // ESC //
                        if(AT_BRIGHT_SCR == active_page) {
							BSP_PWMSet(0, BSP_PWM_LCD, pPara->bright_backup);
                        } else {
							// Set back OLD Volume level into Sound HW // 
							APP_SetVal(ST_AT_VOL, pPara->vol_backup, FALSE);	// Dont store to flash, only change in RAM // 
							if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND)) 
								while(TODO_ON_ERR);
							key_state = FALSE;	// Dont play button-ok sound // 
						}
                        pPara->ScreenExit = TRUE;
                        new_page = AT_MENU;
                        break;
                    case KEY_UP_EVENT:
                        if((AT_BRIGHT_VOL_BAR_COUNT) > pPara->bar_indx)
                            pPara->bar_indx++;
						if(AT_BRIGHT_SCR == active_page)
							BSP_PWMSet(0, BSP_PWM_LCD, AT_BRIGHT_VOL_BAR_COUNT * pPara->bar_indx);
						else {
							APP_SetVal(ST_AT_VOL, pPara->bar_indx * AT_BRIGHT_VOL_BAR_COUNT, FALSE);
							if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND))	
								while(TODO_ON_ERR);
							start_dac_audio(SAMPLE_SOUND, FALSE); // Dont wait until finished, DMA & ISR will handle rest // 
							key_state = FALSE;	// Dont play button-ok sound // 
						}
                        WM_InvalidateWindow(hWin);
                        break;
                    case KEY_DOWN_EVENT:
                        if(0 < pPara->bar_indx)
                            pPara->bar_indx--;
						if(AT_BRIGHT_SCR == active_page) {
							if(0 != pPara->bar_indx) 
								BSP_PWMSet(0, BSP_PWM_LCD, pPara->bar_indx * AT_BRIGHT_VOL_BAR_COUNT);
							else
								BSP_PWMSet(0, BSP_PWM_LCD, 1);								
						} else if (AT_VOL_SCR == active_page) {
							APP_SetVal(ST_AT_VOL, pPara->bar_indx * AT_BRIGHT_VOL_BAR_COUNT, FALSE);
							if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND))	
								while(TODO_ON_ERR);
							start_dac_audio(SAMPLE_SOUND, FALSE); // Dont wait until finished, DMA & ISR will handle rest // 
							key_state = FALSE;	// Dont play button-ok sound // 
						}
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
uint8_t AT_BrightVol(void) 
{
	int xLCDSize;
	int yLCDSize;
	volatile uint8_t indx;
	AT_PARA			* pPara;
	WM_HWIN hATBrightVol;   //:TODO: If crashes do this static //

    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(AT_BRIGHT_VOL_PICs, 0xFF))
		while(TODO_ON_ERR);

	new_page = ATRADAR_SCREEN;
	pPara->ScreenExit = FALSE;
	pPara->OldDesktopCallback = NULL;
	if(AT_BRIGHT_SCR == active_page) {
		pPara->bright_backup = APP_GetValue(ST_AT_BRIGTH);
		pPara->bar_indx = pPara->bright_backup / AT_BRIGHT_VOL_BAR_COUNT;
	} else { 
		pPara->vol_backup = APP_GetValue(ST_AT_VOL);
	    pPara->bar_indx= pPara->vol_backup / AT_BRIGHT_VOL_BAR_COUNT;
	}
	pPara->ready4pkey = TRUE;
		
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);

	// Create a rectange window for loading bar pictures //
	hATBrightVol = \
		WM_CreateWindowAsChild(AT_BRIGHT_VOL_WIN_LEFT_X, AT_BRIGHT_VOL_WIN_LEFT_Y, \
			AT_BRIGHT_VOL_WIN_SIZE_X, AT_BRIGHT_VOL_WIN_SIZE_Y, \
				WM_HBKWIN, WM_CF_SHOW, _cbDraw_BarsAndNums, sizeof(pPara));
	WM_SetUserData(hATBrightVol,   &pPara, sizeof(pPara));
	WM_SetFocus(hATBrightVol);
	
	SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
	// Animation loop for AT_LOADING_SCREEN_DURATION_MS	miliseconds // 
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
	WM_DeleteWindow(hATBrightVol);
	
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
		case WM_PAINT: {
			static char temp_str[32];
			// locate background image // 	
			if(0 != SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle, 0, 0);
			}
			// Locate screen icons // 
			if(AT_BRIGHT_SCR == active_page) {
				if(0 != SBResources[AT_BRIGHT_VOL_PICs][AT_BRIGHT_ICON].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_BRIGHT_VOL_PICs][AT_BRIGHT_ICON].hMemHandle, \
						AT_BRIGHT_VOL_ICON_LEFT_X, AT_BRIGHT_VOL_ICON_LEFT_Y);
				}
			} else {
				if(0 != SBResources[AT_BRIGHT_VOL_PICs][AT_VOL_ICON].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_BRIGHT_VOL_PICs][AT_VOL_ICON].hMemHandle, \
						AT_BRIGHT_VOL_ICON_LEFT_X, AT_BRIGHT_VOL_ICON_LEFT_Y);
				}
			}
			// locate brightness/volume screen string // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_32B_FONT);
			if(AT_BRIGHT_SCR == active_page)
				GUI_DispStringAt(GetString(STR_BRIGHT_INDX), AT_BRIGHT_VOL_STR_LEFT_X, AT_BRIGHT_VOL_STR_LEFT_Y);
			else 
				GUI_DispStringAt(GetString(STR_VOLUME_INDX), AT_BRIGHT_VOL_STR_LEFT_X, AT_BRIGHT_VOL_STR_LEFT_Y);
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


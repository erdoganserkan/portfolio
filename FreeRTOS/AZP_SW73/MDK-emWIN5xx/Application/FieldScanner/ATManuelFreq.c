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
#include "ATManuelFreq.h"

#define AT_MANUEL_ANIM_MS	(250)

typedef struct {
	uint8_t edit_section;
	uint8_t show_edited;
	uint8_t freq_changed;
	uint8_t ScreenExit;
	uint8_t ready4pkey;
	uint16_t man_freq;
	WM_HWIN hManFreqWin;
	WM_CALLBACK *OldDesktopCallback;
	char str_buf1[16];
	char str_buf2[16];
	char str_buf_float[32];
} AT_PARA;

static uint8_t new_page;
static WM_HTIMER hTimerAnim;

static void _cbDraw_ManFreq_Recs(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);


/*********************************************************************
*
*       _cbDraw_Distance_Select
*
*  Function description:
*    callback function for distance selection rectangles drawing window
*/
static void _cbDraw_ManFreq_Recs(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	AT_PARA      * pPara;
	WM_KEY_INFO * pInfo;

	hWin = pMsg->hWin;
    WM_GetUserData(hWin, &pPara, sizeof(pPara));

	switch (pMsg->MsgId) {
		case WM_PAINT: { 
			if(TRUE == pPara->freq_changed) {
				uint16_t num1 = pPara->man_freq/1000;
				uint16_t num2 = (pPara->man_freq - (num1 * 1000)) / 100;

				// init & set num1 buffer // 
				memset(pPara->str_buf1, 0, sizeof(pPara->str_buf1));
				sprintf(pPara->str_buf1, "%u", num1);
				// init & set num2 buffer // 
				memset(pPara->str_buf2, 0, sizeof(pPara->str_buf2));
				sprintf(pPara->str_buf2, "%u", num2);

				// init & set float string buffer // 
				memset(pPara->str_buf_float, 0, sizeof(pPara->str_buf_float));
				sprintf(pPara->str_buf_float, "%u.%u", num1, num2);
			}

			// 1- Locate digit-win background picture // 
			if(0 != SBResources[AT_MAN_FREQ_PICs][AT_MAN_FREQ_DIGIT_WIN_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MAN_FREQ_PICs][AT_MAN_FREQ_DIGIT_WIN_BACK].hMemHandle, \
					AT_MAN_FREQ_WIN_LEFT_X, AT_MAN_FREQ_WIN_LEFT_Y);
			}
			// Locate 32 punto numbers in both of ellipses // 
            {
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_YELLOW);
				GUI_SetFont(APP_32B_FONT);

                if(0 == pPara->edit_section) {
					if(TRUE == pPara->show_edited)
						GUI_DispStringAt(pPara->str_buf1, AT_MAN_FREQ_NUM1_LEFT_X, AT_MAN_FREQ_NUM1_LEFT_Y);
					GUI_DispStringAt(pPara->str_buf2, AT_MAN_FREQ_NUM2_LEFT_X, AT_MAN_FREQ_NUM2_LEFT_Y);
                }
				else if(1 == pPara->edit_section) {
					GUI_DispStringAt(pPara->str_buf1, AT_MAN_FREQ_NUM1_LEFT_X, AT_MAN_FREQ_NUM1_LEFT_Y);
					if(TRUE == pPara->show_edited)
						GUI_DispStringAt(pPara->str_buf2, AT_MAN_FREQ_NUM2_LEFT_X, AT_MAN_FREQ_NUM2_LEFT_Y);
				}
            }

            // Locate Floating String & Unit //
			GUI_DispStringAt(pPara->str_buf_float, AT_MAN_FREQ_FLOAT_STR_LEFT_X, AT_MAN_FREQ_FLOAT_STR_LEFT_Y);
            GUI_DispStringAt(GetString(STR_AT_MAN_FREQ_UNIT), AT_MAN_FREQ_FLOAT_STR_UNIT_LEFT_X, AT_MAN_FREQ_FLOAT_STR_UNIT_LEFT_Y);
		}
		break;
        case WM_CREATE:
            hTimerAnim = WM_CreateTimer(hWin, ID_TIMER_AT_MANUEL, AT_MANUEL_ANIM_MS, 0);
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
				case ID_TIMER_AT_MANUEL: {
					pPara->show_edited ^= 1;
					WM_InvalidateWindow(hWin);
					WM_RestartTimer(pMsg->Data.v, AT_MANUEL_ANIM_MS);
				}
				break;
				default:
					break;
			}
		}
		break;
		case WM_KEY:
			TRACEM("AT MAN FREQ KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
			    uint8_t key_state = TRUE;
				switch (pInfo->Key) {
					case KEY_CONFIRM_EVENT:	// OK //
						if(0 == pPara->edit_section) 
							pPara->edit_section = 1;
						else if(1 == pPara->edit_section) {
							// store auto frequency into flash memory and enable auto freq selection // 
							APP_SetVal(ST_AT_MAN_FREQ, pPara->man_freq, TRUE);
	                        APP_SetVal(ST_AT_MAN_AUTO_SEL, AT_MAN_FREQ_SELECTED, TRUE);
							// exit and return to ATMenu page
							pPara->ScreenExit = TRUE;
							new_page = AT_LOADING_SCR;
						}
						break;
					case KEY_MENU_EVENT:	// ESC // 
						pPara->ScreenExit = TRUE;
						new_page = AT_MENU;
						break;
					case KEY_DOWN_EVENT:
						if(0 == pPara->edit_section) {	// right side editing // 
							if((AT_MAN_FREQ_MIN + (AT_MAN_FREQ_STEPs_HZ * 10)) <= pPara->man_freq) {
							    pPara->man_freq -= (AT_MAN_FREQ_STEPs_HZ * 10);
								pPara->freq_changed = TRUE;	
							}
						} else {	// left side editing // 
							if((AT_MAN_FREQ_MIN + (AT_MAN_FREQ_STEPs_HZ)) <= pPara->man_freq) {
							    pPara->man_freq -= (AT_MAN_FREQ_STEPs_HZ);
								pPara->freq_changed = TRUE;	
							}
						}
						WM_InvalidateWindow(hWin);
						break;
					case KEY_UP_EVENT:
						if(0 == pPara->edit_section) {
	                        if((AT_MAN_FREQ_MAX - (AT_MAN_FREQ_STEPs_HZ * 10)) >= pPara->man_freq) {
	                            pPara->man_freq += (AT_MAN_FREQ_STEPs_HZ * 10); 
								pPara->freq_changed = TRUE;
							}
						} else {
	                        if((AT_MAN_FREQ_MAX - AT_MAN_FREQ_STEPs_HZ) >= pPara->man_freq) {
	                            pPara->man_freq += AT_MAN_FREQ_STEPs_HZ;
								pPara->freq_changed = TRUE;
							}
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
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*********************************************************************
*
*       AT_ManuelFreq
*
*  Function description:
*    Creates and executes manuel frequency setting resourcess
*/
uint8_t AT_ManuelFreq(void)
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
	if(0 != InitGroupRes(AT_MAN_FREQ_PICs, 0xFF))
		while(TODO_ON_ERR);

	pPara->ScreenExit = FALSE;
	new_page = AT_MENU;
	pPara->OldDesktopCallback = NULL;
	pPara->man_freq = APP_GetValue(ST_AT_MAN_FREQ);
	pPara->ready4pkey = TRUE;
	pPara->edit_section = 0;
	pPara->show_edited = TRUE;
	pPara->freq_changed = TRUE;	// Force initial values setting to be done // 
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);

	// Create a rectange window on right-side for manuel frequency numbers //
	pPara->hManFreqWin = \
		  WM_CreateWindowAsChild(AT_MAN_FREQ_WIN_LEFT_X, AT_MAN_FREQ_WIN_LEFT_Y, AT_MAN_FREQ_WIN_SIZE_X, AT_MAN_FREQ_WIN_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_ManFreq_Recs, sizeof(pPara));
	WM_SetUserData(pPara->hManFreqWin,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hManFreqWin);
	
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
	WM_DeleteWindow(pPara->hManFreqWin);
	
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

	switch (pMsg->MsgId) {
		case WM_PAINT:
			// locate background image // 	
			if(0 != SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_NULL_BACK].hMemHandle, 0, 0);
			}
			// locate at manuel frequency icon //
			if(0 != SBResources[AT_MAN_FREQ_PICs][AT_MAN_FREQ_ICON].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_MAN_FREQ_PICs][AT_MAN_FREQ_ICON].hMemHandle, AT_MAN_FREQ_ICON_LEFT_X, AT_MAN_FREQ_ICON_LEFT_Y);
			}
			// locate screen string // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_24B_FONT);
			{
				GUI_RECT const grect = {AT_SUB_MENU_PAGE_STR_LEFT_X, AT_SUB_MENU_PAGE_STR_LEFT_Y, \
					AT_SUB_MENU_PAGE_STR_LEFT_X + 200, AT_SUB_MENU_PAGE_STR_LEFT_Y + 50};
				GUI_DispStringInRectWrap(GetString(STR_ATM_MAN_FREQ), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_TOP, \
					GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


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
#include "APlay.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h" 
#include "RuntimeLoader.h"
#include "SYSSettings.h"
#include "Dac.h"
#include "APlay.h"
#include "DevSelect.h"

#define DEV_SELECT_ANIM_MS	(250)

typedef struct {
	uint8_t ready4pkey;
	uint8_t first_update;
	uint8_t icon_state;
	uint8_t ScreenExit;
	uint8_t active_win;
	uint8_t new_active_win;
	int screen_enter_time_ms;
	WM_HTIMER hTimerAnim;
	WM_HWIN  hDetectSelect; // Left Side window //
    WM_HWIN  hFSSelect;     // Right Side Window //
    GUI_RECT gRects[3];	
    WM_CALLBACK *OldDesktopCallback;
} AT_PARA;

typedef struct {
	uint16_t x;
	uint16_t y;
} sPoint;

static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static uint8_t const new_page = SYS_LOADING_SCR;
static AT_PARA *pPara;
static void on_key_ok_pressed(uint8_t store_selection);

uint8_t Dev_Select(void)
{
    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(DEV_SELECT_PICs, 0xFF))	// AT_PICs Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

    pPara->active_win = 1;
    pPara->new_active_win = 0;
    pPara->ScreenExit = SCR_RUNNING;
	pPara->OldDesktopCallback = NULL;
	pPara->hTimerAnim = 0;
	pPara->icon_state = 0;
	pPara->first_update = TRUE;
	pPara->ready4pkey = TRUE;
	pPara->screen_enter_time_ms = GUI_X_GetTime();
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);
	WM_SetFocus(WM_HBKWIN);
	
	SB_init(SB_REDUCED_MODE_USE_RIGHT);

	// WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
	// Modulator frequency is 3 times increased for GB screen, this will cause faster frequency switch on dac output // 
	// WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_JACK_DETECT_AMP);	// Start DAC Wave for minimum GAUGE // 
	// WAVE_Update_FreqAmp_Gauge((uint16_t)10*GAUGE_FRACTUATION);	// Update Sound Frequency // 

	// Animation loop	
	while (likely(SCR_EXIT_CONFIRMED != pPara->ScreenExit)) {
		if(!GUI_Exec1())  {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
		if((SCR_RUNNING == pPara->ScreenExit) && (DEV_SELECT_TIMEOUT_MS < (GUI_X_GetTime() - pPara->screen_enter_time_ms))) {
			DEBUGM("DEV SELECT TIMEOUT occured\n");
			on_key_ok_pressed(FALSE);
		}
	}

	//WAVE_Generator_stop(TRUE, TRUE, TRUE);

	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	SB_delete();
	
	free(pPara);
	pPara = NULL;
	return new_page;
}

static void locate_dev_type_string(uint8_t dev_type, uint8_t str_state, uint8_t bckgrnd_state) {
	if(APP_IS_DETECTOR== dev_type) {
		GUI_RECT gx = { DEV_SELECT_DETECTOR_STR_LEFT_X-2, DEV_SELECT_DETECTOR_STR_LEFT_Y-2, \
			DEV_SELECT_DETECTOR_STR_RIGHT_X, DEV_SELECT_DETECTOR_STR_RIGHT_Y };
		if(TRUE == bckgrnd_state)
			GUI_DrawGradientRoundedH(gx.x0, gx.y0, gx.x1, gx.y1, 5, GUI_LIGHTRED, GUI_DARKRED);
		if(TRUE == str_state) {
			pPara->gRects[1] = gx;
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_24B_FONT);
			GUI_DispStringInRectWrap(GetString(STR_DETECTOR), &(pPara->gRects[1]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
				GUI_WRAPMODE_WORD);
		} 
	} else if(APP_IS_FIELD_SCANNER == dev_type) {
		GUI_RECT gx = { DEV_SELECT_FS_STR_LEFT_X-2, DEV_SELECT_FS_STR_LEFT_Y-2, DEV_SELECT_FS_STR_RIGHT_X, \
			DEV_SELECT_FS_STR_RIGHT_Y };
		if(TRUE == bckgrnd_state)
			GUI_DrawGradientRoundedH(gx.x0, gx.y0, gx.x1, gx.y1, 5, GUI_LIGHTBLUE, GUI_DARKBLUE);
		if(TRUE == str_state) {
			pPara->gRects[2] = gx;
			GUI_SetColor(GUI_YELLOW);
			GUI_SetFont(APP_24B_FONT);
			GUI_DispStringInRectWrap(GetString(STR_FIELD_SCANNER), &(pPara->gRects[2]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
				GUI_WRAPMODE_WORD);
			}
	}
	else 
		while(STALLE_ON_ERR);
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
        case WM_SET_FOCUS:
            pMsg->Data.v = 0;
            break;
		case WM_PAINT:
			if(TRUE == pPara->first_update) {
				pPara->first_update = FALSE;
                // Locate Desktop background picture //
                if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_BACK].hMemHandle) {
                    GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_BACK].hMemHandle, 0, 0);
                }
                // Locate Screen String //
                {
					GUI_RECT gx = { DEV_SELECT_STR_LEFT_X-2, DEV_SELECT_STR_LEFT_Y-2, DEV_SELECT_STR_RIGHT_X, DEV_SELECT_STR_RIGHT_Y };
	                pPara->gRects[0] = gx;
					GUI_DrawGradientRoundedH(gx.x0, gx.y0, gx.x1, gx.y1, 5, GUI_LIGHTYELLOW, GUI_WHITE);
	                GUI_SetTextMode(GUI_TM_TRANS);
	                GUI_SetColor(GUI_BLACK);
	                if(LANG_EN != APP_GetValue(ST_LANG))
						GUI_SetFont(APP_32B_FONT);
					else
						GUI_SetFont(APP_24B_FONT);
	                GUI_DispStringInRectWrap(GetString(STR_SELECT_DEVICE), &(pPara->gRects[0]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
						GUI_WRAPMODE_WORD);
                }
				// Locate Detector String // 
				locate_dev_type_string(APP_IS_DETECTOR, TRUE,  TRUE);	// Both of detector atr & background are TRUE // 
				
				// Locate Field Scanner String // 
				locate_dev_type_string(APP_IS_FIELD_SCANNER, TRUE, TRUE);	// both of fs str and background are TRUE // 
				// create animation timer // 
				pPara->hTimerAnim = WM_CreateTimer(WM_HBKWIN, ID_TIMER_DEV_SELECT_ANIM, DEV_SELECT_ANIM_MS*2, 0);
			}
			// Update Left & Right Selection Pictures //
            if(pPara->new_active_win != pPara->active_win) {
                pPara->active_win = pPara->new_active_win;
                pPara->icon_state = 0;

                // Reset BOTH windows to initial state //
                if(0 == pPara->active_win) {
                    if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_DETECTOR_and_ARROW].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_DETECTOR_and_ARROW].hMemHandle, \
                            DEV_SELECT_DETECTOR_LEFT_X, DEV_SELECT_DETECTOR_LEFT_Y);
                    }
                    if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_FS].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_FS].hMemHandle, \
                            DEV_SELECT_FS_LEFT_X, DEV_SELECT_FS_LEFT_Y);
                    }
                } else {
                    if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_DETECTOR].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_DETECTOR].hMemHandle, \
                            DEV_SELECT_DETECTOR_LEFT_X, DEV_SELECT_DETECTOR_LEFT_Y);
                    }
                    if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_FS_and_ARROW].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITH_FS_and_ARROW].hMemHandle, \
                            DEV_SELECT_FS_LEFT_X, DEV_SELECT_FS_LEFT_Y);
                    }
                }
            } else {    // Update active window //
				if(SCR_EXIT_REQUESTED == pPara->ScreenExit) {
					// Locate Desktop background picture //
					if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_BACK].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_BACK].hMemHandle, 0, 0);
					}
					// Locate Screen String //
					{
						GUI_RECT gx = { DEV_SELECT_STR_LEFT_X-2, DEV_SELECT_STR_LEFT_Y-2, DEV_SELECT_STR_RIGHT_X, DEV_SELECT_STR_RIGHT_Y };
						pPara->gRects[0] = gx;
						GUI_DrawGradientRoundedH(gx.x0, gx.y0, gx.x1, gx.y1, 5, GUI_LIGHTYELLOW, GUI_WHITE);
						GUI_SetTextMode(GUI_TM_TRANS);
						GUI_SetColor(GUI_BLACK);
						if(LANG_EN != APP_GetValue(ST_LANG))
							GUI_SetFont(APP_32B_FONT);
						else
							GUI_SetFont(APP_24B_FONT);
						GUI_DispStringInRectWrap(GetString(STR_SELECT_DEVICE), &(pPara->gRects[0]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
							GUI_WRAPMODE_WORD);
					}
				}
                if(0 == pPara->active_win) {
                    uint8_t const pic_indx[3] = {DEV_SELECT_WITHOUT_DETECTOR, DEV_SELECT_WITH_DETECTOR, DEV_SELECT_WITH_DETECTOR_and_ARROW};
					if(SCR_EXIT_REQUESTED == pPara->ScreenExit) {	// Hide FS pictures // 
						if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITHOUT_FS].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITHOUT_FS].hMemHandle, \
								DEV_SELECT_FS_LEFT_X, DEV_SELECT_FS_LEFT_Y);
						}
						// SET FS STRING STATE // 
						if(SCR_EXIT_REQUESTED == pPara->ScreenExit) {
							locate_dev_type_string(APP_IS_FIELD_SCANNER, FALSE, FALSE);	// fs str & background FALSE // 
							locate_dev_type_string(APP_IS_DETECTOR, TRUE, TRUE);	// detector str and background are TRUE // 
						}
						else
							locate_dev_type_string(APP_IS_FIELD_SCANNER, FALSE, TRUE);	// fs str FALSE, background TRUE // 
					}
					if(0 != SBResources[DEV_SELECT_PICs][pic_indx[pPara->icon_state]].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][pic_indx[pPara->icon_state]].hMemHandle, \
                            DEV_SELECT_DETECTOR_LEFT_X, DEV_SELECT_DETECTOR_LEFT_Y);
                    }
                } else {
                    uint8_t const pic_indx[3] = {DEV_SELECT_WITHOUT_FS, DEV_SELECT_WITH_FS, DEV_SELECT_WITH_FS_and_ARROW};
					if(SCR_EXIT_REQUESTED == pPara->ScreenExit) {	// Hide DETECTOR pictures // 
						if(0 != SBResources[DEV_SELECT_PICs][DEV_SELECT_WITHOUT_DETECTOR].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][DEV_SELECT_WITHOUT_DETECTOR].hMemHandle, \
								DEV_SELECT_DETECTOR_LEFT_X, DEV_SELECT_DETECTOR_LEFT_Y);
						}
						// Clear DETECTOR STRING // 
						if(SCR_EXIT_REQUESTED == pPara->ScreenExit) {
							locate_dev_type_string(APP_IS_DETECTOR, FALSE, FALSE);	// detector str & background are FALSE // 
							locate_dev_type_string(APP_IS_FIELD_SCANNER, TRUE, TRUE);	// fs str & background are TRUE // 
						} else	
							locate_dev_type_string(APP_IS_DETECTOR, FALSE, TRUE);	// detector str FALSE, background TRUE // 
					}
                    if(0 != SBResources[DEV_SELECT_PICs][pic_indx[pPara->icon_state]].hMemHandle) {
                        GUI_MEMDEV_WriteAt(SBResources[DEV_SELECT_PICs][pic_indx[pPara->icon_state]].hMemHandle, \
                            DEV_SELECT_FS_LEFT_X, DEV_SELECT_FS_LEFT_Y);
                    }
                }
            }
			break;
        case WM_TIMER: {
			if(SCR_RUNNING != pPara->ScreenExit)
				break;
            int Id = WM_GetTimerId(pMsg->Data.v);
            switch (Id) {
                case ID_TIMER_ATMENU_ANIM: {
                    if(3 == (++pPara->icon_state))
                        pPara->icon_state = 0;
                    WM_RestartTimer(pMsg->Data.v, DEV_SELECT_ANIM_MS);
                    WM_InvalidateWindow(WM_HBKWIN);
                }
                break;
                default:
                    break;
            }
        }
        break;
        case WM_KEY:
            TRACEM("DEVICE SELECT Handler Working");
            if((TRUE != pPara->ready4pkey) || (SCR_RUNNING != pPara->ScreenExit))
                break;  // DONT process key press events if GUI not ready //
            pInfo = (WM_KEY_INFO *)pMsg->Data.p;
            if (pInfo->PressedCnt) {
                uint8_t key_valid = TRUE;
                switch (pInfo->Key) {
                    case KEY_LEFT_EVENT:
                        pPara->new_active_win = 0;
                        WM_InvalidateWindow(WM_HBKWIN);
                        break;
                    case KEY_RIGHT_EVENT:
                        pPara->new_active_win = 1;
                        WM_InvalidateWindow(WM_HBKWIN);
                        break;
                    case KEY_OK_EVENT: 
						on_key_ok_pressed(TRUE);
                        break;
                    break;
                    default:
                        key_valid = FALSE;
                        break;
                }
                if(TRUE == key_valid) {
                    start_dac_audio(BUTTON_OK_SOUND, FALSE);    // Dont wait for audio file play complete //
                }
            }
            break;
		case WM_POST_PAINT:
			if(SCR_EXIT_REQUESTED == pPara->ScreenExit)
				pPara->ScreenExit = SCR_EXIT_CONFIRMED;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

static void on_key_ok_pressed(uint8_t store_selection) 
{
	WM_DeleteTimer(pPara->hTimerAnim);
	pPara->ready4pkey = FALSE;
	pPara->icon_state = 2;	// Print FULL image //
	WM_InvalidateWindow(WM_HBKWIN);
	if(store_selection) {
		if(0 == pPara->active_win) {
			INFOM("DEVICE TYPE : DETECTOR\n");
			APP_SetVal(ST_DEV_TYPE, APP_IS_DETECTOR, TRUE);
		} else {
			INFOM("DEVICE TYPE : FIELD SCANNER\n");
			APP_SetVal(ST_DEV_TYPE, APP_IS_FIELD_SCANNER, TRUE);
		}
	}
	pPara->ScreenExit = SCR_EXIT_REQUESTED;
}
/*************************** End of file ****************************/

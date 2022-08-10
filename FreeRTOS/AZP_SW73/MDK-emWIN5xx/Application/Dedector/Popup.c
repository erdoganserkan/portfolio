#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "system_LPC177x_8x.h"
#include "lpc177x_8x_clkpwr.h"
#include "lpc177x_8x_gpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "BSP.h"
#include "PROGBAR.h"
#include "WM.h"
#include "GUIDEMO.h"	
#include "AppCommon.h"
#include "AppSettings.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "UMDshared.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "RuntimeLoader.h"
#include "StatusBar.h"
#include "GB.h"
#include "UartInt.h"
#include "APlay.h"
#include "SYSSettings.h"
#include "Popup.h"

#define POPUP_RECTANGLE_ROUND_PIXEL		(6)

static void _ClosePopup(sPopup *pPara);
static uint8_t FirstVal;
static uint8_t StepVal;
static uint8_t MaxVal;
static uint8_t MinVal;
static uint8_t HandAnimState;
static WM_HWIN AnimhWin;
static uint8_t FlagVisible;
static uint8_t LangPopupFirst;

static uint16_t vol_backup;
static uint16_t bright_backup;

// GB Popup Related Data // 
static WM_HTIMER GBPopupTmr = 0;
static WM_HTIMER LangPopupTmr = 0;
static void _cbGBPopAnimhWin(WM_MESSAGE * pMsg);

#if(0)
static int _Min(int a, int b) {
  return((a < b) ? a : b);
}

static void _IntToString(char* pStr, int Value) {
  char* Ptr = pStr + 6;
  *(--Ptr) = 0;
  Value = _Min(Value, 99999);
  do {
    *(--Ptr) = (Value % 10) + '0';
    Value /= 10;
  } while (Value != 0);
  strcpy(pStr, Ptr);
}

static void _DrawPopup_Text(uint16_t xPos, uint16_t yPos, const char *str)
{
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetFont(POPUP_BIGSTR_FONT);	
	GUI_SetColor(GUI_YELLOW);
	GUI_DispStringAt(str, xPos, yPos);
}

static void _ProgbarSettext(PROGBAR_Handle hObj, uint8_t val)
{	
	char str[10];
	_IntToString(str, val);
	PROGBAR_SetText(hObj, str);
}
#endif
/*********************************************************************
*
*       _cbPop
*/
static void _cbPop(WM_MESSAGE * pMsg) {
  GUI_RECT   Rect = {0};
  const char     * pMessage;
	sPopup		*pPara;
  WM_KEY_INFO * pInfo;
  WM_HWIN     hWin;

	hWin = pMsg->hWin;   
	WM_GetUserData(hWin, &pPara, sizeof(pPara));
	//--------------------------------------//	
  switch (pMsg->MsgId) {
	//--------------------------------------//	
  case WM_PAINT:
		// Common Popup Drawing //
		if(MESSAGE_POPUP == pPara->type) {
			GUI_DrawGradientRoundedV(0, 0, WM_GetWindowSizeX(hWin) - 1, \
				WM_GetWindowSizeY(hWin) - 1, POPUP_RECTANGLE_ROUND_PIXEL, \
					GUI_MAKE_ALPHA(0x00, 0xA02020), GUI_MAKE_ALPHA(0x00, 0x000000));
			GUI_SetColor(GUI_WHITE);
			GUI_DrawRoundedFrame(0, 0, WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin), 8,4);
		}
		else if((GB_POPUP == pPara->type) || (BATTERY_POPUP == pPara->type)){
			uint8_t ResIndx;
			if(GB_POPUP == pPara->type)
				ResIndx = (TRUE == pPara->PopupData.GBPopupData.Result)?(GB_POPUP_OK_BACK):(GB_POPUP_FAIL_BACK);
			else 
				ResIndx = \
					(BAT_WARNING == pPara->PopupData.BatPopupData.BatMode)?(GB_POPUP_OK_BACK):(GB_POPUP_FAIL_BACK);
			if(0 != SBResources[GB_PICs][ResIndx].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[GB_PICs][ResIndx].hMemHandle, \
					WM_GetWindowOrgX(hWin), WM_GetWindowOrgY(hWin));
			}
			GUI_SetColor(GUI_MAGENTA);
			GUI_DrawRoundedFrame(0, 0, WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin), 8,4);
		}
		else if((LANG_POPUP == pPara->type) || (FACTORY_POPUP == pPara->type)) {
			// Draw Rounded Black Background Rectangle // 
			//GUI_SetColor(GUI_BLACK);
			//GUI_FillRoundedRect(0,0,LANG_POPUP_SIZEX,LANG_POPUP_SIZEY,8);
			GUI_DrawGradientRoundedH(0,0,WM_GetWindowSizeX(hWin)-1, WM_GetWindowSizeY(hWin)-1, 8, \
				GUI_DARKGREEN, GUI_DARKBLUE);
			GUI_SetColor(GUI_CYAN);
			GUI_DrawRoundedFrame(0, 0, WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin), 8,4);
		}
		else if((SENS_POPUP == pPara->type) || (VOLUME_POPUP == pPara->type) || (BRIGHT_POPUP == pPara->type)) {
			// Draw Rounded Black Background Rectangle // 
			GUI_SetColor(BACKGROUND_COLOR);
			GUI_FillRoundedRect(0,0,WM_GetWindowSizeX(hWin),WM_GetWindowSizeY(hWin),8);
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedFrame(0, 0, WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin), 8,4);
		}
		else if((FERROS_POPUP == pPara->type)) {
			GUI_DrawGradientRoundedH(0,0,WM_GetWindowSizeX(hWin)-1, WM_GetWindowSizeY(hWin)-1, 8, \
				GUI_CYAN, GUI_MAGENTA);
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedFrame(0, 0, WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin), 8,4);
		}
		// Type scpecific Popup Drawing // 
		switch(pPara->type) {
			case FERROS_POPUP: {
				uint16_t selxpos;
				GUI_RECT gRect = {SETTINGS_STR_UPX, SETTINGS_STR_UPY, SETTINGS_STR_DOWNX-1, SETTINGS_STR_DOWNY-1};
				// Draw Setting Name & Gradients //
				GUI_DrawGradientRoundedH(gRect.x0, gRect.y0, gRect.x1, gRect.y1, 8, GUI_ORANGE, GUI_DARKRED);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_BLUE);
				{
					GUI_DispStringInRectWrap(GetString(STR_FERROS_SETTING_INDX), &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
						GUI_WRAPMODE_WORD);
					// Draw First & Second Pictures //
					if(0 != SBResources[SYS_PICs][SYS_FERROOK].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_FERROOK].hMemHandle, \
							PIC1_POSX, PICs_POSY);
					}
					if(0 != SBResources[SYS_PICs][SYS_NOFERRO].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_NOFERRO].hMemHandle, \
							PIC2_POSX, PICs_POSY);
					}
				}
				
				// Draw Button according to setting // 
				// Draw ACTIVE & PASSIVE strings //
				if(TRUE == FirstVal)
					selxpos = ACTIVE_BUTTON_POSX;
				else 
					selxpos = PASSIVE_BUTTON_POSX;
				if(0 != SBResources[SYS_PICs][SYS_BUTTON].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_BUTTON].hMemHandle, \
						selxpos, BUTTONs2_POSY - WM_GetWindowOrgY(hWin));
				}
				// Draw YES & NO strings //
				{
					GUI_DispStringAt(GetString(STR_YES_INDX), YES_STR_POSX2, YES_STR_POSY2);
					GUI_DispStringAt(GetString(STR_NO_INDX), NO_STR_POSX2, NO_STR_POSY2);
				}
			}
			break;
			case VOLUME_POPUP:
			case BRIGHT_POPUP:
			case SENS_POPUP:
			{	
				char const *str;
				GUI_RECT gRect = {110, 30, 300, 80};
				// Draw Level-Bars according to FirstVal //
				uint8_t indx, res_indx;
				uint16_t angle;
				char buf[16];
				uint8_t bar_count = (FirstVal-MinVal)/StepVal;
				if(0 != MinVal) bar_count++;
				else {
					if(0 != bar_count)
						bar_count--;
				}
				if((0 != FirstVal) && (0 == bar_count))
					bar_count = 1;
				for(indx=0 ; indx<TOT_BAR_COUNT ; indx++) {
					if(indx < bar_count) {
						if(0 != SBResources[FBAR_PICs][indx + FBAR10].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[FBAR_PICs][indx + FBAR10].hMemHandle, \
								BAR_XPOS_START + (indx*BAR_POSX_INT), BAR_YPOS);
						}
					}
					else {
						if(0 != SBResources[NBAR_PICs][indx + NBAR10].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[NBAR_PICs][indx + NBAR10].hMemHandle, \
								BAR_XPOS_START + (indx*BAR_POSX_INT), BAR_YPOS);
						}
					}	
				}
				// Draw setting Icon & Str //
				if(VOLUME_POPUP == pPara->type) {
					res_indx = SYS_VOLUME;
					str = GetString(STR_VOLUME_INDX);
				}
				else if(BRIGHT_POPUP == pPara->type) {
					res_indx = SYS_BRIGHT;
					str = GetString(STR_BRIGHT_INDX);
				}
				else {
					res_indx = SYS_SENS;
					str = GetString(STR_SENS_INDX);
				}
				if(0 != SBResources[SYS_PICs][res_indx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][res_indx].hMemHandle, \
						SETTING_ICON_XPOS, SETTING_ICON_YPOS);
				}
				GUI_DrawGradientH(gRect.x0, gRect.y0, gRect.x0 + (gRect.x1 - gRect.x0)/2, gRect.y1, \
					BACKGROUND_COLOR, BACKGROUND_COLOR | 0x00FF00);
				GUI_DrawGradientH(gRect.x0 + (gRect.x1 - gRect.x0)/2, gRect.y0, gRect.x1, gRect.y1, \
					BACKGROUND_COLOR | 0x00FF00, BACKGROUND_COLOR);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_BLUE);
				GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				// Draw Circle & Pie //
				angle = (((uint32_t)FirstVal)*360U)/(uint32_t)MaxVal;
				GUI_SetColor(GUI_DARKBLUE);
				GUI_DrawPie(CIRCLE_XPOS, CIRCLE_YPOS, CIRCLE_DIAMETER, 0, angle, 0);
				GUI_SetColor(GUI_LIGHTBLUE);
				GUI_DrawPie(CIRCLE_XPOS, CIRCLE_YPOS, CIRCLE_DIAMETER, angle, 360, 0);
				GUI_SetColor(GUI_GREEN);
				GUI_DrawCircle(CIRCLE_XPOS, CIRCLE_YPOS, CIRCLE_DIAMETER);
				GUI_DrawCircle(CIRCLE_XPOS, CIRCLE_YPOS, CIRCLE_DIAMETER+1);
				GUI_DrawCircle(CIRCLE_XPOS, CIRCLE_YPOS, CIRCLE_DIAMETER+2);
				// Render FirstVal number in Circle // 
				GUI_SetFont(APP_32B_FONT);
				GUI_SetColor(GUI_YELLOW);
				GUI_SetTextMode(GUI_TM_TRANS);
				sprintf(buf, "%u", FirstVal);
				GUI_DispStringHCenterAt(buf, CIRCLE_XPOS, CIRCLE_YPOS-10);
			}
			break;
			case MESSAGE_POPUP:
			#if(0)
				pMessage = pPara->PopupData.Msg;
				Rect.x1 = WM_GetWindowSizeX(hWin) - 1;
				Rect.y1 = WM_GetWindowSizeY(hWin) - 1;
				GUI_SetFont(GUI_FONT_16B_ASCII);
				GUI_SetColor(0xFFFFFF);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_DispStringInRectWrap(pMessage, &Rect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				break;
			#endif
			case FACTORY_POPUP: {
				uint16_t selxpos;
				GUI_RECT gRect = { FACTORY_STR_UPX, FACTORY_STR_UPY, FACTORY_STR_DOWNX, FACTORY_STR_DOWNY };
				// Draw Fctory Icon //
				if(0 != SBResources[SYS_PICs][SYS_FACTORY].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_FACTORY].hMemHandle, \
						FACTORY_ICON_POSX, FACTORY_ICON_POSY);
				}
				// Draw Explanation String //
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_WHITE);
				{
					uint8_t lang = APP_GetValue(ST_LANG);
					if(LANG_AR == lang)
						GUI_SetFont(APP_16B_FONT);
					else
						GUI_SetFont(APP_24B_FONT);
				}
				GUI_DispStringInRectWrap(GetString(STR_FACTORY_CONFIRM_INDX), &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
				// Draw selection Box //
				if(TRUE == FirstVal)
					selxpos = YES_BUTTON_POSX;
				else 
					selxpos = NO_BUTTON_POSX;
				if(0 != SBResources[SYS_PICs][SYS_BUTTON].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_BUTTON].hMemHandle, \
						selxpos, BUTTONs_POSY);
				}
				// Draw YES & NO strings //
				GUI_DispStringAt(GetString(STR_YES_INDX), YES_STR_POSX, YES_STR_POSY);
				GUI_DispStringAt(GetString(STR_NO_INDX), NO_STR_POSX, NO_STR_POSY);
			}
			break;	
			case LANG_POPUP: {
				volatile uint8_t indx;
				GUI_RECT gRect = {117, 94, 274, 126};
				static struct {
					uint8_t res_indx;
					uint16_t posx;
					uint16_t posy;
					char const *name;
				} app_langs[LANG_COUNT] = {
					{SYS_LANG_TURKISH, FLAG_POS1X, FLAG_UPY_POS},
					{SYS_LANG_ENGLISH, FLAG_POS2X, FLAG_UPY_POS},
					{SYS_LANG_ARABIC, FLAG_POS3X, FLAG_UPY_POS},
					{SYS_LANG_GERMAN, FLAG_POS4X, FLAG_UPY_POS},
					{SYS_LANG_SPAIN, FLAG_POS5X, FLAG_DOWNY_POS},
					{SYS_LANG_PERSIAN, FLAG_POS6X, FLAG_DOWNY_POS},
					{SYS_LANG_RUSSIAN, FLAG_POS7X, FLAG_DOWNY_POS},
					{SYS_LANG_FRENCH, FLAG_POS8X, FLAG_DOWNY_POS},
				};
				// Draw Language Flags //
				for(indx=0 ; indx<LANG_COUNT ; indx++) {
					app_langs[indx].name = GetString2Lang(STR_OWN_LANG_INDX, indx);
					if((indx == FirstVal) && (FALSE == FlagVisible))
						continue;
					if(0 != SBResources[SYS_PICs][app_langs[indx].res_indx].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][app_langs[indx].res_indx].hMemHandle, \
							app_langs[indx].posx, app_langs[indx].posy);
					}
				}
				// Draw active lanugage string //
				GUI_DrawGradientRoundedH(gRect.x0, gRect.y0, gRect.x1, gRect.y1, 5, GUI_BLUE, GUI_GREEN);
				GUI_SetTextMode(GUI_TM_TRANS);
				//if((LANG_AR != FirstVal) && (LANG_PR != FirstVal))
					//GUI_SetFont(GUI_FONT_24B_ASCII);
				//else
					GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_BLACK);
				GUI_DispStringInRectWrap(app_langs[FirstVal].name, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				// Start timer for Blinking Flag animation //
				if(TRUE == LangPopupFirst) {
					LangPopupFirst = FALSE;
					LangPopupTmr = WM_CreateTimer(hWin, ID_TIMER_LANG_POPUP, LANG_POPUP_FLAG_ANIM_MS, 0);
				}
			}
			break;
			case GB_POPUP: {
				//1- Display big string @ upper location // 
				char const *str;
				static char buf[64];
				GUI_RECT gRect = {GB_RESULT_POPUP_STR_LEFTX, GB_RESULT_POPUP_STR_LEFTY, \
					GB_RESULT_POPUP_STR_RIGHTX, GB_RESULT_POPUP_STR2_LEFTY};
				GUI_SetFont(APP_32B_FONT);
				GUI_SetBkColor(GUI_BLACK);
				if(TRUE == pPara->PopupData.GBPopupData.Result) {	// "Balans is OK/FAIL" string @ up-center // 
					GUI_SetColor(GUI_WHITE);
					str = GetString(STR_BALANS_OK_INDX);
				}
				else {	
					GUI_SetColor(GUI_RED);
					str = GetString(STR_BALANS_FAILED_INDX);
				}
				GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				// Display small string @ downer location : "GROUND ID :" string // 
				gRect.x0 += 20;
				gRect.x1 -= 20;
				gRect.y0 = GB_RESULT_POPUP_STR2_LEFTY + 10;
				gRect.y1 = GB_RESULT_POPUP_STR_RIGHTY;
				GUI_SetFont(APP_32B_FONT);
				if(TRUE == pPara->PopupData.GBPopupData.Result) {
					char temp[64];
					uint16_t gid = APP_GetValue(ST_GROUND_ID);
					memset(temp, 0, sizeof(temp));
					strncpy(temp, GetString(STR_GROUND_ID_INDX), sizeof(temp));
					sprintf(buf, "%s : \n%u.%u", temp, 0xFF & (gid>>8), (gid & 0x00FF));
					str = buf;
				}
				else {
					str = GetString(STR_TRY_AGAIN_INDX);
				}
				GUI_SetColor(GUI_YELLOW);
				GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

				//2- Draw ARmors //
				if(TRUE == pPara->PopupData.GBPopupData.Result) {	// If completed draw green arrow // 
					if(0 != SBResources[GB_PICs][GB_ARMOR_GREEN].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_ARMOR_GREEN].hMemHandle, \
							WM_GetWindowOrgX(hWin) + GB_RESULT_POPUP_ARMOR_LEFTX, \
								WM_GetWindowOrgY(hWin) + GB_RESULT_POPUP_ARMOR_LEFTY);
					}
				}
				else {	// If failed draw red arrow // 
					if(0 != SBResources[GB_PICs][GB_ARMOR_RED].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_ARMOR_RED].hMemHandle, \
							WM_GetWindowOrgX(hWin) + GB_RESULT_POPUP_ARMOR_LEFTX, \
								WM_GetWindowOrgY(hWin) + GB_RESULT_POPUP_ARMOR_LEFTY);
					}
				}
			} 
			break;
			case BATTERY_POPUP: {
				char const *str;
				GUI_RECT gRect = {
					BAT_POPUP_STR_LEFTX, BAT_POPUP_STR_LEFTY, BAT_POPUP_STR_RIGHTX, BAT_POPUP_STR_RIGHTY
				};
				GUI_SetFont(APP_24B_FONT);
				GUI_SetBkColor(GUI_BLACK);
				if(BAT_WARNING == pPara->PopupData.BatPopupData.BatMode) {
					// locate battery warning picture //
					if(0 != SBResources[SB_PICs][SB_BATPOPUP_WARNNG].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[SB_PICs][SB_BATPOPUP_WARNNG].hMemHandle, \
							WM_GetWindowOrgX(hWin) + BAT_POPUP_ICON_LEFTX, \
								WM_GetWindowOrgY(hWin) + BAT_POPUP_ICON_LEFTY);
					}
					// Locate String // 
					GUI_SetColor(GUI_YELLOW);
					str = GetString(STR_BAT_WARN_INDX);
					GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
					// Start timer to close popup // 
					GBPopupTmr = WM_CreateTimer(hWin, ID_TIMER_BATTERY_POPUP, BAT_POPUP_DISPLAY_MS, 0);
				}
				else {
					// locate battery EMPTY picture //
					if(0 != SBResources[SB_PICs][SB_BATPOPUP_WARNNG].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[SB_PICs][SB_BATPOPUP_WARNNG].hMemHandle, \
							WM_GetWindowOrgX(hWin) + BAT_POPUP_ICON_LEFTX, \
								WM_GetWindowOrgY(hWin) + BAT_POPUP_ICON_LEFTY);
					}
					if(FALSE == pPara->PopupData.BatPopupData.CDStarted) {
						// Locate String // 
						GUI_SetColor(GUI_RED);
						str = GetString(STR_BAT_FAIL_INDX);
						GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
						// Start timer to start CountDown // 
						GBPopupTmr = WM_CreateTimer(hWin, ID_TIMER_BATTERY_POPUP, BAT_POPUP_CD_MS, 0);
					}
					else {
						static char buf[16];
						// Draw Digit Surrounding Rectangl Frame // 
						GUI_DrawGradientRoundedH(gRect.x0+20, gRect.y0+20, \
							gRect.x1-20, gRect.y1-20, 8, GUI_DARKRED, GUI_LIGHTRED);
						// Draw CD (Count Down) Digits // 
						GUI_SetFont(&GUI_FontD80);
						GUI_SetBkColor(GUI_BLACK);
						GUI_SetColor(GUI_BLACK);
						GUI_SetTextMode(GUI_TM_TRANS);
						memset(buf, 0, sizeof(buf));
						sprintf(buf, "%u", pPara->PopupData.BatPopupData.ActiveNum);
						GUI_DispStringInRectWrap(buf, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
					}					
				}
			}
			break;	
			default:
				while(STALLE_ON_ERR);
				break;
		}
    break;
	//--------------------------------------//
  case WM_SET_FOCUS:
	//--------------------------------------//
    pMsg->Data.v = 0;
    break;
	//--------------------------------------//
	case WM_DELETE:
	//--------------------------------------//
		if(0 != GBPopupTmr) {
			WM_DeleteTimer(GBPopupTmr);
			GBPopupTmr = 0;
		}
		if(0 != LangPopupTmr) {
			WM_DeleteTimer(LangPopupTmr);
			LangPopupTmr = 0;
		}
		break;
	//--------------------------------------//
	case WM_TIMER: {
	//--------------------------------------//
		int Id = WM_GetTimerId(pMsg->Data.v);
		switch (Id) {
			case ID_TIMER_BATTERY_POPUP:
				if(BAT_WARNING == pPara->PopupData.BatPopupData.BatMode) {
					_ClosePopup(pPara);
				}
				else {
					if(FALSE == pPara->PopupData.BatPopupData.CDStarted) {
						// Start Count Down mode // 
						pPara->PopupData.BatPopupData.CDStarted = TRUE;
						pPara->PopupData.BatPopupData.ActiveNum = 10;
						WM_InvalidateWindow(hWin);
						GUI_Delay(250);
						WM_RestartTimer(pMsg->Data.v, BAT_POPUP_CD_MS);
					}
					else {
						GUI_Delay(250);
						if(0 == (pPara->PopupData.BatPopupData.ActiveNum--)) {
							SB_delete();
							GUI_Clear();
							GUI_Exit();
							vTaskSuspendAll();
							LPC_LCD->CTRL = 0;
							// Completely remove LCD backlight // 
							BSP_PWMSet(0, BSP_PWM_LCD, 0);
							#if(POWERMCU_DEV_BOARD == USED_HW)
								GPIO_ClearValue(2, (0x1<<1));  // Power Down LCD PWM pin // 
							#elif(UMD_DETECTOR_BOARD == USED_HW)
								GPIO_SetValue(2, (1<<1) );	// Shotdown LCD-Backlight regulator //   
								GPIO_ClearValue(1, (1<<1));	// Disable power connection to LCD-Backlight //   
							#endif							
							while(1) CLKPWR_Sleep();
						}
						WM_InvalidateWindow(hWin);
						WM_RestartTimer(pMsg->Data.v, BAT_POPUP_CD_MS);
					}
				}
				break;
			case ID_TIMER_LANG_POPUP:
				if(TRUE == FlagVisible) FlagVisible = FALSE;
				else FlagVisible = TRUE;
				WM_InvalidateWindow(hWin);
				WM_RestartTimer(pMsg->Data.v, BAT_POPUP_CD_MS);
				break;
			default:
				break;
		}
	}
	break;
	//--------------------------------------//
  case WM_KEY:
	//--------------------------------------//
	DEBUGM("PopUP KEY Handler Working");
    pInfo = (WM_KEY_INFO *)pMsg->Data.p;
    if (pInfo->PressedCnt) {
		uint8_t play_button_ok = TRUE;
		pPara->last_key = pInfo->Key;	// Store last key for parent usage // 
		switch (pInfo->Key) {
			case KEY_LEFT_EVENT:
				switch(pPara->type) {
					case BRIGHT_POPUP:
					case VOLUME_POPUP:
					case SENS_POPUP: {
						uint8_t update_gui = FALSE;
						if((FirstVal > StepVal) && ((FirstVal-StepVal) >= MinVal)) {
							FirstVal -= StepVal;
							if(5 == FirstVal) 
								FirstVal = 0;	// Patch for UI //
							update_gui = TRUE;
						}
						if(BRIGHT_POPUP == pPara->type) {
							if(0 != FirstVal)
								BSP_PWMSet(0, BSP_PWM_LCD, FirstVal);
							else
								BSP_PWMSet(0, BSP_PWM_LCD, 1);								
						}
						else if(VOLUME_POPUP == pPara->type) {
							// Set NEW Volume Level and play sample sound // 
							APP_SetVal(ST_VOL, FirstVal, FALSE);	// Dont store to flash, only change in RAM // 
							if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND))	
								while(TODO_ON_ERR);
							start_dac_audio(SAMPLE_SOUND, FALSE); // Dont wait until audio file comlete // 
							play_button_ok = FALSE;
						}
						// start gui update LASTLY // 
						if(TRUE == update_gui)
							WM_InvalidateWindow(pPara->hPop);
					}
					break;
					case LANG_POPUP:
						if((FirstVal-StepVal) >= MinVal) 
							FirstVal -= StepVal;
						else 
							FirstVal = MaxVal;
						WM_InvalidateWindow(pPara->hPop);
						break;
					case FACTORY_POPUP:
					case FERROS_POPUP: 
						if(FALSE == FirstVal) FirstVal = TRUE;
						else FirstVal = FALSE;
						WM_InvalidateWindow(pPara->hPop);
						break;
					default:
						play_button_ok = FALSE;
						break;
				}
				break;
			case KEY_RIGHT_EVENT:
				switch(pPara->type) {
					case FERROS_POPUP: 
					case FACTORY_POPUP:
						if(FALSE == FirstVal) 
							FirstVal = TRUE;
						else 
							FirstVal = FALSE;
						WM_InvalidateWindow(pPara->hPop);
						break;
					case BRIGHT_POPUP:
					case VOLUME_POPUP:
					case SENS_POPUP: {
						uint8_t update_gui = FALSE;
						if((FirstVal+StepVal) <= MaxVal) {
							FirstVal += StepVal;
							if(5 == FirstVal) 
								FirstVal = 10;	// Patch for UI //
							update_gui = TRUE;
						}
						if(BRIGHT_POPUP == pPara->type) {
							BSP_PWMSet(0, BSP_PWM_LCD, FirstVal);
						}
						else if(VOLUME_POPUP == pPara->type) {
							// Set NEW Volume Level and play sample sound // 
							APP_SetVal(ST_VOL, FirstVal, FALSE);
							if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND))	
								while(TODO_ON_ERR);
							start_dac_audio(SAMPLE_SOUND, FALSE); // Dont wait until finished, DMA & ISR will handle rest // 
							play_button_ok = FALSE;
						}
						// start gui update LASTLY // 
						if(TRUE == update_gui)
							WM_InvalidateWindow(pPara->hPop);
					}
					break;
					case LANG_POPUP:
						if((FirstVal+StepVal) > MaxVal) 
							FirstVal=MinVal;
						else
							FirstVal += StepVal;
							WM_InvalidateWindow(pPara->hPop);
						break;
					default:
						play_button_ok = FALSE;
						break;
				}
				break;
			case KEY_OK_EVENT: {
				uint8_t wait = FALSE;
				switch(pPara->type) {
					case LANG_POPUP:
						if(APP_GetValue(ST_LANG) != FirstVal) {	
								struct WM_MESSAGE msg;
								// Store new language selection //
								APP_SetVal(ST_LANG, FirstVal, TRUE);
								// Send GUI_USER_LANG_CHANGED to parent window // 
								msg.hWin = pPara->hParent;
								msg.MsgId = GUI_USER_LANG_CHANGED;
								msg.Data.v = FirstVal;
								WM_SendMessage(msg.hWin, &msg);
						}
						break;
					case FACTORY_POPUP:
						if(TRUE == FirstVal) { // Reset application settings to safe default values 
							AppSettings_StoreDefaults();
							// Send ferros state and sensitivity to detector // 
							{
								UmdPkt_Type send_msg;
								send_msg.cmd = CMD_SET_SENSITIVITY;
								send_msg.length = 3;
								send_msg.data.sensitivity = APP_GetValue(ST_SENS);
								UARTSend((uint8_t *)&send_msg, \
									UMD_CMD_TIMEOUT_SET_SENSE_MS, NULL);	// Because of pop-up close takes long, increase timeout not to get //
								App_waitMS(UMD_CMD_TIMEOUT_SET_SENSE_MS + 200);	// SysSetting screen message/timeout handler will process responses // 
								
								send_msg.cmd = CMD_SET_FERROS_STATE;
								send_msg.data.ferros_state = APP_GetValue(ST_FERROs);
								UARTSend((uint8_t *)&send_msg, \
									UMD_CMD_TIMEOUT_SET_FERRO_MS, NULL);	// Because of pop-up close takes long, increase timeout not to get //
									
							}
							// Apply LCD Backlight and volume level to own hardware // 
							if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))	// Force reinit audio files on ram according to new volume-level // 	
								while(TODO_ON_ERR);
							APP_SetVal(ST_BRIGHT, APP_GetValue(ST_BRIGHT), FALSE);	// Apply new lcd-backlight pwm level to hw // 						
						}
						break;
					case VOLUME_POPUP:
						APP_SetVal(ST_VOL, FirstVal, TRUE);
						if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))	// Reinit all audio files with new Volume level //  
							while(TODO_ON_ERR);
						play_button_ok = FALSE;
						break;
					case BRIGHT_POPUP:
						APP_SetVal(ST_BRIGHT, FirstVal, TRUE);
						break;
					case SENS_POPUP:
						APP_SetVal(ST_SENS, FirstVal, TRUE);
						{
							UmdPkt_Type send_msg;
							send_msg.cmd = CMD_SET_SENSITIVITY;
							send_msg.length = 3;
							send_msg.data.sensitivity = FirstVal;
							UARTSend((uint8_t *)&send_msg, \
								UMD_CMD_TIMEOUT_SET_SENSE_MS, NULL);	// Because of pop-up close takes long, increase timeout not to get //
						}
						break;
					case FERROS_POPUP: 
						APP_SetVal(ST_FERROs, FirstVal, TRUE);
						{
							UmdPkt_Type send_msg;
							send_msg.cmd = CMD_SET_FERROS_STATE;
							send_msg.length = 3;
							send_msg.data.ferros_state = FirstVal;
							UARTSend((uint8_t *)&send_msg, \
								UMD_CMD_TIMEOUT_SET_FERRO_MS, NULL);
						}
						break;
					case GB_POPUP:	// This is information popup, just close it // 
						break;
					default:
						play_button_ok = FALSE;
						wait = TRUE;
						break;
				}
				if(FALSE == wait)
					_ClosePopup(pPara);
			}
			break;
			case KEY_ESC_EVENT:
				if(BRIGHT_POPUP == pPara->type) {
					// Set back OLD LCD-Brightness to PWM // 
					BSP_PWMSet(0, BSP_PWM_LCD, bright_backup);
				}
				if(VOLUME_POPUP == pPara->type) {
					// Set back OLD Volume level into Sound HW // 
					APP_SetVal(ST_VOL, vol_backup, FALSE);	// Dont store to flash, only change in RAM // 
					if(0 != InitGroupRes(AUDIO_TRACs, OLD_SAMPLE_SOUND))	
						while(TODO_ON_ERR);
					play_button_ok = FALSE;
				}
				// Exit without saving // 
				if(BATTERY_POPUP != pPara->type)
					_ClosePopup(pPara);
				break;
			default:	
				play_button_ok = FALSE;
				break;
    	}
		if(TRUE == play_button_ok) {
			start_dac_audio(BUTTON_OK_SOUND, FALSE); // Dont wait until finished, DMA & ISR will handle rest // 
		}
    }
    break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*********************************************************************
*
*       _ShowPopupBase
*/
static void _ShowPopupBase(sPopup * pPara) 
{
	uint16_t xSize, ySize;
	uint16_t xPos, yPos;

	GUI_SetFont(APP_24B_FONT);		// Set active font before active font based calculations // 

	switch(pPara->type) {
		case MESSAGE_POPUP:
			xSize = POPUP_MSG_XSIZE;
			ySize = POPUP_MSG_YSIZE;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = (LCD_GetYSize()-ySize)>>1;
			break;
		case LANG_POPUP:
			MaxVal = aSettings[ST_LANG].max;
			MinVal = aSettings[ST_LANG].min;
			StepVal = 1;
			FirstVal = APP_GetValue(ST_LANG);
			xSize = LANG_POPUP_SIZEX;
			ySize = LANG_POPUP_SIZEY;
			FlagVisible = TRUE;
			LangPopupFirst = TRUE;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		case FACTORY_POPUP:
			FirstVal = APP_GetValue(ST_LANG);
			xSize = FACTORY_POPUP_SIZEX;
			ySize = FACTORY_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		case VOLUME_POPUP:
			vol_backup = APP_GetValue(ST_VOL);
			StepVal = 5;
			MaxVal = aSettings[ST_VOL].max;
			MinVal = aSettings[ST_VOL].min;
			FirstVal = APP_GetValue(ST_VOL);
			xSize = VOLUME_POPUP_SIZEX;
			ySize = VOLUME_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		case SENS_POPUP:
			StepVal = 5;
			MaxVal = aSettings[ST_SENS].max;
			MinVal = aSettings[ST_SENS].min;
			FirstVal = APP_GetValue(ST_SENS);
			xSize = SENS_POPUP_SIZEX;
			ySize = SENS_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		case BRIGHT_POPUP:
			bright_backup = APP_GetValue(ST_BRIGHT);
			StepVal = 5;
			MaxVal = aSettings[ST_BRIGHT].max;
			MinVal = aSettings[ST_BRIGHT].min;
			FirstVal = APP_GetValue(ST_BRIGHT);
			xSize = BRIGHT_POPUP_SIZEX;
			ySize = BRIGHT_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		case GB_POPUP:
			xSize = GB_RESULT_POPUP_SIZEX;
			ySize = GB_RESULT_POPUP_SIZEY;
			HandAnimState = TRUE;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = (LCD_GetYSize()-ySize)>>1;
			break;
		case BATTERY_POPUP:
			xSize = GB_RESULT_POPUP_SIZEX;
			ySize = GB_RESULT_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = (LCD_GetYSize()-ySize)>>1;
			break;
		case FERROS_POPUP:
			MaxVal = aSettings[ST_FERROs].max;
			MinVal = aSettings[ST_FERROs].min;
			StepVal = aSettings[ST_FERROs].step;
			FirstVal = APP_GetValue(ST_FERROs);
			xSize = LANG_POPUP_SIZEX;
			ySize = LANG_POPUP_SIZEY;
			xPos = (LCD_GetXSize()-xSize)>>1;
			yPos = ((LCD_GetYSize()-ySize)>>1) + 15;
			break;
		default:
			while(STALLE_ON_ERR);
			break;
	}

  // Create semi transparent pop up window
	pPara->hPop = WM_CreateWindowAsChild(xPos, yPos, \
		xSize, ySize, WM_HBKWIN, \
			WM_CF_HASTRANS | WM_CF_MEMDEV, _cbPop, sizeof(sPopup *));
  WM_SetUserData(pPara->hPop, &pPara, sizeof(pPara));
	// Create anim windows //
	if(GB_POPUP == pPara->type) {
		AnimhWin = WM_CreateWindowAsChild(GB_RESULT_POPUP_SHAND_LEFTX - WM_GetWindowOrgX(pPara->hPop), \
			GB_RESULT_POPUP_SHAND_LEFTY - WM_GetWindowOrgX(pPara->hPop), \
			GB_RESULT_POPUP_SHAND_SIZEX, GB_RESULT_POPUP_SHAND_SIZEY, pPara->hPop, \
				WM_CF_SHOW | WM_CF_MEMDEV, _cbGBPopAnimhWin, sizeof(sPopup *));
		WM_SetUserData(AnimhWin, &pPara, sizeof(pPara));
	}
	pPara->hPrevFocused = WM_GetFocussedWindow();
	WM_SetFocus(pPara->hPop);	// Set FOCUS to get key_press  events // 
  // Fade in and move out semi transparent message window
  //WM_EnableMemdev(hParent);
  GUI_Exec();
  //WM_EnableMemdev(WM_HBKWIN);
  if(LANG_POPUP == pPara->type)
		GUI_MEMDEV_MoveInWindow(pPara->hPop, 0, 0, 100, POPUP_MOVING_DELAY_MS);
	else if((VOLUME_POPUP == pPara->type) || (SENS_POPUP == pPara->type) || (BRIGHT_POPUP == pPara->type))
		GUI_MEMDEV_ShiftInWindow(pPara->hPop, POPUP_SHIFTING_DELAY_MS, GUI_MEMDEV_EDGE_LEFT);
	else
		GUI_MEMDEV_FadeInWindow(pPara->hPop, POPUP_FADING_DELAY_MS);
  //WM_ValidateWindow(WM_HBKWIN);
}

static void _ClosePopup(sPopup *pPara) {
  if(LANG_POPUP == pPara->type)
		GUI_MEMDEV_MoveOutWindow(pPara->hPop, LCD_GetXSize(), LCD_GetYSize(), 100, POPUP_MOVING_DELAY_MS);
	else if((VOLUME_POPUP == pPara->type) || (SENS_POPUP == pPara->type) || (BRIGHT_POPUP == pPara->type))
		GUI_MEMDEV_ShiftOutWindow(pPara->hPop, POPUP_SHIFTING_DELAY_MS, GUI_MEMDEV_EDGE_RIGHT);
	else
		GUI_MEMDEV_FadeOutWindow(pPara->hPop, POPUP_FADING_DELAY_MS);
		
  //WM_DisableMemdev(WM_HBKWIN);
  WM_DeleteWindow(pPara->hPop);
  //WM_DisableMemdev(hParent);
	GUI_ClearKeyBuffer();
	WM_SetFocus(pPara->hPrevFocused);	// Recovering Back FOCUS must be done // 
}

static void _cbGBPopAnimhWin(WM_MESSAGE * pMsg) {
	sPopup		*pPara;
  WM_HWIN     hWin;

	hWin = pMsg->hWin;   
	WM_GetUserData(pMsg->hWin, &pPara, sizeof(pPara));
	//--------------------------------------//	
  switch (pMsg->MsgId) {
	//--------------------------------------//	
		case WM_PAINT:
			GUI_Clear();
			GUI_SetBkColor(GUI_BLACK);
			// Draw Blinking small Hand Icon // 
			if(TRUE == HandAnimState) {
				if(0 != SBResources[GB_PICs][GB_HAND_SMALL].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HAND_SMALL].hMemHandle, \
						GB_RESULT_POPUP_SHAND_LEFTX, GB_RESULT_POPUP_SHAND_LEFTY);
				}
			}
			break;
		case WM_CREATE:
			if(0 == GBPopupTmr)
				GBPopupTmr = WM_CreateTimer(hWin, ID_TIMER_GB_POPUP, GB_RESULT_POPUP_ANIM_MS, 0);
			break;
		case WM_DELETE:
			if(0 != GBPopupTmr) {
				WM_DeleteTimer(GBPopupTmr);
				GBPopupTmr = 0;
			}
			break;
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_GB_POPUP:
					HandAnimState = (TRUE == HandAnimState)?(FALSE):(TRUE);
					WM_InvalidateWindow(hWin);
					WM_RestartTimer(pMsg->Data.v, HAND_ON_OFF_ANIM_MS);
					break;
				default:
					break;
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}
/*********************************************************************
*
*       ShowMessages
*/
void ShowMessages(WM_HWIN hWinParent) {
	static sPopup Para;
	Para.type = MESSAGE_POPUP;
	Para.hParent = hWinParent;
	Para.hPrevFocused = WM_GetFocussedWindow();
	Para.PopupData.Msg = "emWin\nIconSlide Demo\n\nwww.segger.com";
  _ShowPopupBase(&Para);
	Para.PopupData.Msg = "Images from:\n\nwww.iconshock.com";
  _ShowPopupBase(&Para);
  GUI_Delay(500);	// Fine for GUI Redraw background of Popup again // 
}
 
/*********************************************************************
*
*       ShowProgbar
*/
void ShowPopup(sPopup *pPara) 
{
  _ShowPopupBase(pPara);
  GUI_Delay(100);	// Fine for GUI Redraw background of Popup again // 
}



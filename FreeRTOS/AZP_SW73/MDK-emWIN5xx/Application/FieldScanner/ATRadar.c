#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <queue.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include <BSP.h>
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "monitor.h"
#include "GuiConsole.h"
#include "Serial.h"
#include "UartInt.h"
#include "Aplay.h"
#include "StatusBar.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "ATManuelFreq.h"
#include "ATRadar.h"

typedef struct {
	uint8_t	ready4pkey;
    uint8_t ScreenExit;
	uint8_t RadarIndx;
	uint16_t timer_ms;
	WM_HWIN WinRadar;
	char volume_str[16];
	char bright_str[16];
	char distance_str[16];
	char freq_str[16];
	WM_CALLBACK * OldDesktopCallback;
} AT_PARA;

static uint8_t new_page;
static   AT_PARA  * pPara;
extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

// Function Prototypes //
static void AT_RADAR_on_comm_timeout(void *last_msgp);
static void ATRadar_on_msg_ready(void);
static void _cbBk(WM_MESSAGE * pMsg);

static void _cbDrawRadar(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  AT_PARA      * pPara;
  WM_KEY_INFO * pInfo;
  static     WM_HTIMER hTimerAnim;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_CREATE:
			hTimerAnim      = WM_CreateTimer(hWin, ID_TIMER_AT_RADAR, pPara->timer_ms, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(hTimerAnim);
			hTimerAnim = 0;
			break;
		case WM_TIMER: {
			WM_RestartTimer(pMsg->Data.v, pPara->timer_ms);
			if(AT_RADAR_PICs_MAX != pPara->RadarIndx)
				(pPara->RadarIndx)++;
			else
				pPara->RadarIndx = AT_RADAR_PICs_MIN;
			WM_InvalidateWindow(hWin);
		}
		break;
		case WM_PAINT: {
			// Write Radar pics to window sequentially // 
			if(0 != SBResources[AT_RADAR_BLUE_PICs][pPara->RadarIndx].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_RADAR_BLUE_PICs][pPara->RadarIndx].hMemHandle, RADAR_PICs_UPX, RADAR_PICs_UPY);
			}
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("AT_RADAR KEY Handler Working");
			WM_GetUserData(hWin, &pPara, sizeof(pPara));
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_ESC_EVENT: 
						pPara->ScreenExit = TRUE;
						new_page = AT_MENU;	// Return to AT Menu page // 
						break;
					default:	
						key_valid = FALSE;
						break;
				}
				if(TRUE == key_valid) {
					start_dac_audio(BUTTON_OK_SOUND, FALSE);	// Dont wait for audio file play complete // 
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
*       _RadialMenu
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t ATRadar(void) 
{
  int               xSizeWindow;
  int               ySizeWindow;
  int               xLCDSize;
  int               yLCDSize;
  extern uint16_t const at_auto_freqs[AT_AUTO_FREQ_COUNT];

    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_RADAR_BLUE_PICs, 0xFF))	// RadialMenu Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = TRUE;
	pPara->RadarIndx = 0;
	pPara->ScreenExit = FALSE;
	pPara->OldDesktopCallback = NULL;
	new_page = 0xFF;

	// Calculate animation timer periode // 
	uint8_t man_freq = (APP_GetValue(ST_AT_MAN_AUTO_SEL) == AT_MAN_FREQ_SELECTED)?1:0;
	uint32_t freq_range = AT_MAN_FREQ_MAX - AT_MAN_FREQ_MIN;
	uint32_t timer_ms_range = RADAR_ANIM_MS_SLOW - RADAR_ANIM_MS_FAST;	
	uint32_t active_freq = (man_freq)?APP_GetValue(ST_AT_MAN_FREQ):at_auto_freqs[APP_GetValue(ST_AT_AUTO_FREQ_INDX)];
	pPara->timer_ms = RADAR_ANIM_MS_SLOW - (((active_freq - AT_MAN_FREQ_MIN) * timer_ms_range)/freq_range);
	
	DEBUGM("ATRadar timer periode : %u ms\n", pPara->timer_ms);

  WM_MULTIBUF_Enable(1);
	GUI_Clear();
	
  xLCDSize = LCD_GetXSize();
  yLCDSize = LCD_GetYSize();
  // Reduce size of desktop window to size of display
  WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
  pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk);
	WM_InvalidateWindow(WM_HBKWIN);

	// 1- Create GAUGE Update Window // 
  pPara->WinRadar = WM_CreateWindowAsChild(RADAR_PICs_UPX, RADAR_PICs_UPY, RADAR_PICs_SIZEX, RADAR_PICs_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbDrawRadar, sizeof(pPara));
  WM_SetFocus(pPara->WinRadar);
  WM_SetUserData(pPara->WinRadar,   &pPara, sizeof(pPara));

  SB_init(SB_REDUCED_MODE_USE_BOTTOM_RIGHT);
  App_SetHWTypeStates(FALSE, TRUE);
	set_lcd_bcklight_reduce_state(FALSE);

  // Animation loop
  while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				ATRadar_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
  }
	
	set_lcd_bcklight_reduce_state(TRUE);
	App_SetHWTypeStates(FALSE, FALSE);	// 
	// Do Deletion of created objects & Release of Resources //
	GUI_ClearKeyBuffer();
  	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(pPara->WinRadar);

	free(pPara);
	pPara = NULL;
	return new_page;
}

static void ATRadar_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			default:
				// Ignore silently rest of messages // 
				ERRM("UNEXPECTED MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", gui_msg[0], gui_msg[1], gui_msg[2], gui_msg[3]);
				break;
		}
	}
}

/*********************************************************************
*
*       _cbBk
*
*  Function description:
*    Callback routine of desktop window
*/
static void _cbBk(WM_MESSAGE * pMsg) {
  extern uint16_t const at_auto_freqs[AT_AUTO_FREQ_COUNT];
  switch (pMsg->MsgId) {
		case WM_PAINT:
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			// Write Screen Background Memdev to LCD // 
			if(0 != SBResources[AT_RADAR_BLUE_PICs][AT_RADAR_BCKGD_INDX].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[AT_RADAR_BLUE_PICs][AT_RADAR_BCKGD_INDX].hMemHandle, 0, 0);
			}
			// Draw SCREEN NAME STRING // 
			{
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_CYAN);
				{
					GUI_RECT const grect = {93, 7, 382, 53};
					GUI_DispStringInRectWrap(GetString(STR_FS_ACTIVE), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
						GUI_WRAPMODE_WORD);
				}
			}
			// Draw active language string // 
			{	
				GUI_SetFont(APP_16B_FONT);
				GUI_RECT const grect = {12, 83, 75, 107};
				GUI_DispStringInRectWrap(GetString(STR_OWN_LANG_INDX), (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			// print at-volume // 
			{
				memset(pPara->volume_str, 0, sizeof(pPara->volume_str));
				sprintf(pPara->volume_str, "%u", APP_GetValue(ST_AT_VOL));
				GUI_SetFont(APP_24B_FONT);
				GUI_RECT const grect = {42, 124, 71, 180};
				GUI_DispStringInRectWrap(pPara->volume_str, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			// print at-brihtness // 
			{
				memset(pPara->bright_str, 0, sizeof(pPara->volume_str));
				sprintf(pPara->bright_str, "%u", APP_GetValue(ST_AT_BRIGTH));
				GUI_SetFont(APP_24B_FONT);
				GUI_RECT const grect = {41, 202, 71, 254};
				GUI_DispStringInRectWrap(pPara->bright_str, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			// print frequency // 
			{
				uint16_t freq;
				uint8_t at_man_auto = APP_GetValue(ST_AT_MAN_AUTO_SEL);
				if(AT_MAN_FREQ_SELECTED == at_man_auto)
					freq = APP_GetValue(ST_AT_MAN_FREQ);
				else {
					freq = at_auto_freqs[APP_GetValue(ST_AT_AUTO_FREQ_INDX)];
				}
				memset(pPara->freq_str, 0, sizeof(pPara->volume_str));
				sprintf(pPara->freq_str, "%u.%u %s", freq/1000, (freq%1000)/100, GetString(STR_AT_MAN_FREQ_UNIT));
				GUI_SetFont(APP_16B_FONT);
				GUI_RECT const grect = {406, 162, 468, 182};
				GUI_DispStringInRectWrap(pPara->freq_str, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			// print range(distance) // 
			{
				uint16_t range = APP_GetValue(ST_AT_DIST_RANGE);
				memset(pPara->distance_str, 0, sizeof(pPara->volume_str));
				sprintf(pPara->distance_str, "%s", GetString(STR_SHORT_RANGE+range));
				GUI_SetFont(APP_16B_FONT);
				GUI_RECT const grect = {405, 73, 468, 106};
				GUI_DispStringInRectWrap(pPara->distance_str, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}


/*************************** End of file ****************************/

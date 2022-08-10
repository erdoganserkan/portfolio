#include <string.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <event_groups.h>
#include <task.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include "BSP.h"
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Strings.h"
#include "UartInt.h"
#include "APlay.h"
#include "SYSSettings.h"

typedef struct {
	uint8_t ready4pkey;
	uint8_t active_icon; 
	uint8_t ScreenExit;
	uint8_t visible;
	char const *ScrStrs[SYS_SETTINGs_ICON_COUNT];
} PARA;

typedef struct
{
	uint8_t res_indx;
	uint16_t posx;
	uint16_t posy;
} sSYSIconType;

static uint8_t new_page;
static const sSYSIconType SYSIcons[SYS_SETTINGs_ICON_COUNT] = {
	{SYS_VOLUME, VOLUME_ICON_POSX, LINEUP_ICONs_POSY},
	{SYS_LANG, LANG_ICON_POSX, LINEUP_ICONs_POSY},
	{SYS_BRIGHT, BRIGHT_ICON_POSX, LINEUP_ICONs_POSY},
	{SYS_FERROs, FERROS_ICON_POSX, LINEDOWN_ICONs_POSY},
	{SYS_SENS, SENS_ICON_POSX, LINEDOWN_ICONs_POSY},
	{SYS_FACTORY, FACTORY_POSX, LINEDOWN_ICONs_POSY}
};
static void SYS_on_msg_ready(void);
static PARA* pPara = NULL;

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

static void _cbDraw(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  WM_KEY_INFO * pInfo;
  PARA      * pPara;
  static     WM_HTIMER hTimerAnim;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {

			// Draw Backgrorund & Screen Name String //
			if(0 != SBResources[SYS_PICs][SYS_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_BACK].hMemHandle, 0, 0);
			}
			{
				GUI_RECT gRect = {SYS_SETTINGs_STR_UPX, SYS_SETTINGs_STR_UPY, SYS_SETTINGs_STR_DOWNX, SYS_SETTINGs_STR_DOWNY};
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_YELLOW);
				if(LANG_RS != APP_GetValue(ST_LANG)) 
					GUI_SetFont(APP_32B_FONT);
				else
					GUI_SetFont(APP_24B_FONT);
				GUI_DispStringInRectWrap(GetString(STR_SYS_SETTINGs_INDX), &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			// Draw Active ICON & Selection Circle //
			if(0 != SBResources[SYS_PICs][SYSIcons[pPara->active_icon].res_indx].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYSIcons[pPara->active_icon].res_indx].hMemHandle, \
					SYSIcons[pPara->active_icon].posx, SYSIcons[pPara->active_icon].posy);
			}
			if((TRUE == pPara->visible) && (0 != SBResources[SYS_PICs][SYS_SELECT].hMemHandle)) {
				GUI_MEMDEV_WriteAt(SBResources[SYS_PICs][SYS_SELECT].hMemHandle, \
					SYSIcons[pPara->active_icon].posx-SELECT_CIRCLE_X_DIFF, \
						SYSIcons[pPara->active_icon].posy-SELECT_CIRCLE_Y_DIFF);
			}
			// Draw Active ICON's String //
			{
				GUI_RECT gRect = {ACTIVE_STR_UPX, ACTIVE_STR_UPY, ACTIVE_STR_DOWNX, ACTIVE_STR_DOWNY};
				GUI_SetFont(APP_24B_FONT);
				GUI_DispStringInRectWrap(pPara->ScrStrs[pPara->active_icon], &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			}	
		}
		break;
		case WM_CREATE:
			hTimerAnim      = WM_CreateTimer(hWin, ID_TIMER_SYS_ANIMATION, SELECT_ANIM_INTERVAL_MS, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(hTimerAnim);
			break;
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_SYS_ANIMATION: {
					if(TRUE == pPara->visible) 
						pPara->visible = FALSE;
					else pPara->visible = TRUE;
					if(WM_GetFocussedWindow() == hWin)
						WM_InvalidateWindow(hWin);
				}
				break;
				default:
					break;
			}
			WM_RestartTimer(pMsg->Data.v, SELECT_ANIM_INTERVAL_MS);
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			DEBUGM("SYS Settings KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_LEFT_EVENT:
						if(0 != pPara->active_icon) pPara->active_icon--;
						else pPara->active_icon = SYS_SETTINGs_ICON_COUNT-1;
						WM_InvalidateWindow(hWin);	
						break;
					case KEY_RIGHT_EVENT:
						if((SYS_SETTINGs_ICON_COUNT-1) != pPara->active_icon) pPara->active_icon++;
						else pPara->active_icon = 0;
						WM_InvalidateWindow(hWin);	
						break;
					case KEY_OK_EVENT: {
							static sPopup Para;
							switch(SYSIcons[pPara->active_icon].res_indx) {
								case SYS_LANG:
									// Create IconSelect Type Popup // 
									Para.type = LANG_POPUP;
									break;
								case SYS_VOLUME:
									Para.type = VOLUME_POPUP;
									break;
								case SYS_BRIGHT:
									Para.type = BRIGHT_POPUP;
									break;
								case SYS_SENS:
									Para.type = SENS_POPUP;
									break;
								case SYS_FERROs:
									Para.type = FERROS_POPUP;
									break;
								case SYS_FACTORY:
									Para.type = FACTORY_POPUP;
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
							Para.hParent = hWin;
							ShowPopup(&Para);	// Open desired screen's pop-up // 
						}
						break;
					case KEY_OTO_EVENT:
						pPara->ScreenExit = TRUE;
						new_page = OTO_SEARCH;
						break;
					#if(0)
					case KEY_DEPTH_EVENT: {
						pPara->ScreenExit = TRUE;
						new_page = DEPTH_CALC;
					}
					break;
					#endif
					case KEY_ESC_EVENT:
						pPara->ScreenExit = TRUE;
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
		case GUI_USER_LANG_CHANGED: {
				volatile uint8_t indx;
				for(indx=0 ; indx<SYS_SETTINGs_ICON_COUNT ; indx++)
					pPara->ScrStrs[indx] = GetString(STR_SYS_SETTINGS_START + indx);
				WM_InvalidateWindow(hWin);
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
uint8_t SYSSettings(void) 
{
	WM_HWIN           hDraw;
	volatile uint8_t indx;
	
	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);
	if(0 != InitGroupRes(SYS_PICs, 0xFF))	// SYSSettings Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(FBAR_PICs, 0xFF))	// Full Bar Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(NBAR_PICs, 0xFF))	// Null Bar Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	new_page = RM_SCREEN;
	pPara->ScreenExit = FALSE;
	pPara->ready4pkey = TRUE;
	pPara->active_icon = 0;
	pPara->visible = TRUE;
	for(indx=0 ; indx<SYS_SETTINGs_ICON_COUNT ; indx++)
		pPara->ScrStrs[indx] = GetString(STR_SYS_SETTINGS_START + indx);

  WM_MULTIBUF_Enable(1);
	GUI_Clear();
	
  // Reduce size of desktop window to size of display
  WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());

	// Create radial menu window
  hDraw = WM_CreateWindowAsChild(0, 0, LCD_GetXSize(), LCD_GetYSize(), WM_HBKWIN, WM_CF_SHOW, _cbDraw, sizeof(pPara));
  WM_SetUserData(hDraw,   &pPara, sizeof(pPara));
  WM_SetFocus(hDraw);

  SB_init(SB_REDUCED_MODE_USE_RIGHT);

  // Animation loop
  while (FALSE == pPara->ScreenExit) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				SYS_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
  }
	
	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
	SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(hDraw);

	free(pPara);
	pPara = NULL;
	return new_page;
}

void SYS_on_comm_fail(void *msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)msgp;
	ERRM("COMM TIMEOUT on CMD(0x%02X)\n", msg->cmd);
	if(TRUE == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_SET_SENSITIVITY:
		case CMD_SET_FERROS_STATE:
			// We have FAILED	at Dedector Parameter Setting // 
			break;
		default:
			break;
	}
}

// Send detector parameter setting messages // 
static void SYS_on_msg_ready(void)
{
	//UmdPkt_Type send_msg;
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case RSP_SET_SENSITIVITY:
				StopCommTimeout();
				break;
			case RSP_SET_FERROS_STATE:
				StopCommTimeout();
				break;
			default:
				break;	// Ignore silently other cmd & rsp // 
		}
	}
}

/*************************** End of file ****************************/

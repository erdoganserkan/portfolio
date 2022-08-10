#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <FreeRTOS.h>
#include <queue.h>
#include <event_groups.h>
#include <task.h>
#include "GLCD.h"
#include "GUIDEMO.h"
#include "DIALOG.h"
#include "BSP.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "AppSettings.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "UartInt.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "DepthCalc.h"
#include "APlay.h"
#include "Analog.h"
#include "RadialMenu.h"

#define RM_ICONS_SPACE		100

#define RM_BLINK_ANIM_MS	250

// RM New icon positions (relative positions in MID-window) // 
#define RM_GB_ICON_POSX			((GLCD_X_SIZE - ((3*RM_ICON_SIZE_X) + (2*RM_ICONS_SPACE)))/2)
#define RM_GB_ICON_POSY			0
#define RM_STD_ICON_POSX		(RM_GB_ICON_POSX + RM_ICON_SIZE_X + RM_ICONS_SPACE)
#define RM_STD_ICON_POSY		0
#define RM_MIN_ICON_POSX		(RM_STD_ICON_POSX + RM_ICON_SIZE_X + RM_ICONS_SPACE)
#define RM_MIN_ICON_POSY		0
#define RM_OTO_ICON_POSX		RM_GB_ICON_POSX
#define RM_OTO_ICON_POSY		90
#define RM_DPT_ICON_POSX		RM_STD_ICON_POSX
#define RM_DPT_ICON_POSY		90
#define RM_SYS_ICON_POSX		RM_MIN_ICON_POSX
#define RM_SYS_ICON_POSY		90

#define DOWN_STR_SIZEY	30
#define RM_PRODUCT_ICON_SIZEY	60

static uint16_t icon_pos[RM_PAGEs_COUNT][2] = {
	{RM_GB_ICON_POSX, RM_GB_ICON_POSY}, 
	{RM_STD_ICON_POSX, RM_STD_ICON_POSY}, 
	{RM_MIN_ICON_POSX, RM_MIN_ICON_POSY}, 
	{RM_OTO_ICON_POSX, RM_OTO_ICON_POSY}, 
	{RM_DPT_ICON_POSX, RM_DPT_ICON_POSY}, 
	{RM_SYS_ICON_POSX, RM_SYS_ICON_POSY}, 
};

typedef struct {
	uint8_t ready4pkey;
	uint8_t ScreenExit;
	uint8_t show_icon;		
	WM_HTIMER hTimerAnim;	// Arrow animation timer // 
	WM_HWIN hMidDraw, hDownDraw;
	WM_CALLBACK * OldDesktopCallback;
} PARA;

static uint8_t new_page;
static 	uint8_t LastActiveIndx = 0;
static PARA	  * pPara = NULL;

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void RM_on_msg_ready(void);

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

/*********************************************************************
*
*       Private routines
*
**********************************************************************
*/
static void _cbDownDraw(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
		case WM_PAINT: {
			const char *str;
			// Draw DOWN Bacground Picture //
			if(1 && (0 != SBResources[RM_PICs][RM_DOWNBACK].hMemHandle)) {
				GUI_MEMDEV_WriteAt(SBResources[RM_PICs][RM_DOWNBACK].hMemHandle, \
				0, LCD_GetYSize() - DOWN_STR_SIZEY);
			}	
			// Draw active icon string //
			GUI_SetTextMode(GUI_TM_TRANS);
			if(LANG_RS == APP_GetValue(ST_LANG))
				GUI_SetFont(APP_24B_FONT);
			else
				GUI_SetFont(APP_32B_FONT);
			GUI_SetColor(GUI_YELLOW);
			str = GetString(STR_BALANS_INDX + LastActiveIndx);
			if(TRUE == pPara->ready4pkey) {
				GUI_RECT const grect = {0, 0, + WM_GetWindowSizeX(hWin), WM_GetWindowSizeY(hWin)};
				GUI_DispStringInRectWrap(str, (GUI_RECT *)&grect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
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
*       _cbMidDraw
*
*  Function description:
*    Callback routine of radial menu
*/
static void _cbMidDraw(WM_MESSAGE * pMsg) 
{
  WM_HWIN     hWin;
  WM_KEY_INFO * pInfo;
  volatile uint8_t indx;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
		case WM_PAINT:
			// Draw Middle Bacground Picture //
			if(1 && (0 != SBResources[RM_PICs][RM_MIDBACK].hMemHandle)) {
				GUI_MEMDEV_WriteAt(SBResources[RM_PICs][RM_MIDBACK].hMemHandle, 0, RM_PRODUCT_ICON_SIZEY);
			}	

			// Draw icons // 
			for (indx = 0; indx < RM_PAGEs_COUNT; indx++) {
				if(((LastActiveIndx == indx) && (pPara->show_icon)) || (indx != LastActiveIndx)){
					if(NULL != SBResources[RM_PICs][RM_GB + indx].hMemHandle)
						GUI_MEMDEV_WriteAt(SBResources[RM_PICs][RM_GB + indx].hMemHandle, icon_pos[indx][0], \
						icon_pos[indx][1] + RM_PRODUCT_ICON_SIZEY);
				}
			}
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;	// Standart Operation // 
			break;
		case WM_KEY: {
			uint8_t key_valid = TRUE;
			TRACEM("RadialMenu KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				//const char *keys[] = {"UP:DEPTH", "DOWN:OTO", "LEFT:MINUS", "RIGHT:PLUS", "OK:CONFIRM", "ESC:MENU"};
				//TRACEM("(%u:%s)Key pressed\n", pInfo->Key - KEY_EVENT_OFFSET_CH, keys[pInfo->Key - KEY_EVENT_OFFSET_CH]);
				switch (pInfo->Key) {
					case KEY_RIGHT_EVENT:
						if(RM_PAGEs_COUNT == (++LastActiveIndx))
							LastActiveIndx = 0;
						WM_Invalidate(pPara->hMidDraw);
						WM_Invalidate(pPara->hDownDraw);
						break;
					case KEY_LEFT_EVENT:
						if(0 == LastActiveIndx)
							LastActiveIndx = RM_PAGEs_COUNT -1;
						else
							--LastActiveIndx;
						WM_Invalidate(pPara->hMidDraw);
						WM_Invalidate(pPara->hDownDraw);
						break;
					case KEY_OK_EVENT:
						pPara->ready4pkey = FALSE;
						{
							uint8_t selected_page = LastActiveIndx;
							switch(selected_page) {
								case STD_SEARCH:	// enter to Standard Search Mode // 
								case MIN_SEARCH:	// Enter mineralized search // 
								case OTO_SEARCH:	// Enter otomatical search // 
									if(STD_SEARCH == selected_page) {	// Check for long-gb // 
										if(FALSE == AppStatus.long_gb_done) {
											AppStatus.gb_type_required = GB_TYPE_LONG;
											AppStatus.search_before_gb = selected_page;
											INFOM("LONG GB TYPE is required\n");
											new_page = GB_SCREEN;
										} else
											new_page = selected_page;
									} else {	// Check for short-gb // 
										if(FALSE == AppStatus.short_gb_done) {
											AppStatus.gb_type_required = GB_TYPE_SHORT;
											AppStatus.search_before_gb = selected_page;
											INFOM("SHORT GB TYPE is required\n");
											new_page = GB_SCREEN;
										} else
											new_page = selected_page;
									}
									pPara->ScreenExit = TRUE;
									break;									
								case GB_SCREEN:		// Enter to GB Screen // 
									// If GB is entered from Radial Menu, Always execute LONG GB // 
									// If GB is entered from Radial Menu,	Always goto STD_SEARCH if GB completes // 
									AppStatus.gb_type_required = GB_TYPE_LONG;	
									AppStatus.search_before_gb = STD_SEARCH;	
									LastActiveIndx = new_page = selected_page;
									pPara->ScreenExit = TRUE;
									break;
								case DEPTH_CALC: {	// enter depth calculation // 
									extern uint8_t page_before_depth;	// defined in DeptchCalc.c module // 
									page_before_depth = RM_SCREEN;
								}
								case SYSTEM_SETTINGs:
									LastActiveIndx = new_page = selected_page;
									pPara->ScreenExit = TRUE;
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
						break;
					case KEY_ESC_EVENT:
						key_valid = FALSE;
						break;
					#if(0)
					case KEY_DEPTH_EVENT: {
						extern uint8_t page_before_depth;	// defined in DeptchCalc.c module // 
						page_before_depth = RM_SCREEN;
						LastActiveIndx = new_page = DEPTH_CALC;
						pPara->ScreenExit = TRUE;
					}
					break;
					#endif
					case KEY_OTO_EVENT:
						if(FALSE == AppStatus.short_gb_done) {
							AppStatus.gb_type_required = GB_TYPE_SHORT;
							AppStatus.search_before_gb = OTO_SEARCH;
							INFOM("SHORT GB TYPE is required\n");
							LastActiveIndx = new_page = GB_SCREEN;
						}
						else
							LastActiveIndx = new_page = OTO_SEARCH;
						pPara->ScreenExit = TRUE;
						break;
					default:	
						while(STALLE_ON_ERR);
						break;
				}
			}
			if(TRUE == key_valid)	
				start_dac_audio(BUTTON_OK_SOUND, FALSE); // Dont wait until audio file complete // 
		}
		break;
		case WM_CREATE:
			pPara->hTimerAnim	= WM_CreateTimer(hWin, ID_TIMER_RM_ANIM, RM_BLINK_ANIM_MS, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(pPara->hTimerAnim);
			pPara->hTimerAnim = 0;
			break;
		case WM_TIMER: 
			if(TRUE == pPara->ready4pkey) {
				int Id = WM_GetTimerId(pMsg->Data.v);
				switch (Id) {
					case ID_TIMER_RM_ANIM:
						pPara->show_icon ^= 1;
						break;
					default:
						break;
				}
				WM_InvalidateWindow(hWin);
				WM_RestartTimer(pMsg->Data.v, RM_BLINK_ANIM_MS);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}


static void inform_detector(void) {
	GPIO_ClearValue(RADIAL_MENU_ENTERENCE_PORT, (1<<RADIAL_MENU_ENTERENCE_PIN));	// Default Case is HIGH //		
	vTaskDelay(250 * (configTICK_RATE_HZ/1000));
	GPIO_SetValue(RADIAL_MENU_ENTERENCE_PORT, (1<<RADIAL_MENU_ENTERENCE_PIN));	// Default Case is HIGH //		
}

/*********************************************************************
*
*       _RadialMenu
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t RadialMenu(void) 
{
	volatile uint8_t indx;

    pPara = (PARA *)calloc(sizeof(PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	else 
		memset(pPara,0,sizeof(PARA));
	if(0 != InitGroupRes(RM_PICs, 0xFF))	// RadialMenu Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(DPT_PICs, 0xFF))	// Depts Menu Resource Initialization //
		while(TODO_ON_ERR);

	{
		uint32_t ramused = SDRAM_MAX_ADDR_ - SDRAM_BASE_ADDR;
		uint32_t mb = ramused / (1024u*1024u);
		uint32_t kb = (ramused - (mb * (1024u*1024u)))/(1024u);
		INFOL("%u MB + %u KB RAM is USED\n", mb, kb);
		if(SDRAM_SIZE < ramused) { 
			while(1);
		}
	}

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ScreenExit = FALSE;
	pPara->show_icon = 1;
	pPara->ready4pkey  = TRUE;
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	new_page = RM_SCREEN;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();
	
	// Inform detector about Radial Menu Enterence // 
	inform_detector();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	
	// Create radial menu MIDDLE window
	pPara->hMidDraw              = \
	WM_CreateWindowAsChild(0, RM_PRODUCT_ICON_SIZEY, LCD_GetXSize(), LCD_GetYSize() - DOWN_STR_SIZEY - RM_PRODUCT_ICON_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbMidDraw, sizeof(pPara));
	WM_SetUserData(pPara->hMidDraw,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hMidDraw);
	// Create selected Icon String Window // 
	pPara->hDownDraw              = \
	WM_CreateWindowAsChild(0, LCD_GetYSize() - DOWN_STR_SIZEY, LCD_GetXSize(), DOWN_STR_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbDownDraw, 0);
	
  SB_init(SB_REDUCED_MODE_USE_RIGHT);
	//WM_MOTION_SetMovement(pPara->hMotion, GUI_COORD_X, 0, 0);	// force initial draw // 
	
	// Animation loop
  while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				RM_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
  }
	
	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
  	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(pPara->hDownDraw);
	WM_DeleteWindow(pPara->hMidDraw);
	
	free(pPara);
	pPara=NULL;
	return new_page;
}

static void RM_on_msg_ready(void)
{
	UmdPkt_Type send_msg;
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case IND_DEDECTOR_IS_READY:
				AppStatus.detector_hs_state = DETECTOR_TOTAL_INIT_COMPLETED;
				INFOM("DETECTOR INIT DONE received\n");
				break;
			default:
				ERRM("UNEXPECTED MSG(0x%02X) on RM PAGEn", gui_msg[0]);
				while(TODO_ON_ERR);
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
static void _cbBk_Desktop(WM_MESSAGE * pMsg) {
  int       NCode;
  int       Id;

  switch (pMsg->MsgId) {
  case WM_NOTIFY_PARENT:
    Id    = WM_GetId(pMsg->hWinSrc);      // Id of widget
    NCode = pMsg->Data.v;                 // Notification code
    switch (Id) {
			default:
				break;
    }
    break;
  case WM_PAINT:
		// Draw UP BACKGROUND image // 
		if(1 && (0 != SBResources[RM_PICs][RM_UPBACK].hMemHandle)) {
			GUI_MEMDEV_WriteAt(SBResources[RM_PICs][RM_UPBACK].hMemHandle, 0, 0);
		}	
		// Set Product Name on Screen // 
		if(0) { 
			GUI_SetFont(APP_24B_FONT);
			GUI_DrawGradientRoundedV(0, 10, 100, 50, 4, \
				GUI_MAKE_ALPHA(0x00, 0xFF0C10), GUI_MAKE_ALPHA(0x00, 0x460C10));
			GUI_SetColor(GUI_YELLOW);
			GUI_DrawRoundedRect(0, 0, 100, 50, 4);
			GUI_SetColor(GUI_YELLOW);
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_DispStringHCenterAt("GOLD FINDER++", 0, 0);
		}
	break;
	default:
		WM_DefaultProc(pMsg);
		break;
  }
}
/*************************** End of file ****************************/


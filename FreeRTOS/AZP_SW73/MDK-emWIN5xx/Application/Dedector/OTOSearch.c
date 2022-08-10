#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "LPC177x_8x.h"         // Device specific header file, contains CMSIS
#include "system_LPC177x_8x.h"  // Device specific header file, contains CMSIS
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_adc.h"
#include "lpc177x_8x_timer.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <task.h>
#include <queue.h>
#include <GUIDEMO.h>
#include <DIALOG.h> 
#include "BSP.h"
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "UMDShared.h"
#include "UartInt.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "BSP.h"
#include "Battery.h"
#include "StatusBar.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Analog.h"
#include "Dac.h"
#include "APlay.h"
#include "OTOSearch.h"

static const char *FTID_names[TARGET_ID_COUNT] = {0,0,0,0,0,0};
static GUI_COLOR const FTID_COlors[TARGET_ID_COUNT] = \
	{GUI_YELLOW, GUI_BLUE, GUI_BROWN, GUI_GREEN, GUI_RED, GUI_ORANGE};	// Same sequence with "eTARGET_TargetIDs" typedef // 
static GUI_RECT tid_gRect = {100,0,200,35};

typedef struct {
	uint8_t   ready4pkey;
	uint8_t ScreenExit, SearchStarted;
	uint8_t targetID, active_targetIDMax;
	uint8_t Gauge;
	uint8_t FerrosState;
	uint8_t gauge_is_valid;
	uint32_t search_start_time;
	WM_CALLBACK * OldDesktopCallback;
	WM_HWIN WinGauge, WinInfo, WinGStr;
	OTOBarType OTOBarCoords[OTO_BAR_COUNT];
} PARA;

static uint8_t new_page;
static PARA* pPara = NULL;

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

// Function Prototypes //
static void OTO_on_comm_timeout(void *last_msgp);
static void OTO_on_msg_ready(void);
static void _cbBk(WM_MESSAGE * pMsg);

static uint8_t GET_BAR_FROM_GAUGE(uint8_t GG)  {
	if(100 == GG) return OTO_BAR_COUNT;
	else if(0 == GG) return 0;
	else return (((uint32_t)GG * ((uint32_t)OTO_BAR_COUNT))/100U) + 1;
}

static void _BalcBarCoords(void)
{
	volatile uint8_t indx;
	uint8_t BorderIndx = GAUGES_BORDERX/(GAUGES_SIZEX+GAUGES_INTERVALX);
	for(indx=0;indx<OTO_BAR_COUNT;indx++) {
		pPara->OTOBarCoords[indx].xup = indx * (GAUGES_SIZEX + GAUGES_INTERVALX);
		if(indx < BorderIndx) {
			pPara->OTOBarCoords[indx].yup = GAUGES_STARTY;
		}
		else {			
			pPara->OTOBarCoords[indx].yup = GAUGES_STARTY - \
				(((GAUGES_STARTY-10)*(indx-BorderIndx))/(OTO_BAR_COUNT - BorderIndx));
		}
		TRACEM("indx(%u), xup(%u), yup(%u)\n", indx, pPara->OTOBarCoords[indx].xup, pPara->OTOBarCoords[indx].yup);
	}
}

static void _cbDrawGauge(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;
  WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		#if(TRUE == SERIAL_TASK_STATE)
			case GUI_USER_MSG_NEW_GAUGE: {
				uint8_t TGauge = pMsg->Data.v;
				DEBUGM("NEW GAUGE: %u\n", TGauge);
				if(UMD_GAUGE_MAX < TGauge)
					TGauge = UMD_GAUGE_MAX;
				if(pPara->Gauge != TGauge) {
					pPara->Gauge = TGauge;
					WAVE_Update_FreqAmp_Gauge((uint16_t)TGauge*GAUGE_FRACTUATION);	// Update Sound Frequency only if MORE THAN ZERO // 
					WM_InvalidateWindow(hWin);	// Invoke emWin to redraw this window // 
					WM_InvalidateWindow(pPara->WinGStr);	// Redraw target Gauge Value String Window // 
				}
			}
			break;
			case GUI_USER_MSG_NEW_TARGET_ID: {
				uint8_t TempID = pMsg->Data.v;
				DEBUGM("NEW TARGET_ID: %u\n", TempID);
				if(pPara->active_targetIDMax < TempID)
					TempID = pPara->active_targetIDMax;	// Truncation to maximum value for safety // 
				pPara->targetID = TempID;
				WM_InvalidateWindow(hWin);	// Invoke emWin to redraw this window // 
			}
			break;
		#endif
		case WM_PAINT: {
			volatile uint8_t indx = 0;
			uint8_t BarCount;

			// Write Background Memdev to LCD // 
			if(0 != SBResources[OTO_PICs][OTO_GBACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_GBACK].hMemHandle, GAUGE_WINDOW_STARTX, GAUGE_WINDOW_STARTY);
			}
			// Update correspoding Target picture //
			if((TARGET_NOTARGET != pPara->targetID) && (0 != SBResources[OTO_PICs][OTO_CAVITY + (pPara->targetID-1)].hMemHandle)) {
				GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_CAVITY + (pPara->targetID-1)].hMemHandle, \
				GAUGE_WINDOW_STARTX+5, GAUGE_WINDOW_STARTY + 5);
			}		
			// Draw Target Name //
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetFont(APP_24B_FONT);
			GUI_SetColor(FTID_COlors[pPara->targetID]);
			GUI_DispStringInRectWrap(FTID_names[pPara->targetID], &tid_gRect, \
				GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			// Set Stsrting BarCount Index //
				
			GUI_SetColor(FTID_COlors[pPara->targetID]);
			BarCount = GET_BAR_FROM_GAUGE(pPara->Gauge);
			TRACEM("NEW BARCOUNT=%u\n", BarCount);
			for(indx=0;indx<OTO_BAR_COUNT ; indx++) {
				if(indx == BarCount)
					GUI_SetColor(OTO_BAR_BCKGRND_COLOR);
				GUI_FillRoundedRect(pPara->OTOBarCoords[indx].xup, pPara->OTOBarCoords[indx].yup, \
					pPara->OTOBarCoords[indx].xup + GAUGES_SIZEX, 159, 1);
			}

			// Send Search START Command to Dedector //
			// Because of WM_PAINT event is called multiple time, we must be sure about single CMD send // 
			if((FALSE == pPara->SearchStarted) && (CMD_START_SEARCH != last_send_msg.cmd)) {
				UmdPkt_Type msg;
				msg.cmd = CMD_START_SEARCH;
				msg.length = 3;	// CMD(U8) + LENGTH(U8) + DATA(U8) // 
				msg.data.search_type = AUTOMATIC_SEARCH_TYPE;
				UARTSend((uint8_t *)&msg, \
					UMD_CMD_TIMEOUT_SEARCH_CMD_MS, OTO_on_comm_timeout);
				pPara->search_start_time = GUI_X_GetTime();	
			}
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("OTOSearch KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					#if(0)
					case KEY_DEPTH_EVENT: {
						extern uint8_t page_before_depth;	// defined in DeptchCalc.c module // 
						page_before_depth = OTO_SEARCH;
						new_page = DEPTH_CALC;
					}
					break;
					#endif
					case KEY_ESC_EVENT: {
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2;	// CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, OTO_on_comm_timeout);
					}
					break;
					default:	
						key_valid = FALSE;
						break;
				}
				if(TRUE == key_valid) {
					WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
					start_dac_audio(BUTTON_OK_SOUND, FALSE);	// DONT wait for audio file play complete // 
				}
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

// Ferros/NFerros Display Icons Windows // 
// Not related to Gauge / targetID values // 
static void _cbDrawInfo(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			static volatile uint8_t cnt = 0; 
			// Write Background Memdev to LCD // 
			if(0 != SBResources[OTO_PICs][OTO_INFO_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_INFO_BACK].hMemHandle, \
					INFO_WINDOW_STARTX, INFO_WINDOW_STARTY);
			}
			// Update Noferros / YESferros //
			if(TRUE == pPara->FerrosState) {
				pPara->FerrosState = FALSE;
				if(0 != SBResources[OTO_PICs][OTO_YESFERROs].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_YESFERROs].hMemHandle, \
						INFO_WINDOW_STARTX + 9, INFO_WINDOW_STARTY + 81);
				}
			}
			else {
				pPara->FerrosState = TRUE;
				if(0 != SBResources[OTO_PICs][OTO_NOFERROs].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_NOFERROs].hMemHandle, \
						INFO_WINDOW_STARTX + 9, INFO_WINDOW_STARTY + 81);
				}
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

// Gauge Value display as string in FIXED (YELLOW) color //
// Because of string color is independent to targetID, DONT invoke this callback on //
// targetID change // 
static void _cbDrawGStr(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			GUI_Clear();
			GUI_SetColor(GSTR_BACK_COLOR);
			GUI_FillRect(0, 0, GSTR_DOWNX - GSTR_UPX, GSTR_DOWNY - GSTR_UPY);
			// Draw GAUGE LEVEL STRING // 
			{
				char str[32];
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_32B_FONT);
				GUI_SetColor(FTID_COlors[pPara->targetID]);
				sprintf(str, "%%%u", pPara->Gauge);
				GUI_DispStringAt(str, 0 , 0);
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
uint8_t OTOSearch(void) 
{
	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);
	if(0 != InitGroupRes(OTO_PICs, 0xFF))	// RadialMenu Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);
	_BalcBarCoords();

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = TRUE;
	pPara->SearchStarted = FALSE;
	pPara->ScreenExit = FALSE;
	new_page = RM_SCREEN;
	pPara->targetID = TARGET_NOTARGET;
	pPara->OldDesktopCallback = NULL;
	pPara->active_targetIDMax = TARGET_ID_MAX;	// Full  target list for automatic search type // 
	pPara->FerrosState = APP_GetValue(ST_FERROs);
	pPara->gauge_is_valid = FALSE;
	pPara->search_start_time = 0;
	{
		static uint8_t FirstTime = TRUE;
		if(TRUE == FirstTime) {
			FirstTime = FALSE;
			
			FTID_names[TARGET_NOTARGET] = "    ";
			FTID_names[TARGET_CAVITY] = GetString(STR_CAVITY_INDX);
			FTID_names[TARGET_FERROs] = GetString(STR_FERROS_INDX);
			FTID_names[TARGET_NFERROs] = GetString(STR_NONFERROS_INDX);
			FTID_names[TARGET_GOLD] = GetString(STR_GOLD_INDX);
			FTID_names[TARGET_MINERAL] = GetString(STR_MINERAL_INDX);
		}
	}

	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk);
	WM_InvalidateWindow(WM_HBKWIN);

	// 1- Create GAUGE Update Window // 
	pPara->WinGauge = WM_CreateWindowAsChild(GAUGE_WINDOW_STARTX, GAUGE_WINDOW_STARTY, GAUGE_WINDOW_SIZEX, GAUGE_WINDOW_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbDrawGauge, sizeof(pPara));
	WM_SetFocus(pPara->WinGauge);
	// Add pointer to parameter structure to windows
	WM_SetUserData(pPara->WinGauge,   &pPara, sizeof(pPara));

	// 2- Create Information Window //
	pPara->WinInfo = WM_CreateWindowAsChild(INFO_WINDOW_STARTX, INFO_WINDOW_STARTY, INFO_WINDOW_SIZEX, INFO_WINDOW_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbDrawInfo, sizeof(pPara));
	// Add pointer to parameter structure to windows
	WM_SetUserData(pPara->WinInfo,   &pPara, sizeof(pPara));

	// 3- Create Gauge String Window //
	pPara->WinGStr = WM_CreateWindowAsChild(GSTR_UPX, GSTR_UPY, GSTR_DOWNX - GSTR_UPX, GSTR_DOWNY - GSTR_UPY, \
		WM_HBKWIN, WM_CF_SHOW, _cbDrawGStr, sizeof(pPara));
	// Add pointer to parameter structure to windows
	WM_SetUserData(pPara->WinGStr,   &pPara, sizeof(pPara));

	SB_init(SB_FULL_MODE);
	set_lcd_bcklight_reduce_state(FALSE);

	// Animation loop
	while (FALSE == pPara->ScreenExit) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				OTO_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}
	WAVE_Generator_stop(TRUE, TRUE, TRUE);
	// Do Deletion of created objects & Release of Resources //
	#if(0 && (TRUE == CLOCK_GENERATION_STATE))
		stop_clock_generation();
	#endif
	
	set_lcd_bcklight_reduce_state(TRUE);
	GUI_ClearKeyBuffer();
  	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(pPara->WinGauge);
	WM_DeleteWindow(pPara->WinInfo);
	WM_DeleteWindow(pPara->WinGStr);

	free(pPara);
	pPara=NULL;
	return new_page;
}

static void OTO_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(TRUE == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_STOP_SEARCH:
			pPara->ScreenExit = TRUE;
			break;
		case CMD_START_SEARCH:
			pPara->SearchStarted = FALSE;	// Search starting failed // 
			pPara->ScreenExit = TRUE;
			break;
		default:
			ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
				((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
					((uint8_t *)last_msgp)[3]);
			break;
	}
}

static void OTO_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case IND_GET_GAUGE:	
				if(FALSE == pPara->gauge_is_valid) {
					if(GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS <= (GUI_X_GetTime() - pPara->search_start_time))
						pPara->gauge_is_valid = TRUE;
					else
						break;	// Ignore GAUGE for a while at startuup // 
				}
				if((TRUE == pPara->SearchStarted) && (FALSE == pPara->ScreenExit)) {
					uint8_t TGauge = msg_ptr->data.gauge;
					if(UMD_GAUGE_MAX < TGauge)
						TGauge = UMD_GAUGE_MAX;	// Truncation to maximum value for safety // 
					if(pPara->Gauge != TGauge) {
						pPara->Gauge = TGauge;
						WAVE_Update_FreqAmp_Gauge((uint16_t)TGauge*GAUGE_FRACTUATION);	// Update Sound Frequency only if MORE THAN ZERO // 
						WM_InvalidateWindow(pPara->WinGauge);
						WM_InvalidateWindow(pPara->WinGStr);
					}
				}
				break;
			case IND_GET_TARGET_ID: 
				if(FALSE == pPara->gauge_is_valid) {
					if(GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS <= (GUI_X_GetTime() - pPara->search_start_time))
						pPara->gauge_is_valid = TRUE;
					else
						break;	// Ignore target-id for a while at startuup // 
				}
				if ((TRUE == pPara->SearchStarted) && (FALSE == pPara->ScreenExit)) {				
					uint8_t TempID = msg_ptr->data.target_id;
					if(pPara->active_targetIDMax < TempID)
						TempID = pPara->active_targetIDMax;	// Truncation to maximum value for safety // 
					if(TempID != pPara->targetID) {	// If last targetID changed redraw GAUGE related UI objects with new color // 
						pPara->targetID = TempID;
						WM_InvalidateWindow(pPara->WinGauge);
					}
				}
				break;
			case RSP_STOP_SEARCH:
				StopCommTimeout();
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					/* CMD stop implemented by Detector */
				}
				else {
					/* CMD stop implementation FAILED */
				}
				pPara->ScreenExit = TRUE;
				break;
			case RSP_START_SEARCH:
				StopCommTimeout();
				if(CMD_DONE == msg_ptr->data.cmd_status) { /* We are happy, Search Started */
					pPara->SearchStarted = TRUE;
					// start sinus wave output from DAC // 
					WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
					// Modulator frequency is 3 times increased for automatic search mode, this will cause faster frequency switch on dac output // 
					WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_DEFAULT_AMP); // Start DAC Wave for minimum GAUGE // 
				}
				else { /*We are sad */
					pPara->SearchStarted = FALSE;
				}
				break;
			case CMD_SET_A_CLOCK_DELAY:
			case CMD_SET_B_CLOCK_DELAY: 
			case CMD_SET_C_CLOCK_DELAY: 
			case CMD_SET_REF_CLOCK_FREQ:
				Process_Clk_gen_Msg(gui_msg);
				break;
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
  switch (pMsg->MsgId) {
		case WM_PAINT:
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			// Write Background Memdev to LCD // 
			if(0 != SBResources[OTO_PICs][OTO_SCRSTRBACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_SCRSTRBACK].hMemHandle, SCRNAME_WINDOW_STARTX, SCRNAME_WINDOW_STARTY);
			}
			// Draw SCREEN NAME STRING // 
			{
				GUI_SetTextMode(GUI_TM_TRANS);
				if(LANG_GE != APP_GetValue(ST_LANG))
					GUI_SetFont(APP_32B_FONT);
				else
					GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_YELLOW);
				GUI_DispStringAt(GetString(STR_OTO_SEARCH_INDX), STATUS_BAR_X_SIZE + 55, 2);
			}
			// Write GSTR Background Memdev to LCD // 
			if(0 != SBResources[OTO_PICs][OTO_GSTRBACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][OTO_GSTRBACK].hMemHandle, GSTR_WINDOW_STARTX, GSTR_WINDOW_STARTY);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}


/*************************** End of file ****************************/

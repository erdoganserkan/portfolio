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
#include <BSP.h>
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
#include "StatusBar.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "APlay.h"
#include "DepthCalc.h"

#define DEPTH_GUI_ANIM_MS		250

typedef enum
{
	DEPTH_LEFT_SELECT = 0,
	DEPTH_RIGHT_SELECT,
	DEPTH_START_CALC_SENT,
	DEPTH_RESULT_SHOW,
	
	DEPTH_STATE_COUNT
} eDepthState;
typedef struct PARA_s {
	uint8_t 	ready4pkey;
	uint8_t DepthCalcState;
	uint8_t ScreenExit;
	uint8_t LeftVal, RightVal;
	uint8_t ScrState;
	uint8_t Selection;	// Selected is "1" // 
	uint16_t CalcDpt;
	WM_CALLBACK *OldDesktopCallback;
	WM_HWIN WinDepth;
	WM_HTIMER hTimerGUI;
	char 		buf1[32];
	char 		buf2[32];
	char 		buf[6][8];
} PARA;

typedef enum {
	DEPTH_CALC_OK	= 0,		// 
	DEPTH_CALC_FAIL,			// CMD response received with CMD_FAIL state // 
	DEPTH_CALC_TIMEOUT,		// CMD response not receivd // 
	
	DEPTH_CALC_COUNT
} eDepthCalcState;

// Internal Resource Definitions // 
static uint8_t new_page;
uint8_t page_before_depth = RM_SCREEN;
static PARA * pPara = NULL;

// Shared Resource Definitions // 
uint8_t DepthCalcEnabled = FALSE;	// Depth Calculation Screen enterence permitted or forbidden // 
extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 
extern uint8_t active_page; // Defined in GUIDEMO_Start.c module //

// Function Prototypes //
static void DEPTH_on_msg_ready(void);
static void DEPTH_on_comm_timeout(void *last_msgp);
static void _cbBk(WM_MESSAGE * pMsg);

static void _cbDrawDepth(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
		#if(TRUE == SERIAL_TASK_STATE)
			case GUI_USER_MSG_NEW_DEPTH:	// This message will be received from Serial task // 
				DEBUGM("NEW DPT: %u\n", pMsg->Data.v);
				if(DEPTH_MAX_CM < (pMsg->Data.v))
					pPara->CalcDpt = DEPTH_MAX_CM;
				else 
					pPara->CalcDpt = pMsg->Data.v;
				break;
		#endif
		case WM_CREATE:
			pPara->hTimerGUI = WM_CreateTimer(hWin, ID_TIMER_DEPTH_GUI, DEPTH_GUI_ANIM_MS, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(pPara->hTimerGUI);
			break;
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_DEPTH_GUI:
					if((DEPTH_LEFT_SELECT == pPara->ScrState) || (DEPTH_RIGHT_SELECT == pPara->ScrState)) {
						if(0 == pPara->Selection) 
							pPara->Selection = 1;
						else 
							pPara->Selection = 0;
						WM_InvalidateWindow(hWin);
					}
					WM_RestartTimer(pMsg->Data.v, DEPTH_GUI_ANIM_MS);	// Restart software timer // 
					break;
				default:
					break;				
			}
		}
		break;
		case WM_PAINT: {
			volatile uint8_t indx = 0;
			uint8_t DrawNums = FALSE;
			uint8_t ShowDpt = FALSE;
			uint16_t str_rect_upx_pos, str_rect_upy_pos, str_rect_downy_pos;

			switch(pPara->ScrState) { 
				case DEPTH_LEFT_SELECT:
					if(1 == pPara->Selection) {
						if(0 != SBResources[DPT_PICs][DPT_SEL].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_SEL].hMemHandle, \
								LEFT_NUM_POSX, LEFT_NUM_POSY);
						}
					} else {
						if(0 != SBResources[DPT_PICs][DPT_UNSEL].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_UNSEL].hMemHandle, \
								LEFT_NUM_POSX, LEFT_NUM_POSY);
						}
					}
					if(0 != SBResources[DPT_PICs][DPT_UNSEL].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_UNSEL].hMemHandle, \
							RIGHT_NUM_POSX, RIGHT_NUM_POSY);
					}
					DrawNums = TRUE;
					break;
				case DEPTH_RIGHT_SELECT:
				case DEPTH_START_CALC_SENT:
					if(0 != SBResources[DPT_PICs][DPT_UNSEL].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_UNSEL].hMemHandle, \
							LEFT_NUM_POSX, LEFT_NUM_POSY);
					}
					if(1 == pPara->Selection){
						if(0 != SBResources[DPT_PICs][DPT_SEL].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_SEL].hMemHandle, \
								RIGHT_NUM_POSX, RIGHT_NUM_POSY);
						}
					} else {
						if(0 != SBResources[DPT_PICs][DPT_UNSEL].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_UNSEL].hMemHandle, \
								RIGHT_NUM_POSX, RIGHT_NUM_POSY);
						}
					}
					DrawNums = TRUE;
					break;
				case DEPTH_RESULT_SHOW: {
					volatile uint8_t indx;
					// 1- Draw Ruler and Strs 
					if(0 != SBResources[DPT_PICs][DPT_RULER].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_RULER].hMemHandle, \
							DPT_RULER_POSX, DPT_RULER_POSY);
					}
					for(indx=0 ;; indx += (DEPTH_MAX_CM/DEPTH_GUI_STEPs_COUNT)) {
						memset(pPara->buf1, 0, sizeof(pPara->buf1));
						sprintf(pPara->buf1, "%u.%um", indx/100, (indx%100)/10);
						GUI_DispStringAt(pPara->buf1, DPT_RULER_STRs_POSX, \
							DPT_RULER_FIRST_STR_POSY+((indx/(DEPTH_MAX_CM/DEPTH_GUI_STEPs_COUNT))*DPT_RULER_STRs_Y_INTERVAL)-5);
						if(DEPTH_MAX_CM <= indx)
							break;
					}
					// 2- Draw Coil
					if(0 != SBResources[DPT_PICs][DPT_COIL].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_COIL].hMemHandle, \
							DPT_COIL_POSX, DPT_COIL_POSY);
					}
					// 3- Draw target according to value received from detector 
					if(0 != SBResources[DPT_PICs][DPT_TARGET].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_TARGET].hMemHandle, \
							DPT_TARGET_POSX, DPT_TARGET_POSY_MIN + \
								((uint32_t)pPara->CalcDpt * (uint32_t)(DPT_TARGET_POSY_MAX - DPT_TARGET_POSY_MIN))/DEPTH_MAX_CM);
					}
					// 4- Draw right side background //
					if(0 != SBResources[DPT_PICs][DPT_REPORT_BACK].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_REPORT_BACK].hMemHandle, \
							DPT_REPORT_POSX, DPT_REPORT_POSY);
					}
					ShowDpt = TRUE;
				}
				break;
				default:
					break;
			}
			// Draw Surrounding Rectangle for Instruction Text / Depth // 
			if(DEPTH_RESULT_SHOW != pPara->ScrState) {
				str_rect_upx_pos = STR_RECT_UPX;
				str_rect_upy_pos = STR_RECT_UPY;
				str_rect_downy_pos = STR_RECT_DOWNY;
			}
			else {
				str_rect_upx_pos = STR_RECT_UPX+3;
				str_rect_upy_pos = STR_RECT_UPY-3;
				str_rect_downy_pos = STR_RECT_DOWNY2;
			}
			GUI_SetColor(GUI_MAKE_ALPHA(0x00, 0x1D94F7));
			GUI_DrawRoundedFrame(str_rect_upx_pos, str_rect_upy_pos, STR_RECT_DOWNX+6, str_rect_downy_pos+3, 4, 5);
			// Draw Instruction Text / Dept in Rectangle 
			if(1){
				GUI_RECT gRect = {str_rect_upx_pos, str_rect_upy_pos, STR_RECT_DOWNX, str_rect_downy_pos};
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_WHITE);
				if(FALSE == ShowDpt) {
					uint8_t num_shift = 0;

					if(LANG_FR != APP_GetValue(ST_LANG)) 
						GUI_SetFont(APP_24B_FONT);
					else {
						GUI_SetFont(APP_19B_FONT);
						num_shift = 9;
					}
					GUI_DispStringAt(GetString(STR_WIDTH_INDX), EN_STR_UPX-num_shift, EN_STR_UPY);
					GUI_DispStringAt(GetString(STR_HEIGHT_INDX), BOY_STR_UPX-num_shift, BOY_STR_UPY);
					memset(pPara->buf2, 0, sizeof(pPara->buf2));
					sprintf(pPara->buf2, "%s", GetString(STR_SIZE_SELECT_INDX));
				}
				else {
					if(LANG_FR != APP_GetValue(ST_LANG)) 
						GUI_SetFont(APP_32B_FONT);
					else 
						GUI_SetFont(APP_24B_FONT);
					memset(pPara->buf2, 0, sizeof(pPara->buf2));
					if(DEPTH_CALC_OK == pPara->DepthCalcState)
						sprintf(pPara->buf2, "%s %u.%u M", GetString(STR_DEPTH_RESULT_INDX), \
							pPara->CalcDpt/100, (pPara->CalcDpt-((pPara->CalcDpt/100)*100))/10);
					else if(DEPTH_CALC_FAIL == pPara->DepthCalcState)
						sprintf(pPara->buf2, "%s X.X M", GetString(STR_DEPTH_RESULT_INDX));
					else if(DEPTH_CALC_TIMEOUT== pPara->DepthCalcState)
						sprintf(pPara->buf2, "%s *.* M", GetString(STR_DEPTH_RESULT_INDX));
				}
				GUI_DispStringInRectWrap(pPara->buf2, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			}
			
			if(TRUE == DrawNums) {
				// Draw First Nums  // 
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_BLACK);
				GUI_SetFont(APP_24B_FONT);
				// Low & High Numbers //
				// Left Nums //
				sprintf(pPara->buf[0], "%02u\n",pPara->LeftVal+5);
				GUI_DispStringAt(pPara->buf[0], LOWNUM_LEFT_UPX+6, LOWNUM_LEFT_UPY);
				//GUI_DispStringInRectWrap(buf, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				sprintf(pPara->buf[1], "%02u\n",pPara->LeftVal-5);
				GUI_DispStringAt(pPara->buf[1], HIGHNUM_LEFT_UPX+6, HIGHNUM_LEFT_UPY);
				// Right Nums //
				sprintf(pPara->buf[2], "%02u\n",pPara->RightVal+5);
				GUI_DispStringAt(pPara->buf[2], LOWNUM_RIGHT_UPX+6, LOWNUM_RIGHT_UPY);
				sprintf(pPara->buf[3], "%02u\n",pPara->RightVal-5);
				GUI_DispStringAt(pPara->buf[3], HIGHNUM_RIGHT_UPX+6, HIGHNUM_RIGHT_UPY);
				// real numbers //
				// Left //
				GUI_SetColor(GUI_YELLOW);
				//GUI_SetFont(APP_32B_FONT);
				GUI_SetFont(&GUI_FontD32);
				sprintf(pPara->buf[4], "%02u\n",pPara->LeftVal);
				GUI_DispStringAt(pPara->buf[4], REALNUM_LEFT_UPX-3, REALNUM_LEFT_UPY);
				// Right // 
				sprintf(pPara->buf[5], "%02u\n",pPara->RightVal);
				GUI_DispStringAt(pPara->buf[5], REALNUM_RIGHT_UPX-3, REALNUM_RIGHT_UPY);
			}
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("DEPTH KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_LEFT_EVENT:	// 62
						switch(pPara->ScrState) {
							case DEPTH_LEFT_SELECT:
								if(DEPTH_TARGET_SIDE_STEP_CM < pPara->LeftVal)
									pPara->LeftVal -= DEPTH_TARGET_SIDE_STEP_CM;
								break;
							case DEPTH_RIGHT_SELECT:
								if(DEPTH_TARGET_SIDE_STEP_CM < pPara->RightVal)
									pPara->RightVal -= DEPTH_TARGET_SIDE_STEP_CM;
								break;
							default:
								break;
						}
						break;
					case KEY_RIGHT_EVENT:	// 61
						switch(pPara->ScrState) {
							case DEPTH_LEFT_SELECT:
								if((DEPTH_MAX_TARGET_SIDE_CM - DEPTH_TARGET_SIDE_STEP_CM) >= pPara->LeftVal)
									pPara->LeftVal += DEPTH_TARGET_SIDE_STEP_CM;
								break;
							case DEPTH_RIGHT_SELECT:
								if((DEPTH_MAX_TARGET_SIDE_CM - DEPTH_TARGET_SIDE_STEP_CM) >= pPara->RightVal)
									pPara->RightVal += DEPTH_TARGET_SIDE_STEP_CM;
								break;
							default:
								break;
						}
						break;
					case KEY_OK_EVENT:	// 65
						switch(pPara->ScrState) {
							case DEPTH_LEFT_SELECT:
								pPara->ScrState = DEPTH_RIGHT_SELECT;
								break;
							case DEPTH_RIGHT_SELECT: {	// Send Start-Depth-Calculation command // 
								UmdPkt_Type msg;
								msg.cmd = CMD_START_DEPTH_CALCULATION;
								msg.length = 4;	// CMD(U8) + LENGTH(U8) + WIDTH(U8) + HEIGHT(U8) // 
								msg.data.Depth_Params.width = pPara->LeftVal;
								msg.data.Depth_Params.Height = pPara->RightVal;
								UARTSend((uint8_t *)&msg, \
									UMD_CMD_TIMEOUT_DEPTH_CALC_MS, DEPTH_on_comm_timeout);
								pPara->ScrState = DEPTH_START_CALC_SENT;
							}
							break;
							case DEPTH_START_CALC_SENT:
								key_valid = FALSE;
								break;
							case DEPTH_RESULT_SHOW:
								pPara->ScrState = DEPTH_LEFT_SELECT;	// Start from initial screen // 
								WM_InvalidateWindow(hWin);	
								break;
							default:
								ERRM("UNEXPECT DEPTH-ScrState(%u)\n", pPara->ScrState);
								break;
						};
						break;
					case KEY_ESC_EVENT: 	// 66
						pPara->ScreenExit = TRUE;
						new_page = page_before_depth;
						break;
					case KEY_OTO_EVENT:	// 63
						new_page = OTO_SEARCH;
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
uint8_t DEPTHCalc(void) 
{
    pPara = (PARA *)calloc(sizeof(PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	else 
		memset(pPara,0,sizeof(PARA));
	if(0 != InitGroupRes(DPT_PICs, 0xFF))	// DEPTH Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = TRUE;

	pPara->ScreenExit = FALSE;
	new_page = RM_SCREEN;
	pPara->OldDesktopCallback = NULL;
	pPara->LeftVal = pPara->RightVal = 5;
	pPara->ScrState = DEPTH_LEFT_SELECT;
	pPara->Selection = 1;
	pPara->WinDepth = 0;
	pPara->DepthCalcState = DEPTH_CALC_OK;
	
  WM_MULTIBUF_Enable(1);
	GUI_Clear();
	
  // Reduce size of desktop window to size of display
  WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
  pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk);
	WM_InvalidateWindow(WM_HBKWIN);

  pPara->WinDepth = WM_CreateWindowAsChild(DEPTH_CALC_WINDOW_POSX, DEPTH_CALC_WINDOW_POSY, \
		DEPTH_CALC_WINDOW_SIZEX, DEPTH_CALC_WINDOW_SIZEY, \
			WM_HBKWIN, WM_CF_SHOW | WM_CF_HASTRANS, _cbDrawDepth, 0);
	// Add pointer to parameter structure to windows
  WM_SetFocus(pPara->WinDepth);

	SB_init(SB_REDUCED_MODE_USE_LEFT);
  // Animation loop
  while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				DEPTH_on_msg_ready();
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
	WM_DeleteWindow(pPara->WinDepth);

	free(pPara);
	pPara = NULL;
	return new_page;
}

// Function to be called when timeout occured after a command send (CMD_RESPONSE_TIMEOUT_MAX_COUNT times) //
	// but response not received // 
static void DEPTH_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(TRUE == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_START_DEPTH_CALCULATION:
			// Dedecetor doesn't give response to cmd // 
			pPara->DepthCalcState = DEPTH_CALC_TIMEOUT;
			pPara->ScrState = DEPTH_RESULT_SHOW;
			WM_InvalidateWindow(pPara->WinDepth);	
			break;
		default:
				ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
					((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
						((uint8_t *)last_msgp)[3]);
			break;
	}
}

// Function to be called when a pkt received from Detector // 
static void DEPTH_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case RSP_START_DEPTH_CALCULATION:
				StopCommTimeout();
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					/* CMD depth calculation implemented by Detector */
					DEBUGM("NEW DPT: %u\n", msg_ptr->data.Depth_Rsp.depth);
					if(DEPTH_MAX_CM < msg_ptr->data.Depth_Rsp.depth)
						pPara->CalcDpt = DEPTH_MAX_CM;
					else 
						pPara->CalcDpt = msg_ptr->data.Depth_Rsp.depth;
					pPara->DepthCalcState = DEPTH_CALC_OK;
				}
				else {	// Depth calculation failed // 
					pPara->DepthCalcState = DEPTH_CALC_FAIL;
				}
				pPara->ScrState = DEPTH_RESULT_SHOW;
				WM_InvalidateWindow(pPara->WinDepth);	
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
			if(0 != SBResources[DPT_PICs][DPT_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[DPT_PICs][DPT_BACK].hMemHandle, \
					DEPTH_CALC_WINDOW_POSX, DEPTH_CALC_WINDOW_POSY);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/

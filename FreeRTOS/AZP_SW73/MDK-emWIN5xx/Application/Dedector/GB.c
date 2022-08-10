// GB SCREEN
	// Windows :
		// 1- Button Pressing Hand 
		// 2- Pointer Holder
		// 3- Coil Window 
	// Popups
		// 1- BalansOK
		// 2- BalansFAIL

/*********************************************************************
*
*       Includes
*
**********************************************************************
*/
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "LPC177x_8x.h"
#include "lpc177x_8x_timer.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "lpc177x_8x_exti.h"
#include "lpc177x_8x_clkpwr.h"
#include <FreeRTOS.h>
#include <event_groups.h>
#include <queue.h>
#include <DIALOG.h>
#include "BSP.h"
#include "debug_frmwrk.h"
#include "AppCommon.h"
#include "AppSettings.h"
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "GUIDEMO.h"
#include "UartInt.h"
#include "StatusBar.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Analog.h"
#include "Dac.h"
#include "APlay.h"
#include "GB.h"

#define DAC_PLAY_COUNTER_MAX	50

typedef struct {
	uint8_t ready4pkey;
	uint8_t dac_cnt;
	uint8_t ScreenExit;
	uint8_t ScrState;
	uint8_t HandAnimState;
	uint8_t PointerDirection;	// TRUE: Left->Right, FALSE:Right->Left // 
	uint8_t CoilDirection;	// TRUE: Up->down, FALSE:Down->Up // 
	uint16_t CoilYPos;
	uint16_t PointerXPos;
	int32_t pointer_max_phase;
	int32_t pointer_min_phase;
	int32_t pointer_last_phase;
	int32_t pointer_new_xpos;
	WM_HWIN hWinHand, hWinPointer, hWinCoil;
	WM_HTIMER GBTimerREAL;
	WM_CALLBACK * OldDesktopCallback;
	sPopup popPara;
	char strbuf[64];
	char tid_str_buf[64];
} PARA;
static uint8_t new_page;

// Active Pointer GIU values // 
static int32_t const xpos_half_range = (ACTIVE_POINTER_MAX_POSX - ACTIVE_POINTER_MIN_POSX - ACTIVE_POINTER_MIDDLE_WIDTH)/2;
static int32_t const xpos_middle = ((ACTIVE_POINTER_MAX_POSX + ACTIVE_POINTER_MIN_POSX)/2);

static PARA* pPara = NULL;

static void _cbBk(WM_MESSAGE * pMsg);
static void __GB_on_completed(uint8_t state);
static void GB_on_msg_ready(void);
static void GB_on_comm_timeout(void *msgp);

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

static void _cbDrawHand(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
	
  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
		case WM_PAINT:
			// Read GB State //
			// If GB_WAIT_to_START, Draw Hand with blink animated //
			// else Draw Hand with %50 Alpha Mask 
			switch(pPara->ScrState) {
				case GB_WAIT_to_START:
					if(TRUE == pPara->HandAnimState) {	// Hand & Button on Background // 
						if(0 != SBResources[GB_PICs][GB_HAND].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HAND].hMemHandle, \
								HAND_WINDOW_LEFTX, HAND_WINDOW_LEFTY);
						}
					}
					else {	// Only Background // 
						if(0 != SBResources[GB_PICs][GB_HAND_BACK].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HAND_BACK].hMemHandle, \
								HAND_WINDOW_LEFTX, HAND_WINDOW_LEFTY);
						}
					}
					break;
				case GB_PROCESSING:
				case GB_COMPLETED:
					if(0 != SBResources[GB_PICs][GB_HAND50].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HAND50].hMemHandle, \
							HAND_WINDOW_LEFTX, HAND_WINDOW_LEFTY);
					}
					break;
				default:
					while(STALLE_ON_ERR);
					break;
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbDrawCoil(WM_MESSAGE * pMsg) {
	// Read GB State //
	// If GB_WAIT_to_START, Draw Coil50 @ Up minimum position //
	// If GB_PROCESSING, Draw Coil as ANIMATED
	// If GB_COMPLETED, Draw Coil @ Down Max Position //
	// else kill yourself //
  WM_HWIN     hWin;
	
  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
		case WM_PAINT:
			switch(pPara->ScrState) {
				case GB_COMPLETED:
				case GB_WAIT_to_START:
					if(0 != SBResources[GB_PICs][GB_COIL50].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_COIL50].hMemHandle, \
							COIL_LEFTX, COIL_MIN_LEFTY);
					}
					break;
				case GB_PROCESSING:
					// Draw Background of Coil // 
					if(0 != SBResources[GB_PICs][GB_COIL_BACK].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_COIL_BACK].hMemHandle, \
							COIL_LEFTX, COIL_MIN_LEFTY);
					}
					// Draw Coil @ new position // 
					if(0 != SBResources[GB_PICs][GB_COIL].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_COIL].hMemHandle, \
							COIL_LEFTX, pPara->CoilYPos);
					}
					break;
				default:
					while(STALLE_ON_ERR);
					break;
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}
static void _cbDrawPointer(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  switch (pMsg->MsgId) {
  case WM_TIMER: {
    int Id = WM_GetTimerId(pMsg->Data.v);
		static uint8_t BatSimul = 0;
    switch (Id) {
			case ID_TIMER_GB_REAL:
				if(++BatSimul == 20) {
					WM_InvalidateWindow(WM_HBKWIN);
					BatSimul = 0;
				}
 				switch(pPara->ScrState) {
					case GB_WAIT_to_START:	// Hand ON / OFF Animation // 
						pPara->HandAnimState = (TRUE == pPara->HandAnimState)?(FALSE):(TRUE);
						WM_InvalidateWindow(pPara->hWinHand);	
						WM_RestartTimer(pMsg->Data.v, HAND_ON_OFF_ANIM_MS);
						break;
					case GB_PROCESSING:	// Calculate new pos of pointer & coil // 
						if(TRUE == pPara->CoilDirection) {	// Up -> Down // 
							if((pPara->CoilYPos += COIL_STEP_VAL) >= COIL_MAX_LEFTY) {
								pPara->CoilYPos = COIL_MAX_LEFTY;
								pPara->CoilDirection = FALSE;
							}
						}
						else {	// Down -> Up // 
							if((pPara->CoilYPos -= COIL_STEP_VAL) <= COIL_MIN_LEFTY) {
								pPara->CoilYPos = COIL_MIN_LEFTY;
								pPara->CoilDirection = TRUE;
							}
						}
						#if(FALSE == ACTIVE_GB_POINTER)
							if(TRUE == pPara->PointerDirection) {	// Left -> Right // 
								if((pPara->PointerXPos += POINTER_STEP_VAL) >= POINTER_MAX_POSX) {
									pPara->PointerXPos = POINTER_MAX_POSX;
									pPara->PointerDirection = FALSE;
								}
							}
							else {	// Right -> Left // 
								if((pPara->PointerXPos -= POINTER_STEP_VAL) <= POINTER_MIN_POSX) {
									pPara->PointerXPos = POINTER_MIN_POSX;
									pPara->PointerDirection = TRUE;
								}
							}
							// do update request pointer window //
							WM_InvalidateWindow(pPara->hWinPointer);	// pointer will do new left-right movement // 
						#endif
						WM_InvalidateWindow(pPara->hWinCoil);	// Coil's new up-down movement will be drawed // 
						WM_RestartTimer(pMsg->Data.v, GB_ACTIVE_ANIM_MS);
						break;
					case GB_COMPLETED:	// There is no animation, DONT restart timer //
						break;
					default:
						while(STALLE_ON_ERR);
						break;
				}
				break;
			default:	// Ignore silently // 
				break;
    }
	}
	break;
  case WM_PAINT: {
		if(0 == pPara->GBTimerREAL)
			pPara->GBTimerREAL = WM_CreateTimer(hWin, ID_TIMER_GB_REAL, HAND_ON_OFF_ANIM_MS, 0);
		else
			WM_RestartTimer(pPara->GBTimerREAL, HAND_ON_OFF_ANIM_MS);

		// According to State Draw Widgets //
		switch(pPara->ScrState) {
			// Draw Holder50 @ initial location //
			case GB_WAIT_to_START:
			case GB_COMPLETED:	// Only %50 Alpha Channel HOLDER, NO POINTER // 
				if(0 != SBResources[GB_PICs][GB_HOLDER50].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HOLDER50].hMemHandle, \
						HOLDER_LEFTX, HOLDER_LEFTY);
				}
				break;
			case GB_PROCESSING:
				GUI_Clear();
				// Draw Holder & Pointer as animated // 
				if((0 != SBResources[GB_PICs][GB_HOLDER].hMemHandle)) {
					GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_HOLDER].hMemHandle, \
						HOLDER_LEFTX, HOLDER_LEFTY);
				}
				#if(FALSE == ACTIVE_GB_POINTER)				
					if(0 != SBResources[GB_PICs][GB_POINTER].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_POINTER].hMemHandle, \
							pPara->PointerXPos, POINTER_POSY);
					}
				#else
					// Draw new active pointer position // 
					if(xpos_middle < pPara->pointer_new_xpos) {	// Draw GREEN rectangle to right side // 
						GUI_SetColor(GUI_LIGHTGREEN);
						GUI_FillRect(xpos_middle + (ACTIVE_POINTER_MIDDLE_WIDTH/2), ACTIVE_POINTER_MIN_POSY, pPara->pointer_new_xpos, ACTIVE_POINTER_MAX_POSY);		
					} else if(xpos_middle > pPara->pointer_new_xpos) {	// Draw BLUE rectangle to left side // 
						GUI_SetColor(GUI_LIGHTBLUE);
						GUI_FillRect(pPara->pointer_new_xpos, ACTIVE_POINTER_MIN_POSY, xpos_middle-(ACTIVE_POINTER_MIDDLE_WIDTH/2), ACTIVE_POINTER_MAX_POSY);		
					} else {
						// Dont draw any 
					}
					// Draw Middle static BLACK bar // 
					GUI_SetColor(GUI_BLACK);
					GUI_FillRect(xpos_middle-(ACTIVE_POINTER_MIDDLE_WIDTH/2), ACTIVE_POINTER_MIN_POSY, \
						xpos_middle+(ACTIVE_POINTER_MIDDLE_WIDTH/2), ACTIVE_POINTER_MAX_POSY);							
				#endif
				break;
			default:
				while(STALLE_ON_ERR);
				break;
		}
	}
	break;
  case WM_SET_FOCUS:
    pMsg->Data.v = 0;
    break;
  case WM_KEY:
	TRACEM("GB KEY Handler Working");
	if(TRUE != pPara->ready4pkey)
		break;	// DONT process key press events if GUI not ready // 
    pInfo = (WM_KEY_INFO *)pMsg->Data.p;
    if (pInfo->PressedCnt) {
      switch (pInfo->Key) {
		case KEY_CONFIRM_EVENT:	// OK // 
			switch(pPara->ScrState) {
				case GB_WAIT_to_START: 
				case GB_COMPLETED:
				if(CMD_START_GROUND_BALANCE != last_send_msg.cmd) {	// Be sure about send command only once // 
					UmdPkt_Type msg;
					WAVE_Generator_stop(TRUE, TRUE, TRUE);
					start_dac_audio(BUTTON_OK_SOUND, TRUE);	// Wait until DAC audio file play finishes // 
					WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_DEFAULT_AMP); // REStart DAC Wave //
					msg.cmd = CMD_START_GROUND_BALANCE;
					msg.length = 3;
					msg.data.gb_type = AppStatus.gb_type_required;
					UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS, GB_on_comm_timeout);
				}
				break;
				default:	// Silently IGNORE all OK key press events @ other cases // 
					break;
			}
			break;
		#if(GB_ANIM == TRUE)
			case KEY_MINUS_EVENT:	// MINUS // 
				pPara->ScrState = GB_COMPLETED;
				OPResult = FAIL;
				WM_InvalidateWindow(WM_HBKWIN);	// Reconstruct all screen from stratch // 
				break;
			case KEY_PLUS_EVENT:	// PLUS // 
				pPara->ScrState = GB_COMPLETED;
				OPResult = FAIL;
				WM_InvalidateWindow(WM_HBKWIN);	// Reconstruct all screen from stratch // 
				break;
		#endif
		case KEY_MENU_EVENT:	// ESC // 
			if(CMD_STOP_GROUND_BALANCE != last_send_msg.cmd) {	// Avoid the crash because of Multiple enterence of that case by GUI // 
				UmdPkt_Type msg;
				WAVE_Generator_stop(TRUE, TRUE, TRUE);
				start_dac_audio(BUTTON_OK_SOUND, FALSE);	// Dont wait for audio file play complete // 
				msg.cmd = CMD_STOP_GROUND_BALANCE;
				msg.length = 2;
				UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS, GB_on_comm_timeout);
			}
			break;
		case KEY_OTO_EVENT:	// DOWN / OTO //
		#if(0)
		case KEY_DEPTH_EVENT:	// UP / DEPTH // 
		#endif
			if((GB_WAIT_to_START == pPara->ScrState) || (GB_COMPLETED == pPara->ScrState)) {
				// Set new target pages // 
				#if(0)
				if(KEY_DEPTH_EVENT == pInfo->Key) {
				#else
					if(0) {
				#endif
					extern uint8_t page_before_depth;	// defined in DeptchCalc.c module // 
					page_before_depth = GB_SCREEN;
					new_page = DEPTH_CALC;
				}
				else
					new_page = OTO_SEARCH;
				// Send GB STOP to dedector // 
				if(CMD_STOP_GROUND_BALANCE != last_send_msg.cmd) {
					UmdPkt_Type msg;
					WAVE_Generator_stop(TRUE, TRUE, TRUE);
					start_dac_audio(BUTTON_OK_SOUND, FALSE);	// Dont wait for audio file play complete // 
					msg.cmd = CMD_STOP_GROUND_BALANCE;
					msg.length = 2;
					UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS, GB_on_comm_timeout);
				}
			}			
			break;
		default:	
			break;
      }
    }
    break;
  	case WM_DELETE:
		WM_DeleteTimer(pPara->GBTimerREAL);
		break;
	default:
		WM_DefaultProc(pMsg);
		break;
  }
}

static void GB_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(TRUE == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_STOP_GROUND_BALANCE:
			pPara->ScreenExit = TRUE;	// Leave screen to default target // 
			break;
		case CMD_START_GROUND_BALANCE:
			pPara->ScreenExit = TRUE;	// Leave screen to default target // 
			break;
		default:
			break;
	}
}


uint8_t GB(void) 
{
	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);
	if(0 != InitGroupRes(GB_PICs, 0xFF))	// GB Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = TRUE;
	pPara->ScreenExit = FALSE;

	new_page = RM_SCREEN;	// Default target on screen exit is RM SCREEN // 
	pPara->ScrState = GB_WAIT_to_START;
	pPara->CoilYPos = COIL_MIN_LEFTY;
	pPara->CoilDirection = TRUE;
	pPara->PointerXPos = POINTER_MIN_POSX;
	pPara->PointerDirection = TRUE;
	pPara->HandAnimState = TRUE;
	pPara->pointer_max_phase = ACTIVE_POINTER_MAX_SAFE;
	pPara->pointer_min_phase = ACTIVE_POINTER_MIN_SAFE;
	pPara->pointer_last_phase = 900;
	pPara->pointer_new_xpos = xpos_middle;
	pPara->dac_cnt = 0;

  	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Do AUDIO-DAC INITIALIZATION Before all of the GUI Setup // 
	WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
	// Modulator frequency is 3 times increased for GB screen, this will cause faster frequency switch on dac output // 
	WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
	//WAVE_Update_FreqAmp_Gauge((uint16_t)10*GAUGE_FRACTUATION);	// Update Sound Frequency // 

  	// Reduce size of desktop window to size of display
  	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk);
	WM_InvalidateWindow(WM_HBKWIN);	// Force to redraw Desktop Window // 

	// 1- Create Required Windows //
  	pPara->hWinHand = WM_CreateWindowAsChild(HAND_WINDOW_LEFTX, HAND_WINDOW_LEFTY, \
		HAND_WINDOW_SIZEX, HAND_WINDOW_SIZEY, WM_HBKWIN, WM_CF_SHOW, \
		_cbDrawHand, 0);

	pPara->hWinCoil = WM_CreateWindowAsChild(COIL_LEFTX, COIL_MIN_LEFTY, \
		COIL_SIZEX, (COIL_MAX_LEFTY - COIL_MIN_LEFTY) + COIL_SIZEY, WM_HBKWIN, WM_CF_SHOW, \
		_cbDrawCoil, 0);

	pPara->hWinPointer = WM_CreateWindowAsChild(HOLDER_LEFTX, HOLDER_LEFTY, \
		HOLDER_SIZEX, HOLDER_SIZEY, WM_HBKWIN, WM_CF_SHOW, \
		_cbDrawPointer, 0);
	WM_SetFocus(pPara->hWinPointer);
	SB_init(SB_REDUCED_MODE_USE_RIGHT);

	// Do detector & analog specific job // 
  	// Animation loop
  	while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				GB_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
		if((0 != pPara->dac_cnt) && (0 == (--pPara->dac_cnt))) {
			// Set Gauge as ZERO to mute DAC-Out // 
			WAVE_Update_FreqAmp_Gauge(0);	// Mute // 
		}
  	}
	
	WAVE_Generator_stop(TRUE, TRUE, TRUE);
	// Do Deletion of created objects & Release of Resources //
	#if(0 && (TRUE == CLOCK_GENERATION_STATE))
		stop_clock_generation();
	#endif
	last_send_msg.cmd = 0xFF;
	GUI_ClearKeyBuffer();
  	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_SetFocus(WM_HBKWIN);
	SB_delete();
	WM_DeleteWindow(pPara->hWinHand);
	WM_DeleteWindow(pPara->hWinCoil);
	WM_DeleteWindow(pPara->hWinPointer);

	free(pPara);
	pPara = NULL;
	return new_page;
}

static void GB_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		TRACEM("PKT RECEIVED : \"0x%02X 0x%02X 0x%02X 0x%02X\" \n", gui_msg[0], gui_msg[1], gui_msg[2], gui_msg[3]);
		switch(msg_ptr->cmd) {
			case CMD_SET_A_CLOCK_DELAY: {
				WAVE_Update_FreqAmp_Gauge(100 * GAUGE_FRACTUATION);	// Update Sound Frequency only if MORE THAN ZERO // 
				pPara->dac_cnt = DAC_PLAY_COUNTER_MAX;
				int32_t phase = ((int32_t)gui_msg[2])*CLOCK_GENERATE_PHASE_FRACTION + (((int32_t)gui_msg[3])%10);
				int32_t phase_diff = phase - pPara->pointer_last_phase;
				DEBUGM("Phase (%u.%u), diff(%d)\n", gui_msg[2], gui_msg[3], phase_diff/CLOCK_GENERATE_PHASE_FRACTION);
				#if(0)
				if((ACTIVE_POINTER_MAX_SAFE == pPara->pointer_max_phase) && (ACTIVE_POINTER_MIN_SAFE == pPara->pointer_min_phase)) {
					uint8_t skip_this_time = FALSE;
					if(((phase_diff >= 0) && (phase_diff < CLOCK_GENERATE_PHASE_FRACTION)) || \
							((phase_diff < 0) && (phase_diff > (-CLOCK_GENERATE_PHASE_FRACTION)))) {
						// DONT set phase_max and phase_min values // 
					} else {
						pPara->pointer_max_phase = \
							(((phase_diff<0)?(0-phase_diff):(phase_diff)) * ACTIVE_POINER_GUI_RANGE_MULTIPLER);
						if(pPara->pointer_max_phase > ACTIVE_POINTER_DEF_MAX)
							pPara->pointer_max_phase = ACTIVE_POINTER_DEF_MAX;
						pPara->pointer_min_phase = 0 - pPara->pointer_max_phase;					
					}
				}
				#endif
				if(0 < phase_diff) {
					if(phase_diff > pPara->pointer_max_phase) {
						// phase_diff = pPara->pointer_max_phase;
						pPara->pointer_new_xpos = ACTIVE_POINTER_MAX_POSX;
					}
					else {
						pPara->pointer_new_xpos = \
							xpos_middle + (ACTIVE_POINTER_MIDDLE_WIDTH/2) + ((phase_diff * xpos_half_range)/ pPara->pointer_max_phase);
					}
				} else if (phase_diff < 0){
					if(phase_diff < pPara->pointer_min_phase) {
						// phase_diff = pPara->pointer_min_phase;
						pPara->pointer_new_xpos = ACTIVE_POINTER_MIN_POSX;
					}
					else {
						pPara->pointer_new_xpos = \
							xpos_middle - ((ACTIVE_POINTER_MIDDLE_WIDTH/2) + ((phase_diff * xpos_half_range)/ pPara->pointer_min_phase));
					}
				} else {
					pPara->pointer_new_xpos = xpos_middle;
				}
 
				pPara->pointer_last_phase = phase;
				WM_InvalidateWindow(pPara->hWinPointer);	
			}
			case CMD_SET_B_CLOCK_DELAY: 
			case CMD_SET_C_CLOCK_DELAY: 
			case CMD_SET_REF_CLOCK_FREQ:
				Process_Clk_gen_Msg(gui_msg);
				break;
			case CMD_SET_GROUND_ID:	{
				// send response msg //
				UmdPkt_Type send_msg;
				send_msg.cmd = msg_ptr->cmd + 1;
				send_msg.length = 3;
				send_msg.data.cmd_status = CMD_DONE;
				UARTSend((uint8_t *)&send_msg, 0, NULL);
				// Only store long gb  results into non-volatile memory // 
				if((GB_PROCESSING == pPara->ScrState) && (GB_TYPE_LONG == AppStatus.gb_type_required)){
					uint16_t gnd_id = ((((uint16_t)gui_msg[2])<<8) | ((uint16_t)gui_msg[3])); 
					APP_SetVal(ST_GROUND_ID, gnd_id, TRUE);
				} 
				else 
					ERRM("CMD(0x%02X) received @ GB_State(%u)", msg_ptr->cmd, pPara->ScrState);
			}
			break;
			case RSP_START_GROUND_BALANCE:
				StopCommTimeout();
				// Check "operation status" and start animations if it is ok // 
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					pPara->ScrState = GB_PROCESSING;
					WM_InvalidateWindow(WM_HBKWIN);	// Reconstruct all screen // 
					WM_InvalidateWindow(pPara->hWinHand);	// Reconstruct all screen // 
				}
				else 
					ERRM("cmd_status(0x%02X) @ RSP(0x%02X)", msg_ptr->data.cmd_status, msg_ptr->cmd);
				break;
			case CMD_GB_COMPLETED: {
					// send response msg //
					UmdPkt_Type send_msg;
					send_msg.cmd = msg_ptr->cmd + 1;
					send_msg.length = 3;
					send_msg.data.cmd_status = CMD_DONE;
					UARTSend((uint8_t *)&send_msg, 0, NULL);

					DEBUGM("CMD_GB_COMPLETED received\n");
					if((FALSE == AppStatus.long_gb_done) && (GB_TYPE_LONG == AppStatus.gb_type_required))
						AppStatus.long_gb_done = TRUE;
					else if((FALSE == AppStatus.short_gb_done) && (GB_TYPE_SHORT == AppStatus.gb_type_required))
						AppStatus.short_gb_done = TRUE;

					// Open "gb ok" case popup // 
					__GB_on_completed(TRUE);
				}
				break;
			case CMD_GB_FAILED: {
					// send response msg //
					UmdPkt_Type send_msg;
					send_msg.cmd = msg_ptr->cmd + 1;
					send_msg.length = 3;
					send_msg.data.cmd_status = CMD_DONE;
					UARTSend((uint8_t *)&send_msg, 0, NULL);

					// Open "gb failed" case popup // 
					__GB_on_completed(FALSE);
				}
				break;
			case RSP_STOP_GROUND_BALANCE:
				StopCommTimeout();
				// Trigger an screen quit action // 
				pPara->ScreenExit = TRUE;
				break;	
			default:
				// Ignore silently rest of messages // 
				break;
		}
	}
}

static void __GB_on_completed(uint8_t state)
{
	pPara->popPara.type = GB_POPUP;	// Default popup type // 
	pPara->popPara.hParent = WM_GetFocussedWindow();
	pPara->popPara.PopupData.GBPopupData.Result = state;
	pPara->popPara.PopupData.GBPopupData.ID = APP_GetValue(ST_GROUND_ID);

	pPara->ScrState = GB_COMPLETED;
	WM_InvalidateWindow(WM_HBKWIN);	// Reconstruct all screen // 
	WM_InvalidateWindow(pPara->hWinCoil);	// Reconstruct all screen // 
	WM_InvalidateWindow(pPara->hWinPointer);	// Reconstruct all screen // 
	WM_InvalidateWindow(pPara->hWinHand);	// Reconstruct all screen // 
	
	WAVE_Generator_stop(TRUE, TRUE, TRUE);
	ShowPopup(&(pPara->popPara));	// Open desired screen's pop-up // 
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
		case WM_NOTIFY_PARENT: {
			if(pMsg->hWinSrc == pPara->popPara.hPop) {
				int NCode = pMsg->Data.v;                 // Notification code
				switch (NCode) {	// Jump to STANDARD SEARCH SCREEN // 
					case WM_NOTIFICATION_CHILD_DELETED:	// Popup has been destroyed // 
						if(TRUE == pPara->popPara.PopupData.GBPopupData.Result) {
							if(KEY_OK_EVENT == pPara->popPara.last_key) {
								INFOM("GB popup completed by OK press\n");
								INFOM("jumping to PAGE: %u\n", AppStatus.search_before_gb);
								new_page = AppStatus.search_before_gb;
							}
							else
								new_page = RM_SCREEN;	// Return to RadialMenu if GB-popup is closed with ESC key // 
							pPara->ScreenExit = TRUE;
						}
						else {	// Return to INITIAL SCREEN STATE // 							
							if(KEY_OK_EVENT == pPara->popPara.last_key) {
								WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
								WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
								pPara->ScrState = GB_WAIT_to_START;
								last_send_msg.cmd = 0xFF;
								pPara->CoilYPos = COIL_MIN_LEFTY;
								pPara->CoilDirection = TRUE;
								pPara->PointerXPos = POINTER_MIN_POSX;
								pPara->PointerDirection = TRUE;
								pPara->HandAnimState = TRUE;
								WM_InvalidateWindow(WM_HBKWIN);	// Reconstruct all screen // 
								WM_InvalidateWindow(pPara->hWinCoil);	// Reconstruct all screen // 
								WM_InvalidateWindow(pPara->hWinPointer);	// Reconstruct all screen // 
								WM_InvalidateWindow(pPara->hWinHand);	// Reconstruct all screen // 
							} else {
								pPara->ScreenExit = TRUE;
							}
						}
						break;
					default:
						break;
				}
			}
		}
		break;
		case WM_PAINT: 
			// Draw Background Image //
			if(0 != SBResources[GB_PICs][GB_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[GB_PICs][GB_BACK].hMemHandle, \
					GBBACK_LEFTX, GBBACK_LEFTY);
			}
			GUI_SetFont(APP_32B_FONT);
			// Draw String Cover Rectangle & Gradient //
			GUI_DrawGradientRoundedV(GB_SCR_NAME_STR_LEFTUPX-5, GB_SCR_NAME_STR_LEFTUPY-5, \
				GB_SCR_NAME_STR_LEFTUPX + 231, GB_SCR_NAME_STR_LEFTUPY + GUI_GetFontDistY() + 5, \
					6, GUI_MAKE_ALPHA(0x00, 0xFFFF0C), GUI_MAKE_ALPHA(0x00, 0xFF2616));
			GUI_SetColor(GUI_WHITE);
			GUI_DrawRoundedRect(GB_SCR_NAME_STR_LEFTUPX-5, GB_SCR_NAME_STR_LEFTUPY-5, \
				GB_SCR_NAME_STR_LEFTUPX + 231, GB_SCR_NAME_STR_LEFTUPY + GUI_GetFontDistY()+5, 6);
			// Draw Screen Name //
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetColor(GUI_BLACK);
			{
				GUI_RECT gRect = {GB_SCR_NAME_STR_LEFTUPX-5, GB_SCR_NAME_STR_LEFTUPY-5, \
				GB_SCR_NAME_STR_LEFTUPX + 231, GB_SCR_NAME_STR_LEFTUPY + GUI_GetFontDistY()+5};
				GUI_DispStringInRectWrap(GetString(STR_BALANS_INDX), &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			}
			// Draw Screen Explanation / Instruction // 
			{
				GUI_RECT gRect = {EXPLANATION_STR_LEFTUPX, EXPLANATION_STR_LEFTUPY, \
					EXPLANATION_STR_RIGHTDOWNX, EXPLANATION_STR_RIGHTDOWNY};
				GUI_SetColor(GUI_WHITE);
				GUI_SetFont(APP_24B_FONT);
				switch(pPara->ScrState) {
					case GB_WAIT_to_START:
						sprintf(pPara->strbuf, "%s", GetString(STR_CONFIRM_INDX));
						break;
					case GB_PROCESSING:
						sprintf(pPara->strbuf, "%s", GetString(STR_PROCESSING_INDX));
						break;
					case GB_COMPLETED:
						memset(pPara->strbuf, 0, sizeof(pPara->strbuf));
						break;
					default:
						while(TODO_ON_ERR);
						break;
				}
				GUI_DispStringInRectWrap(pPara->strbuf, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			}
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/

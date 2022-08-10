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
#include "Strings.h"
#include "AppPics.h"
#include "AppFont.h"
#include "DiffCalculator.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "UartInt.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Analog.h"
#include "Dac.h"
#include "APlay.h"
#include "AZPFast.h"

// New type definitions // 
typedef struct PARA_s {
	uint8_t ready4pkey;
	uint8_t tnum_is_valid;
	uint8_t tnum;
	GUI_RECT tnum_gRect;
	char tnum_str[8];
	uint8_t ScreenExit;
	uint32_t search_start_time;
	WM_HWIN	hMidWin;
	WM_CALLBACK * OldDesktopCallback;
	uint8_t cursor_pos;
	uint8_t cursor_show;
	uint16_t fast_state;
	GUI_RECT gRect[AZP_FAST_NUMSTR_COUNT];
} PARA;

// Function Prototypes //
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void AZPFast_on_comm_timeout(void *msgp);
static void AZPFast_on_msg_ready(void);

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 
extern uint8_t active_page;	// Defined in GUIDEMO_Start.c //
static PARA* pPara = NULL;
static uint8_t new_page;
extern uint8_t SearchState;		// defined in GUIDEMO_Start.c // 

// Draw Left & Right Bars and Filled Scope // 
static void _cbMidDraw(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;
	WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			// Set background color for Mid Window // 
			GUI_SetBkColor(AZP_BACKGROUND_COLOR);
			GUI_Clear();

			// Draw Active Area Color and Borders // 
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedRect(2, 2, GLCD_X_SIZE-2, (GLCD_Y_SIZE - AZP_FAST_CURSOR_UPY-5)-2, 2);
			GUI_SetColor(AZP_ACTIVE_AREA_COLOR);
			GUI_FillRoundedRect(3, 3, GLCD_X_SIZE-3, (GLCD_Y_SIZE - AZP_FAST_CURSOR_UPY-5)-3, 2);
			//1- draw cursor if shown 
			if(pPara->cursor_show) {
				GUI_SetColor(GUI_BLACK);
				uint16_t startX = ((uint16_t)(pPara->cursor_pos) * AZP_FAST_CURSOR_SIZEX);
				GUI_FillRoundedRect(startX+4, 3, startX + AZP_FAST_CURSOR_SIZEX-4, 3 + AZP_FAST_CURSOR_SIZEY, 2);
			}
			{
				//2- draw target id number string //
				if(pPara->tnum) {
					snprintf(pPara->tnum_str, sizeof(pPara->tnum_str)-1, "%u", pPara->tnum);
					GUI_RECT gRect = {0,0, GLCD_X_SIZE, (2*GLCD_Y_SIZE)/3};
					pPara->tnum_gRect = gRect;
					GUI_SetFont(&GUI_FontD80);
					GUI_SetColor(GUI_BLACK);
					GUI_SetTextMode(GUI_TM_TRANS);
					GUI_DispStringInRectWrap(pPara->tnum_str, &(pPara->tnum_gRect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				}
			}			
		}
		break;
		case WM_POST_PAINT:
			pPara->ready4pkey = TRUE;
			// Send Search START Command to Dedector //
			// Because of WM_PAINT event is called multiple time, we must be sure about single CMD send // 
			if((SEARCH_IDLE == SearchState) && (SCR_RUNNING == pPara->ScreenExit)) {
				SearchState = SEARCH_START_REQUESTED;
				UmdPkt_Type msg;
				msg.cmd = CMD_START_SEARCH;
				msg.length = 3;	// CMD(U8) + LENGTH(U8) + DATA(U8) // 
				msg.data.search_type = AZP_TARGET_NUM_ANALYSIS;
				UARTSend((uint8_t *)&msg, \
					UMD_CMD_TIMEOUT_SEARCH_CMD_MS*2, AZPFast_on_comm_timeout);
				BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_BRIGHT));	
			}
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("AZP FAST KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_AZP_ENTER_EVENT:
						if(pPara->cursor_show) {
							uint8_t state = (pPara->fast_state & ((0x0001)<<pPara->cursor_pos))? 1 : 0;
							if(state)
								pPara->fast_state &= (~((0x0001)<<pPara->cursor_pos));
							else
								pPara->fast_state |= ((0x0001)<<pPara->cursor_pos);
							WM_InvalidateWindow(pPara->hMidWin);
							WM_InvalidateWindow(WM_HBKWIN);
							pPara->cursor_show = 0;
							pPara->ready4pkey = FALSE;
						} else {
							// send RESET command to analog mcu // 
							{
								UmdPkt_Type msg;
								msg.cmd = IND_ANALOG_RESET;
								msg.length = 2; // CMD(U8) + LENGTH(U8) // 
								UARTSend((uint8_t *)&msg, 0, NULL);
							}
							// clear pointsY buffer and screen // 
							pPara->tnum = 0;
							WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
							// invoke windows // 
							WM_InvalidateWindow(pPara->hMidWin);
							WM_InvalidateWindow(WM_HBKWIN);
							pPara->ready4pkey = FALSE;
						}
						break;
					case KEY_MINUS_EVENT: 
						if(pPara->cursor_show) {
							if(pPara->cursor_pos) {
								pPara->cursor_pos--;
								WM_InvalidateWindow(pPara->hMidWin);
								pPara->ready4pkey = FALSE;
							} else
								key_valid = FALSE;
						} else {
							pPara->cursor_show = 1;
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						}
						break;
					case KEY_PLUS_EVENT: 
						if(pPara->cursor_show) {
							if(pPara->cursor_pos < (AZP_FAST_NUMSTR_COUNT-1)) {
								pPara->cursor_pos++;
								WM_InvalidateWindow(pPara->hMidWin);
								pPara->ready4pkey = FALSE;
							} else
								key_valid = FALSE;
						} else {
							pPara->cursor_show = 1;
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						}
						break;
					case KEY_AZP_FAST_EVENT: {
						SearchState = SEARCH_STOP_REQUESTED;
						pPara->ScreenExit = SCR_EXIT_REQUESTED;
						new_page = AZP_ALL_METAL_SCR;
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPFast_on_comm_timeout);
					}
					break;
					case KEY_MENU_EVENT: {
						SearchState = SEARCH_STOP_REQUESTED;
						pPara->ScreenExit = SCR_EXIT_REQUESTED;
						new_page = AZP_MENU_SCR;
						uint16_t active_sb_pos = AZP_SB_POS_SYS_SET;
						SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2;	// CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPFast_on_comm_timeout);
					}
					break;
					default:	
						key_valid = FALSE;
						break;
				}
				if(SCR_EXIT_REQUESTED == pPara->ScreenExit)
						BSP_PWMSet(0, BSP_PWM_LCD, 0);	// Make All Screen DARK // 
				if(SEARCH_STOP_REQUESTED == SearchState) {
					if(TRUE == is_dac_playing()) {
						WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
						WAVE_Generator_stop(TRUE, TRUE, TRUE);
					}
					// clear pointsY buffer and screen // 
					pPara->tnum = 0;
					// invoke windows // 
					WM_InvalidateWindow(pPara->hMidWin);
					WM_InvalidateWindow(WM_HBKWIN);
					pPara->ready4pkey = FALSE;
				}
				if(TRUE == key_valid) {
					WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
					uint32_t freq_backup = get_DAC_DMA_Update_Freq();
					DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
					start_dac_audio(BUTTON_OK_SOUND, TRUE);	// Dont wait for audio file play complete // 
					DAC_DMA_Update_Freq(freq_backup);
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
*    Creates and executes Metal Analysis, Mineralized Search & STD Search screens 
*/
uint8_t AZP_Fast(void) 
{
	volatile uint8_t indx;
	static uint8_t first_time = TRUE;

	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = FALSE;
	pPara->OldDesktopCallback = NULL;
	pPara->ScreenExit = SCR_RUNNING;
	
	new_page = AZP_FAST_SCR;
	SearchState = SEARCH_IDLE;
	pPara->search_start_time = 0;
	pPara->tnum_is_valid = FALSE;
	pPara->cursor_pos = 0;
	pPara->cursor_show = 0;
	if(TRUE == first_time) {
		first_time = FALSE;
		uint16_t fast_initial = 0xFFFF;	// ignore the stored value @ applcation startup // 
		APP_SetVal(ST_AZP_FAST, fast_initial, TRUE);
	} 
	pPara->fast_state = APP_GetValue(ST_AZP_FAST);

	for(indx=0 ; indx<AZP_FAST_NUMSTR_COUNT ; indx++) {
		pPara->gRect[indx].x0 = AZP_FAST_NUMSTRs_UPX + (AZP_FAST_NUMSTRs_SIZEX*indx)+3;
		pPara->gRect[indx].y0 = AZP_FAST_NUMSTRs_UPY+2;
		pPara->gRect[indx].x1 = pPara->gRect[indx].x0 + AZP_FAST_NUMSTRs_SIZEX-3;
		pPara->gRect[indx].y1 = pPara->gRect[indx].y0 + AZP_FAST_NUMSTRs_SIZEY-2;
	}
	
	WM_MULTIBUF_Enable(1);
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();	// Is This necessary? We are already drawing a picture that fully covering LCD // 
	GUI_ClearKeyBuffer();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);	// Force to redraw Desktop Window // 

	/* CREATE Windows and Do Supplementary Operations */
	pPara->hMidWin = WM_CreateWindowAsChild(0, AZP_FAST_CURSOR_UPY+1, \
		GLCD_X_SIZE, GLCD_Y_SIZE - AZP_FAST_CURSOR_UPY-5, WM_HBKWIN, WM_CF_SHOW, _cbMidDraw, sizeof(pPara));
	WM_SetUserData(pPara->hMidWin,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hMidWin);

	SB_init(SB_FULL_TOP);
	// Do detector & analog specific job // 
	#if(TRUE == CLOCK_GENERATION_STATE)
		DiffCalc_init();
	#endif
	set_lcd_bcklight_reduce_state(FALSE);

	// Animation loop
	while (likely(SCR_EXIT_CONFIRMED != pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				AZPFast_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}
	
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();	
	//if(SEARCH_START_FAILED == SearchState)
		//vTaskDelay(1000 * (configTICK_RATE_HZ/1000));

	set_lcd_bcklight_reduce_state(TRUE);
	if(TRUE == is_dac_playing()) {
		WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
		WAVE_Generator_stop(TRUE, TRUE, TRUE);
	}
	DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
	// Do Deletion of created objects & Release of Resources // 
	#if(0 && (TRUE == CLOCK_GENERATION_STATE))
		stop_clock_generation();
	#endif
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_SetFocus(WM_HBKWIN);
	SB_delete();
	WM_DeleteWindow(pPara->hMidWin);
	GUI_Clear();	

	free(pPara);
	pPara=NULL;
	return new_page;
}

// Function to be called when timeout occured after a command send but response not received // 
static void AZPFast_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(SCR_EXIT_CONFIRMED == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_STOP_SEARCH:			
			SearchState = SEARCH_STOP_FAILED;
			pPara->ScreenExit = SCR_EXIT_CONFIRMED;
			break;
		case CMD_START_SEARCH: {
		#if(0)
			// ReEnter to this screen // 
			extern uint8_t prev_page;
			SearchState = SEARCH_START_FAILED;
			new_page = AZP_FAST_SCR;
			pPara->ScreenExit = SCR_EXIT_CONFIRMED;
		#else
			pPara->search_start_time = GUI_X_GetTime();
			SearchState = SEARCH_STARTED;
			uint16_t AppVolume = APP_GetValue(ST_VOL);
			if(AppVolume)
				start_dac_playing(3);
		#endif
		} break;
		default:
			ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
				((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
					((uint8_t *)last_msgp)[3]);
			break;
	}
}

// Function to be called when a pkt received from Detector // 
static void AZPFast_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case IND_GET_TARGET_NUM:	
				if((SEARCH_STARTED != SearchState))
					break;
				if(FALSE == pPara->tnum_is_valid) {
					if(GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS <= (GUI_X_GetTime() - pPara->search_start_time))
						pPara->tnum_is_valid = TRUE;
					else
						break;	// Ignore GAUGE for a while at startuup // 
				}
				if(SCR_RUNNING == pPara->ScreenExit){
					uint8_t tnum = msg_ptr->data.target_num;
					if(AZP_TARGET_NUM_MAX < tnum)
						tnum = AZP_TARGET_NUM_MAX;	// Truncation to maximum value for safety // 
					if((AZP_TARGET_NUM_MIN > tnum) && (0 != tnum))
						tnum = AZP_TARGET_NUM_MIN;	// Truncation to mminimum value for safety // 
					if(pPara->tnum != tnum) {
						pPara->tnum = tnum;
						// Check fast state, and determine SHOW or IGNORE new target number // 
						uint16_t tnum_gauge = tnum;
						if(tnum) {
							tnum = (tnum/10)-1;
							if(((0x0001)<<tnum) & (pPara->fast_state)) {
								if(40 > tnum_gauge)
									WAVE_SetFreq_FTID(TARGET_FERROs);
								else if(70 <= tnum_gauge)
									WAVE_SetFreq_FTID(TARGET_NFERROs);
								else
									WAVE_SetFreq_FTID(TARGET_GOLD);									
								WAVE_Update_FreqAmp_Gauge(tnum_gauge * GAUGE_FRACTUATION);	// Update Sound Frequency // 
								WM_InvalidateWindow(pPara->hMidWin);
							}
						} else {
							WAVE_Update_FreqAmp_Gauge(pPara->tnum = 0);	// Update Sound Frequency // 
							WM_InvalidateWindow(pPara->hMidWin);
						}
					}
				}
				break;
			case RSP_STOP_SEARCH:
				StopCommTimeout();
				if(SEARCH_STOP_REQUESTED != SearchState)
					break;
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					/* CMD stop implemented by Detector */
					// store fast state informatio 
					SearchState = SEARCH_IDLE;
					uint16_t fast_stored = APP_GetValue(ST_AZP_FAST);
					if(pPara->fast_state != fast_stored){
						APP_SetVal(ST_AZP_FAST, pPara->fast_state, TRUE);
					}
				}
				else {
					/* CMD stop implementation FAILED */
					SearchState = SEARCH_STOP_FAILED;
				}
				pPara->ScreenExit = SCR_EXIT_CONFIRMED;
				break;
			case RSP_START_SEARCH:
				StopCommTimeout();
				if(SEARCH_START_REQUESTED != SearchState)
					break;
				pPara->search_start_time = GUI_X_GetTime();
				SearchState = SEARCH_STARTED;
				uint16_t AppVolume = APP_GetValue(ST_VOL);
				if(AppVolume)
					start_dac_playing(3);
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
*       _cbBk_Desktop
*
*  Function description:
*    Callback routine of desktop window
*/
static void _cbBk_Desktop(WM_MESSAGE * pMsg) {
  switch (pMsg->MsgId) {
		case WM_PAINT: {
			const char *tstr[]={"10", "20", "30", "40", "50", "60", "70", "80", "90"};
			// Draw Background Color & active color & borders // 
			GUI_SetBkColor(AZP_BACKGROUND_COLOR);
			GUI_Clear();
			GUI_SetColor(AZP_ACTIVE_AREA_COLOR);
			GUI_FillRoundedRect(1, SB_FULL_TOP_SIZE_Y+1, GLCD_X_SIZE-1, AZP_FAST_CURSOR_UPY-1, 2);
			GUI_SetColor(GUI_BLACK);	
			GUI_DrawRoundedRect(1, SB_FULL_TOP_SIZE_Y+1, GLCD_X_SIZE-1, AZP_FAST_CURSOR_UPY-1, 2);

			volatile uint16_t indx;
			for(indx=0 ; indx<AZP_FAST_NUMSTR_COUNT ; indx++) {
				// number strings // 
				GUI_SetFont(APP_24B_FONT);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetColor(GUI_BLACK);
				GUI_DispStringInRectWrap(tstr[indx], &(pPara->gRect[indx]), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				// inside of boxes //
				if((0x1<<indx) & pPara->fast_state) {
					if(3 > indx)
						GUI_SetColor(GUI_RED);
					else if((3 <= indx) && (5 >= indx))
						GUI_SetColor(GUI_YELLOW);
					else
						GUI_SetColor(GUI_GREEN);
					GUI_FillRoundedRect(pPara->gRect[indx].x0, AZP_FAST_BOXES_UPY, pPara->gRect[indx].x1, AZP_FAST_BOXES_UPY + AZP_FAST_BOX_SIZEY-2,2);
				}
				// Border of Boxes // 
				GUI_SetColor(GUI_BLACK);	
				GUI_DrawRoundedRect(pPara->gRect[indx].x0, AZP_FAST_BOXES_UPY, \
					pPara->gRect[indx].x1, AZP_FAST_BOXES_UPY + AZP_FAST_BOX_SIZEY-2,2); 
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


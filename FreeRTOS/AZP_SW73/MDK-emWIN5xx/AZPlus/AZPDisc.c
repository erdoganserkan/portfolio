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
#include "AZPDisc.h"

// New type definitions // 
typedef struct PARA_s {
	uint8_t ready4pkey;
	uint8_t gauge_is_valid;
	uint8_t ScreenExit;
	uint8_t targetIDMax;
	uint8_t targetID;
	uint8_t Gauge, gauge_max, gauge_cnt, prev_gauge;
	uint32_t search_start_time;
	WM_HWIN	hMidWin;
	WM_CALLBACK * OldDesktopCallback;
	uint16_t diffY[AZP_DISC_FS_PARTS_COUNT];
	uint8_t tIDs[AZP_DISC_FS_PARTS_COUNT];
	uint16_t active_indx;
	GUI_RECT gRect;
	char str[16];
} PARA;
static uint16_t const midY = (AZP_DISC_FS_SIZEY)/2;
static uint16_t rangeY = (AZP_DISC_FS_SIZEY-6)/2;

// Function Prototypes //
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void AZPDisc_on_comm_timeout(void *msgp);
static void AZPDisc_on_msg_ready(void);

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 
extern uint8_t expo_gauges[UMD_GAUGE_MAX+1];	// GUIDemo_Start.c // 
extern uint8_t active_page;	// Defined in GUIDEMO_Start.c //
static PARA* pPara = NULL;
static uint8_t new_page;
extern uint8_t SearchState;		// defined in GUIDEMO_Start.c // 

static void clear_scope(void) {
	// clear pointsY buffer and screen // 
	volatile uint16_t indx;
	for(indx=(AZP_DISC_FS_PARTS_COUNT)-1;;indx--) {
		pPara->diffY[indx] = 0;
		if(0 == indx) break;
	}
	pPara->active_indx = 0;
}

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
			// Draw Active Area Borders // 
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedRect(4, 4, AZP_DISC_FS_RIGHT_DOWNX-4, AZP_DISC_FS_SIZEY-4, 2);
			GUI_DrawRoundedRect(3, 3, AZP_DISC_FS_RIGHT_DOWNX-3, AZP_DISC_FS_SIZEY-3, 2);
			// Active colored area 
			GUI_SetColor(AZP_ACTIVE_AREA_COLOR);
			GUI_FillRoundedRect(5, 5, AZP_DISC_FS_RIGHT_DOWNX-5, AZP_DISC_FS_SIZEY-5, 2); 
			// FS middle line, filled scope // 
			GUI_SetColor(GUI_BLACK);
			GUI_DrawHLine(midY, 2, AZP_DISC_FS_RIGHT_DOWNX-2);

			GUI_COLOR const tid_color[TARGET_ID_COUNT] = {GUI_YELLOW, GUI_BLUE, GUI_RED, GUI_GREEN, GUI_ORANGE, GUI_BLUE};
			{
				uint16_t startX;
				volatile uint16_t indx;
				uint16_t buf_indx = pPara->active_indx;
				for(indx=0 ; indx<AZP_DISC_FS_PARTS_COUNT ; indx++, buf_indx++) {
					if(buf_indx >= AZP_DISC_FS_PARTS_COUNT)
						buf_indx -= AZP_DISC_FS_PARTS_COUNT;
					startX = AZP_DISC_FS_LEFT_UPX + 4 + (AZP_DISC_FS_PARTS_WIDTH * indx);
					if(pPara->tIDs[buf_indx] < TARGET_ID_COUNT) {
						GUI_SetColor(tid_color[pPara->tIDs[buf_indx]]); 
						#if(1 == AZP_DISC_FS_PARTS_WIDTH)
							GUI_DrawVLine(startX, midY-pPara->diffY[buf_indx], midY-1);
						#else
							GUI_FillRect(startX, midY-pPara->diffY[buf_indx], startX + AZP_DISC_FS_PARTS_WIDTH, midY-1);
						#endif
						uint16_t diffS = (pPara->diffY[buf_indx])/3 ;
						if(diffS) {
							#if(1 == AZP_DISC_FS_PARTS_WIDTH)
								GUI_DrawVLine(startX, midY+1, midY+diffS);
							#else
								GUI_FillRect(startX, midY+1, startX + AZP_DISC_FS_PARTS_WIDTH, midY+diffS);
							#endif
						}
					}
				}
			} 
			// Draw target id string // 
			if((pPara->Gauge) && (TARGET_NOTARGET != pPara->targetID) && (TARGET_ID_COUNT > pPara->targetID)){  
				GUI_RECT const gr_tid_str = {AZP_DISC_TID_STR_LEFT_UPX, AZP_DISC_TID_STR_LEFT_UPY-5, \
					AZP_DISC_TID_STR_RIGHT_DOWNX, AZP_DISC_TID_STR_RIGHT_DOWNY-10}; 
				uint16_t const tid_str_indxes[TARGET_ID_COUNT] = \
					{0, STR_CAVITY_INDX, STR_FERROS_INDX, STR_NONFERROS_INDX, STR_GOLD_INDX, STR_MINERAL_INDX};
				uint16_t lang = APP_GetValue(ST_LANG);
				pPara->gRect = gr_tid_str;
				char const *tstr = GetString(tid_str_indxes[pPara->targetID]);
				uint32_t str_pixel_len = strlen(tstr) * 25;
				if(LANG_RS == lang)
					str_pixel_len /= 2;	// strlen of russian strings are received more than real // 
				pPara->gRect.x0 = (GLCD_X_SIZE/2) - (str_pixel_len/2) - 5;
				pPara->gRect.x1 = (GLCD_X_SIZE/2) + (str_pixel_len/2) + 5;

				// Draw TID string BLACK borders and Filled Bacground Frame // 
				{
					GUI_SetColor(GUI_BLACK);
					GUI_DrawRoundedRect(pPara->gRect.x0-1, pPara->gRect.y0-1, pPara->gRect.x1+1, pPara->gRect.y1+1, 2);
					GUI_SetColor(tid_color[pPara->targetID]);
					GUI_FillRoundedRect(pPara->gRect.x0, pPara->gRect.y0, pPara->gRect.x1, pPara->gRect.y1, 2);
				}
				GUI_SetColor(GUI_BLACK);
				GUI_SetFont(APP_32B_FONT);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_DispStringInRectWrap(tstr, &(pPara->gRect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
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
				msg.data.search_type = METAL_ANALYSIS_TYPE;
				UARTSend((uint8_t *)&msg, \
					UMD_CMD_TIMEOUT_SEARCH_CMD_MS*2, AZPDisc_on_comm_timeout);
				BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_BRIGHT));	
			}
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY:
			TRACEM("MASearch KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_AZP_ENTER_EVENT: {
						// send RESET command to analog mcu // 
						{
							pPara->gauge_max = 0;
							pPara->gauge_cnt = 0;
							UmdPkt_Type msg;
							msg.cmd = IND_ANALOG_RESET;
							msg.length = 2; // CMD(U8) + LENGTH(U8) // 
							UARTSend((uint8_t *)&msg, 0, NULL);
						}
						clear_scope();
						WM_InvalidateWindow(pPara->hMidWin);
						WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
						pPara->ready4pkey = FALSE;
					} break;
					case KEY_AZP_GROUND_EVENT: {
						SearchState = SEARCH_STOP_REQUESTED;
						pPara->ScreenExit = SCR_EXIT_REQUESTED;
						new_page = AZP_MENU_SCR;
						uint16_t active_sb_pos = AZP_SB_POS_GB;
						SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPDisc_on_comm_timeout);
					}
					break;
					case KEY_AZP_DISC_EVENT: {
						SearchState = SEARCH_STOP_REQUESTED;
						pPara->ScreenExit = SCR_EXIT_REQUESTED;
						new_page = AZP_ALL_METAL_SCR;
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPDisc_on_comm_timeout);
					}
					break;
					case KEY_AZP_FAST_EVENT: {
						SearchState = SEARCH_STOP_REQUESTED;
						pPara->ScreenExit = SCR_EXIT_REQUESTED;
						new_page = AZP_FAST_SCR;
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPDisc_on_comm_timeout);
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
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, AZPDisc_on_comm_timeout);
					}
					break;
					default:	
						key_valid = FALSE;
						break;
				}
				if(SCR_EXIT_REQUESTED == pPara->ScreenExit)
					BSP_PWMSet(0, BSP_PWM_LCD, 0);	
				if(SEARCH_STOP_REQUESTED == SearchState) {
					if(TRUE == is_dac_playing()) {
						WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
						WAVE_Generator_stop(TRUE, TRUE, TRUE);
					}
					clear_scope();
					WM_InvalidateWindow(pPara->hMidWin);
					pPara->ready4pkey = FALSE;
				}
				if(TRUE == key_valid) {
					WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
					uint32_t freq_backup = get_DAC_DMA_Update_Freq();
					DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
					start_dac_audio(BUTTON_OK_SOUND, TRUE);
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
uint8_t AZP_Disc(void) 
{
	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = FALSE;
	pPara->OldDesktopCallback = NULL;
	pPara->ScreenExit = SCR_RUNNING;
	
	new_page = AZP_DISC_SCR;
	pPara->targetID = TARGET_NOTARGET;
	SearchState = SEARCH_IDLE;
	pPara->search_start_time = 0;
	pPara->Gauge = 0;
	pPara->gauge_is_valid = FALSE;
	pPara->targetIDMax = TARGET_ID_MAX;	// All metal screen show full targets //
	volatile uint16_t indx;
	for(indx=(AZP_DISC_FS_PARTS_COUNT)-1;;indx--) {
		pPara->diffY[indx] = 0;
		if(0 == indx) break;
	}
	pPara->active_indx = 0;
	pPara->gauge_cnt = 0;
	pPara->gauge_max = 0;
	pPara->prev_gauge = 0;
	
	WM_MULTIBUF_Enable(1);
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();	// Is This necessary? We are already drawing a picture that fully covering LCD // 
	GUI_ClearKeyBuffer();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);	// Force to redraw Desktop Window // 

	/* CREATE Windows and Do Supplementary Operations */
	pPara->hMidWin = WM_CreateWindowAsChild(AZP_DISC_FS_LEFT_UPX, AZP_DISC_FS_LEFT_UPY, \
		AZP_DISC_FS_RIGHT_DOWNX - AZP_DISC_FS_LEFT_UPX, AZP_DISC_FS_RIGHT_DOWNY - AZP_DISC_FS_LEFT_UPY, 
		WM_HBKWIN, WM_CF_SHOW, _cbMidDraw, sizeof(pPara));
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
				AZPDisc_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}
	
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();	// Is This necessary? We are already drawing a picture that fully covering LCD // 
	//if((SEARCH_START_FAILED == SearchState) || (SEARCH_STOP_FAILED == SearchState))
		//vTaskDelay(2000 * (configTICK_RATE_HZ/1000));

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

	free(pPara);
	pPara=NULL;
	return new_page;
}

// Function to be called when timeout occured after a command send but response not received // 
static void AZPDisc_on_comm_timeout(void *last_msgp)
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
		case CMD_START_SEARCH:
		#if(0)
			// ReEnter to this screen // 
			SearchState = SEARCH_START_FAILED;
			new_page = AZP_DISC_SCR;
			pPara->ScreenExit = SCR_EXIT_CONFIRMED;
		#else
			pPara->search_start_time = GUI_X_GetTime();
			SearchState = SEARCH_STARTED;
			uint16_t AppVolume = APP_GetValue(ST_VOL);
			if(AppVolume)
				start_dac_playing(1);
		#endif
		break;
		default:
			ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
				((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
					((uint8_t *)last_msgp)[3]);
			break;
	}
}

// Function to be called when a pkt received from Detector // 
static void AZPDisc_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case IND_GET_GAUGE:	
				if((SEARCH_STARTED != SearchState))
					break;
				if(FALSE == pPara->gauge_is_valid) {
					if(GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS <= (GUI_X_GetTime() - pPara->search_start_time))
						pPara->gauge_is_valid = TRUE;
					else
						break;	// Ignore GAUGE for a while at startuup // 
				}
				if(SCR_RUNNING == pPara->ScreenExit){
					uint8_t TGauge = msg_ptr->data.gauge;
					if(UMD_GAUGE_MAX < TGauge)
						TGauge = UMD_GAUGE_MAX;	// Truncation to maximum value for safety // 
					#if(1)
						if(TGauge > pPara->gauge_max)
							pPara->gauge_max = TGauge;
						#if(1 == AZP_DISC_FS_PARTS_WIDTH)
							if(3 == (++pPara->gauge_cnt)) {
						#elif(3 == AZP_ALLM_FS_PARTS_WIDTH)
							if(6 == (++pPara->gauge_cnt)) {
						#else
							if(10 == (++pPara->gauge_cnt)) {
						#endif
							pPara->gauge_cnt = 0;
							TGauge = pPara->gauge_max;
							pPara->gauge_max = 0;
						} else
							break;
					#endif
					{
						pPara->Gauge = TGauge;
						// Calculate new posY and store it into circular buffer with targetID // 
						{
							// convert raw gauges to normalized gauges ONLY FOR SCOPE object // 
							extern uint8_t expo_gauges[UMD_GAUGE_MAX+1];
							uint8_t TGauge_expo = expo_gauges[TGauge];

							uint16_t diffL = (TGauge_expo * (uint32_t)rangeY) / UMD_GAUGE_MAX;
							pPara->diffY[pPara->active_indx] = diffL;
							pPara->tIDs[pPara->active_indx] = pPara->targetID;
							if(AZP_DISC_FS_PARTS_COUNT == (++pPara->active_indx))
								pPara->active_indx = 0;
						}	
						{
							uint8_t SGauge = TGauge;
							if((TARGET_MINERAL == pPara->targetID) && (pPara->prev_gauge > 10) && (0 == TGauge))
								SGauge = pPara->prev_gauge;
							if(TRUE == is_dac_playing())
								WAVE_Update_FreqAmp_Gauge((uint16_t)SGauge * GAUGE_FRACTUATION);	// Update Sound Frequency // 
						}
						WM_InvalidateWindow(pPara->hMidWin);
					}
					pPara->prev_gauge = TGauge;
				}
				break;
			case IND_GET_TARGET_ID: 
				if((SEARCH_STARTED != SearchState))
					break;
				if (SCR_RUNNING == pPara->ScreenExit) {				
					uint8_t TempID = msg_ptr->data.target_id;
					if(pPara->targetIDMax < TempID)
						TempID = pPara->targetIDMax;	// Truncation to maximum value for safety // 
					pPara->targetID = TempID;
					WAVE_SetFreq_FTID(pPara->targetID);
				}
				break;
			case RSP_STOP_SEARCH:
				StopCommTimeout();
				if(SEARCH_STOP_REQUESTED != SearchState)
					break;
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					/* CMD stop implemented by Detector */
					SearchState = SEARCH_IDLE;
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
					start_dac_playing(1);
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
			char const *str;
			GUI_RECT gRect = {AZP_SCR_STR_UPX, AZP_SCR_STR_UPY, AZP_SCR_STR_DOWNX, AZP_SCR_STR_DOWNY};
			pPara->gRect = gRect;
			// Draw Background Color 
			GUI_SetBkColor(AZP_BACKGROUND_COLOR);
			GUI_Clear();
			// Draw strng border and fill string area //
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedRect(AZP_SCR_STR_UPX+1, AZP_SCR_STR_UPY+1, AZP_SCR_STR_DOWNX-1, AZP_SCR_STR_DOWNY-1, 2);
			GUI_DrawRoundedRect(AZP_SCR_STR_UPX+2, AZP_SCR_STR_UPY+2, AZP_SCR_STR_DOWNX-2, AZP_SCR_STR_DOWNY-2, 2);
			GUI_SetColor(AZP_ACTIVE_AREA_COLOR);
			GUI_FillRoundedRect(AZP_SCR_STR_UPX+3, AZP_SCR_STR_UPY+3, AZP_SCR_STR_DOWNX-3, AZP_SCR_STR_DOWNY-3, 2);
			
			// Locate Screen Name String //
			GUI_SetTextMode(GUI_TM_TRANS);
			uint16_t lang = APP_GetValue(ST_LANG);
			if((LANG_TR == lang) || (LANG_AR == lang) ||(LANG_PR == lang))
				GUI_SetFont(APP_32B_FONT);
			else if(LANG_RS == lang)		
				GUI_SetFont(APP_19B_FONT);
			else
				GUI_SetFont(APP_24B_FONT);
			GUI_SetColor(GUI_BLACK);
			str = GetString(STR_AZP_DISC);
			GUI_DispStringInRectWrap(str, &(pPara->gRect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


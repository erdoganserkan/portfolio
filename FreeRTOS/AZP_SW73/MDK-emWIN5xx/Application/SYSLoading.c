#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include <task.h>
#include <queue.h>
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
#include "UartInt.h"
#include "APlay.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "Analog.h"
#include "SYSSettings.h"
#include "ATRadar.h"
#include "APlay.h"
#include "Dac.h"
#include "SYSLoading.h"

typedef struct {
	uint8_t dev_type;
	uint8_t ScreenExit;
	uint8_t first_mid_update;
	uint8_t first_desktop_update;
	uint8_t RadarIndx;
	uint8_t LBarLevel_prev;
	uint8_t LBarLevel_new;
	uint8_t RBarLevel_prev;
	uint8_t RBarLevel_new;
	uint8_t zigzag_radar;
	uint16_t total_steps;
	WM_HTIMER hTimerAnim;
	WM_HWIN hMidWin;
	WM_HWIN hLeftBarWin;
	WM_HWIN hRightBarWin;
	WM_CALLBACK *OldDesktopCallback;
	GUI_RECT gRects[3];
	uint8_t icons_state_event[SYS_LOAD_ICON_COUNT];
	uint8_t icons_state_real[SYS_LOAD_ICON_COUNT];
	uint8_t icons_state_timer[SYS_LOAD_ICON_COUNT];
	char str_buf1[16];
	char str_buf2[16];
} PARA;

static uint8_t new_page;
static PARA *pPara = NULL;
extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void _cbDraw_MidWin(WM_MESSAGE * pMsg);
static void set_left_bar(uint8_t new_level);
static void set_right_bar(uint8_t new_level);

static void HandShake_on_msg_ready(void);
static void Handshake_comm_fail(void *msgp);
static uint32_t hand_shake_start_time = 0;

/*********************************************************************
*
*       _cbDraw_LeftBar
*
*  Function description:
*    callback function for left bar window 
*/
static void _cbDraw_LeftBar(WM_MESSAGE * pMsg)
{
	switch (pMsg->MsgId) {
		case WM_PAINT:  
			set_left_bar(pPara->LBarLevel_new);
			pPara->LBarLevel_prev = pPara->LBarLevel_new;
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*********************************************************************
*
*       _cbDraw_LeftBar
*
*  Function description:
*    callback function for right bar window 
*/
static void _cbDraw_RightBar(WM_MESSAGE * pMsg)
{
	switch (pMsg->MsgId) {
		case WM_PAINT: 
			set_right_bar(pPara->RBarLevel_new);
			pPara->RBarLevel_prev = pPara->RBarLevel_new;
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

/*********************************************************************
*
*       _cbDraw_MidWin
*
*  Function description:
*    callback function for middle window 
*/
static void _cbDraw_MidWin(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	hWin = pMsg->hWin;

	switch (pMsg->MsgId) {
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_SYS_LOAD_ANIM: {
					if(APP_IS_FIELD_SCANNER == (pPara->dev_type)) {	// Increment RADAR pictures index and invalidate window // 
						if(FALSE == pPara->zigzag_radar) {
							if(AT_RADAR_PICs_MAX != pPara->RadarIndx) 
								pPara->RadarIndx++;
							else 
								pPara->RadarIndx = AT_RADAR_PICs_MIN;
						} else {
							if(0 == pPara->RadarIndx) 
								pPara->RadarIndx = 1;
							else 
								pPara->RadarIndx = 0;
						}
					} else if(APP_IS_DETECTOR == (pPara->dev_type)) {
						if(FALSE == pPara->zigzag_radar) {
							if(AT_RADAR_PICs_MIN != pPara->RadarIndx) 
								pPara->RadarIndx--;
							else 
								pPara->RadarIndx = AT_RADAR_PICs_MAX;
						} else {
							if(9 == pPara->RadarIndx) 
								pPara->RadarIndx = 10;
							else 
								pPara->RadarIndx = 9;
						}
					}
					if(pPara->total_steps < SYS_LOAD_TOTAL_STEPs)
						pPara->total_steps++;
					// Update left bar (overal process control) according to total_steps // 
					pPara->LBarLevel_new = (pPara->total_steps >= SYS_LOAD_TOTAL_STEPs) ? \
						(100) : ((uint32_t)(pPara->total_steps)*100U)/SYS_LOAD_TOTAL_STEPs;
					if(pPara->LBarLevel_new != pPara->LBarLevel_prev )
						WM_InvalidateWindow(pPara->hLeftBarWin);
					// Update right bar(inner steps control) according to total_steps // 
					if(100 != pPara->LBarLevel_new) {
						pPara->RBarLevel_new = (((uint16_t)(pPara->total_steps % SYS_LOAD_ICON_STEP))*100U)/SYS_LOAD_ICON_STEP;
						if((0 != pPara->total_steps) && (0 == pPara->RBarLevel_new))
							pPara->RBarLevel_new = 100;
					}
					else 
						pPara->RBarLevel_new = 100;
					if(pPara->RBarLevel_new != pPara->RBarLevel_prev)
						WM_InvalidateWindow(pPara->hRightBarWin);
					// Update middle window for radar animation and/or icon update // 
					WM_InvalidateWindow(hWin);
					// Check SCREEN EXIT action // 
					{
						uint32_t diff_ms = GUI_X_GetTime() - hand_shake_start_time;
						if(diff_ms >= SYS_LOAD_TOTAL_LENGHT_MS) {
							if(FALSE == pPara->zigzag_radar)
								INFOM("SYS LOAD TOTAL LENGTH(%u ms) PASSED\n", SYS_LOAD_TOTAL_LENGHT_MS);
							if(APP_IS_FIELD_SCANNER == (pPara->dev_type)) 
								pPara->ScreenExit = TRUE;	// If dev is FS and animation time passes, exit screen //
							else {
								// If dev is DETECTOR and animation time passes, 
									// wait until timeout or new detector responses //
								#if(1)
								if((DETECTOR_TOTAL_INIT_COMPLETED != AppStatus.detector_hs_state) && \
									(DETECTOR_HS_FAILED != AppStatus.detector_hs_state)) {
								#else
								// this code is ONLY for testing, it waits on sys-loading \
									// until init-done timeout occurs even command timeout occurs before // 
								if(DETECTOR_TOTAL_INIT_COMPLETED != AppStatus.detector_hs_state) {
								#endif
									pPara->zigzag_radar = TRUE;
									if(diff_ms >= DETECTOR_INIT_TOTAL_TIMEOUT_MS) {
										ERRM("Handshake TOTAL-DETECTOR-INIT TIMEOUT Occured\n");
										AppStatus.detector_hs_state = DETECTOR_TOTAL_INIT_FAILED;
										pPara->ScreenExit = TRUE;
									}
								} else {
									pPara->ScreenExit = TRUE;
								}
							}
						}
					}
					// Restart timer for next time //  
					if(TRUE != pPara->ScreenExit)
						WM_RestartTimer(pMsg->Data.v, SYS_LOAD_ANIM_MS);
				}
				break;
				default:	// Ignore silently // 
					break;
			}
		}
		break;
		case WM_PAINT: { 
			uint16_t const icon_pos[SYS_LOAD_ICON_COUNT][2] = {
				{SYS_LOAD_ICON1_LEFT_X, SYS_LOAD_ICON1_LEFT_Y},
				{SYS_LOAD_ICON2_LEFT_X, SYS_LOAD_ICON2_LEFT_Y},
				{SYS_LOAD_ICON3_LEFT_X, SYS_LOAD_ICON3_LEFT_Y},
				{SYS_LOAD_ICON4_LEFT_X, SYS_LOAD_ICON4_LEFT_Y},
				{SYS_LOAD_ICON8_LEFT_X, SYS_LOAD_ICON8_LEFT_Y},
				{SYS_LOAD_ICON7_LEFT_X, SYS_LOAD_ICON7_LEFT_Y},
				{SYS_LOAD_ICON6_LEFT_X, SYS_LOAD_ICON6_LEFT_Y},
				{SYS_LOAD_ICON5_LEFT_X, SYS_LOAD_ICON5_LEFT_Y}
			};
#if(1)
			if(APP_IS_FIELD_SCANNER == (pPara->dev_type)) {
				if(0 != SBResources[SYS_LOADING_PICs][SYS_LOAD_MIDDLE_WIN_BACK_BLUE].hMemHandle) 
					GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOAD_MIDDLE_WIN_BACK_BLUE].hMemHandle, \
						SYS_LOAD_MID_WIN_LEFT_X, SYS_LOAD_MID_WIN_LEFT_Y);
			} else if(APP_IS_DETECTOR == (pPara->dev_type)) {
				if(0 != SBResources[SYS_LOADING_PICs][SYS_LOAD_MIDDLE_WIN_BACK_RED].hMemHandle) 
					GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOAD_MIDDLE_WIN_BACK_RED].hMemHandle, \
						SYS_LOAD_MID_WIN_LEFT_X, SYS_LOAD_MID_WIN_LEFT_Y);
			}
#else
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
#endif
			// Locate RADAR picture // 
			if(APP_IS_FIELD_SCANNER == (pPara->dev_type)) { // Update new sonar image // 
				if(0 != SBResources[AT_RADAR_BLUE_PICs][pPara->RadarIndx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_RADAR_BLUE_PICs][pPara->RadarIndx].hMemHandle, RADAR_PICs_UPX, RADAR_PICs_UPY);
				}
			} else if(APP_IS_DETECTOR == (pPara->dev_type)){
				if(0 != SBResources[AT_RADAR_RED_PICs][pPara->RadarIndx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_RADAR_RED_PICs][pPara->RadarIndx].hMemHandle, RADAR_PICs_UPX, RADAR_PICs_UPY);
				}
			}
			// Locate RADAR Truncator picture // 
			//if((0 != SBResources[SYS_LOADING_PICs][SYS_LOADING_RADAR_TRUNCATOR].hMemHandle)) {
				//GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOADING_RADAR_TRUNCATOR].hMemHandle, RADAR_PICs_UPX, RADAR_PICs_UPY);
			//}
			// Update left icons according to total_steps & correspoding status array members // 
			for(volatile uint8_t indx=0 ; indx<SYS_LOAD_ICON_COUNT ; indx++) {
				// Update icons pics timer positions //
				if(pPara->total_steps >= (SYS_LOAD_ICON_STEP*indx))
					pPara->icons_state_timer[indx] = TRUE;
				// Check for the icons that whose both of timer and event states are TRUE // 
				if((TRUE == pPara->icons_state_timer[indx]) && (TRUE == pPara->icons_state_event[indx])) 
					pPara->icons_state_real[indx] = TRUE;
				if(((SYS_LOAD_ICON_COUNT-1) != indx) || (FALSE == pPara->zigzag_radar)) {
					if((TRUE == pPara->icons_state_real[indx]) && (0 != SBResources[SYS_LOADING_PICs][SYS_LOADING_ICON1 + indx].hMemHandle)) {
						GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOADING_ICON1 + indx].hMemHandle, icon_pos[indx][0], icon_pos[indx][1]);
					}

				} else {
					// If zigzag radar animation is started DRAW LAST ICON AS BLINK ANIMATION // 
					if((0x1 & pPara->RadarIndx) && (0 != SBResources[SYS_LOADING_PICs][SYS_LOADING_ICON1 + indx].hMemHandle)) {
						GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOADING_ICON1 + indx].hMemHandle, icon_pos[indx][0], icon_pos[indx][1]);
					}
				}
			}
		}
		break;
		case WM_CREATE:
			// Create animation timer // 
			pPara->hTimerAnim = WM_CreateTimer(hWin, ID_TIMER_SYS_LOAD_ANIM, SYS_LOAD_ANIM_MS, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(pPara->hTimerAnim);
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

uint8_t SYS_Loading(void) 
{
	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
		while(STALLE_ON_ERR);
	pPara->dev_type = APP_GetValue(ST_DEV_TYPE);

	if(0 != InitGroupRes(SYS_LOADING_PICs, 0xFF))
		while(TODO_ON_ERR);
	if(APP_IS_FIELD_SCANNER == (pPara->dev_type)) {
		if(0 != InitGroupRes(AT_RADAR_BLUE_PICs, 0xFF))	
			while(TODO_ON_ERR);
	}
	else if(APP_IS_DETECTOR == (pPara->dev_type)) {
		if(0 != InitGroupRes(AT_RADAR_RED_PICs, 0xFF))	
			while(TODO_ON_ERR);
	}  	

	new_page = (APP_IS_DETECTOR == (pPara->dev_type)) ? RM_SCREEN : AT_MENU;
	pPara->ScreenExit = FALSE;
	pPara->OldDesktopCallback = NULL;
	pPara->hTimerAnim = 0;
	pPara->first_mid_update = TRUE;
	pPara->first_desktop_update = TRUE;
	pPara->total_steps = 0;
	pPara->RadarIndx = \
		(APP_IS_FIELD_SCANNER== (pPara->dev_type)) ? AT_RADAR_PICs_MIN : AT_RADAR_PICs_MAX;
	pPara->LBarLevel_new = pPara->RBarLevel_new = 0;
	pPara->LBarLevel_prev = pPara->RBarLevel_prev = 0;
	pPara->zigzag_radar = FALSE;
	{
		uint8_t default_events_state = FALSE;
		if(APP_IS_FIELD_SCANNER == (pPara->dev_type))
			default_events_state = TRUE;	// If dev is FS, suppose that all cmd already received from decetor //
		for(volatile uint8_t indx=0 ; indx<SYS_LOAD_ICON_COUNT ; indx++) 
			pPara->icons_state_event[indx] = default_events_state;
	}
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	//xLCDSize = LCD_GetXSize();
	//yLCDSize = LCD_GetYSize();
	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);
	
	// Create Left Bar updating window // 
	pPara->hLeftBarWin = \
		  WM_CreateWindowAsChild(SYS_LOAD_LBAR_LEFT_X, SYS_LOAD_LBAR_LEFT_Y, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_LeftBar, 0);
	
	// Create Left Bar updating window // 
	pPara->hRightBarWin = \
		  WM_CreateWindowAsChild(SYS_LOAD_RBAR_LEFT_X, SYS_LOAD_RBAR_LEFT_Y, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_RightBar, 0);

	// Create a rectange window on the middle of screen for sonar animation pictures // 
	pPara->hMidWin = \
		  WM_CreateWindowAsChild(SYS_LOAD_MID_WIN_LEFT_X, SYS_LOAD_MID_WIN_LEFT_Y, SYS_LOAD_MID_WIN_SIZE_X, SYS_LOAD_MID_WIN_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_MidWin, 0);
	WM_SetFocus(pPara->hMidWin);

	//SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
  	// First of all, be sure detector had enough time before handhaking // 
	{
		extern int detector_power_on_time_ms;	// Defined in GUIDEMO_Start.c // 
		int current_ms = GUI_X_GetTime();
		if((detector_power_on_time_ms + WAIT_BEFORE_HANDSHAKE_MS) > current_ms) {
			App_waitMS(detector_power_on_time_ms + WAIT_BEFORE_HANDSHAKE_MS - current_ms);
		}
  	}	
	// Start handshaking with sending first CMD message //
	{
		UmdPkt_Type send_msg;
		send_msg.cmd = CMD_SET_SENSITIVITY;
		send_msg.length = 3;
		send_msg.data.sensitivity = APP_GetValue(ST_SENS);
		UARTSend((uint8_t *)&send_msg, \
			UMD_CMD_TIMEOUT_SET_SENSE_MS, Handshake_comm_fail);
		pPara->icons_state_event[SYS_LOADING_SENSE_CMD_SEND_INDX] = TRUE;
		hand_shake_start_time = GUI_X_GetTime();
	}

	// WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
	// Modulator frequency is 3 times increased for GB screen, this will cause faster frequency switch on dac output // 
	// WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
	// WAVE_Update_FreqAmp_Gauge((uint16_t)10*GAUGE_FRACTUATION);	// Update Sound Frequency // 

	// Animation loop
	while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1())  {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else
				HandShake_on_msg_ready();
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}

	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
 	//SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(pPara->hMidWin);
	WM_DeleteWindow(pPara->hLeftBarWin);
	WM_DeleteWindow(pPara->hRightBarWin);

	free(pPara);
	pPara = NULL;
	
	return new_page;
}

// Sequentially send initialization commands to dedector // 
static void HandShake_on_msg_ready(void)
{
	UmdPkt_Type send_msg;
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case RSP_SET_SENSITIVITY:
			{
				StopCommTimeout();
				pPara->icons_state_event[SYS_LOADING_SENSE_RSP_RECEIVED_INDX] = TRUE;
				// Set ground id level //
				uint16_t gid = APP_GetValue(ST_GROUND_ID);
				send_msg.cmd = CMD_SET_GROUND_ID;
				send_msg.length = 4;
				*(((uint8_t *)&send_msg) + 2) = ((gid>>8) & 0xFF);	// MSB First //
				*(((uint8_t *)&send_msg) + 3) = (gid & 0xFF);	// LSB Second // 
				UARTSend((uint8_t *)&send_msg, \
					UMD_CMD_TIMEOUT_SET_GID_MS, Handshake_comm_fail);
				pPara->icons_state_event[SYS_LOADING_GID_CMD_SEND_INDX] = TRUE;
			}
			break;
			case RSP_SET_GROUND_ID:
				StopCommTimeout();
				pPara->icons_state_event[SYS_LOADING_GID_RSP_RECEIVED_INDX] = TRUE;
				// Set ferros state level //
				send_msg.cmd = CMD_SET_FERROS_STATE;
				send_msg.length = 3;
				send_msg.data.ferros_state = APP_GetValue(ST_FERROs);
				UARTSend((uint8_t *)&send_msg, \
					UMD_CMD_TIMEOUT_SET_FERRO_MS, Handshake_comm_fail);
				pPara->icons_state_event[SYS_LOADING_FERRO_CMD_SEND_INDX] = TRUE;
				break;
			case RSP_SET_FERROS_STATE:
				StopCommTimeout();	
				pPara->icons_state_event[SYS_LOADING_FERRO_RSP_RECEIVED_INDX] = TRUE;
				AppStatus.detector_hs_state = DETECTOR_FIRST_CMDs_OK;
				// There is nothing TODO; We will wait for dedector to set REF, A, B, C clocks // 
				break;
			case CMD_SET_A_CLOCK_DELAY:
			case CMD_SET_B_CLOCK_DELAY: 
			case CMD_SET_C_CLOCK_DELAY: 
			case CMD_SET_REF_CLOCK_FREQ:
				pPara->icons_state_event[SYS_LOADING_CLOCK_SET_CMDs_RECEIVED_INDX] = TRUE;
				Process_Clk_gen_Msg(gui_msg);
				if(CMD_SET_C_CLOCK_DELAY == msg_ptr->cmd) {
					// We have completed the Dedector Initial Parameter Setting // 
					AppStatus.detector_hs_state = DETECTOR_HS_COMPLETED;
				}
				break;
			case CMD_SET_SENSITIVITY: { 	// Detector has set new SENSITIVITY value and send back to me // 
					DEBUGM("NEW SENSIVITY from DETECTOR (%u)\n", msg_ptr->data.sensitivity);
					if(SENSITIVITY_MAX < msg_ptr->data.sensitivity) {
						msg_ptr->data.sensitivity = SENSITIVITY_MAX;
						ERRM("NEW Sensitivity(%u) MORE than max(%u), TRUNCATING\n", \
							msg_ptr->data.sensitivity, SENSITIVITY_MAX);
					} else if(SENSITIVITY_MIN > msg_ptr->data.sensitivity) {
						msg_ptr->data.sensitivity = SENSITIVITY_MIN;
						ERRM("NEW Sensitivity(%u) LESS than max(%u), INCREASING\n", \
							msg_ptr->data.sensitivity, SENSITIVITY_MIN);
					}
					{
						uint8_t div = msg_ptr->data.sensitivity/5;
						uint8_t res = msg_ptr->data.sensitivity - (div * 5);
						if(0 != res) {
							if(res >= 2)
								msg_ptr->data.sensitivity = 5 * (div+1);
							else
								msg_ptr->data.sensitivity = 5* div;
						}	
 					}
					// save new sentivity value in non-volaitle memory // 
					APP_SetVal(ST_SENS, msg_ptr->data.sensitivity, TRUE);
					// send response packet // 
					msg_ptr->cmd = RSP_SET_SENSITIVITY;	
					msg_ptr->length = 3;
					msg_ptr->data.cmd_status = CMD_DONE;
					UARTSend((uint8_t *)msg_ptr, 0, NULL);	// This is our response and so no TIMEOUT detection is required // 
					// Set the corresponding event state to TRUE // 
					pPara->icons_state_event[SYS_LOADING_INIT_DONE_RECEIVED_INDX] = TRUE;
				}
				break;
			case IND_DEDECTOR_IS_READY:
				AppStatus.detector_hs_state = DETECTOR_TOTAL_INIT_COMPLETED;
				INFOM("DETECTOR INIT DONE received\n");
				break;
			default:
				break;	// Ignore silently other cmd & rsp // 
		}
	}
}

static void Handshake_comm_fail(void *msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)msgp;
	ERRM("CMD(0x%02X) TIMEOUT OCCURED\n", msg->cmd);
	switch(msg->cmd) {
		case CMD_SET_SENSITIVITY:
		case CMD_SET_GROUND_ID:
		case CMD_SET_FERROS_STATE:
			// We have FAILED	the Dedector Initial Parameter Setting // 
			AppStatus.detector_hs_state = DETECTOR_HS_FAILED;
			break;
		default:
			ERRM("UNEXPECTED CMD TIMEOUT(0x%02X)\n", msg->cmd);
			break;
	}
}

static void set_left_bar(uint8_t new_level) {
	// Locate Vertical Gradiented rounded rectangle // 
	uint16_t ydiff = ((uint32_t)SYS_LOAD_BAR_SIZE_Y * new_level)/100;
	if(100 < new_level)
		ydiff = SYS_LOAD_BAR_SIZE_Y;
	// 1- Clear BAR Location // 
	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, SYS_LOAD_RECTANGLE_ROUND);
	// 2- Draw Covering rectangle // 
	//GUI_SetColor(GUI_LIGHTRED);
	//GUI_DrawRoundedRect(2, 2, SYS_LOAD_BAR_SIZE_X - 2, SYS_LOAD_BAR_SIZE_Y - 2, SYS_LOAD_RECTANGLE_ROUND);
	// 3- Draw progress bar as gradiented rectangle // 
	if(0 != ydiff)
		GUI_DrawGradientRoundedV(0, 0, SYS_LOAD_BAR_SIZE_X, ydiff, SYS_LOAD_RECTANGLE_ROUND, \
			(APP_IS_FIELD_SCANNER == (pPara->dev_type)) ? GUI_DARKBLUE : GUI_DARKRED, GUI_LIGHTGREEN);	
	else {
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, SYS_LOAD_RECTANGLE_ROUND);	
	}
	// 4- Display total-level as rotated string on Gradient // 
	{
		memset(pPara->str_buf1, 0, sizeof(pPara->str_buf1));
		snprintf(pPara->str_buf1, sizeof(pPara->str_buf1)-1, "%%%u", new_level);
		// snprintf() has failed to draw 7,8,9 numbers and so convesion done manually // 
		// pPara->str_buf1[0]='%';
		// pPara->str_buf1[1] = '0' + new_level/10;
		// pPara->str_buf1[2] = '0' + new_level%10;
		// pPara->str_buf1[3] = '\0';
		GUI_RECT gx = {0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y};
		pPara->gRects[1] = gx;
		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(APP_32B_FONT);
		GUI_SetColor(GUI_WHITE);
		GUI_DispStringInRectEx(pPara->str_buf1, &(pPara->gRects[1]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
			sizeof(pPara->str_buf1)-1, GUI_ROTATE_CCW);
	}
}

static void set_right_bar(uint8_t new_level) {
	// Locate Vertical Gradiented rounded rectangle // 
	uint16_t ydiff = ((uint32_t)SYS_LOAD_BAR_SIZE_Y * (uint32_t)new_level/100);
	if(100 < new_level)
		ydiff = SYS_LOAD_BAR_SIZE_Y;
	// 1- Clear BAR Location // 
	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, SYS_LOAD_RECTANGLE_ROUND);
	// 2- Draw Covering rectangle // 
	//GUI_SetColor(GUI_LIGHTRED);
	//GUI_DrawRoundedRect(2, 2, SYS_LOAD_BAR_SIZE_X - 2, SYS_LOAD_BAR_SIZE_Y - 2, SYS_LOAD_RECTANGLE_ROUND);
	// 3- Draw progress bar as gradiented rectangle // 
	if(0 != ydiff)
		GUI_DrawGradientRoundedV(0, 0, SYS_LOAD_BAR_SIZE_X, ydiff, SYS_LOAD_RECTANGLE_ROUND, GUI_LIGHTGREEN, \
			(APP_IS_FIELD_SCANNER == (pPara->dev_type)) ? GUI_DARKBLUE : GUI_DARKRED);	
	else {
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y, SYS_LOAD_RECTANGLE_ROUND);	
	}
		
	// 4- Display inner-level as rotated string on Gradient // 
	{
		memset(pPara->str_buf2, 0, sizeof(pPara->str_buf2));
		snprintf(pPara->str_buf2, sizeof(pPara->str_buf2)-1, "%%%u", new_level);
		// snprintf() has failed to draw 7,8,9 numbers and so convesion done manually // 
		// pPara->str_buf2[0]='%';
		// pPara->str_buf2[1] = '0' + new_level/10;
		// pPara->str_buf2[2] = '0' + new_level%10;
		// pPara->str_buf2[3] = '\0';
		GUI_RECT gx = {0, 0, SYS_LOAD_BAR_SIZE_X, SYS_LOAD_BAR_SIZE_Y};
		pPara->gRects[2] = gx;
		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_SetFont(APP_32B_FONT);
		GUI_SetColor(GUI_WHITE);
		GUI_DispStringInRectEx(pPara->str_buf2, &(pPara->gRects[2]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
			sizeof(pPara->str_buf2)-1, GUI_ROTATE_CCW);
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
			if(TRUE == pPara->first_desktop_update) {
				pPara->first_desktop_update = FALSE;
				if(0 != SBResources[SYS_LOADING_PICs][SYS_LOADING_BACK].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[SYS_LOADING_PICs][SYS_LOADING_BACK].hMemHandle, 0, 0);
				}
				// Locate screen string // 
				{
					GUI_RECT gx = {SYS_LOAD_STR_LEFT_X, SYS_LOAD_STR_LEFT_Y, SYS_LOAD_STR_RIGHT_X, SYS_LOAD_STR_RIGHT_Y};
					pPara->gRects[0] = gx;
					GUI_SetTextMode(GUI_TM_TRANS);
					GUI_SetFont(APP_24B_FONT);
					GUI_SetColor(GUI_BLACK);
					GUI_DispStringInRectWrap(GetString(STR_LOADING_PLEASE_WAIT), &(pPara->gRects[0]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
						GUI_WRAPMODE_WORD);
				}
			} else {
				while(STALLE_ON_ERR);
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

/*************************** End of file ****************************/


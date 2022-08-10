#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include <FreeRTOS.h>
#include <Queue.h>
#include <event_groups.h>
#include <task.h>
#include <GUIDEMO.h>
#include <DIALOG.h>
#include <BSP.h>
#include "AppCommon.h" 
#include "UMDShared.h"
#include "AppPics.h"
#include "AppFont.h"
#include "Strings.h"
#include "UartInt.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "Analog.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h" 
#include "RuntimeLoader.h"
#include "SYSSettings.h"
#include "Dac.h"
#include "APlay.h"
#include "AZPLoading.h"

#define AZP_LOADING_ANIM_MS	(1000)

typedef struct {
	uint8_t ready4pkey;
	uint8_t ScreenExit;
	uint8_t seconds;
	int screen_enter_time_ms;
	char str_buf[16];
	GUI_RECT gRects[2];
	WM_HTIMER hTimerAnim;
	WM_HWIN  hMidWin;
	WM_CALLBACK *OldDesktopCallback;
} AT_PARA;

typedef struct {
	uint16_t x;
	uint16_t y;
} sPoint;

static void HandShake_on_msg_ready(void);
static void Handshake_comm_fail(void *msgp);
static uint32_t hand_shake_start_time = 0;

static void _cbDraw_MidWin(WM_MESSAGE * pMsg);
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static uint8_t const new_page = AZP_ALL_METAL_SCR;
static AT_PARA *pPara;
extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 

/*********************************************************************
*
*       _cbDraw_MidWin
*
*  Function description:
*    callback function for loading Anim Window 
*/
static void _cbDraw_MidWin(WM_MESSAGE * pMsg)
{
	WM_HWIN     hWin;
	hWin = pMsg->hWin;

	switch (pMsg->MsgId) {
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_AZP_LOAD_ANIM: {
					if(AZP_LOAD_MAX_LENGTH_SECONDs < (++pPara->seconds)) {
						if(DETECTOR_HS_FAILED != AppStatus.detector_hs_state) {
							if((AZP_LOAD_MAX_LENGTH_SECONDs*2) < pPara->seconds) {
								ERRM("Handshake TOTAL-DETECTOR-INIT TIMEOUT Occured\n");
								AppStatus.detector_hs_state = DETECTOR_TOTAL_INIT_FAILED;
								pPara->ScreenExit = SCR_EXIT_REQUESTED;
							} 
							if(DETECTOR_TOTAL_INIT_COMPLETED == AppStatus.detector_hs_state)
								pPara->ScreenExit = SCR_EXIT_REQUESTED;
						} else
							pPara->ScreenExit = SCR_EXIT_REQUESTED;
					}
					if(SCR_EXIT_REQUESTED == pPara->ScreenExit)
						BSP_PWMSet(0, BSP_PWM_LCD, 0);	
					// Update middle window for loading animation //
					WM_InvalidateWindow(hWin);
					if(SCR_RUNNING == pPara->ScreenExit) {
						// Restart timer for next time //  
						WM_RestartTimer(pMsg->Data.v, AZP_LOADING_ANIM_MS);
					}
				}
				break;
				default:	// Ignore silently // 
					break;
			}
		}
		break;
		case WM_PAINT: { 
			// 1- Draw Seconds 
			{
				// Draw window background same with Desktop // 
				GUI_RECT gx = {0, 0, AZP_LOADING_WIN_SIZE_X, AZP_LOADING_WIN_SIZE_Y};
				pPara->gRects[0] = gx;
				GUI_SetColor(AZP_LOADING_BACKGROUND_COLOR);
				GUI_FillRect(gx.x0, gx.y0, gx.x1, gx.y1);

				// Draw countdown huge numbers // 
				GUI_SetFont(&GUI_FontD64);
				GUI_SetColor(GUI_BLACK);
				GUI_SetTextMode(GUI_TM_TRANS);
				memset(pPara->str_buf, 0, sizeof(pPara->str_buf));
				snprintf(pPara->str_buf, sizeof(pPara->str_buf)-1, "%u", pPara->seconds);
				GUI_DispStringInRectWrap(pPara->str_buf, &(pPara->gRects[0]), GUI_TA_HCENTER | TEXT_CF_TOP, GUI_WRAPMODE_WORD);
			}
			// 2- Draw Loading Bar and Black background Bar
			{
				uint32_t xsize = (((uint32_t)AZP_LOADING_WIN_SIZE_X)*pPara->seconds)/AZP_LOAD_MAX_LENGTH_SECONDs;
				GUI_RECT gx = {4, (AZP_LOADING_WIN_SIZE_Y*2)/3+4, xsize-4, AZP_LOADING_WIN_SIZE_Y-4};
				GUI_SetColor(GUI_BLACK);
				GUI_FillRoundedRect(gx.x0-4, gx.y0-4, gx.x1+4, gx.y1+4, 3);
				GUI_DrawGradientRoundedH(gx.x0, gx.y0, gx.x1, gx.y1, 3, GUI_LIGHTGREEN, GUI_DARKGREEN);
			}
		} break;
		case WM_CREATE:
			// Create animation timer // 
			pPara->hTimerAnim = WM_CreateTimer(hWin, ID_TIMER_AZP_LOAD_ANIM, AZP_LOADING_ANIM_MS, 0);
			break;
		case WM_DELETE:
			WM_DeleteTimer(pPara->hTimerAnim);
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_POST_PAINT:
			if(SCR_EXIT_REQUESTED == pPara->ScreenExit)
				pPara->ScreenExit = SCR_EXIT_CONFIRMED;
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

uint8_t AZP_Loading(void)
{
    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AZP_LOADING_PICs, 0xFF))	// AZP_LOADING_PICs Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

    pPara->ScreenExit = SCR_RUNNING;
	pPara->OldDesktopCallback = NULL;
	pPara->hTimerAnim = 0;
	pPara->ready4pkey = TRUE;
	pPara->screen_enter_time_ms = GUI_X_GetTime();
	pPara->seconds = 0;
	
	WM_MULTIBUF_Enable(1);
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();
	GUI_ClearKeyBuffer();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);
	
	// Create a rectange window on the middle of screen for sonar animation pictures // 
	pPara->hMidWin = \
		  WM_CreateWindowAsChild(AZP_LOADING_WIN_LEFT_X, AZP_LOADING_WIN_LEFT_Y, AZP_LOADING_WIN_SIZE_X, AZP_LOADING_WIN_SIZE_Y, \
			  WM_HBKWIN, WM_CF_SHOW, _cbDraw_MidWin, 0);
	WM_SetFocus(pPara->hMidWin);
	
	//SB_init(SB_REDUCED_MODE_USE_RIGHT);

	// WAVE_Generator_init(AUTOMATIC_SEARCH_TYPE);
	// Modulator frequency is 3 times increased for GB screen, this will cause faster frequency switch on dac output // 
	// WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3U, DAC_JACK_DETECT_AMP);	// Start DAC Wave for minimum GAUGE // 
	// WAVE_Update_FreqAmp_Gauge((uint16_t)10*GAUGE_FRACTUATION);	// Update Sound Frequency // 

	//:TODO: Add welcome sound play here

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
		hand_shake_start_time = GUI_X_GetTime();
	}

	BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_BRIGHT));	// Apply stored LCD Backlight level // 
	// Animation loop	
	while (likely(SCR_EXIT_CONFIRMED != pPara->ScreenExit)) {
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

	//WAVE_Generator_stop(TRUE, TRUE, TRUE);

	// Do Deletion of created objects & Release of Resources // 
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
 	//SB_delete();
	WM_SetFocus(WM_HBKWIN);
	WM_DeleteWindow(pPara->hMidWin);
	
	free(pPara);
	pPara = NULL;
	return new_page;
}

/*********************************************************************
*
*       _cbBk_Desktop
*
*  Function description:
*    Callback routine of desktop window
*/
static void _cbBk_Desktop(WM_MESSAGE * pMsg) {
	uint8_t the_res_indx = 0;
	WM_KEY_INFO * pInfo;

	switch (pMsg->MsgId) {
        case WM_SET_FOCUS:
            pMsg->Data.v = 0;
            break;
		case WM_PAINT:
			// 1- Add fullscreen color 
			GUI_SetColor(AZP_LOADING_BACKGROUND_COLOR);
			GUI_FillRect(0, 0, LCD_GetXSize(), LCD_GetYSize());
			// 2- locate company logo
            if(0 != SBResources[AZP_LOADING_PICs][AZP_COMPANY_LOGO].hMemHandle) {
                GUI_MEMDEV_WriteAt(SBResources[AZP_LOADING_PICs][AZP_COMPANY_LOGO].hMemHandle, AZP_COMPANY_LOGO_LEFT_X, AZP_COMPANY_LOGO_LEFT_Y);
            }
			// 3- locate device logo
            if(0 != SBResources[AZP_LOADING_PICs][AZP_DEVICE_LOGO].hMemHandle) {
                GUI_MEMDEV_WriteAt(SBResources[AZP_LOADING_PICs][AZP_DEVICE_LOGO].hMemHandle, AZP_DEVICE_LOGO_LEFT_X, AZP_DEVICE_LOGO_LEFT_Y);
            }
			// 4- Locate loading string under animation window // 
			{
				GUI_RECT gx = {(GLCD_X_SIZE/2)-150, GLCD_Y_SIZE-50, (GLCD_X_SIZE/2)+150, GLCD_Y_SIZE};
				pPara->gRects[1] = gx;
				GUI_SetColor(GUI_BLACK);
				GUI_SetFont(APP_24B_FONT);
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_DispStringInRectWrap(GetString(STR_LOADING_PLEASE_WAIT), &(pPara->gRects[1]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
					GUI_WRAPMODE_WORD);
			}
			break;
        case WM_KEY:
            TRACEM("AZP LOADING Handler Working");
            break;
		default:
 			WM_DefaultProc(pMsg);
			break;
  }
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
				// Set ground id level //
				uint16_t gid = APP_GetValue(ST_GROUND_ID);
				send_msg.cmd = CMD_SET_GROUND_ID;
				send_msg.length = 4;
				*(((uint8_t *)&send_msg) + 2) = ((gid>>8) & 0xFF);	// MSB First //
				*(((uint8_t *)&send_msg) + 3) = (gid & 0xFF);	// LSB Second // 
				UARTSend((uint8_t *)&send_msg, \
					UMD_CMD_TIMEOUT_SET_GID_MS, Handshake_comm_fail);
			}
			break;
			case RSP_SET_GROUND_ID:
				StopCommTimeout();
				// Set ferros state level //
				send_msg.cmd = CMD_SET_FERROS_STATE;
				send_msg.length = 3;
				send_msg.data.ferros_state = APP_GetValue(ST_FERROs);
				UARTSend((uint8_t *)&send_msg, \
					UMD_CMD_TIMEOUT_SET_FERRO_MS, Handshake_comm_fail);
				break;
			case RSP_SET_FERROS_STATE:
				StopCommTimeout();	
				AppStatus.detector_hs_state = DETECTOR_FIRST_CMDs_OK;
				// There is nothing TODO; We will wait for dedector to set REF, A, B, C clocks // 
				break;
			case CMD_SET_A_CLOCK_DELAY:
			case CMD_SET_B_CLOCK_DELAY: 
			case CMD_SET_C_CLOCK_DELAY: 
			case CMD_SET_REF_CLOCK_FREQ:
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

/*************************** End of file ****************************/

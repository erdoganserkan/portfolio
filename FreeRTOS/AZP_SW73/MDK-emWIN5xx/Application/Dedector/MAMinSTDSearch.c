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
#include "OTOSearch.h"
#include "MAMinSTDSearch.h"

// New type definitions // 
typedef struct PARA_s {
	uint8_t ready4pkey;
	uint8_t gauge_is_valid;
	uint8_t ScreenExit;
	uint8_t targetIDMax;
	uint8_t targetID;
	uint8_t Gauge;
	uint8_t FirstTime;
	uint8_t FerrosState, SearchStarted;
	uint32_t search_start_time;
	WM_HWIN	hGWin, hGNumWin, hMidWin;
	WM_CALLBACK * OldDesktopCallback;
	char gauge_str_buf[32];
	char tid_str_buf[32];
	int16_t Points[SCOPE_GRAPH_SIZEX];
	GUI_RECT gnum_gRect;
} PARA;

// Function Prototypes //
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void MAMinSTD_on_comm_timeout(void *msgp);
static void MAMinSTD_on_msg_ready(void);
static inline void start_dac_playing(void);

// Reduced target ID colors are used for STD & Mineralized Search Types // 
static const GUI_COLOR RTID_COlors[RTID_COUNT] = \
	{GUI_YELLOW, GUI_BLUE, GUI_RED, GUI_ORANGE};	// Same sequence with "eReduced_TIDs" typedef // 
static const char *RTID_names[RTID_COUNT] = {0,0,0,0};
static uint8_t RTID_Pics_Indx[RTID_COUNT] = {	// Cavity, Metal, Mineral // 
	OTO_CAVITY, OTO_METAL, OTO_MINERAL
};

// Full target ID colors are used for MA Search & Automatic Search Types // 
static GUI_COLOR const FTID_COlors[TARGET_ID_COUNT] = \
	{GUI_YELLOW, GUI_BLUE, GUI_BROWN, GUI_GREEN, GUI_RED, GUI_ORANGE};	// Same sequence with "eTARGET_TargetIDs" typedef // 
static const char *FTID_names[TARGET_ID_COUNT] = {0,0,0,0,0,0};
static uint8_t FTID_Pics_Indx[TARGET_ID_COUNT] = {	// Cavity, Ferro, NonFerro, Gold, Mineral // 
	OTO_CAVITY, OTO_FERROs, OTO_NFERROs, OTO_GOLD, OTO_MINERAL
};

#if(0)
	// First Release; there is gap between bars 
	static GUI_RECT left_gbars[MA_GAUGE_BAR_COUNT] = {
		{155,68,158,71},
		{146,65,150,71},
		{136,62,141,71},
		{127,58,131,71},
		{118,55,122,71},
		{108,51,113,71},
		{98,47,103,71},
		{88,43,93,71},
		{78,39,83,71},
		{67,35,73,71},
		{57,31,62,71},
		{47,26,52,71},
		{36,23,41,71},
		{25,18,31,71},
		{15,14,20,71},
		{5,9,10,71}
	};
	static GUI_RECT right_gbars[MA_GAUGE_BAR_COUNT] = {
		{215,68,218,71},
		{223,65,227,71},
		{232,62,237,71},
		{242,58,246,71},
		{251,55,255,71},
		{260,51,265,71},
		{270,47,275,71},
		{280,43,285,71},
		{290,39,295,71},
		{300,35,306,71},
		{311,31,316,71},
		{321,26,326,71},
		{332,23,337,71},
		{342,18,348,71},
		{353,14,358,71},
		{363,9,368,71}
	};
#else
	// Second Release; There is no gap between bars 
	static GUI_RECT left_gbars[MA_GAUGE_BAR_COUNT] = {
		{152,68,158,71},
		{143,65,150,71},
		{133,62,141,71},
		{124,58,131,71},
		{115,55,122,71},
		{105,51,113,71},
		{95,47,103,71},
		{85,43,93,71},
		{75,39,83,71},
		{64,35,73,71},
		{54,31,62,71},
		{43,26,52,71},
		{33,23,41,71},
		{22,18,31,71},
		{12,14,20,71},
		{3,9,10,71}
	};
	static GUI_RECT right_gbars[MA_GAUGE_BAR_COUNT] = {
		{215,68,221,71},
		{223,65,230,71},
		{232,62,240,71},
		{242,58,249,71},
		{251,55,258,71},
		{260,51,268,71},
		{270,47,278,71},
		{280,43,288,71},
		{290,39,298,71},
		{300,35,309,71},
		{311,31,319,71},
		{321,26,330,71},
		{332,23,340,71},
		{342,18,351,71},
		{353,14,361,71},
		{363,9,370,71}
	};
#endif

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 
extern uint8_t active_page;	// Defined in GUIDEMO_Start.c //
static PARA* pPara = NULL;
static uint8_t new_page;

static uint8_t GET_BAR_FROM_GAUGE(uint8_t GG)  {
	if(100 == GG)
		return MA_GAUGE_BAR_COUNT;
	else if(0 == GG)
		return 0;
	else
		return (((uint32_t)GG * ((uint32_t)MA_GAUGE_BAR_COUNT))/100U) + 1;
}


// STD Search : Draw scope graph 
// MIN Search : Draw Reduced Target ID Pics and TargetID STR
// MA Search : Draw Full Target ID pics and Target ID STR
static void _cbMidDraw(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			uint8_t pen_size = GUI_GetPenSize();
			GUI_Clear();
			GUI_SetPenSize(MA_DRAWING_LINE_WIDTH);
			GUI_SetColor(GUI_WHITE);
			GUI_DrawRoundedRect(MA_DRAWING_LINE_WIDTH/2, MA_DRAWING_LINE_WIDTH/2, \
				MA_MID_WIN_SIZEX - (MA_DRAWING_LINE_WIDTH/2), MA_MID_WIN_SIZEY - (MA_DRAWING_LINE_WIDTH/2), \
					MA_DRAWING_LINE_WIDTH/2); 
			GUI_SetPenSize(pen_size);
			
			if(STD_SEARCH == active_page){
				int16_t new_val;
				memmove(pPara->Points+1, pPara->Points, ((uint32_t)(SCOPE_GRAPH_SIZEX-1)*sizeof(int16_t)));
				new_val = (pPara->Gauge * ((uint32_t)SCOPE_GRAPH_SIZEY))/((uint32_t)UMD_GAUGE_MAX);	// Convert from angle to Graph range 
				pPara->Points[0] = SCOPE_GRAPH_SIZEY - new_val;
				GUI_SetColor(RTID_COlors[pPara->targetID]);	
				GUI_DrawGraph(pPara->Points, SCOPE_GRAPH_SIZEX, SCOPE_GRAPH_POSX, SCOPE_GRAPH_POSY);
			} else {
				uint8_t pic_indx;
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_32B_FONT);
				memset(pPara->tid_str_buf, 0, sizeof(pPara->tid_str_buf));
				if(MA_SEARCH == active_page) {
					if(TARGET_NOTARGET != pPara->targetID) 
						pic_indx = FTID_Pics_Indx[pPara->targetID-1];
					sprintf(pPara->tid_str_buf, "%s", FTID_names[pPara->targetID]);
					GUI_SetColor(FTID_COlors[pPara->targetID]); 
				} else if(MIN_SEARCH == active_page) {
					if(TARGET_NOTARGET != pPara->targetID)
						pic_indx = RTID_Pics_Indx[pPara->targetID-1];
					sprintf(pPara->tid_str_buf, "%s", RTID_names[pPara->targetID]);
					GUI_SetColor(RTID_COlors[pPara->targetID]); 
				}
				// Draw reduced/full target-id pictures (same pictures with auto-search)
				if((TARGET_NOTARGET != pPara->targetID) && (0 != SBResources[OTO_PICs][pic_indx].hMemHandle)) {
					GUI_MEMDEV_WriteAt(SBResources[OTO_PICs][pic_indx].hMemHandle, \
						MA_MID_WIN_POSX + SCOPE_GRAPH_POSX, MA_MID_WIN_POSY + SCOPE_GRAPH_POSY);
				}
				// Draw target-id string near pics 
				GUI_DispStringAt(pPara->tid_str_buf, TARGET_PICs_SIZEX + 5 , TARGET_PICs_SIZEY/5);
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
} 

static void _cbGNumDraw(WM_MESSAGE * pMsg)
{
	WM_HWIN 	hWin;
	PARA	  * pPara;
	static	   WM_HTIMER hTimerAnim;
	WM_KEY_INFO * pInfo;
	
	hWin = pMsg->hWin;
	WM_GetUserData(hWin, &pPara, sizeof(pPara));
	
	switch (pMsg->MsgId) {
		case WM_PAINT: 
			// locate backgound picture // 
			if(0 != SBResources[MA_PICs][MA_LRBACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[MA_PICs][MA_LRBACK].hMemHandle, \
					MA_GAUGE_NUM_POSX, MA_GAUGE_NUM_POSY);
			}
			// 2- Update Gauge Percentage //
			GUI_SetTextMode(GUI_TM_TRANS);
			if(UMD_GAUGE_MAX != pPara->Gauge)
				GUI_SetFont(&GUI_FontD32);
			else
				GUI_SetFont(APP_32B_FONT);
			if(MA_SEARCH == active_page)
				GUI_SetColor(FTID_COlors[pPara->targetID]);		// MA search // 
			else
				GUI_SetColor(RTID_COlors[pPara->targetID]);		// Mineral and STD Search // 
			memset(pPara->gauge_str_buf, 0, sizeof(pPara->gauge_str_buf));
			sprintf(pPara->gauge_str_buf, "%u", pPara->Gauge);
			GUI_DispStringInRectWrap(pPara->gauge_str_buf, &(pPara->gnum_gRect), \
				GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
	}
}

static void _cbGaugeDraw(WM_MESSAGE * pMsg) 
{
  WM_HWIN     hWin;
  PARA      * pPara;
  static     WM_HTIMER hTimerAnim;
  WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			volatile uint8_t indx = 0;
			uint8_t BarCount, Pensize;
			{
				if(0 != SBResources[MA_PICs][MA_GBACK].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[MA_PICs][MA_GBACK].hMemHandle, \
						MA_GAUGE_WINDOW_POSX, MA_GAUGE_WINDOW_POSY);
				}
			}

			BarCount = GET_BAR_FROM_GAUGE(pPara->Gauge);
			if(MA_SEARCH == active_page) {
				// MA Search : Left Bars for Cavity; Right Bars for others 
				if(TARGET_CAVITY == pPara->targetID) {
					// Draw LEFT Bars // 
					GUI_SetColor(FTID_COlors[pPara->targetID]); 
					for(indx = 0 ; indx<MA_GAUGE_BAR_COUNT ; indx++) {
						if(indx == BarCount)
							GUI_SetColor(GUI_BLACK);
						GUI_FillRect(left_gbars[indx].x0, left_gbars[indx].y0, left_gbars[indx].x1, left_gbars[indx].y1);
					}
					// Draw RIGHT Bars as COMPLETELY BLACK // 
					GUI_SetColor(GUI_BLACK);
					for(indx = 0 ; indx<MA_GAUGE_BAR_COUNT ; indx++) 
						GUI_FillRect(right_gbars[indx].x0, right_gbars[indx].y0, right_gbars[indx].x1, right_gbars[indx].y1);
				} else {
					// Draw RIGHT Bars // 
					GUI_SetColor(FTID_COlors[pPara->targetID]); 
					for(indx = 0 ; indx<MA_GAUGE_BAR_COUNT ; indx++) {
						if(indx == BarCount)
							GUI_SetColor(GUI_BLACK);
						GUI_FillRect(right_gbars[indx].x0, right_gbars[indx].y0, right_gbars[indx].x1, right_gbars[indx].y1);
					}
					// Draw LEFT Bars as COMPELETELY BLACK // 
					GUI_SetColor(GUI_BLACK);
					for(indx = 0 ; indx<MA_GAUGE_BAR_COUNT ; indx++)
						GUI_FillRect(left_gbars[indx].x0, left_gbars[indx].y0, left_gbars[indx].x1, left_gbars[indx].y1);
				}
			}
			else {
				// STD Search : left & right bars together 
				// MIN Search : Left & Right bars together 
				GUI_SetColor(RTID_COlors[pPara->targetID]);	// STD Search & MIN Search // 
				// Draw LEFT & RIGHT Bars together // 
				for(indx = 0 ; indx<MA_GAUGE_BAR_COUNT ; indx++) {
					if(indx == BarCount)
						GUI_SetColor(GUI_BLACK);
					GUI_FillRect(left_gbars[indx].x0, left_gbars[indx].y0, left_gbars[indx].x1, left_gbars[indx].y1);
					GUI_FillRect(right_gbars[indx].x0, right_gbars[indx].y0, right_gbars[indx].x1, right_gbars[indx].y1);
				}
			}
			// Draw center filled circle //
			if(0 != pPara->Gauge) {
				if(MA_SEARCH == active_page) 
					GUI_SetColor(FTID_COlors[pPara->targetID]); 
				else
					GUI_SetColor(RTID_COlors[pPara->targetID]);	// STD Search & MIN Search // 
			} else
				GUI_SetColor(GUI_BLACK); // STD Search & MIN Search // 
			
			GUI_FillCircle(MA_GAUGE_CIRCLE_POSX, MA_GAUGE_CIRCLE_POSY, MA_GAUGE_CIRCLE_RADIUS);

			// Send Search START Command to Dedector //
			// Because of WM_PAINT event is called multiple time, we must be sure about single CMD send // 
			if((FALSE == pPara->SearchStarted) && (CMD_START_SEARCH != last_send_msg.cmd)) {
				UmdPkt_Type msg;
				msg.cmd = CMD_START_SEARCH;
				msg.length = 3;	// CMD(U8) + LENGTH(U8) + DATA(U8) // 
				switch(active_page) {
					case STD_SEARCH:
						msg.data.search_type = STD_SEARCH_TYPE;
						break;
					case MA_SEARCH:
						msg.data.search_type = METAL_ANALYSIS_TYPE;
						break;
					case MIN_SEARCH:
						msg.data.search_type = MINERALIZED_SEARCH_TYPE;
						break;
					default:
						msg.data.search_type = STD_SEARCH_TYPE;
						ERRM("UNEXPECTED active_page(%u)\n", active_page);
						break;
				}
				UARTSend((uint8_t *)&msg, \
					UMD_CMD_TIMEOUT_SEARCH_CMD_MS, MAMinSTD_on_comm_timeout);
				pPara->search_start_time = GUI_X_GetTime();
			}
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
					case KEY_OK_EVENT:
						if(STD_SEARCH == active_page) {	// Metal Alaysis search is accessed from here // 
							UmdPkt_Type msg;
							new_page = MA_SEARCH;
							msg.cmd = CMD_STOP_SEARCH;
							msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
							UARTSend((uint8_t *)&msg, \
								UMD_CMD_TIMEOUT_SEARCH_CMD_MS, MAMinSTD_on_comm_timeout);
						} else
							key_valid = FALSE;
						break;
					#if(0)
					case KEY_DEPTH_EVENT: {
						extern uint8_t page_before_depth;	// defined in DeptchCalc.c module // 
						new_page = DEPTH_CALC;
						page_before_depth = active_page;

						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2; // CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, MAMinSTD_on_comm_timeout);
					}
					break;
					#endif
					case KEY_ESC_EVENT: {
						if(MA_SEARCH == active_page)
							new_page = STD_SEARCH;
						UmdPkt_Type msg;
						msg.cmd = CMD_STOP_SEARCH;
						msg.length = 2;	// CMD(U8) + LWNGTH(U8)  // 
						UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SEARCH_CMD_MS, MAMinSTD_on_comm_timeout);
					}
					break;
					default:	
						key_valid = FALSE;
						break;
				}
				if(TRUE == key_valid) {
					WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
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
*    Creates and executes Metal Analysis, Mineralized Search & STD Search screens 
*/
uint8_t MAMinSTDSearch(void) 
{
	volatile uint8_t indx;

	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);
	if(0 != InitGroupRes(MA_PICs, 0xFF))	
		while(TODO_ON_ERR);
	if(0 != InitGroupRes(OTO_PICs, 0xFF))	
		while(TODO_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = TRUE;
	pPara->OldDesktopCallback = NULL;
	pPara->ScreenExit = FALSE;
	
	new_page = RM_SCREEN;
	pPara->targetID = RTID_NOTARGET;
	pPara->FerrosState = APP_GetValue(ST_FERROs);
	pPara->SearchStarted = FALSE;
	pPara->Gauge = 0;
	pPara->gauge_is_valid = FALSE;
	pPara->search_start_time = 0;
	pPara->FirstTime = TRUE;
	if((STD_SEARCH == active_page) || (MA_SEARCH == active_page)) { 
		pPara->targetIDMax = TARGET_ID_MAX;
		for(indx=(SCOPE_GRAPH_SIZEX)-1;;indx--) {
			pPara->Points[indx] = SCOPE_GRAPH_SIZEY>>1;
			if(0 == indx) break;
		}
	}
	else
		pPara->targetIDMax = RTID_MAX;
	pPara->gnum_gRect.x0 = 0;
	pPara->gnum_gRect.y0 = 0;
	pPara->gnum_gRect.x1 = MA_GAUGE_NUM_SIZEX;
	pPara->gnum_gRect.y1 = MA_GAUGE_NUM_SIZEY;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();	// Is This necessary? We are already drawing a picture that fully covering LCD // 
	{
		static uint8_t FirstTime = TRUE;
		if(TRUE == FirstTime) {
			FirstTime = FALSE;
			
			RTID_names[RTID_NOTARGET] = "     ";
			RTID_names[RTID_CAVITY] = GetString(STR_CAVITY_INDX);
			RTID_names[RTID_METAL] = GetString(STR_METAL_INDX);
			RTID_names[RTID_MINERAL] = GetString(STR_MINERAL_INDX);

			FTID_names[TARGET_NOTARGET] = "     ";
			FTID_names[TARGET_CAVITY] = GetString(STR_CAVITY_INDX);
			FTID_names[TARGET_FERROs] = GetString(STR_FERROS_INDX);
			FTID_names[TARGET_NFERROs] = GetString(STR_NONFERROS_INDX);
			FTID_names[TARGET_GOLD] = GetString(STR_GOLD_INDX);
			FTID_names[TARGET_MINERAL] = GetString(STR_MINERAL_INDX);
		}
	}
	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);	// Force to redraw Desktop Window // 

	/* CREATE Windows and Do Supplementary Operations */
	// Create Gauge window
	pPara->hGWin = WM_CreateWindowAsChild(MA_GAUGE_WINDOW_POSX, MA_GAUGE_WINDOW_POSY, MA_GAUGE_WINDOW_SIZEX, MA_GAUGE_WINDOW_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbGaugeDraw, sizeof(pPara));
	WM_SetUserData(pPara->hGWin,   &pPara, sizeof(pPara));

	// Create Gauge Num Percenratge window //
	pPara->hGNumWin = WM_CreateWindowAsChild(MA_GAUGE_NUM_POSX, MA_GAUGE_NUM_POSY, MA_GAUGE_NUM_SIZEX, MA_GAUGE_NUM_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbGNumDraw, sizeof(pPara));
	WM_SetUserData(pPara->hGNumWin,   &pPara, sizeof(pPara));

	pPara->hMidWin = WM_CreateWindowAsChild(MA_MID_WIN_POSX, MA_MID_WIN_POSY, MA_MID_WIN_SIZEX, MA_MID_WIN_SIZEY, \
		WM_HBKWIN, WM_CF_SHOW, _cbMidDraw, sizeof(pPara));
	WM_SetUserData(pPara->hMidWin,   &pPara, sizeof(pPara));

	// Give the focus to most-active (GAUGE WINDOW) window // 
	WM_SetFocus(pPara->hGWin);

	SB_init(SB_FULL_MODE);
	// Do detector & analog specific job // 
	#if(TRUE == CLOCK_GENERATION_STATE)
		DiffCalc_init();
	#endif
	set_lcd_bcklight_reduce_state(FALSE);

	// Animation loop
	while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				MAMinSTD_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}
	
	set_lcd_bcklight_reduce_state(TRUE);
	WAVE_Generator_stop(TRUE, TRUE, TRUE);
	// Do Deletion of created objects & Release of Resources // 
	#if(0 && (TRUE == CLOCK_GENERATION_STATE))
		stop_clock_generation();
	#endif
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_SetFocus(WM_HBKWIN);
	SB_delete();
	WM_SetFocus(WM_HBKWIN);	// Set FOCUS back to the desktop window // 
	WM_DeleteWindow(pPara->hGWin);
	WM_DeleteWindow(pPara->hGNumWin);
	WM_DeleteWindow(pPara->hMidWin);

	free(pPara);
	pPara=NULL;
	return new_page;
}

// Function to be called when timeout occured after a command send but response not received // 
static void MAMinSTD_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(TRUE == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_STOP_SEARCH:
			pPara->ScreenExit = TRUE;
			new_page = RM_SCREEN;
			break;
		case CMD_START_SEARCH:
			pPara->SearchStarted = FALSE;
			pPara->ScreenExit = TRUE;
			break;
		default:
			ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
				((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
					((uint8_t *)last_msgp)[3]);
			break;
	}
}

// Function to be called when a pkt received from Detector // 
static void MAMinSTD_on_msg_ready(void)
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
				if((TRUE == pPara->SearchStarted) && (FALSE == pPara->ScreenExit)){
					#if(0)
						int32_t TGauge = gui_msg[2] + 256*(gui_msg[3] & 0x0F);
						//TGauge = (TGauge * UMD_GAUGE_MAX) / 4096; 
						TGauge = DiffCalc_getdiff(TGauge);
						if(TGauge < 0)
							TGauge *= -1;
					#else
						uint8_t TGauge = msg_ptr->data.gauge;
					#endif
					if(UMD_GAUGE_MAX < TGauge)
						TGauge = UMD_GAUGE_MAX;	// Truncation to maximum value for safety // 
					if(pPara->Gauge != TGauge) {
						pPara->Gauge = TGauge;
						WAVE_Update_FreqAmp_Gauge((uint16_t)TGauge*GAUGE_FRACTUATION);	// Update Sound Frequency // 
						WM_InvalidateWindow(pPara->hGNumWin);
						WM_InvalidateWindow(pPara->hGWin);
					}
					// If we are in STD search update scope-window regardless of value of gauge // 
					if(STD_SEARCH == active_page)
						WM_InvalidateWindow(pPara->hMidWin);	// Redraw Scope Graph Window // 
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
					if(pPara->targetIDMax < TempID)
						TempID = pPara->targetIDMax;	// Truncation to maximum value for safety // 
					if(TempID != pPara->targetID) {	// If last targetID changed redraw GAUGE related UI objects with new color // 
						pPara->targetID = TempID;
						WM_InvalidateWindow(pPara->hMidWin);	// Change scope color (std) or target-id pictures (ma & mineral) //
						WM_InvalidateWindow(pPara->hGWin);	// Change guage bar drawing color // 
						WM_InvalidateWindow(pPara->hGNumWin);	// Change guage percentage drawing color according to targat-id // 
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
					start_dac_playing();
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
*       _cbBk_Desktop
*
*  Function description:
*    Callback routine of desktop window
*/
static void _cbBk_Desktop(WM_MESSAGE * pMsg) {
  switch (pMsg->MsgId) {
		case WM_PAINT: {
			const char *str;
			GUI_RECT gRect = {MA_SCR_STR_UPX, MA_SCR_STR_UPY, MA_SCR_STR_DOWNX, MA_SCR_STR_DOWNY};
			// Draw Background Image //
			if(0 != SBResources[MA_PICs][MA_BACK].hMemHandle) {
				GUI_MEMDEV_WriteAt(SBResources[MA_PICs][GB_BACK].hMemHandle, 0, 0);
			}
			// Display Screen Name // 
			GUI_SetTextMode(GUI_TM_TRANS);
			GUI_SetFont(APP_32B_FONT);
			GUI_SetColor(GUI_YELLOW);
			if(STD_SEARCH == active_page) 
				str = GetString(STR_STD_SEARCH_INDX);
			else if(MIN_SEARCH == active_page) 
				str = GetString(STR_MINERALIZED_INDX);
			else 
				str = GetString(STR_METAL_ANALYSIS_INDX);
			GUI_DispStringInRectWrap(str, &gRect, GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

			// Draw Ferro / NonFerro selection pictures // 
			if(pPara->FerrosState) {
				if(0 != SBResources[MA_PICs][MA_FERROOK].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[MA_PICs][MA_FERROOK].hMemHandle, \
						MA_FERRO_PIC_POSX, MA_FERRO_PIC_POSY);
				}
			}
			else {
				if(0 != SBResources[MA_PICs][MA_NOFERRO].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[MA_PICs][MA_NOFERRO].hMemHandle, \
						MA_FERRO_PIC_POSX, MA_FERRO_PIC_POSY);
				}
			}
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

static inline void start_dac_playing(void) {
	uint8_t active_search_type;
	switch(active_page) {
		case STD_SEARCH:
			active_search_type = STD_SEARCH_TYPE;
			break;
		case MA_SEARCH:
			active_search_type = METAL_ANALYSIS_TYPE;
			break;
		case MIN_SEARCH:
			active_search_type = MINERALIZED_SEARCH_TYPE;
			break;
		default:
			active_search_type = STD_SEARCH_TYPE;
			ERRM("UNEXPECTED active_page(%u)\n", active_page);
			break;
	}
	WAVE_Generator_init(active_search_type);
	WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
}
/*************************** End of file ****************************/

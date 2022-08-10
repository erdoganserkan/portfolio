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
#include "GuiConsole.h"
#include "WM.h"
#include "UMDShared.h"
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "UartInt.h"
#include "AppCommon.h"
#include "AppFont.h"
#include "AppPics.h" 
#include "RuntimeLoader.h"
#include "Popup.h"
#include "BSP.h"
#include "Battery.h" 
#include "AZPAllMetal.h"
#include "StatusBar.h"

static WM_HWIN hWinStatus;
static sSBData_Type SBData;
static uint8_t SBCounter;
static sPopup popPara;
static WM_HTIMER hWarnTimer;
static uint32_t bat_raw_sum = 0;

static uint32_t jd_sum = 0;
static uint8_t jd_indx = 0;
static uint16_t jd_samples[JACK_DETECT_AVG_DEPTH];

static void DrawHB(void);

/*********************************************************************
*
*       _cbStatus
*/
static void _cbStatus(WM_MESSAGE * pMsg) {
	WM_HWIN    hWin;
	#if(SB_ANIM == TRUE)
		static     WM_HTIMER hTimerAnim;
	#endif
	int        xSize;
	int        ySize;
	int        Id;

	hWin      = pMsg->hWin;
	switch (pMsg->MsgId) {
		#if(0)
		  case WM_PRE_PAINT:
		    GUI_MULTIBUF_Begin();	// Buna  gerek var mi? Otomatik enable edilince "WM_MULTIBUF_Enable(1)" //
		    break;
		  case WM_POST_PAINT:
		    GUI_MULTIBUF_End();	// Buna  gerek var mi? Otomatik enable edilince "WM_MULTIBUF_Enable(1)" //
		    break;
		#endif
	  	case WM_CREATE:
			#if(SB_ANIM == TRUE)
				hTimerAnim      = WM_CreateTimer(hWin, ID_TIMER_SB_UPDATE, SB_ANIM_RES_MS, 0);
			#endif
			if(TRUE == SBData.BatWarn) 
				hWarnTimer      = WM_CreateTimer(hWin, ID_TIMER_SB_BAT_WARN, SB_BAT_WARN_ANIM_RES_MS, 0);
	    	break;
	  	case WM_DELETE:
			#if(SB_ANIM == TRUE)
				WM_DeleteTimer(hTimerAnim);
			#endif
			if(0 != hWarnTimer) {
				WM_DeleteTimer(hWarnTimer);
				hWarnTimer = 0;
			}
	    	break;
	  	case WM_TIMER: {
			Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_SB_UPDATE: {
					if(TRUE == ADC_done) {
						static uint8_t FirstTime = TRUE;
						// JACK STATE READING RELATED ADC PROCESSING // 
						uint16_t jack_det_ch_raw, jack_detect_mv;
						jack_det_ch_raw = BSP_GET_RAW_ADC(JACK_DETECT_ADC_CH);
						jack_detect_mv = __RAW_ADC_TO_MV(jack_det_ch_raw);

						// update jack detect results moving average // 
						if(TRUE == FirstTime) {
							FirstTime = FALSE;
							volatile uint8_t indx;
							jd_sum = jack_detect_mv * JACK_DETECT_AVG_DEPTH;
							for(indx=0; indx<JACK_DETECT_AVG_DEPTH ; indx++)
								jd_samples[indx] = jack_detect_mv;
						}
						jd_sum -= jd_samples[jd_indx];
						jd_sum += jack_detect_mv;
						jd_samples[jd_indx] = jack_detect_mv;
						if(++jd_indx == JACK_DETECT_AVG_DEPTH)
							jd_indx=0;
						#if(8 == JACK_DETECT_AVG_DEPTH)
							jack_detect_mv = (jd_sum>>3);
						#elif(1 == JACK_DETECT_AVG_DEPTH)
							jack_detect_mv = jd_sum;
						#else
							#error "HOW TO get average of jack_detect samples???"
						#endif
						#if(1)
						if(((JACK_DETECTION_THRESHOLD_MV*103)/100) > jack_detect_mv) { 
							// voltage is high, UNMUTE spaeakers // 
							uint16_t Jack_State = HP_JACK_NOT_INSERTED;
							SB_setget_component(SB_JACK_STATE, TRUE, &Jack_State);
							SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, UNMUTE_SPEAKERS_PIN_STATE);
						} else if((((JACK_DETECTION_THRESHOLD_MV*103)/100) >= jack_detect_mv) && (((JACK_DETECTION_THRESHOLD_MV*97)/100) <= jack_detect_mv)) {
							// This is safe threshold area for oscillation, hold the previous position, dont do changes // 
						}
						else { 	// volatage is LOW, MUTE speakers // 					
							uint16_t Jack_State = HP_JACK_INSERTED;
							SB_setget_component(SB_JACK_STATE, TRUE, &Jack_State);
							SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, MUTE_SPEAKERS_PIN_STATE);
						}
						#else
							uint16_t Jack_State = HP_JACK_NOT_INSERTED;
							SB_setget_component(SB_JACK_STATE, TRUE, &Jack_State);
							SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, UNMUTE_SPEAKERS_PIN_STATE);
						#endif

						// BATTERY READING RELATED ADC PROCESSING // 
						bat_raw_sum += BSP_GET_RAW_ADC(BATTERY_READ_ADC_CH);	// Periodic job for  battery adc channel // 
						if((SB_ANIM_MS/SB_ANIM_RES_MS) == (++SBCounter)) {
							uint16_t new_level, old_level;
							SBCounter = 0;
							//level = rand()%GROUND_ID_MAX;
							//SB_setget_component(SB_GROUND_ID, TRUE, &level);
							new_level = BSP_get_percentage((bat_raw_sum/(SB_ANIM_MS/SB_ANIM_RES_MS)));
							bat_raw_sum = 0;
							SB_setget_component(SB_BATTERY, FALSE, &old_level);
							if(old_level != new_level) {
								SB_setget_component(SB_BATTERY, TRUE, &new_level);	// Set new battery level into global sdram object // 
								WM_InvalidateWindow(hWin);	// Battery Level changed, update GUI // 
							}
						}
						BSP_ADCStart(); // Burst Mode ADC Start (JackDetect + BatteryRead)
					}
					WM_RestartTimer(pMsg->Data.v, SB_ANIM_RES_MS);
				}
				break;
				case ID_TIMER_SB_BAT_WARN: {
					if(TRUE == SBData.BatWarn) {
						if(TRUE == SBData.BatVisible) 
							SBData.BatVisible = FALSE;
						else SBData.BatVisible = TRUE;
						WM_RestartTimer(pMsg->Data.v, SB_BAT_WARN_ANIM_RES_MS);
					}
					WM_InvalidateWindow(hWin);	// Update GUI // 
				}
				break;
				default:
					break;
		}
  	}
	break;
  case WM_PAINT:
	{
		static char value_strs[SB_MEMBER_COUNT][10];
		static GUI_RECT gRect[SB_MEMBER_COUNT];

	    xSize = WM_GetWindowSizeX(hWin);
	    ySize = WM_GetWindowSizeY(hWin);

		switch(SBData.mode) {
			case SB_FULL_MODE: {
				// Draw background Picture //
				if(0 != SBResources[SB_PICs][SB_BACKGROUD].hMemHandle)
					GUI_MEMDEV_WriteAt(SBResources[SB_PICs][SB_BACKGROUD].hMemHandle, 0, 0);
				
				GUI_SetTextMode(GUI_TM_TRANS);
				GUI_SetFont(APP_24B_FONT);
				GUI_SetColor(GUI_DARKBLUE);
				// Draw Volume Level //
				gRect[SB_VOLUME].x0 = SB_STR_COMMON_POSX;
				gRect[SB_VOLUME].x1 = SB_STR_ENDX;
				gRect[SB_VOLUME].y0 = SB_VOLUME_STR_POSY;
				gRect[SB_VOLUME].y1 = gRect[SB_VOLUME].y0 + STATUS_BAR_STR_Y_SZIE;
				sprintf(value_strs[SB_VOLUME], "%02u", SBData.Volume);
				GUI_DispStringInRectWrap(value_strs[SB_VOLUME], &(gRect[SB_VOLUME]), \
					GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

				// Draw Sensitivity Level //
				gRect[SB_SENSITIVITY].x0 = SB_STR_COMMON_POSX;
				gRect[SB_SENSITIVITY].x1 = SB_STR_ENDX;
				gRect[SB_SENSITIVITY].y0 = SB_SENSITIVITY_STR_POSY;
				gRect[SB_SENSITIVITY].y1 = gRect[SB_SENSITIVITY].y0 + STATUS_BAR_STR_Y_SZIE;
				sprintf(value_strs[SB_SENSITIVITY], "%02u", SBData.Sensitivity);
				GUI_DispStringInRectWrap(value_strs[SB_SENSITIVITY], &(gRect[SB_SENSITIVITY]), \
					GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

				// Draw Brightness Level //
				gRect[SB_BRIGHTNESS].x0 = SB_STR_COMMON_POSX;
				gRect[SB_BRIGHTNESS].x1 = SB_STR_ENDX;
				gRect[SB_BRIGHTNESS].y0 = SB_BRIGHTNESS_STR_POSY;
				gRect[SB_BRIGHTNESS].y1 = gRect[SB_BRIGHTNESS].y0 + STATUS_BAR_STR_Y_SZIE;
				sprintf(value_strs[SB_BRIGHTNESS], "%02u", SBData.Brightness);
				GUI_DispStringInRectWrap(value_strs[SB_BRIGHTNESS], &(gRect[SB_BRIGHTNESS]), \
					GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				
				// Draw Mineralization (Ground ID) Level // 
				gRect[SB_GROUND_ID].x0 = SB_STR_COMMON_POSX;
				gRect[SB_GROUND_ID].x1 = SB_STR_ENDX;
				gRect[SB_GROUND_ID].y0 = SB_GID_STR_POSY;
				gRect[SB_GROUND_ID].y1 = gRect[SB_GROUND_ID].y0 + STATUS_BAR_STR_Y_SZIE;
				sprintf(value_strs[SB_GROUND_ID], "%02u", ((((uint32_t)SBData.Mineralization)>>8) & 0xFF));
				GUI_DispStringInRectWrap(value_strs[SB_GROUND_ID], &(gRect[SB_GROUND_ID]), \
					GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				
				// Draw Battery Icon & Level
				#if(1)
				if((FALSE == SBData.BatWarn) || \
					((TRUE == SBData.BatWarn) && (TRUE == SBData.BatVisible))) {
					volatile uint8_t indx;
					struct {
							uint8_t level;
							ResInfoType *ResPicPtr;
						} BatPics[] = { 
							{0, &SBResources[SB_PICs][SB_BATTERY0]}, {10, &SBResources[SB_PICs][SB_BATTERY10]}, \
							{30, &SBResources[SB_PICs][SB_BATTERY30]}, {45, &SBResources[SB_PICs][SB_BATTERY45]}, \
							{55, &SBResources[SB_PICs][SB_BATTERY55]}, {60, &SBResources[SB_PICs][SB_BATTERY60]}, \
							{75, &SBResources[SB_PICs][SB_BATTERY75]}, {90, &SBResources[SB_PICs][SB_BATTERY90]}, \
							{100, &SBResources[SB_PICs][SB_BATTERY100]}, {0xFF, NULL}
						};
					for(indx=0 ; 0xFF != BatPics[indx].level ; indx++) {
						if(BatPics[indx].level > SBData.Battery) {
							indx--;
							break;
						}
						else if(BatPics[indx].level == SBData.Battery) {
							break;
						}
					}
					if(0 != BatPics[indx].ResPicPtr->hMemHandle) {
						GUI_MEMDEV_WriteAt(BatPics[indx].ResPicPtr->hMemHandle, SB_BAT_ICON_POSX, SB_BAT_ICON_POSY);
					}
					// Draw Battery Level // 
					sprintf(value_strs[SB_BATTERY], "%02u", SBData.Battery);
					{
						gRect[SB_BATTERY].x0 = SB_STR_COMMON_POSX;
						gRect[SB_BATTERY].x1 = SB_STR_ENDX;
						gRect[SB_BATTERY].y0 = SB_BATTERY_STR_POSY;
						gRect[SB_BATTERY].y1 = gRect[SB_BATTERY].y0 + STATUS_BAR_STR_Y_SZIE;
						GUI_DispStringInRectWrap(value_strs[SB_BATTERY], (GUI_RECT *)&(gRect[SB_BATTERY]), GUI_TA_HCENTER | GUI_TA_VCENTER, \
							GUI_WRAPMODE_WORD);
					}
				}
				#endif
			}
			break;
			case SB_FULL_TOP:
			{
				// fill background color 				
				GUI_SetColor(AZP_BACKGROUND_COLOR);
				GUI_FillRect(0, 0, SB_FULL_TOP_SIZE_X, SB_FULL_TOP_SIZE_Y);
				volatile uint8_t indx;
				uint16_t const partX = SB_FULL_TOP_SIZE_X / SB_FULL_TOP_PARt_COUNT;
				for(indx=0 ; indx<SB_FULL_TOP_PARt_COUNT ; indx++) {
					// draw black boxes (draw active one's background as yellow)
					if(indx == SBData.active_pos)
						GUI_SetColor(GUI_YELLOW);
					else
						GUI_SetColor(GUI_BLACK);
					uint16_t posX = indx * partX;
					uint16_t iconX, iconY;
					if(2 > indx) 
						iconX = posX + (partX - SB_TOP_ICON_SIZEX)/2;
					else
						iconX = posX + 10;

					iconY = (SB_FULL_TOP_SIZE_Y-SB_TOP_ICON_SIZEY)/2;
					GUI_FillRoundedRect(posX+2, 4, posX+partX-2, SB_FULL_TOP_SIZE_Y-4,2);
					if(indx == SBData.active_pos) {	// Draw black box arund yellow color // 
						GUI_SetColor(GUI_BLACK);	
						GUI_DrawRoundedRect(posX+2, 4, posX+partX-2, SB_FULL_TOP_SIZE_Y-4,2);
					}
					// locate icons inside of black boxes 
					if(0 != SBResources[AZP_SB_PICs][SB_AZP_SETTINGs + indx].hMemHandle)
						GUI_MEMDEV_WriteAt(SBResources[AZP_SB_PICs][SB_AZP_SETTINGs + indx].hMemHandle, iconX, iconY);
					// draw string values near icons
					gRect[indx].x0 = posX + (partX/2);
					gRect[indx].x1 = posX + partX;
					gRect[indx].y0 = 0;
					gRect[indx].y1 = SB_FULL_TOP_SIZE_Y;
					if(indx == SBData.active_pos)
						GUI_SetColor(GUI_BLUE);
					else
						GUI_SetColor(GUI_WHITE);
					GUI_SetTextMode(GUI_TM_TRANS);
					GUI_SetFont(APP_32B_FONT);
					//GUI_SetFont(&GUI_FontD24x32);
					switch(indx){
						case AZP_SB_POS_SENS: {
							// Draw Sensitivity Level //
							sprintf(value_strs[SB_SENSITIVITY], "%u", SBData.Sensitivity/(SENSITIVITY_MAX / AZP_SYS_SET_MENU_MAX));
							GUI_DispStringInRectWrap(value_strs[SB_SENSITIVITY], &(gRect[indx]), \
								GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
						} break;
						case AZP_SB_POS_VOL: {
							// Draw Volume Level //
							sprintf(value_strs[SB_VOLUME], "%u", SBData.Volume/(VOLUME_MAX/ AZP_SYS_SET_MENU_MAX));
							GUI_DispStringInRectWrap(value_strs[SB_VOLUME], &(gRect[indx]), \
								GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
						} break;
						case AZP_SB_POS_BRIGHT: {
							// Draw Bright Level //
							sprintf(value_strs[SB_BRIGHTNESS], "%u", SBData.Brightness/(BRIGHTNESS_MAX / AZP_SYS_SET_MENU_MAX));
							GUI_DispStringInRectWrap(value_strs[SB_BRIGHTNESS], &(gRect[indx]), \
								GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
						} break;
						case AZP_SB_POS_BAT: {
							// Draw Battery Level //
							uint16_t temp = (SBData.Battery/10);
							if(10 == temp)
								temp--;
							sprintf(value_strs[SB_BATTERY], "%d", temp);
							GUI_DispStringInRectWrap(value_strs[SB_BATTERY], &(gRect[indx]), \
								GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
						} break;
						case AZP_SB_POS_GB:
						case AZP_SB_POS_SYS_SET:
							break;	// There is NO value string for this icons // 
						default:
							while(STALLE_ON_ERR);
							break;
					}
				}
			} break;
			case SB_REDUCED_MODE_USE_RIGHT:
			case SB_REDUCED_MODE_USE_LEFT:
			case SB_REDUCED_MODE_USE_BOTTOM_RIGHT:
			case SB_REDUCED_MODE_USE_BOTTOM_LEFT:
				if((FALSE == SBData.BatWarn) || \
					((TRUE == SBData.BatWarn) && (TRUE == SBData.BatVisible)))
					DrawHB();
				else
					GUI_Clear();
				break;
			default:
				while(STALLE_ON_ERR);
				break;
		}
	}
	break;
  default:
    WM_DefaultProc(pMsg);
  }
}

void SB_delete(void)
{
	WM_DeleteWindow(hWinStatus);
	BSP_ADCStop();
}

void SB_init(uint8_t mode)
{
	uint16_t xpos, ypos, xsize, ysize;
	static uint8_t First = TRUE;

	BSP_ADCInit((0x1<<BATTERY_READ_ADC_CH) | (0x1<<JACK_DETECT_ADC_CH));	// Init ADC channel settings for battery read // 
	BSP_ADCStart();	// Start Burst mode ADC for enabled channels //

	if(SB_FULL_TOP != mode) {
		if(0 != InitGroupRes(SB_PICs, 0xFF))	// Status Bar Resource Initialization, it is common for all pages // 
			while(TODO_ON_ERR);
		if(0 != InitGroupRes(HB_PICs, 0xFF))	// Horizontal Battery Pics Initialization @ SDRAM for quick access //
			while(TODO_ON_ERR);
		if(0 != InitGroupRes(GB_PICs, 0xFF))	// GB Pics Initialization @ SDRAM for quick access, There is some shared pics //
			while(TODO_ON_ERR);
	}
	else { 
		if(0 != InitGroupRes(AZP_SB_PICs, 0xFF))	// AZP SB Pics Initialization @ SDRAM for quick access //
			while(TODO_ON_ERR);
	}

	// Set shared resources and initialize them with initial values // 
	SBData.Brightness = APP_GetValue(ST_BRIGHT);
	SBData.Mineralization = APP_GetValue(ST_GROUND_ID);
	SBData.Sensitivity = APP_GetValue(ST_SENS);
	SBData.Volume = APP_GetValue(ST_VOL);
	SBData.Battery = BSP_get_percentage(BSP_GET_RAW_ADC(BATTERY_READ_ADC_CH));	// Do initial battery read from application // 
	SBData.mode = mode;
	SBData.Jack_State = HP_JACK_NOT_INSERTED;
	SBCounter = (SB_ANIM_MS/SB_ANIM_RES_MS) - 1;
	hWarnTimer = 0;
	bat_raw_sum = BSP_GET_RAW_ADC(BATTERY_READ_ADC_CH) * ((SB_ANIM_MS/SB_ANIM_RES_MS) - 1);
 
	if (TRUE == First) {
		First = FALSE;
		SBData.BatWarn = FALSE;
		SBData.BatVisible = TRUE;
		SBData.active_pos = 0xFF;	// AZP dedector system settings related // 
		
		SET_TP(MUTE_SPK_PORT_NUM, MUTE_SPK_PIN_NUM, UNMUTE_SPEAKERS_PIN_STATE);
		jd_sum = 0;
		jd_indx = 0;
		memset(jd_samples, 0, sizeof(JACK_DETECT_AVG_DEPTH));
	}
	
	// Create status window //
	switch(mode) {
		case SB_FULL_MODE:
			xpos = ypos = 0;
			xsize = STATUS_BAR_X_SIZE;
			ysize = LCD_GetYSize();
			break;
		case SB_REDUCED_MODE_USE_RIGHT:
		case SB_REDUCED_MODE_USE_LEFT:
			if(SB_REDUCED_MODE_USE_RIGHT == mode)
				xpos = REDUCED_SB_UR_POSX;
			else
				xpos = REDUCED_SB_UL_POSX;
			ypos = REDUCED_SB_POSY;
			xsize = REDUCED_SB_SIZEX;
			ysize = REDUCED_SB_SIZEY;
			break;
		case SB_REDUCED_MODE_USE_BOTTOM_RIGHT:
		case SB_REDUCED_MODE_USE_BOTTOM_LEFT:
			if(SB_REDUCED_MODE_USE_BOTTOM_RIGHT == mode)
				xpos = REDUCED_SB_DR_POSX;
			else
				xpos = REDUCED_SB_DL_POSX;
			ypos = REDUCED_SB_DLR_POSY;
			xsize = REDUCED_SB_SIZEX;
			ysize = REDUCED_SB_SIZEY;
			break;
		case SB_FULL_TOP:
			xpos = ypos = 0;
			xsize = SB_FULL_TOP_SIZE_X;
			ysize = SB_FULL_TOP_SIZE_Y;
			break;
		default:
			while(STALLE_ON_ERR);
			break;
	}
  	hWinStatus = WM_CreateWindowAsChild( xpos, ypos, xsize, ysize, \
			WM_HBKWIN, WM_CF_SHOW | WM_CF_MEMDEV, _cbStatus, 0);
	// Register callback function to update status bar //
	static sSBData_Type *tptr = &SBData;

}

uint16_t SB_setget_component(uint8_t indx, uint8_t set, uint16_t *valptr)
{ 
	uAppStore TempSettings;

	if(NULL == valptr) {
		ERRM("NULL PARAMETER\n");
		return 0;
	}

	App_ReloadSettings(&TempSettings, RELOAD_FROM_RAM);
	if(0 != hWinStatus)	{ // Check if status-bar is created or not // 

		// Update shared resource and then call_back function will handle rest of operation // 
		if(indx < SB_MEMBER_COUNT) {
			switch(indx) {
				case SB_SENSITIVITY:			// hassasiyet seviyesi
					if(TRUE == set) SBData.Sensitivity = TempSettings.str.Sensitivity	= *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Sensitivity;
					break;
				case SB_VOLUME:						// Ses seviyesi
					if(TRUE == set) SBData.Volume = TempSettings.str.Volume = *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Volume;
					break;
				case SB_BRIGHTNESS:
					if(TRUE == set) SBData.Brightness = TempSettings.str.Brightness = *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Brightness;
					break;
				case SB_GROUND_ID:
					if(TRUE == set) SBData.Mineralization	= TempSettings.str.Mineralization =	*((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Mineralization;
					break;
				case SB_BATTERY:								// Kalan pil seviyesi (% olarak)
					if(TRUE == set) SBData.Battery = *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Battery;
					break;
				case SB_JACK_STATE:
					if(TRUE == set) SBData.Jack_State= *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.Jack_State;
					break;
				case SB_AZP_ACTIVE_POS:
					if(TRUE == set) SBData.active_pos = *((uint16_t *)valptr);
					else *((uint16_t *)valptr) = SBData.active_pos;
					break;
				default:
					while(STALLE_ON_ERR);
					return 0;
					break;
			}
		}
	}
	if(TRUE == set) {
		APP_StoreSettings(&TempSettings, FALSE);
		if(0 != hWinStatus) 
			WM_InvalidateWindow(hWinStatus);	// Invoke emWin to redraw GUI object // 
		return 0;
	}
	else 
		return (*((uint16_t *)valptr));
}

void DrawHB(void)
{
	uint8_t BatLevel = SBData.Battery;
	uint16_t x_pos, y_pos = REDUCED_SB_POSY;
	if(SB_REDUCED_MODE_USE_RIGHT == SBData.mode)
		x_pos = REDUCED_SB_UR_POSX;
	else if(SB_REDUCED_MODE_USE_LEFT == SBData.mode)
		x_pos = REDUCED_SB_UL_POSX;
	else if(SB_REDUCED_MODE_USE_BOTTOM_RIGHT == SBData.mode) {
		x_pos = REDUCED_SB_DR_POSX;
		y_pos = REDUCED_SB_DLR_POSY;
	}
	else if(SB_REDUCED_MODE_USE_BOTTOM_LEFT == SBData.mode) {
		x_pos = REDUCED_SB_DL_POSX;
		y_pos = REDUCED_SB_DLR_POSY;
	}
	else {
		while(STALLE_ON_ERR);
		x_pos = REDUCED_SB_UR_POSX;
	}
	BatLevel = (BatLevel / (100/BATTERY_LEVELs)) + 1;
	if(BatLevel > BATTERY_LEVELs)
		 BatLevel = BATTERY_LEVELs;
	if(0 != SBResources[HB_PICs][HB_0 + BatLevel].hMemHandle) {
		GUI_MEMDEV_WriteAt(SBResources[HB_PICs][HB_0 + BatLevel].hMemHandle, \
			x_pos, y_pos);
	}
}

void OpenBatteryPopup(uint8_t mode)
{
	if(BAT_WARNING == mode) {
		SBData.BatWarn = TRUE;
		hWarnTimer      = \
			WM_CreateTimer(hWinStatus, ID_TIMER_SB_BAT_WARN, SB_BAT_WARN_ANIM_RES_MS, 0);
	}
	popPara.type = BATTERY_POPUP;	// Default popup type // 
	popPara.hParent = WM_HBKWIN;
	popPara.PopupData.BatPopupData.BatMode = mode;
	
	ShowPopup(&popPara);	// Open desired screen's pop-up // 
}


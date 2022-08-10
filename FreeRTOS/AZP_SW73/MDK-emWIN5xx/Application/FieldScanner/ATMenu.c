#include <string.h>
#include "lpc_types.h"
#include "lpc177x_8x_pinsel.h"
#include "lpc177x_8x_gpio.h"
#include <FreeRTOS.h>
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
#include "debug_frmwrk.h"
#include "monitor.h"
#include "Serial.h"
#include "GuiConsole.h"
#include "StatusBar.h"
#include "MyCheckbox.h"
#include "Popup.h"
#include "RuntimeLoader.h"
#include "APlay.h"
#include "SYSSettings.h"
#include "ATMenu.h"

#define AT_MENU_ANIM_MS	(250)

typedef struct {
	uint8_t ready4pkey;
	uint8_t icon_state;
    uint8_t ScreenExit;
    uint8_t first_time;
	WM_HTIMER hTimerAnim;
    WM_CALLBACK *OldDesktopCallback;
} AT_PARA;

typedef struct {
	uint16_t x;
	uint16_t y;
} sPoint;

static uint8_t __active_icon=0; // Active Icon Position Number //
static uint8_t __prev_icon=0;	// Previous active icon // 
static uint8_t new_page;
static AT_PARA *pPara;
static sPoint const win_up_points[AT_MENU_ICON_COUNT]= {
	{ICON1_LEFT_X, ICON1_LEFT_Y},
	{ICON2_LEFT_X, ICON2_LEFT_Y},
	{ICON3_LEFT_X, ICON3_LEFT_Y},
	{ICON4_LEFT_X, ICON4_LEFT_Y},
	{ICON5_LEFT_X, ICON5_LEFT_Y},
	{ICON6_LEFT_X, ICON6_LEFT_Y}
};

static void _cbBk_Desktop(WM_MESSAGE * pMsg);

/*********************************************************************
*
*       _RadialMenu
*
*  Function description:
*    Creates and executes a radial menu with motion support.
*/
uint8_t AT_Menu(void) 
{
	int xLCDSize;
	int yLCDSize;
	volatile uint8_t indx;

    pPara = (AT_PARA *)calloc(sizeof(AT_PARA), 1);
    if(NULL == pPara)
        while(STALLE_ON_ERR);
	if(0 != InitGroupRes(AT_MENU_PICs, 0xFF))	// AT_MENU_PICs Resource Initialization @ SDRAM for quick access //
		while(TODO_ON_ERR);

	pPara->ScreenExit = FALSE;
	new_page = AT_MENU;
	pPara->OldDesktopCallback = NULL;
	WM_HTIMER hTimerAnim = 0;
	pPara->icon_state = 0;
	__prev_icon = __active_icon;
	pPara->first_time = TRUE;
	pPara->ready4pkey = TRUE;
	
	WM_MULTIBUF_Enable(1);
	GUI_Clear();

	// Reduce size of desktop window to size of display
	xLCDSize = LCD_GetXSize();
	yLCDSize = LCD_GetYSize();
	WM_SetSize(WM_HBKWIN, xLCDSize, yLCDSize);
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);
	WM_SetFocus(WM_HBKWIN);
	
	SB_init(SB_REDUCED_MODE_USE_RIGHT);
  
	// Animation loop
	while (likely(FALSE == pPara->ScreenExit)) {
		if(!GUI_Exec1())  {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}

	// Do Deletion of created objects & Release of Resources // 
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_DeleteTimer(hTimerAnim);
	GUI_ClearKeyBuffer();
	SB_delete();
	
	free(pPara);
	pPara = NULL;
	return new_page;
}

static inline void __draw_string(void) {
	// Update active icon's text // 
	// Recover back string area //  
	uint8_t str_indxes[]={STR_ATM_MAN_FREQ, STR_ATM_OTO_FREQ, STR_ATM_DISTANCE, STR_ATM_VOL, STR_ATM_LIGHT, STR_ATM_LANG};
	if(0 != SBResources[AT_MENU_PICs][AT_MENU_STR_BACK].hMemHandle) {
		GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_MENU_STR_BACK].hMemHandle, \
			ACTIVE_ICON_STR_LEFT_X, ACTIVE_ICON_STR_LEFT_Y);
	}
	GUI_SetTextMode(GUI_TM_TRANS);
	GUI_SetColor(GUI_YELLOW);
	GUI_SetFont(APP_32B_FONT);
	GUI_RECT const gc = {ACTIVE_ICON_STR_LEFT_X, ACTIVE_ICON_STR_LEFT_Y, \
		ACTIVE_ICON_STR_LEFT_X + ACTIVE_ICON_STR_SIZE_X, ACTIVE_ICON_STR_LEFT_Y + ACTIVE_ICON_STR_SIZE_Y-2};
	GUI_DispStringInRectWrap(GetString(str_indxes[__prev_icon]), (GUI_RECT *)&gc, \
		GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
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
		case WM_PAINT:
			//GUI_SetBkColor(GUI_BLACK);
			//GUI_Clear();
			if(TRUE == pPara->first_time) {
			    pPara->first_time = FALSE;
				if(0 != SBResources[AT_MENU_PICs][AT_MENU_BACK].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][AT_MENU_BACK].hMemHandle, 0, 0);
				}
				for(volatile uint8_t indx=0 ; indx<AT_MENU_ICON_COUNT ; indx++) {
					// Toggle icon state & get the handle of icon picture that will be used // 
					the_res_indx = AT_MENU_POS1_SELECT + (indx*2) + ((__prev_icon==indx)?pPara->icon_state:0);
					// Draw the required icon picture // 
					if(0 != SBResources[AT_MENU_PICs][the_res_indx].hMemHandle) {
						GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][the_res_indx].hMemHandle, \
							win_up_points[indx].x, win_up_points[indx].y);
					}
				}
				__draw_string();
				// Start animation timer // 
				pPara->hTimerAnim = WM_CreateTimer(WM_HBKWIN, ID_TIMER_ATMENU_ANIM, AT_MENU_ANIM_MS, 0);
			} else {
				// redraw active icon new state // 
				the_res_indx = AT_MENU_POS1_SELECT + (__active_icon*2) + pPara->icon_state;
				if(0 != SBResources[AT_MENU_PICs][the_res_indx].hMemHandle) {
					GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][the_res_indx].hMemHandle, \
						win_up_points[__active_icon].x, win_up_points[__active_icon].y);
				}
				// redraw previcon if required // 
				if(__prev_icon != __active_icon) {
					__prev_icon = __active_icon;
					// Update prev icon // 
					for(volatile uint8_t indx=0 ; indx<AT_MENU_ICON_COUNT ; indx++) {
						// Toggle icon state & get the handle of icon picture that will be used // 
						if(__active_icon==indx)
							continue;
						the_res_indx = AT_MENU_POS1_SELECT + (indx*2);
						// Draw the required icon picture // 
						if(0 != SBResources[AT_MENU_PICs][the_res_indx].hMemHandle) {
							GUI_MEMDEV_WriteAt(SBResources[AT_MENU_PICs][the_res_indx].hMemHandle, \
								win_up_points[indx].x, win_up_points[indx].y);
						}
					}
					__draw_string();
				}
			}
			break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_ATMENU_ANIM: {
				    pPara->icon_state ^= 1;
					WM_RestartTimer(pMsg->Data.v, AT_MENU_ANIM_MS);
					WM_InvalidateWindow(WM_HBKWIN);
				}
				break;
				default:
					break;
			}
		}
		break;
		case WM_KEY:
			TRACEM("AT KEY Handler Working");
            if(TRUE != pPara->ready4pkey)
                break;  // DONT process key press events if GUI not ready //
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				uint8_t key_valid = TRUE;
				switch (pInfo->Key) {
					case KEY_RIGHT_EVENT:
 						if(0 < __active_icon)
 						   __active_icon--;
						else 
						    __active_icon= AT_MENU_ICON_COUNT-1;
						WM_InvalidateWindow(WM_HBKWIN);
						break;
					case KEY_LEFT_EVENT:
						if(__active_icon < (AT_MENU_ICON_COUNT-1))
						    __active_icon++;
						else 
						    __active_icon = 0;
						WM_InvalidateWindow(WM_HBKWIN);
						break;
					case KEY_OK_EVENT: {
							uint8_t at_screen_indxes[] = { \
								AT_MAN_FREQ_SCR, 
								AT_AUTO_FREQ_SCR, 
								AT_DISTANCE_SCR, 
								AT_VOL_SCR, 
								AT_BRIGHT_SCR, 
								AT_LANG_SCR
							};
							new_page = at_screen_indxes[__active_icon];
							pPara->ScreenExit = TRUE;
							break;
						}
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

void AT_enable_PWM(uint16_t at_pwm_freq, uint8_t pwm1_duty) 
{
	// Initialize PWM1 and PWM2 outputs //
		// AT_PWM1 : PWM0.1, P3.16
	// PWM HW initialized in BSP.c module // 
	BSP_set_pwm_freq_hz(0, at_pwm_freq);
	// Enable PWM1 outputs // 
	BSP_PWMSet(0, BSP_PWM_FS1, pwm1_duty);
}

void AT_disable_PWM(void) 
{
	// Disable PWM1 // 
	BSP_PWMSet(0, BSP_PWM_FS1, 0);
}

void AT_set_PWM(uint8_t pwm1_duty, uint8_t pwm2_duty) 
{
	// Change PWM1 duty cycles // 
	BSP_PWMSet(0, BSP_PWM_FS1, pwm1_duty);
}

// Because of a hardware design bug, this feature is not functional in first release // 
void AT_set_voltage_p2p(uint8_t p2p_voltage_def)
{
	static uint8_t first_time = TRUE;
	if(TRUE == first_time) {
		first_time = FALSE;
		// Set gpio & output mode for all pins //
		PINSEL_ConfigPin(AT_P2P_10V_ENABLE_PORT, AT_P2P_10V_ENABLE_PIN, 0);
		GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
		GPIO_SetDir(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN), 1);
		GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	

		PINSEL_ConfigPin(AT_P2P_15V_ENABLE_PORT, AT_P2P_15V_ENABLE_PIN, 0);
		GPIO_ClearValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
		GPIO_SetDir(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN), 1);
		GPIO_ClearValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	

		PINSEL_ConfigPin(AT_P2P_18V_ENABLE_PORT, AT_P2P_18V_ENABLE_PIN, 0);	
		GPIO_ClearValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
		GPIO_SetDir(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN), 1);
		GPIO_ClearValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
		#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
			PINSEL_ConfigPin(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, AT_VOLTAGE_OUTPUT_SHTDOWN_PIN, 0);	
			GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
			GPIO_SetDir(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN), 1);
			GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
		#endif
	}
	
	// set all pins to ZERO // 
	switch(p2p_voltage_def) {
		case AT_VOLTAGE_P2P_9V:
			#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
				GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
				App_waitMS(100);
			#endif
			GPIO_SetValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_SetValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
			App_waitMS(100);
			break;
		case AT_VOLTAGE_P2P_10V:
			#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
				GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
				App_waitMS(100);
			#endif
			GPIO_SetValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
			App_waitMS(100);
			break;
		case AT_VOLTAGE_P2P_15V:
			#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
				GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
				App_waitMS(100);
			#endif
			GPIO_SetValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
			App_waitMS(100);
			break;
		case AT_VOLTAGE_P2P_18V:
			#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
				GPIO_ClearValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
				App_waitMS(100);
			#endif
			GPIO_SetValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
			App_waitMS(100);
			break;
		case AT_VOLTAGE_P2P_0V:
			#if (TRUE == USE_PWM2_CH_AS_AT_VOLTAGE_DISABLE)
				GPIO_SetValue(AT_VOLTAGE_OUTPUT_SHTDOWN_PORT, (1<<AT_VOLTAGE_OUTPUT_SHTDOWN_PIN));	
				App_waitMS(100);
			#endif
			// Make volateg output same with minimum(9V) for safety // 
			GPIO_SetValue(AT_P2P_15V_ENABLE_PORT, (1<<AT_P2P_15V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_SetValue(AT_P2P_18V_ENABLE_PORT, (1<<AT_P2P_18V_ENABLE_PIN));	
			App_waitMS(100);
			GPIO_ClearValue(AT_P2P_10V_ENABLE_PORT, (1<<AT_P2P_10V_ENABLE_PIN));	
			App_waitMS(100);
			break;
		default:
			while(TODO_ON_ERR);
			break;
	}
}

/*************************** End of file ****************************/


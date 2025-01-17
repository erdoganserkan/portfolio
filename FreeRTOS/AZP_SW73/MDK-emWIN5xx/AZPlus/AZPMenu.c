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
#include "AZPMenu.h"

// New type definitions // 
typedef struct PARA_s {
	uint8_t ready4pkey;
	uint8_t gauge_is_valid;
	uint8_t ScreenExit;
	uint8_t Gauge;
	uint8_t GBState;
	uint32_t gb_start_time;
	uint32_t prev_key_press_time, new_key_press_time;
	WM_HWIN	hMidWin;
	WM_CALLBACK * OldDesktopCallback;
	GUI_RECT gRect, gRect2, gRect3, gRect4, gRect5, gRect6, gRect7;
	char val_str[16];
	char ferro_state_str[64];
	char man_gid_str[64];
	char slider_str[16];
	GUI_RECT slider_str_rect;
	uint8_t menu_depth;
	uint8_t dpt1_pos, dpt2_pos;
	uint8_t gb_state;
	uint8_t first_time;
	uint16_t man_gb_step;
	char gb_ins[128];	
	uint16_t calculated_depth;
	char scr_lines[4][128];
	WM_HTIMER hTimerAnim;
	uint16_t diffY, dirY;
	uint16_t vol_backup;
	char sign[16];
	uint16_t lineY[4];
	char line_str[4][16];
} PARA;

// Function Prototypes //
static void _cbBk_Desktop(WM_MESSAGE * pMsg);
static void AZPMenu_on_comm_timeout(void *msgp);
static void AZPMenu_on_msg_ready(void);
static void draw_setting_val(uint16_t active_val, uint16_t val_max);
static void draw_submenu(uint8_t aindx, uint8_t cnt, char **item_str);
static void draw_mangb(void);
static void draw_autogb(void);
static inline void start_dac_playing_mine(uint8_t search_type);
static void set_Aclk(uint16_t mineralization);

extern xQueueHandle	GUI_Task_Queue;		// GUI task main nessage queue // 
extern uint8_t active_page;	// Defined in GUIDEMO_Start.c //
static PARA* pPara = NULL;
static uint8_t new_page;
static uint16_t man_gb_step_range = AZP_MAN_GB_MINERAL_MAX - AZP_MAN_GB_MINERAL_MIN;
static uint16_t gid_phase_range = GROUND_ID_MAX - GROUND_ID_MIN;
static inline uint16_t mangbstep_2_phase(uint16_t man_gb_step);
static inline uint16_t phase_2_mangbstep(uint16_t phase);
static void draw_lang(uint8_t aindx);
static void draw_depth(uint8_t cnt, char **item_str);

// Draw Left & Right Bars and Filled Scope // 
static void _cbMidDraw(WM_MESSAGE * pMsg) {
  WM_HWIN     hWin;
  PARA      * pPara;
	WM_KEY_INFO * pInfo;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(pPara));

  switch (pMsg->MsgId) {
		case WM_PAINT: {
			uint16_t active_sb_pos;
			SB_setget_component(SB_AZP_ACTIVE_POS, FALSE, &active_sb_pos);
			// Draw Active Area Color // 
			GUI_SetBkColor(AZP_BACKGROUND_COLOR);
			GUI_Clear();
			GUI_SetColor(GUI_BLACK);
			GUI_DrawRoundedRect(5, 5, AZP_MENU_MIDWIN_SIZEX-5, AZP_MENU_MIDWIN_SIZEY-5, 2);
			GUI_DrawRoundedRect(6, 6, AZP_MENU_MIDWIN_SIZEX-6, AZP_MENU_MIDWIN_SIZEY-6, 2);
			GUI_SetColor(GUI_WHITE);
			GUI_FillRoundedRect(7, 7, AZP_MENU_MIDWIN_SIZEX-7, AZP_MENU_MIDWIN_SIZEY-7, 2);
			{
				char const *str = NULL;
				// Draw the SubMenu Name String // 
				GUI_SetColor(GUI_BLACK);
				GUI_DrawRoundedRect(AZP_SUBMEN_STR_POSX, AZP_SUBMEN_STR_POSY, AZP_SUBMEN_STR_ENDX, AZP_SUBMEN_STR_ENDY, 2);
				GUI_DrawRoundedRect(AZP_SUBMEN_STR_POSX+1, AZP_SUBMEN_STR_POSY+1, AZP_SUBMEN_STR_ENDX-1, AZP_SUBMEN_STR_ENDY-1, 2);
				if(0 == pPara->menu_depth) 
					GUI_DrawGradientRoundedH(AZP_SUBMEN_STR_POSX+2, AZP_SUBMEN_STR_POSY+2, AZP_SUBMEN_STR_ENDX-2, \
						AZP_SUBMEN_STR_ENDY-2, 2, GUI_WHITE, GUI_WHITE);
				else
					GUI_DrawGradientRoundedH(AZP_SUBMEN_STR_POSX+2, AZP_SUBMEN_STR_POSY+2, AZP_SUBMEN_STR_ENDX-2, \
						AZP_SUBMEN_STR_ENDY-2, 2, GUI_WHITE, GUI_WHITE);
				switch(active_sb_pos){
					case AZP_SB_POS_SYS_SET:
						str = GetString(STR_SYS_SETTINGs_INDX);
						break;
					case AZP_SB_POS_GB:
						if(2 == pPara->menu_depth) 
							str = (DPT1_POS_AUTO_GB == pPara->dpt1_pos) ? GetString(STR_AZP_AUTO_GB) : GetString(STR_AZP_MAN_GB);
						else 
							str = GetString(STR_BALANS_INDX);
						break;
					case AZP_SB_POS_SENS:
						str = GetString(STR_SENS_INDX);
						break;
					case AZP_SB_POS_VOL:
						str = GetString(STR_VOLUME_INDX);
						break;
					case AZP_SB_POS_BRIGHT:
						str = GetString(STR_BRIGHT_INDX);
						break;
					default:
						while(STALLE_ON_ERR);
						break;
				}
				if(NULL != str) {
					GUI_RECT gRect = {AZP_SUBMEN_STR_POSX, AZP_SUBMEN_STR_POSY, AZP_SUBMEN_STR_ENDX, AZP_SUBMEN_STR_ENDY};
					pPara->gRect = gRect;
					GUI_SetFont(APP_32B_FONT);
					GUI_SetColor(GUI_BLACK);
					GUI_SetTextMode(GUI_TM_TRANS);
					GUI_DispStringInRectWrap(str, &(pPara->gRect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				}
			}
			if(1 == pPara->menu_depth) {
				uint16_t active_val = 0;
				switch(active_sb_pos){
					case AZP_SB_POS_SYS_SET: {
						// Draw system settings sub menu // 
						uint16_t ferro_state = APP_GetValue(ST_FERROs);
						char *items[4];
						memset(pPara->ferro_state_str, 0, sizeof(pPara->ferro_state_str));
						snprintf(pPara->ferro_state_str, sizeof(pPara->ferro_state_str)-1, "%s : %s", GetString(STR_FERROS_INDX), \
							(FERROS_DISABLED == ferro_state) ? GetString(STR_AZP_DISABLED) : GetString(STR_AZP_ENABLED));
						items[0] = pPara->ferro_state_str;
						items[1] = (char *)GetString(STR_DEPTH_CALC_INDX);
						items[2] = (char *)GetString(STR_LANG_INDX);
						items[3] = (char *)GetString(STR_FACTORY_INDX);
						draw_submenu(pPara->dpt1_pos, 4, items);
					} break;
					case AZP_SB_POS_GB:  {
						// Draw Manual & Automatic GB selection Menu
						char *items[2];
						items[0] = (char *)GetString(STR_AZP_AUTO_GB);
						items[1] = (char *)GetString(STR_AZP_MAN_GB);
						draw_submenu(pPara->dpt1_pos, 2, items);
					} break;
					case AZP_SB_POS_SENS:
						draw_setting_val(pPara->dpt1_pos, (AZP_SYS_SET_MENU_MAX));
						break;
					case AZP_SB_POS_VOL:
						draw_setting_val(pPara->dpt1_pos, (AZP_SYS_SET_MENU_MAX));
						break;
					case AZP_SB_POS_BRIGHT:
						draw_setting_val(pPara->dpt1_pos, (AZP_SYS_SET_MENU_MAX));
						break;
					default:
						while(STALLE_ON_ERR);
						break;
				}
			}
			else if(2 == pPara->menu_depth) {
					if(AZP_SB_POS_SYS_SET == active_sb_pos) {
						switch(pPara->dpt1_pos) {
							case AZP_SYS_DEPTH:	{
								// draw diameter on upper line // 
								// draw depth result on lower line // 
								memset(pPara->scr_lines, 0, sizeof(pPara->scr_lines));
								sprintf(pPara->scr_lines[0], "%s:", GetString(STR_SIZE_SELECT_INDX));
								sprintf(pPara->scr_lines[1], "%u x %u", 
									AZP_DEPTH_TARGET_SIDE_STEP_CM * pPara->dpt2_pos, AZP_DEPTH_TARGET_SIDE_STEP_CM * pPara->dpt2_pos);
								uint8_t line_cnt = 2;
								if(0x1FF == pPara->calculated_depth) { // initial case // 
										sprintf(pPara->scr_lines[2], "%s", GetString(STR_DEPTH_RESULT_INDX));
										sprintf(pPara->scr_lines[3], "--");
								} else if(0xFF == pPara->calculated_depth) { 	// error case // 
										sprintf(pPara->scr_lines[2], "%s", GetString(STR_DEPTH_RESULT_INDX));
										sprintf(pPara->scr_lines[3], "XX");
										line_cnt = 4;
								} else {	// succesfully calculated result // 
										sprintf(pPara->scr_lines[2], "%s", GetString(STR_DEPTH_RESULT_INDX), pPara->calculated_depth);
										sprintf(pPara->scr_lines[3], "%u cm", pPara->calculated_depth);
										line_cnt = 4;
								}
								char *items[4];
								items[0] = pPara->scr_lines[0];
								items[1] = pPara->scr_lines[1];
								items[2] = pPara->scr_lines[2];
								items[3] = pPara->scr_lines[3];
								draw_depth(line_cnt, items);	// no active selection on menu, just display strings //
							} break;
							case AZP_SYS_LANG: 
								draw_lang(pPara->dpt2_pos);
								break;
							default:
								while(STALLE_ON_ERR);
								break;
						}
					} else if(AZP_SB_POS_GB == active_sb_pos) {
						// Draw Manual & Automatic GB selection Menu
						if(DPT1_POS_AUTO_GB == pPara->dpt1_pos)
							draw_autogb();
						else 
							draw_mangb();
					} else  {
						while(STALLE_ON_ERR);
						break;
					}
			} break;
		}
		break;
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_POST_PAINT:
			pPara->ready4pkey = TRUE;
			if(TRUE == pPara->first_time) {
				pPara->first_time = FALSE;
				BSP_PWMSet(0, BSP_PWM_LCD, APP_GetValue(ST_BRIGHT));	// Apply stored LCD Backlight level // 
			}
			break;
		case WM_KEY:
			TRACEM("AZPMenu KEY Handler Working");
			if(TRUE != pPara->ready4pkey)
				break;	// DONT process key press events if GUI not ready // 
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				pPara->prev_key_press_time = pPara->new_key_press_time;
				pPara->new_key_press_time = GUI_X_GetTime();
				uint8_t key_valid = TRUE;
				uint16_t active_sb_pos;
				SB_setget_component(SB_AZP_ACTIVE_POS, FALSE, &active_sb_pos);
				switch (pInfo->Key) {
				//---------------------------------------------//
				//-------------- ENTER KEY --------------------//
				//---------------------------------------------//
					case KEY_AZP_ENTER_EVENT: {
						if(0 == pPara->menu_depth) {
							pPara->menu_depth++;
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:
									pPara->dpt1_pos = 1;
									break;
								case AZP_SB_POS_GB:
									pPara->dpt1_pos = 0;
									break;
								case AZP_SB_POS_BRIGHT: {
									uint16_t bright;
									SB_setget_component(SB_BRIGHTNESS, FALSE, &bright);
									pPara->dpt1_pos = bright/(BRIGHTNESS_MAX/AZP_SYS_SET_MENU_MAX);
								} break;
								case AZP_SB_POS_SENS:{
									uint16_t sense;
									SB_setget_component(SB_SENSITIVITY, FALSE, &sense);
									pPara->dpt1_pos = sense/(SENSITIVITY_MAX/AZP_SYS_SET_MENU_MAX);
								} break;
								case AZP_SB_POS_VOL:{
									uint16_t vol;
									SB_setget_component(SB_VOLUME, FALSE, &vol);
									pPara->dpt1_pos = vol/(VOLUME_MAX/AZP_SYS_SET_MENU_MAX);
								} break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						}
						else if(1 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:
									switch(pPara->dpt1_pos) {
										case AZP_SYS_FERRO_ELIMINATE: {
											uint16_t ferro_state = APP_GetValue(ST_FERROs);
											if(FERROS_DISABLED == ferro_state)
												ferro_state = FERROS_ENABLED;
											else
												ferro_state = FERROS_DISABLED;
											APP_SetVal(ST_FERROs, ferro_state, TRUE);
											WM_InvalidateWindow(pPara->hMidWin);
											pPara->ready4pkey = FALSE;
											{
												UmdPkt_Type send_msg;
												send_msg.cmd = CMD_SET_FERROS_STATE;
												send_msg.length = 3;
												send_msg.data.ferros_state = ferro_state;
												UARTSend((uint8_t *)&send_msg, \
													UMD_CMD_TIMEOUT_SET_FERRO_MS, AZPMenu_on_comm_timeout);
											}
										} break;
										case AZP_SYS_DEPTH:
											pPara->menu_depth++;
											pPara->dpt2_pos = 0;	// startup diameter is "5x5 cm" // 
											WM_InvalidateWindow(pPara->hMidWin);
											pPara->ready4pkey = FALSE;
											break;
										case AZP_SYS_LANG: 
											pPara->dpt2_pos = APP_GetValue(ST_LANG);
											pPara->menu_depth++;
											WM_InvalidateWindow(pPara->hMidWin);
											pPara->ready4pkey = FALSE;
											break;
										case AZP_SYS_FACTORY: {
											uAppStore test;
											App_ReloadSettings(&test, RELOAD_FROM_SAFE_VALUEs);
											APP_StoreSettings(&test, TRUE);
											pPara->ScreenExit = SCR_EXIT_CONFIRMED;
											new_page = AZP_LOADING_SCR;
											App_SetHWTypeStates(FALSE, FALSE);	// power off filed scanner and enable dedector // 
											App_waitMS(100);
											App_SetHWTypeStates(TRUE, FALSE);	// power off filed scanner and enable dedector // 
										}
										break;
										default:
											while(STALLE_ON_ERR);
											break;
									}
									break;
								case AZP_SB_POS_GB:
									pPara->menu_depth++;
									pPara->Gauge = (UMD_GAUGE_MAX/2);
									pPara->dirY = 1;
									pPara->diffY = 0;
									// 1- send gb start cmd to dedector // 
									{ 
										UmdPkt_Type msg;
										msg.cmd = CMD_START_GROUND_BALANCE;
										msg.length = 3;
										msg.data.gb_type = \
											(DPT1_POS_AUTO_GB == pPara->dpt1_pos) ? GB_TYPE_LONG : GB_TYPE_AZP_MANUAL;
										pPara->gb_start_time = GUI_X_GetTime();
										UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS*2, AZPMenu_on_comm_timeout);
									}
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
									break;
								case AZP_SB_POS_BRIGHT: {
									uint16_t bright = pPara->dpt1_pos * (BRIGHTNESS_MAX / AZP_SYS_SET_MENU_MAX);
									SB_setget_component(SB_BRIGHTNESS, TRUE, &bright);
									APP_SetVal(ST_BRIGHT, bright, TRUE);
									pPara->menu_depth = 0;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
								} break;
								case AZP_SB_POS_SENS:{
									uint16_t sense = pPara->dpt1_pos * (SENSITIVITY_MAX / AZP_SYS_SET_MENU_MAX);
									SB_setget_component(SB_SENSITIVITY, TRUE, &sense);
									APP_SetVal(ST_SENS, sense, TRUE);
									pPara->menu_depth = 0;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
									{
										UmdPkt_Type send_msg;
										send_msg.cmd = CMD_SET_SENSITIVITY;
										send_msg.length = 3;
										send_msg.data.sensitivity = sense;
										UARTSend((uint8_t *)&send_msg, \
											UMD_CMD_TIMEOUT_SET_SENSE_MS, AZPMenu_on_comm_timeout);
									}
								} break;
								case AZP_SB_POS_VOL:{
									uint16_t vol = pPara->dpt1_pos * (VOLUME_MAX / AZP_SYS_SET_MENU_MAX);
									SB_setget_component(SB_VOLUME, TRUE, &vol);
									APP_SetVal(ST_VOL, vol, TRUE);
									if(0 != InitGroupRes(AUDIO_TRACs, 0xFF))
										while(TODO_ON_ERR);
									pPara->menu_depth = 0;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
								} break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
						else if(2 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:
									switch(pPara->dpt1_pos) {
										case AZP_SYS_DEPTH: {
											// Send active diameter to detector 
											pPara->calculated_depth = 0x1FF;	// initial state for paint event // 
											uint8_t x = (pPara->dpt2_pos * AZP_DEPTH_TARGET_SIDE_STEP_CM);
											UmdPkt_Type msg;
											msg.cmd = CMD_START_DEPTH_CALCULATION;
											msg.length = 4; // CMD(U8) + LENGTH(U8) + U8(x) + U8(y)// 
											msg.data.Depth_Params.width = x;
											msg.data.Depth_Params.Height = x;
											UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_DEPTH_CALC_MS, AZPMenu_on_comm_timeout);		
											WM_InvalidateWindow(pPara->hMidWin);	// Do cleaning of previous results // 
											pPara->ready4pkey = FALSE;
										} break;
										case AZP_SYS_LANG: {
											struct WM_MESSAGE msg;
											// Store new language selection //
											APP_SetVal(ST_LANG, pPara->dpt2_pos, TRUE);
											// Send GUI_USER_LANG_CHANGED to parent window // 
											msg.hWin = pPara->hMidWin;
											msg.MsgId = GUI_USER_LANG_CHANGED;
											msg.Data.v = pPara->dpt2_pos;
											WM_SendMessage(msg.hWin, &msg);
											pPara->menu_depth = 1;
										} break;
										default:
											while(STALLE_ON_ERR);
											break;
									}
									break;
								case AZP_SB_POS_GB:{
									if(DPT1_POS_AUTO_GB == pPara->dpt1_pos) {	// AutoGB Selected // 
										if(GB_END_WAIT == pPara->GBState) {
											pPara->GBState = GB_IDLE;
											new_page = AZP_ALL_METAL_SCR;
											pPara->ScreenExit = SCR_EXIT_CONFIRMED;
										} else {
											// send RESET command to analog mcu // 
											{
												UmdPkt_Type msg;
												msg.cmd = IND_ANALOG_RESET;
												msg.length = 2; // CMD(U8) + LENGTH(U8) // 
												UARTSend((uint8_t *)&msg, 0, NULL);
											}
										}
									} else if(DPT1_POS_MAN_GB == pPara->dpt1_pos) {	// Man GB Selected // 
										uint16_t mineralization = mangbstep_2_phase(pPara->man_gb_step);
										SB_setget_component(SB_GROUND_ID, TRUE, &(mineralization));
										APP_SetVal(ST_GROUND_ID, mineralization, TRUE);
										pPara->GBState = GB_CONFIRM_REQ;
										if(TRUE == is_dac_playing()) {
											WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
											WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
										}
										DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
										UmdPkt_Type msg;
										msg.cmd = CMD_STOP_GROUND_BALANCE;
										msg.length = 2; // CMD(U8) + LENGTH(U8) // 
										UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS*2, AZPMenu_on_comm_timeout);											
 									}
								} break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
					}
					break;
					//---------------------------------------------//
					//-------------- MINUS KEY --------------------//
					//---------------------------------------------//
					case KEY_MINUS_EVENT: {
						if(0 == pPara->menu_depth) {
							if(active_sb_pos)
								active_sb_pos--;
							else
								active_sb_pos = AZP_SB_POS_BRIGHT;
							SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						} else if(1 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:{
									if(AZP_SYS_MAX_POS > pPara->dpt1_pos)
										pPara->dpt1_pos++;
									else 
										pPara->dpt1_pos = 0;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
								} break;
								case AZP_SB_POS_GB:
									if(DPT1_POS_MAN_GB == pPara->dpt1_pos)
										pPara->dpt1_pos = DPT1_POS_AUTO_GB;
									else
										pPara->dpt1_pos = DPT1_POS_MAN_GB;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
									break;
								case AZP_SB_POS_VOL:
									if(pPara->dpt1_pos) {
										pPara->dpt1_pos--;
										{
											// Set NEW Volume Level and play sample sound //
											APP_SetVal(ST_VOL, pPara->dpt1_pos * (VOLUME_MAX/ AZP_SYS_SET_MENU_MAX), FALSE);
											if(0 != InitGroupRes(AUDIO_TRACs, SAMPLE_SOUND))
												while(TODO_ON_ERR);
											start_dac_audio(SAMPLE_SOUND, TRUE);
											key_valid = FALSE;
										}
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								case AZP_SB_POS_BRIGHT:
									if(pPara->dpt1_pos) {
										pPara->dpt1_pos--;
										BSP_PWMSet(0, BSP_PWM_LCD, pPara->dpt1_pos * (BRIGHTNESS_MAX / AZP_SYS_SET_MENU_MAX));
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								case AZP_SB_POS_SENS:
									if(pPara->dpt1_pos) {
										pPara->dpt1_pos--;
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
						else if(2 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:{
									switch(pPara->dpt1_pos) {
										case AZP_SYS_DEPTH: 
											if(0 != pPara->dpt2_pos) {	// Decrease the dpt2_pos and giu action 
												--pPara->dpt2_pos;
												pPara->calculated_depth = 0x1FF;	// initial state for paint event // 
												WM_InvalidateWindow(pPara->hMidWin);
												pPara->ready4pkey = FALSE;
											}
											break;
										case AZP_SYS_LANG:
											if((LANG_MAX) > pPara->dpt2_pos) 
												pPara->dpt2_pos++;
											else 
												pPara->dpt2_pos = 0;
											WM_InvalidateWindow(pPara->hMidWin);
											pPara->ready4pkey = FALSE;
											break;
										default:
											while(STALLE_ON_ERR);
											break;
									}
								} break;
								case AZP_SB_POS_GB:	
									if(DPT1_POS_AUTO_GB == pPara->dpt1_pos) {
										// Pressing MINUS during Auto-GB is useless // 
									} else if(DPT1_POS_MAN_GB == pPara->dpt1_pos) {
										uint8_t decrement_req;
										uint32_t last_key_diff = pPara->new_key_press_time - pPara->prev_key_press_time;
										if(1500 < last_key_diff)
											decrement_req = 1;
										else if((last_key_diff > 750) && (last_key_diff < 1500))
											decrement_req = 5;
										else 
											decrement_req = 10;
										if(pPara->man_gb_step >= (AZP_MAN_GB_MINERAL_MIN+decrement_req))
											pPara->man_gb_step -= decrement_req;
										else
											pPara->man_gb_step = AZP_MAN_GB_MINERAL_MIN;
										set_Aclk(pPara->man_gb_step);
										// send RESET command to analog mcu // 
										{
											UmdPkt_Type msg;
											msg.cmd = IND_ANALOG_RESET;
											msg.length = 2; // CMD(U8) + LENGTH(U8) // 
											UARTSend((uint8_t *)&msg, 0, NULL);
										}
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
					}
					break;
					//---------------------------------------------//
					//-------------- PLUS KEY --------------------//
					//---------------------------------------------//
					case KEY_PLUS_EVENT: {
						if(0 == pPara->menu_depth) {
							if(AZP_SB_POS_BRIGHT > active_sb_pos)
								active_sb_pos++;
							else
								active_sb_pos = 0;
							SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						} else if(1 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:{
									if(pPara->dpt1_pos)
										pPara->dpt1_pos--;
									else 
										pPara->dpt1_pos = AZP_SYS_MAX_POS;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
								} break;
								case AZP_SB_POS_GB:
									if(DPT1_POS_MAN_GB == pPara->dpt1_pos)
										pPara->dpt1_pos = DPT1_POS_AUTO_GB;
									else
										pPara->dpt1_pos = DPT1_POS_MAN_GB;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
									break;
								case AZP_SB_POS_VOL:
									if(AZP_SYS_SET_MENU_MAX > pPara->dpt1_pos) {
										pPara->dpt1_pos++;
										{
											// Set NEW Volume Level and play sample sound //
											APP_SetVal(ST_VOL, pPara->dpt1_pos * (VOLUME_MAX/ AZP_SYS_SET_MENU_MAX), FALSE);
											if(0 != InitGroupRes(AUDIO_TRACs, SAMPLE_SOUND))
												while(TODO_ON_ERR);
											start_dac_audio(SAMPLE_SOUND, TRUE);
											key_valid = FALSE;
										}
										WM_InvalidateWindow(pPara->hMidWin);
									} break;
								case AZP_SB_POS_BRIGHT:
									if(AZP_SYS_SET_MENU_MAX > pPara->dpt1_pos) {
										pPara->dpt1_pos++;
										BSP_PWMSet(0, BSP_PWM_LCD, pPara->dpt1_pos * (BRIGHTNESS_MAX / AZP_SYS_SET_MENU_MAX));
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								case AZP_SB_POS_SENS:
									if(AZP_SYS_SET_MENU_MAX > pPara->dpt1_pos) {
										pPara->dpt1_pos++;
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
						else if(2 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_SYS_SET:{
									switch(pPara->dpt1_pos) {
										case AZP_SYS_DEPTH: 
											if((AZP_DEPTH_MAX_TARGET_SIDE_CM/AZP_DEPTH_TARGET_SIDE_STEP_CM) > pPara->dpt2_pos) {	// Decrease the dpt2_pos and giu action 
												++pPara->dpt2_pos;
												pPara->calculated_depth = 0x1FF;	// initial state for paint event // 
												WM_InvalidateWindow(pPara->hMidWin);
												pPara->ready4pkey = FALSE;
											}
											break;
										case AZP_SYS_LANG:
											if(pPara->dpt2_pos) 
												pPara->dpt2_pos--;
											else 
												pPara->dpt2_pos = LANG_MAX;
											WM_InvalidateWindow(pPara->hMidWin);
											pPara->ready4pkey = FALSE;
											break;
										default:
											while(STALLE_ON_ERR);
											break;
									}
								} break;
								case AZP_SB_POS_GB:	
									if(DPT1_POS_AUTO_GB == pPara->dpt1_pos) {
										// Pressing PLUS during Auto-GB is useless // 
									} else if(DPT1_POS_MAN_GB == pPara->dpt1_pos) {
										uint8_t increment_req;
										uint32_t last_key_diff = pPara->new_key_press_time - pPara->prev_key_press_time;
										if(1500 < last_key_diff)
											increment_req = 1;
										else if((last_key_diff > 750) && (last_key_diff < 1500))
											increment_req = 5;
										else 
											increment_req = 10;
										if(pPara->man_gb_step <= (AZP_MAN_GB_MINERAL_MAX-increment_req))
											pPara->man_gb_step += increment_req;
										else
											pPara->man_gb_step = AZP_MAN_GB_MINERAL_MAX;
										set_Aclk(pPara->man_gb_step);
										// send RESET command to analog mcu // 
										{
											UmdPkt_Type msg;
											msg.cmd = IND_ANALOG_RESET;
											msg.length = 2; // CMD(U8) + LENGTH(U8) // 
											UARTSend((uint8_t *)&msg, 0, NULL);
										}
										WM_InvalidateWindow(pPara->hMidWin);
										pPara->ready4pkey = FALSE;
									}
									break;
								default:
									while(STALLE_ON_ERR);
									break;
							}
						}
					}
					break;
					//---------------------------------------------//
					//-------------- MENU/ESC KEY -----------------//
					//---------------------------------------------//
					case KEY_MENU_EVENT: {	// Return to prev_page // 
						if(0 == pPara->menu_depth) {
							extern uint8_t prev_page;	// defined in GUIDEMO_Start.c // 
							new_page = prev_page;
							uint16_t active_sb_pos = 0xFF;
							SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
							pPara->ScreenExit = SCR_EXIT_CONFIRMED;
						} else if(1 == pPara->menu_depth) {
							pPara->dpt1_pos = 0;
							pPara->menu_depth--;
							switch(active_sb_pos) {
								case AZP_SB_POS_VOL: {
									APP_SetVal(ST_VOL, pPara->vol_backup, FALSE);	// Dont store to flash, only change in RAM //
									if(0 != InitGroupRes(AUDIO_TRACs, SAMPLE_SOUND))
										while(TODO_ON_ERR);
								}
								break;
								case AZP_SB_POS_BRIGHT: {
									uint16_t bright_backup = APP_GetValue(ST_BRIGHT);
									BSP_PWMSet(0, BSP_PWM_LCD, bright_backup);
								} break;
								default:
									break;
							}
							WM_InvalidateWindow(pPara->hMidWin);
							pPara->ready4pkey = FALSE;
						} else if(2 == pPara->menu_depth) {
							switch(active_sb_pos) {
								case AZP_SB_POS_GB:{
									pPara->GBState = GB_CANCEL_REQ;
									if(TRUE == is_dac_playing()) {
										WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
										WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
									}
									DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
									UmdPkt_Type msg;
									msg.cmd = CMD_STOP_GROUND_BALANCE;
									msg.length = 2; // CMD(U8) + LENGTH(U8) // 
									UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_GB_CMD_MS*2, AZPMenu_on_comm_timeout);
								} break;
								default:
									pPara->menu_depth--;
									WM_InvalidateWindow(pPara->hMidWin);
									pPara->ready4pkey = FALSE;
									break;

							}
						} else 
							while(STALLE_ON_ERR);
					}
					if((SCR_EXIT_REQUESTED == pPara->ScreenExit) ||(SCR_EXIT_CONFIRMED == pPara->ScreenExit))
						BSP_PWMSet(0, BSP_PWM_LCD, 0);	
					break;
					default:	
						key_valid = FALSE;
						break;
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
		case WM_TIMER: {
			int Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_AZP_GB_ANIM: {
					if((2 == pPara->menu_depth) && (DPT1_POS_AUTO_GB == pPara->dpt1_pos) && (GB_END_WAIT != pPara->GBState)) {
						if(pPara->dirY) {
							if(pPara->diffY < (AZP_SUBMEN_ENDY-AZP_SUBMEN_POSY-AZP_GB_COIL_SIZEY-10))
								pPara->diffY += 10;
							else
								pPara->dirY = 0;
						} else {
							if(pPara->diffY >= 10)
								pPara->diffY -= 10;
							else
								pPara->dirY = 1;
						}
						WM_RestartTimer(pMsg->Data.v, AZP_GB_ANIM_MS);
						WM_InvalidateWindow(hWin);
					}
				}
				break;
				default:	// Ignore silently // 
					break;
			}
		}
		break;
		case WM_DELETE:
			if(-1 != pPara->hTimerAnim)
				WM_DeleteTimer(pPara->hTimerAnim);
			break;
		case GUI_USER_LANG_CHANGED: {
			INFOM("LANG CHANGE evet received\n");				
			//volatile uint8_t indx;
			//for(indx=0 ; indx<SYS_SETTINGs_ICON_COUNT ; indx++)
				//pPara->ScrStrs[indx] = GetString(STR_SYS_SETTINGS_START + indx);
			WM_InvalidateWindow(hWin);
			WM_InvalidateWindow(WM_HBKWIN);
		}
		break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
} 

static void draw_slider(GUI_RECT location, uint8_t gauge, GUI_COLOR bckgrnd, GUI_COLOR slider, uint8_t slider_size_diff) {
	#if(1)
	uint16_t midx = (location.x0 + location.x1)/2;
	uint16_t sizex = (location.x1 - location.x0);
	// Draw horizontal bar as gradient and its border // 
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(location.x0-1, location.y0-1, location.x1+1, location.y1+1, 2);
	GUI_DrawGradientRoundedH(location.x0, location.y0, midx, location.y1, 2, bckgrnd, bckgrnd);
	GUI_DrawGradientRoundedH(midx, location.y0, location.x1, location.y1, 2, bckgrnd, bckgrnd);
	// locate slider // 
	uint16_t posX;
	if((UMD_GAUGE_MAX/2) < gauge) {
		uint16_t diff_g = gauge-(UMD_GAUGE_MAX/2);
		posX = midx + (diff_g * (midx-location.x0))/(UMD_GAUGE_MAX/2);
		{
			memset(pPara->slider_str, 0, sizeof(pPara->slider_str));
			snprintf(pPara->slider_str, sizeof(pPara->slider_str)-1, "+%u\n", diff_g);
			GUI_RECT temp = {location.x1-(sizex/5), location.y0, location.x1, location.y1};
			pPara->slider_str_rect = temp;
			GUI_SetFont(APP_32B_FONT);
			GUI_SetColor(GUI_WHITE);
			GUI_DispStringInRectWrap(pPara->slider_str, &(pPara->slider_str_rect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
		}
	} else if((UMD_GAUGE_MAX/2) > gauge) {
		uint16_t diff_g = (UMD_GAUGE_MAX/2) - gauge;
		posX = midx - (diff_g * (midx-location.x0))/(UMD_GAUGE_MAX/2);
		{
			memset(pPara->slider_str, 0, sizeof(pPara->slider_str));
			snprintf(pPara->slider_str, sizeof(pPara->slider_str)-1, "-%u\n", diff_g);
			GUI_RECT temp = {location.x0, location.y0, location.x0 + (sizex/5), location.y1};
			pPara->slider_str_rect = temp;
			GUI_SetFont(APP_32B_FONT);
			GUI_SetColor(GUI_WHITE);
			GUI_DispStringInRectWrap(pPara->slider_str, &(pPara->slider_str_rect), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
		}
	} else {
		posX = midx;
	}
	// Draw Slider border // 
	GUI_SetColor(GUI_WHITE);
	GUI_DrawRoundedRect(posX-10, location.y0-(slider_size_diff+1), posX+10, location.y1 + (slider_size_diff+1), 2);
	// Draw Sliders Color // 
	GUI_SetColor(slider);
	GUI_FillRoundedRect(posX-9, location.y0 - slider_size_diff, posX+9, location.y1 + slider_size_diff, 2);	
	#endif
}

static void draw_mangb(void)
{
	// Draw submenu border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, AZP_SUBMEN_ENDX+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, AZP_SUBMEN_ENDX+1, AZP_SUBMEN_ENDY+1, 2);

	// Draw GB intruction window - part1 // 
	// Draw submenu border // 
	uint16_t up_item_sizey = (AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/3;
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX, AZP_SUBMEN_POSY, AZP_SUBMEN_ENDX, AZP_SUBMEN_POSY+ up_item_sizey, 2);
	// Draw submenu color // 
	GUI_SetColor(GUI_BLACK);
	GUI_FillRoundedRect(AZP_SUBMEN_POSX+2, AZP_SUBMEN_POSY+2, AZP_SUBMEN_ENDX-2, AZP_SUBMEN_POSY + up_item_sizey-2, 2);
	// Draw submenu string // 
	GUI_RECT gRect = {AZP_SUBMEN_POSX, AZP_SUBMEN_POSY, AZP_SUBMEN_ENDX, AZP_SUBMEN_POSY + up_item_sizey};
	pPara->gRect4 = gRect;
	GUI_SetColor(GUI_WHITE);
	GUI_SetTextMode(GUI_TM_TRANS);
	{
		uint16_t mgb_step = 0;
		memset(pPara->man_gid_str, 0, sizeof(pPara->man_gid_str));
		memset(pPara->sign, 0, sizeof(pPara->sign));
		if((pPara->man_gb_step) > (AZP_MAN_GB_MINERAL_MAX/2)) {
			mgb_step = pPara->man_gb_step - (AZP_MAN_GB_MINERAL_MAX/2);
			snprintf(pPara->sign, sizeof(pPara->sign)-1, "+");
		} else if((pPara->man_gb_step) < (AZP_MAN_GB_MINERAL_MAX/2)){
			mgb_step = (AZP_MAN_GB_MINERAL_MAX/2) - pPara->man_gb_step;
			snprintf(pPara->sign, sizeof(pPara->sign)-1, "-");
		}
			
		snprintf(pPara->man_gid_str, sizeof(pPara->man_gid_str)-1, "%s : %u\n", GetString(STR_AZP_GB_MAN_NUM), mgb_step);
	}
	GUI_SetFont(&GUI_FontD48);
	GUI_DispStringAt(pPara->sign, AZP_SUBMEN_POSX+20, AZP_SUBMEN_POSY-5);
	GUI_DispStringAt(pPara->sign, AZP_SUBMEN_ENDX-60, AZP_SUBMEN_POSY-5);

	GUI_SetFont(APP_24B_FONT);
	GUI_DispStringInRectWrap(pPara->man_gid_str, &(pPara->gRect4), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

	uint16_t down_item_sizey = ((AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/3);
	GUI_RECT location = {AZP_SUBMEN_POSX, AZP_SUBMEN_POSY + up_item_sizey + 20, AZP_SUBMEN_ENDX, AZP_SUBMEN_POSY+ up_item_sizey + down_item_sizey+20};
	draw_slider(location, pPara->Gauge, GUI_BROWN, GUI_BLACK, 10);
}

static void draw_autogb(void)
{
	// Draw submenu border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, AZP_SUBMEN_ENDX+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, AZP_SUBMEN_ENDX+1, AZP_SUBMEN_ENDY+1, 2);

	// Divide Submenu into 3 pages // 
	uint16_t sizex = (AZP_SUBMEN_ENDX - AZP_SUBMEN_POSX)-AZP_GB_COIL_SIZEX;
	uint16_t str_startx = AZP_SUBMEN_POSX + AZP_GB_COIL_SIZEX;
	uint16_t sizey = ((AZP_SUBMEN_ENDY-AZP_SUBMEN_POSY)/3);

	// Draw coil animation (GUI_MEMDEV_WriteAt() uses real pixel point on whole LCD)
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, str_startx+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, str_startx+1, AZP_SUBMEN_ENDY+1, 2);
	GUI_DrawGradientRoundedV(AZP_SUBMEN_POSX+1, AZP_SUBMEN_POSY+1, str_startx-1, AZP_SUBMEN_ENDY-1, 2, GUI_DARKGRAY, GUI_WHITE);
	// Draw Coil height reference lines // 
	uint8_t pen = GUI_GetPenSize();
	GUI_SetPenSize(2);
	GUI_DrawLine(AZP_SUBMEN_POSX-20, pPara->lineY[0], AZP_SUBMEN_POSX+5, pPara->lineY[0]);
	GUI_DrawLine(AZP_SUBMEN_POSX-20, pPara->lineY[1], AZP_SUBMEN_POSX+5, pPara->lineY[1]);
	GUI_DrawLine(AZP_SUBMEN_POSX-20, pPara->lineY[2], AZP_SUBMEN_POSX+5, pPara->lineY[2]);
	GUI_DrawLine(AZP_SUBMEN_POSX-20, pPara->lineY[3], AZP_SUBMEN_POSX+5, pPara->lineY[3]);
	GUI_SetPenSize(pen);
	GUI_SetColor(GUI_BLACK);
	GUI_SetFont(APP_19B_FONT);
	GUI_DispStringAt(pPara->line_str[0], AZP_SUBMEN_POSX-20, pPara->lineY[0]-17);
	GUI_DispStringAt(pPara->line_str[1], AZP_SUBMEN_POSX-20, pPara->lineY[1]-17);
	GUI_DispStringAt(pPara->line_str[2], AZP_SUBMEN_POSX-20, pPara->lineY[2]-17);
	GUI_DispStringAt(pPara->line_str[3], AZP_SUBMEN_POSX-20, pPara->lineY[3]-17);
	// Draw coil object // 
	uint16_t coil_posY;
	if(GB_END_WAIT != pPara->GBState) 
		coil_posY = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY + pPara->diffY;
	else
		coil_posY = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY;
	if(0 != SBResources[AZP_GB_PICs][AZP_GB_COIL].hMemHandle) {
		GUI_MEMDEV_WriteAt(SBResources[AZP_GB_PICs][AZP_GB_COIL].hMemHandle, AZP_SUBMEN_POSX, coil_posY);
	}
	// 
	{
		// pos0 : scr0 //
		GUI_RECT gRect5 = {AZP_SUBMEN_POSX+AZP_GB_COIL_SIZEX, AZP_SUBMEN_POSY, AZP_SUBMEN_ENDX, AZP_SUBMEN_POSY+sizey};
		pPara->gRect5 = gRect5;
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(gRect5.x0+2,gRect5.y0+2,gRect5.x1-2,gRect5.y1-2,2);
		GUI_SetColor(GUI_WHITE);
		GUI_SetFont(APP_24B_FONT);
		GUI_DispStringInRectWrap( pPara->scr_lines[0], &(pPara->gRect5), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

		// pos1: scr1 // 
		GUI_RECT gRect7 = {AZP_SUBMEN_POSX+AZP_GB_COIL_SIZEX, AZP_SUBMEN_POSY+sizey, AZP_SUBMEN_ENDX, AZP_SUBMEN_POSY+(2*sizey)};
		pPara->gRect7 = gRect7;
		if(GB_END_WAIT != pPara->GBState)
			GUI_SetColor(GUI_BLACK);
		else
			GUI_SetColor(GUI_BLUE);
		GUI_FillRoundedRect(gRect7.x0+2,gRect7.y0+2,gRect7.x1-2,gRect7.y1-2,2);
		GUI_SetColor(GUI_WHITE);
		GUI_SetFont(APP_24B_FONT);
		GUI_DispStringInRectWrap(pPara->scr_lines[1], &(pPara->gRect7), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);		
	} 
	// pos2: slider - Update Gauge values // 
	GUI_RECT location = {AZP_SUBMEN_POSX+AZP_GB_COIL_SIZEX+2, AZP_SUBMEN_POSY + 2*sizey + 4, AZP_SUBMEN_ENDX, AZP_SUBMEN_ENDY-4};
	draw_slider(location, pPara->Gauge, GUI_DARKGREEN, GUI_BLACK, 0);
}

static void draw_submenu(uint8_t aindx, uint8_t cnt, char **item_str)
{
	// Draw submenu border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, AZP_SUBMEN_ENDX+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, AZP_SUBMEN_ENDX+1, AZP_SUBMEN_ENDY+1, 2);

	// Draw small boxes around of each menu item // 
	uint16_t item_sizey = (AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/cnt;

	volatile uint16_t indx;
	for(indx=0 ; indx<cnt ; indx++) {
		// Draw submenu border // 
		uint16_t posy = AZP_SUBMEN_POSY + (item_sizey * indx);
		GUI_DrawRoundedRect(AZP_SUBMEN_POSX, posy, AZP_SUBMEN_ENDX, posy + item_sizey, 2);
		// Draw submenu color // 
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(AZP_SUBMEN_POSX+3, posy+3, AZP_SUBMEN_ENDX-3, posy + item_sizey-3, 2);
		if(aindx == indx) {
			uint8_t diff;
			switch(cnt) {
				case 2:
					diff = 13;
					break;
				case 3:
					diff = 9;
					break;
				case 4 : 
					diff = 7;
					break;
				case 5:
				default:
					diff = 5;
					break;
			}
			
			GUI_SetColor(GUI_DARKGREEN);
			GUI_FillRoundedRect(AZP_SUBMEN_POSX+13, posy+diff, AZP_SUBMEN_ENDX-13, posy + item_sizey-diff, 2);
		}
		// Draw submenu string // 
		GUI_RECT gRect = {AZP_SUBMEN_POSX, posy, AZP_SUBMEN_ENDX, posy + item_sizey};
		pPara->gRect3 = gRect;

		if(item_sizey < 30)
			GUI_SetFont(APP_16B_FONT);
		else if((30 <= item_sizey) && (40 >= item_sizey))
			GUI_SetFont(APP_19B_FONT);
		else
			GUI_SetFont(APP_24B_FONT);
		GUI_SetColor(GUI_WHITE);
		if(aindx == indx)
			GUI_SetTextMode(GUI_TM_TRANS);
		GUI_DispStringInRectWrap(item_str[indx], &(pPara->gRect3), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
	}
}

static void draw_depth(uint8_t cnt, char **item_str)
{
	// Draw submenu border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, AZP_SUBMEN_ENDX+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, AZP_SUBMEN_ENDX+1, AZP_SUBMEN_ENDY+1, 2);

	// Draw small boxes around of each menu item // 
	uint16_t big_item_sizey = (((AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/cnt)*4)/3;
	uint16_t little_item_sizey = ((AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/cnt)*2/3;
	uint16_t tot_item_sizey = 0;

	volatile uint16_t indx;
	for(indx=0 ; indx<cnt ; indx++) {
		uint16_t item_sizey = (indx%2) ? big_item_sizey : little_item_sizey;
		// Draw submenu border // 
		uint16_t posy = AZP_SUBMEN_POSY + tot_item_sizey;
		GUI_DrawRoundedRect(AZP_SUBMEN_POSX, posy, AZP_SUBMEN_ENDX, posy + item_sizey, 2);
		// Draw submenu color // 
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(AZP_SUBMEN_POSX+2, posy+2, AZP_SUBMEN_ENDX-2, posy + item_sizey-2, 2);
		GUI_SetColor(GUI_DARKGREEN);
		if(4 == cnt)
			GUI_FillRoundedRect(AZP_SUBMEN_POSX+4, posy+4, AZP_SUBMEN_ENDX-4, posy + item_sizey-4, 2);
		else
			GUI_FillRoundedRect(AZP_SUBMEN_POSX+6, posy+6, AZP_SUBMEN_ENDX-6, posy + item_sizey-6, 2);
		// Draw submenu string // 
		GUI_SetColor(GUI_WHITE);
		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_RECT gRect = {AZP_SUBMEN_POSX, posy, AZP_SUBMEN_ENDX, posy + item_sizey};
		pPara->gRect3 = gRect;
		if(indx%2) {
			if(4 == cnt)
				GUI_SetFont(APP_32B_FONT);
			else {
				// Because of 'x' is not in this font family, draw it with APP_32B_FONT fonts // 
				GUI_SetFont(APP_32B_FONT);
				GUI_DispStringInRectWrap("x", &(pPara->gRect3), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
				GUI_SetFont(&GUI_FontD48);
			}
		} else {
			GUI_SetFont(APP_19B_FONT);
		}
		GUI_DispStringInRectWrap(item_str[indx], &(pPara->gRect3), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);

		tot_item_sizey += item_sizey;
	}
}


static void draw_lang(uint8_t aindx)
{
	volatile uint16_t indx;
	char *item_str[LANG_COUNT];

	for(indx=0 ; indx<LANG_COUNT ; indx++) 
		item_str[indx] = (char *)GetString2Lang(STR_OWN_LANG_INDX, indx);
	
	// Draw submenu border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-2, AZP_SUBMEN_POSY-2, AZP_SUBMEN_ENDX+2, AZP_SUBMEN_ENDY+2, 2);
	GUI_DrawRoundedRect(AZP_SUBMEN_POSX-1, AZP_SUBMEN_POSY-1, AZP_SUBMEN_ENDX+1, AZP_SUBMEN_ENDY+1, 2);

	// Draw small boxes around of each menu item // 
	uint16_t item_sizey = (AZP_SUBMEN_ENDY - AZP_SUBMEN_POSY)/(LANG_COUNT/2);
	uint16_t midX = (AZP_SUBMEN_ENDX + AZP_SUBMEN_POSX)/2;

	for(indx=0 ; indx<(LANG_COUNT/2) ; indx++) {
		// Draw submenu borders // 
		uint16_t posy = AZP_SUBMEN_POSY + (item_sizey * indx);
		GUI_DrawRoundedRect(AZP_SUBMEN_POSX, posy, midX-1, posy + item_sizey, 2);
		GUI_DrawRoundedRect(midX+1, posy, AZP_SUBMEN_ENDX, posy + item_sizey, 2);
		// Draw submenu color // 
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(AZP_SUBMEN_POSX+2, posy+2, midX-2, posy + item_sizey-2, 2);
		if(aindx == indx) {
			GUI_SetColor(GUI_DARKGREEN);
			GUI_FillRoundedRect(AZP_SUBMEN_POSX+4, posy+4, midX-4, posy + item_sizey-4, 2);
		}
		GUI_SetColor(GUI_BLACK);
		GUI_FillRoundedRect(midX+2, posy+2, AZP_SUBMEN_ENDX-2, posy + item_sizey-2, 2);
		if(aindx == (indx + (LANG_COUNT/2))) {
			GUI_SetColor(GUI_DARKGREEN);
			GUI_FillRoundedRect(midX+4, posy+4, AZP_SUBMEN_ENDX-4, posy + item_sizey-4, 2);
		}
		// Draw submenu strings // 
		GUI_RECT gRect3 = {AZP_SUBMEN_POSX, posy, midX, posy + item_sizey};
		pPara->gRect3 = gRect3;
		GUI_RECT gRect6 = {midX, posy, AZP_SUBMEN_ENDX, posy + item_sizey};
		pPara->gRect6 = gRect6;
		GUI_SetFont(APP_19B_FONT);
		GUI_SetColor(GUI_WHITE);
		GUI_SetTextMode(GUI_TM_TRANS);
		GUI_DispStringInRectWrap(item_str[indx], &(pPara->gRect3), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
		GUI_DispStringInRectWrap(item_str[indx + (LANG_COUNT/2)], &(pPara->gRect6), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
	}
}

static void draw_setting_val(uint16_t active_val, uint16_t val_max)
{
	uint16_t rangex = AZP_SLIDER_POSX_MAX - AZP_SLIDER_POSX_MIN;
	// Draw slider border //
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(AZP_SLIDER_POSX_MIN-2, AZP_SLIDER_POSY_MIN-2, AZP_SLIDER_POSX_MAX+2, AZP_SLIDER_POSY_MAX+2, 2);
	GUI_DrawRoundedRect(AZP_SLIDER_POSX_MIN-1, AZP_SLIDER_POSY_MIN-1, AZP_SLIDER_POSX_MAX+1, AZP_SLIDER_POSY_MAX+1, 2);
	// Draw slider horizontal bar // 
	uint16_t posx = AZP_SLIDER_POSX_MIN + ((rangex * active_val)/val_max);
	GUI_DrawGradientRoundedH(AZP_SLIDER_POSX_MIN, AZP_SLIDER_POSY_MIN, posx, AZP_SLIDER_POSY_MAX, 2, GUI_LIGHTBLUE, GUI_DARKBLUE);
	// Draw Slider vertical level lines //
	uint8_t pencil = GUI_GetPenSize();
	GUI_SetPenSize(3);
	volatile uint8_t indx;
	for(indx=0 ; indx<AZP_SYS_SET_MENU_MAX ; indx++)
		GUI_DrawVLine(AZP_SLIDER_POSX_MIN + (indx * rangex)/AZP_SYS_SET_MENU_MAX, AZP_SLIDER_POSY_MIN, AZP_SLIDER_POSY_MAX);
	GUI_SetPenSize(pencil);
	// Draw slider pointer // 
	GUI_SetColor(GUI_BLACK);
	GUI_DrawRoundedRect(posx-20, AZP_SLIDER_POSY_MIN-10, posx+20, AZP_SLIDER_POSY_MAX+10, 2);
	GUI_DrawRoundedRect(posx-19, AZP_SLIDER_POSY_MIN-9, posx+19, AZP_SLIDER_POSY_MAX+9, 2);
	GUI_DrawRoundedRect(posx-18, AZP_SLIDER_POSY_MIN-8, posx+18, AZP_SLIDER_POSY_MAX+8, 2);
	GUI_SetColor(GUI_RED);
	GUI_FillRoundedRect(posx-17, AZP_SLIDER_POSY_MIN-7, posx+17, AZP_SLIDER_POSY_MAX+7, 2);
	
	// Draw active setting value number // 
	GUI_RECT gRect = {0, AZP_SLIDER_POSY_MAX+10, AZP_MENU_MIDWIN_SIZEX, AZP_MENU_MIDWIN_SIZEY};
	pPara->gRect2 = gRect;
	GUI_SetFont(&GUI_FontD80);
	GUI_SetColor(GUI_BLACK);
	GUI_SetTextMode(GUI_TM_TRANS);
	snprintf(pPara->val_str, sizeof(pPara->val_str)-1, "%u", active_val);
	GUI_DispStringInRectWrap(pPara->val_str, &(pPara->gRect2), GUI_TA_HCENTER | GUI_TA_VCENTER, GUI_WRAPMODE_WORD);
}


static inline uint16_t mangbstep_2_phase(uint16_t man_gb_step) {
	return ((uint32_t)man_gb_step * (uint32_t)gid_phase_range) / man_gb_step_range;
}

static inline uint16_t phase_2_mangbstep(uint16_t phase) {
	uint16_t man_gb_step = ((uint32_t)phase * (uint32_t)man_gb_step_range) / gid_phase_range;
	//man_gb_step = (man_gb_step/5)*5;
	return man_gb_step;
}

/*********************************************************************
*
*/
uint8_t AZP_Menu(void) 
{
	volatile uint8_t indx;

	pPara = (PARA *)calloc(sizeof(PARA), 1);
	if(NULL == pPara)
	    while(STALLE_ON_ERR);

	// Set static resourcess because of reentrancy of this menu // 
	pPara->ready4pkey  = FALSE;
	pPara->OldDesktopCallback = NULL;
	pPara->ScreenExit = SCR_RUNNING;
	
	new_page = AZP_DISC_SCR;
	pPara->GBState = GB_IDLE;
	pPara->gb_start_time = 0;
	pPara->Gauge = (UMD_GAUGE_MAX/2);
	pPara->gauge_is_valid = FALSE;
	pPara->menu_depth = 0;
	pPara->dpt1_pos = 0;
	pPara->dpt2_pos = 0;
	pPara->vol_backup = APP_GetValue(ST_VOL);
	pPara->hTimerAnim = -1;
	pPara->new_key_press_time = GUI_X_GetTime();
	pPara->prev_key_press_time = 0;
	pPara->calculated_depth = 0x1FF; // initial case // 
	pPara->first_time = TRUE;

	memset(pPara->line_str, 0, sizeof(pPara->line_str));
	pPara->lineY[0] = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY + 0	+ 5;		// 40 cm
	strcpy(pPara->line_str[0], "40");
	pPara->lineY[1] = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY + 27	+ 5;	// 30 cm
	strcpy(pPara->line_str[1], "30");
	pPara->lineY[2] = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY + 53	+ 5;	// 20 cm
 	strcpy(pPara->line_str[2], "20");
	pPara->lineY[3] = AZP_MENU_LEFT_UPY + AZP_SUBMEN_POSY + 80	+ 5;	// 10 cm
	strcpy(pPara->line_str[3], "10");

	if(0 != InitGroupRes(AZP_GB_PICs, 0xFF))	// AZP GB Resource Initialization //
		while(TODO_ON_ERR);
		
	WM_MULTIBUF_Enable(1);
	GUI_SetBkColor(GUI_BLACK);
	GUI_Clear();	// Is This necessary? We are already drawing a picture that fully covering LCD // 
	GUI_ClearKeyBuffer();

	// Reduce size of desktop window to size of display
	WM_SetSize(WM_HBKWIN, LCD_GetXSize(), LCD_GetYSize());
	pPara->OldDesktopCallback = WM_SetCallback(WM_HBKWIN, _cbBk_Desktop);
	WM_InvalidateWindow(WM_HBKWIN);	// Force to redraw Desktop Window // 

	/* CREATE Windows and Do Supplementary Operations */
	pPara->hMidWin = WM_CreateWindowAsChild(AZP_MENU_LEFT_UPX, AZP_MENU_LEFT_UPY, \
		AZP_MENU_DOWN_RIGHTX-AZP_MENU_LEFT_UPX, AZP_MENU_DOWN_RIGHTY-AZP_MENU_LEFT_UPY,	\
			WM_HBKWIN, WM_CF_SHOW, _cbMidDraw, sizeof(pPara));
	WM_SetUserData(pPara->hMidWin,   &pPara, sizeof(pPara));
	WM_SetFocus(pPara->hMidWin);

	SB_init(SB_FULL_TOP);
	uint16_t active_sb_pos;
	SB_setget_component(SB_AZP_ACTIVE_POS, FALSE, &active_sb_pos);
	if(AZP_SB_POS_GB == active_sb_pos) {	// we have reached here with GB shortcut menu key // 
		pPara->menu_depth = 1;
	} else {	// Start AZPMenu active icon from first one // 
		active_sb_pos = AZP_SB_POS_SYS_SET;
		SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
	}	
	{
		uint16_t mineralization;
		SB_setget_component(SB_GROUND_ID, FALSE, &(mineralization));
		pPara->man_gb_step = phase_2_mangbstep(mineralization);
		if(pPara->man_gb_step > AZP_MAN_GB_MINERAL_MAX)
			pPara->man_gb_step = AZP_MAN_GB_MINERAL_MAX;
	}

	// Animation loop
	while (likely(SCR_EXIT_CONFIRMED != pPara->ScreenExit)) {
		if(!GUI_Exec1()) {	// try to do ONLY ONE gui job and if there is NOT any job todo sleep for 1MS // 
			if(0 == uxQueueMessagesWaiting(GUI_Task_Queue))	// If there is no message pending from dedector hw // 
				vTaskDelay(1 * (configTICK_RATE_HZ/1000));
			else {	// Receive & Handle dedector Message(s) until queue is EMPTY // 
				AZPMenu_on_msg_ready();
			}
			#if(DEBUG_ON_LCD != APP_DEBUG_OUTPUT)
				Print_Mem_Data(FALSE);
			#endif
		}
	}
	if(AZP_LOADING_SCR == new_page)
		NVIC_SystemReset();
	
	GUI_ClearKeyBuffer();
	WM_SetCallback(WM_HBKWIN, pPara->OldDesktopCallback);
	WM_SetFocus(WM_HBKWIN);
	
	active_sb_pos = 0xFF;
	SB_setget_component(SB_AZP_ACTIVE_POS, TRUE, &active_sb_pos);
	SB_delete();
	WM_DeleteWindow(pPara->hMidWin);

	free(pPara);
	pPara=NULL;
	return new_page;
}

// Function to be called when timeout occured after a command send but response not received // 
static void AZPMenu_on_comm_timeout(void *last_msgp)
{
	UmdPkt_Type *msg = (UmdPkt_Type *)last_msgp;
	ERRM("TIMEOUT for CMD:0x%02X\n", msg->cmd);
	if(SCR_EXIT_CONFIRMED == pPara->ScreenExit)
		return;
	switch(msg->cmd) {
		case CMD_START_GROUND_BALANCE:
		case CMD_STOP_GROUND_BALANCE:
		case CMD_SET_GROUND_ID:
		case CMD_SET_SENSITIVITY:
		case CMD_SET_FERROS_STATE:
			pPara->menu_depth = 1;
			pPara->GBState = GB_IDLE;
			WM_InvalidateWindow(pPara->hMidWin);
			break;
		case CMD_START_DEPTH_CALCULATION:
			pPara->calculated_depth = 0xFF; // Error value for gpaint event // 
			WM_InvalidateWindow(pPara->hMidWin);
			break;
		default:
			ERRM("UNEXPECTED LAST_MSG : 0x%02X 0x%02X 0x%02X 0x%02X\n", \
				((uint8_t *)last_msgp)[0], ((uint8_t *)last_msgp)[1], ((uint8_t *)last_msgp)[2], \
					((uint8_t *)last_msgp)[3]);
			break;
	}
}

static void set_Aclk(uint16_t man_gb_step)
{
	uint16_t phase = mangbstep_2_phase(man_gb_step);	
	if(GROUND_ID_MAX <= phase)
		phase = GROUND_ID_MAX -1;
	set_mat_value(A_CLK_TIMER_MAT, phase);
}

// Function to be called when a pkt received from Detector // 
static void AZPMenu_on_msg_ready(void)
{
	uint8_t gui_msg[UMD_FIXED_PACKET_LENGTH]; 
	while(errQUEUE_EMPTY != xQueueReceive(GUI_Task_Queue, &gui_msg, 0)) {
		UmdPkt_Type *msg_ptr = (UmdPkt_Type *)&(gui_msg[0]);
		switch(msg_ptr->cmd) {
			case IND_GET_GAUGE:	{
				if(FALSE == pPara->gauge_is_valid) {
					if(GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS <= (GUI_X_GetTime() - pPara->gb_start_time))
						pPara->gauge_is_valid = TRUE;
					else
						break;	// Ignore GAUGE for a while at startuup // 
				}
				uint16_t active_sb_pos;
				SB_setget_component(SB_AZP_ACTIVE_POS, FALSE, &active_sb_pos);
				if((2 == pPara->menu_depth) && (AZP_SB_POS_GB == active_sb_pos) && (SCR_RUNNING == pPara->ScreenExit)){
					uint8_t TGauge = msg_ptr->data.gauge;
					if(UMD_GAUGE_MAX < TGauge)
						TGauge = UMD_GAUGE_MAX;	// Truncation to maximum value for safety // 
					if(pPara->Gauge != TGauge) {
						pPara->Gauge = TGauge;
						uint8_t soundG = 0;
						if((UMD_GAUGE_MAX/2) < TGauge)
							soundG = TGauge - (UMD_GAUGE_MAX/2);
						else if((UMD_GAUGE_MAX/2) > TGauge)
							soundG = (UMD_GAUGE_MAX/2) - TGauge;
						WAVE_Update_FreqAmp_Gauge(((uint16_t)soundG*2) * GAUGE_FRACTUATION);	// Update Sound Frequency // 
						WM_InvalidateWindow(pPara->hMidWin);
					}
				}
			} break;
			case RSP_STOP_GROUND_BALANCE:
				StopCommTimeout();
				if(CMD_DONE == msg_ptr->data.cmd_status) {
					/* CMD stop implemented by Detector */
					if(DPT1_POS_MAN_GB == pPara->dpt1_pos){
						if(GB_CONFIRM_REQ == pPara->GBState){
							// Send customer selected new grundID value to detector //
							uint16_t phase = mangbstep_2_phase(pPara->man_gb_step);	
							UmdPkt_Type msg;
							msg.cmd = CMD_SET_GROUND_ID;
							msg.length = 4; // CMD(U8) + LENGTH(U8) // 
							msg.data.data[0] = phase / GAUGE_FRACTUATION;
							msg.data.data[1] = phase % GAUGE_FRACTUATION;
							UARTSend((uint8_t *)&msg, UMD_CMD_TIMEOUT_SET_GID_MS, AZPMenu_on_comm_timeout);
						} else {	// Key ESC pressed on MAnualGB // 
							pPara->menu_depth = 1;
							WM_InvalidateWindow(pPara->hMidWin);
						}
					} else {	// We are on AutoGB // 
						pPara->menu_depth = 1;
						WM_InvalidateWindow(pPara->hMidWin);
					}
				}else {
					pPara->menu_depth = 1;
					WM_InvalidateWindow(pPara->hMidWin);
				}
				break;
			case RSP_START_GROUND_BALANCE:
				StopCommTimeout();
				if(CMD_DONE == msg_ptr->data.cmd_status) { /* We are happy, GB Started */
					uint16_t AppVolume = APP_GetValue(ST_VOL);
					if(AppVolume)
						start_dac_playing_mine(AUTOMATIC_SEARCH_TYPE);
					if(-1 == pPara->hTimerAnim)
						pPara->hTimerAnim = WM_CreateTimer(pPara->hMidWin, ID_TIMER_AZP_GB_ANIM, GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS, 0);
					if(DPT1_POS_AUTO_GB == pPara->dpt1_pos) {
						WM_RestartTimer(pPara->hTimerAnim, GAUGE_SUPPRESS_AFTER_SEARCH_ENTER_MS);
						memset(pPara->scr_lines, 0, sizeof(pPara->scr_lines));
						sprintf(pPara->scr_lines[0], "%s\n", GetString(STR_PROCESSING_INDX));
						sprintf(pPara->scr_lines[1], "....\n");
						WM_InvalidateWindow(pPara->hMidWin);
					}
				}
				else { /*We are sad, dedector received the start-gb command but execution failed */
					pPara->GBState = GB_IDLE;
					pPara->menu_depth = 1;
					WM_InvalidateWindow(pPara->hMidWin);
				}
				break;
			case CMD_SET_A_CLOCK_DELAY:	// AUTO-GB //
				if(DPT1_POS_AUTO_GB == pPara->dpt1_pos) 
					Process_Clk_gen_Msg(gui_msg);
				else {
					// Ignore ACLK set command on MANUAL-GB, we are controlling it by ourselves // 
				}
				break;
			case CMD_SET_B_CLOCK_DELAY: 	// AUTO/MAN GB //
			case CMD_SET_C_CLOCK_DELAY: 	// AUTO/MAN GB //
			case CMD_SET_REF_CLOCK_FREQ: 	// AUTO/MAN GB //
				Process_Clk_gen_Msg(gui_msg);
				// If MAN-GB started and RefCLK is received, automatically start to generate A-CLK // 
				if((CMD_SET_REF_CLOCK_FREQ == msg_ptr->cmd) && (DPT1_POS_MAN_GB == pPara->dpt1_pos)) {
					set_Aclk(pPara->man_gb_step);
				}
				break;
			case RSP_START_DEPTH_CALCULATION:
				StopCommTimeout();
				// update depth value on scren, gui action //
				if(CMD_DONE == msg_ptr->data.Depth_Rsp.cmd_status) {
					pPara->calculated_depth = msg_ptr->data.Depth_Rsp.depth;
					WM_InvalidateWindow(pPara->hMidWin);
				} else {
					pPara->calculated_depth = 0xFF;	// Error value for gpaint event // 
					WM_InvalidateWindow(pPara->hMidWin);
				}
				break;
			case RSP_SET_GROUND_ID: 	// MAN-GB //
				StopCommTimeout();
				pPara->ScreenExit = SCR_EXIT_CONFIRMED;
				new_page = AZP_ALL_METAL_SCR;
				pPara->GBState = GB_IDLE;
				break;
			case CMD_SET_GROUND_ID: {	// AUTO-GB // 
				UmdPkt_Type send_msg;
				send_msg.cmd = msg_ptr->cmd + 1;
				send_msg.length = 3;
				send_msg.data.cmd_status = CMD_DONE;
				UARTSend((uint8_t *)&send_msg, 0, NULL);
				if(DPT1_POS_AUTO_GB == pPara->dpt1_pos){
					uint16_t phase = ((((uint16_t)gui_msg[2]) * CLOCK_GENERATE_PHASE_FRACTION) + (gui_msg[3]%CLOCK_GENERATE_PHASE_FRACTION)); 
					APP_SetVal(ST_GROUND_ID, phase, TRUE);
					pPara->man_gb_step = phase_2_mangbstep(phase);
					if(pPara->man_gb_step > AZP_MAN_GB_MINERAL_MAX)
						pPara->man_gb_step = AZP_MAN_GB_MINERAL_MAX;
				}
			}
			break;
			case CMD_GB_COMPLETED:	// AUTO-GB only // 
				if(TRUE == is_dac_playing()) {
					WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
					WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
				}
				DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
				pPara->GBState = GB_END_WAIT;
				memset(pPara->scr_lines, 0, sizeof(pPara->scr_lines));
				sprintf(pPara->scr_lines[0], "%s\n", GetString(STR_BALANS_OK_INDX));
				uint16_t phase = APP_GetValue(ST_GROUND_ID);
				uint16_t mangbstep = phase_2_mangbstep(phase);
				char sign = ' ';
				if(mangbstep > (AZP_MAN_GB_MINERAL_MAX/2)) {
					sign = '+';
					mangbstep -= (AZP_MAN_GB_MINERAL_MAX/2);
				} else if(mangbstep < (AZP_MAN_GB_MINERAL_MAX/2)) {
					mangbstep = (AZP_MAN_GB_MINERAL_MAX/2) - mangbstep;
					sign = '-';
				} else 
					mangbstep = 0;
				sprintf(pPara->scr_lines[1], "%s: %c %u\n", GetString(STR_GROUND_ID_INDX), sign, mangbstep);
				WM_InvalidateWindow(pPara->hMidWin);
				break;
			case CMD_GB_FAILED:		// AUTO-GB only //
				if(TRUE == is_dac_playing()) {
					WAVE_Update_FreqAmp_Gauge(0);	// Update Sound Frequency // 
					WAVE_Generator_stop(TRUE, TRUE, TRUE);		// Stop Wave-DAC output, before audio file DAC play // 
				}
				DAC_DMA_Update_Freq((uint32_t)AUDIO_SAMPLE_FREQ_HZ);
				pPara->GBState = GB_END_WAIT;
				memset(pPara->scr_lines, 0, sizeof(pPara->scr_lines));
				sprintf(pPara->scr_lines[0], "%s\n", GetString(STR_BALANS_FAILED_INDX));
				sprintf(pPara->scr_lines[1], "%s\n", GetString(STR_TRY_AGAIN_INDX));
				WM_InvalidateWindow(pPara->hMidWin);
				break;
			case RSP_SET_SENSITIVITY:
			case RSP_SET_FERROS_STATE:
				StopCommTimeout();
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
  	case WM_PAINT:
			GUI_SetBkColor(GUI_BLACK);
			GUI_Clear();
			break;
		default:
			WM_DefaultProc(pMsg);
			break;
  }
}

static inline void start_dac_playing_mine(uint8_t search_type) {
	WAVE_Generator_init(search_type);
	WAVE_Generator_start(UMD_GAUGE_MIN, MODULATOR_DEF_FREQ_HZ*3, DAC_DEFAULT_AMP);	// Start DAC Wave for minimum GAUGE // 
}

/*************************** End of file ****************************/


/*********************************************************************
*                SEGGER MICROCONTROLLER SYSTEME GmbH                 *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 1996 - 2004  SEGGER Microcontroller Systeme GmbH        *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

***** emWin - Graphical user interface for embedded applications *****
emWin is protected by international copyright laws.   Knowledge of the
source code may not be used to write a similar product.  This file may
only be used in accordance with a license and should not be re-
distributed in any way. We appreciate your understanding and fairness.
----------------------------------------------------------------------
File        : MOTION_IconSlide.c
Purpose     : Icon animation using motion support of WM

              Icons used from http://www.iconshock.com/social-icons
Requirements: WindowManager - (x)
              MemoryDevices - (x)
              AntiAliasing  - ( )
              VNC-Server    - ( )
              PNG-Library   - ( )
              TrueTypeFonts - ( )
----------------------------------------------------------------------
*/

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "WM.h"
#include "AppCommon.h"
#include "AppFont.h"
#include "AppPics.h"
#include "StatusBar.h"
#include "Popup.h"
#include "BSP.h"

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define IMAGES_COUNT	(DEVICE_SCREEN_COUNT)
#define ICONS_XSTART	(100)
#define ICON_XSIZE		(60)
#define ICON_YSIZE		(60)
#define ICON_XSIZE_SMALL		(40)
#define ICON_YSIZE_SMALL		(40)

/*********************************************************************
*
*       Static (const) data
*
**********************************************************************
*/

static GUI_CONST_STORAGE GUI_BITMAP *_bmAppBMs[IMAGES_COUNT] = {
	&bmGB, 
	&bmSearch, 
	&bmSensitivity,
	&bmLanguage,
	&bmVolume, 
	&bmBrightness,
	&bmHelp
};
static GUI_CONST_STORAGE GUI_BITMAP *_bmAppSmallBMs[IMAGES_COUNT] = {
	&bmGBSmall, 
	&bmSearchSmall, 
	&bmSensitivitySmall,
	&bmLanguageSmall,
	&bmVolumeSmall, 
	&bmBrightnessSmall,
	&bmHelpSmall
};
static char *MenuStrs[IMAGES_COUNT] = {
	GB_STR, 
	SS_STR,
	S_STR,
	L_STR,
	V_STR,
	BR_STR,
	H_STR
};
static GUI_MEMDEV_Handle ahMemDev[IMAGES_COUNT];
static uint8_t ActiveIndx = 0;	// Active selected screen index // 
static uint8_t ScreenExit;			// Flag for exit from screen // 

/*********************************************************************
*
*       Types
*
**********************************************************************
*/ 
typedef struct PARA PARA;

struct PARA {
  int xSizeWin, ySizeWin;
  int xSizeLCD, ySizeLCD;
  int xSizeBM, ySizeBM;
  int xPosWin, yPosWin;
  GUI_COLOR Color0, Color1;
  GUI_MEMDEV_Handle ahMemImage[IMAGES_COUNT];
  void (* pfRotate) (GUI_MEMDEV_Handle, GUI_MEMDEV_Handle, int, int, int, int);
  void (* pfOnPaint)(PARA * pPara);
  WM_HWIN hWinIndex;
  WM_HWIN hWinStatus;
  int IndexPos;
  int IndexPaint;
  int Animate;
  int Auto;
};

#if(0)
/*********************************************************************
*
*       Static code
*
**********************************************************************
*/
void myexample(void)
{
	volatile uint8_t indx;
	char temp[32];
	GUI_MEMDEV_Handle hMemSource, hMemSource1;
// 	GUI_MEMDEV_Handle hMemDest;
	GUI_RECT RectSource = {0, 0, 69, 39};
// 	GUI_RECT RectDest = {0, 0, 79, 79};
  
// 	GUI_Init();
	for(indx=0 ; indx<IMAGES_COUNT ; indx++) {
		ahMemDev[indx] = GUI_MEMDEV_CreateFixed(0, 0, _bmAppBMs[indx]->XSize + 1, _bmAppBMs[indx]->YSize + 1, \
				GUI_MEMDEV_HASTRANS, GUI_MEMDEV_APILIST_32, GUI_COLOR_CONV_888);
		GUI_MEMDEV_Select(ahMemDev[indx]);
		#if(0)
			GUI_DrawGradientV(0, 0, _bmAppBMs[indx]->XSize, _bmAppBMs[indx]->YSize, GUI_WHITE, GUI_DARKGREEN);
			GUI_SetColor(GUI_BLUE);
			GUI_SetFont(&GUI_Font20B_ASCII);
			GUI_SetTextMode(GUI_TM_TRANS);
			memset(temp,0,sizeof(temp));
			sprintf(temp,"emWin(%u)", indx);
			GUI_DispStringInRect(temp, &RectSource, GUI_TA_HCENTER | GUI_TA_VCENTER);
			GUI_DrawRect(0, 0, _bmAppBMs[indx]->XSize, _bmAppBMs[indx]->YSize);
		#else
			GUI_DrawBitmap(_bmAppBMs[indx], 0, 0);
		#endif
		GUI_MEMDEV_Select(0);
	}
}
#endif

/*********************************************************************
*
*       _cbWin
*
* Function description
*   Callback routine of icon window
*/
static void _cbWin(WM_MESSAGE * pMsg) {
  GUI_MEMDEV_Handle   hMemOld;
  static WM_HTIMER    hTimerRefinement;
  WM_HWIN             hWin;
  PARA              * pPara;
  int                 xPosWinNew;
  int                 yPosWinNew;
  int                 Id;

  hWin = pMsg->hWin;
  WM_GetUserData(hWin, &pPara, sizeof(PARA *));
  switch (pMsg->MsgId) {
		case WM_DELETE:
			if (hTimerRefinement) {
				WM_DeleteTimer(hTimerRefinement);
				hTimerRefinement = 0;
			}
			break;
#if(0)
			case WM_TIMER:
			Id = WM_GetTimerId(pMsg->Data.v);
			switch (Id) {
				case ID_TIMER_ANIMATION:
					pPara->Animate = 1;
					break;
				case ID_TIMER_REFINEMENT:
					WM_DeleteTimer(hTimerRefinement);
					hTimerRefinement = 0;
					xPosWinNew = WM_GetWindowOrgX(hWin);
					yPosWinNew = WM_GetWindowOrgY(hWin);
					break;
			}
			break;
#endif
		case WM_PAINT: {
			uint16_t Xrange = LCD_GetXSize() - ICONS_XSTART - 30;
			uint16_t Yrange = LCD_GetYSize() - STATUS_BAR_Y_SIZE -5;
			uint16_t xPos, yPos, xIconSize, yIconSize;
			volatile uint8_t indx;
			GUI_DrawGradientV(0, 0, pPara->xSizeWin, pPara->ySizeWin - 1, pPara->Color0, pPara->Color1);
			GUI_SetTextMode(GUI_TM_TRANS);
			for(indx=0 ; indx<IMAGES_COUNT ; indx++) {
				xPos = ICONS_XSTART+(indx%3)*(Xrange/3);
				yPos = STATUS_BAR_Y_SIZE + 10 + (indx/3)*(Yrange/3) - (ICON_YSIZE/2);
				if(indx == ActiveIndx) {	// Draw of ACTIVE icons //
					GUI_DrawBitmap(_bmAppBMs[indx], xPos, yPos);
					GUI_SetFont(GUI_FONT_20B_ASCII);
					GUI_SetColor(GUI_LIGHTRED);
					xIconSize = ICON_XSIZE;
					yIconSize = ICON_YSIZE;
				} else {	// Draw of PASSIVE icons // 
 					GUI_DrawBitmap(_bmAppSmallBMs[indx], xPos, yPos);
					GUI_SetFont(GUI_FONT_16B_ASCII);
					GUI_SetColor(GUI_LIGHTBLUE);
					xIconSize = ICON_XSIZE_SMALL;
					yIconSize = ICON_YSIZE_SMALL;
				}
				GUI_DispStringAt(MenuStrs[indx],\
					xPos - (11*strlen(MenuStrs[indx]) - xIconSize)/2 , yPos + yIconSize);
			}
			break;
		}
		case WM_SET_FOCUS:
			pMsg->Data.v = 0;
			break;
		case WM_KEY: {
			WM_KEY_INFO * pInfo;
			pInfo = (WM_KEY_INFO *)pMsg->Data.p;
			if (pInfo->PressedCnt) {
				switch (pInfo->Key) {
					case KEY_LEFT_EVENT:
						if((ActiveIndx%3) == 0)
							ActiveIndx = (ActiveIndx/3)*3 + 2;
						else
							ActiveIndx--;
						break;
					case KEY_RIGHT_EVENT:
						if((ActiveIndx%3) == 2)
							ActiveIndx = (ActiveIndx/3)*3;
						else
							ActiveIndx++;
						break;
					case KEY_UP_EVENT:
						if(0 == (ActiveIndx/3))
							ActiveIndx = (ActiveIndx%3) + 6;
						else
							ActiveIndx -= 3;
						break;
					case KEY_DOWN_EVENT:
						if(2 == (ActiveIndx/3))
							ActiveIndx = (ActiveIndx%3);
						else
							ActiveIndx += 3;
						break;
					case KEY_OK_EVENT:
						// Open desired screen related page or pop-up // 
						break;
					case KEY_ESC_EVENT:
						ScreenExit = TRUE;
						break;
					default:	
						break;
				}
				WM_InvalidateWindow(hWin);
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
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       MainTask
*/
void IconSlide(void) {
  WM_HWIN         hWin;
  PARA            Para  = {0};
  PARA          * pPara = &Para;
  
	WM_MULTIBUF_Enable(1);
	GUI_Clear();
	ScreenExit = FALSE;

	// Initialize parameter structure
  pPara->pfRotate = GUI_MEMDEV_RotateHQ;
  pPara->xSizeLCD = LCD_GetXSize();
  pPara->ySizeLCD = LCD_GetYSize();
  pPara->xSizeWin = pPara->xSizeLCD;
  pPara->ySizeWin = LCD_GetYSize() - STATUS_BAR_Y_SIZE;
  pPara->xPosWin  = 0;
  pPara->yPosWin  = STATUS_BAR_Y_SIZE;
  pPara->Color0   = 0xA02020;
  pPara->Color1   = GUI_BLACK;
  
  // Reduce size of desktop window to size of display
  WM_SetSize(WM_HBKWIN, pPara->xSizeLCD, pPara->ySizeLCD);

	// Create window to be animated
  hWin = WM_CreateWindowAsChild(
    pPara->xPosWin,
    pPara->yPosWin,
    pPara->xSizeWin,
    pPara->ySizeWin,
    WM_HBKWIN, WM_CF_SHOW  | WM_CF_HASTRANS, _cbWin, sizeof(PARA *)
  );
  WM_SetUserData(hWin, &pPara, sizeof(PARA *));
  WM_SetFocus(hWin);

  // Create status window; Status window (android' teki gibi)
	SB_init();
	
	//myexample();
  #if 1
    while (FALSE == ScreenExit) {
			GUI_Delay(10);
    }

		GUI_ClearKeyBuffer();
		SB_delete();
		WM_SetFocus(WM_HBKWIN);
		WM_DeleteWindow(hWin);
  #else
  #endif
}

/*************************** End of file ****************************/

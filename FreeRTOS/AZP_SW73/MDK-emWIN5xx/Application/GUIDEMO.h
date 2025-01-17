/*********************************************************************
*                SEGGER MICROCONTROLLER GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 2003-2012     SEGGER Microcontroller GmbH & Co KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File        : GUIDEMO.h
Purpose     : Configuration file of GUIDemo
----------------------------------------------------------------------
*/

#ifndef GUIDEMO_H
#define GUIDEMO_H

#if defined(__cplusplus)
extern "C" {     /* Make sure we have C-declarations in C++ programs */
#endif

#include "GUI.h"
#include "FRAMEWIN.h"
#include "PROGBAR.h"
#include "TEXT.h"
#include "BUTTON.h"
#include "SLIDER.h"
#include "HEADER.h"
#include "GRAPH.h"
#include "ICONVIEW.h"
#include "LISTVIEW.h"
#include "AppPics.h"
#include "AppFont.h"
	
#if GUI_WINSUPPORT
  #include "WM.h"
#endif

/*********************************************************************
*
*       Defines
*
**********************************************************************
*/
#define CONTROL_SIZE_X  80
#define CONTROL_SIZE_Y  61
#define INFO_SIZE_Y     65
#define BUTTON_SIZE_X   32
#define BUTTON_SIZE_Y   20
#define PROGBAR_SIZE_X  66
#define PROGBAR_SIZE_Y  12
#define TEXT_SIZE_X     69
#define TEXT_SIZE_Y     7
#define SHOW_PROGBAR_AT 100
#define GUI_ID_HALT     GUI_ID_USER + 0
#define GUI_ID_NEXT     GUI_ID_USER + 1

#define BK_COLOR_0      0xFF5555
#define BK_COLOR_1      0x880000

#define NUMBYTES_NEEDED 0x200000

#define CIRCLE_RADIUS   100

#define LOGO_DIST_BORDER 5

/*********************************************************************
*
*       Configuration of modules to be used
*
**********************************************************************
*/
#ifndef   SHOW_GUIDEMO_AUTOMOTIVE
  #define SHOW_GUIDEMO_AUTOMOTIVE        (1)
#endif
#ifndef   SHOW_GUIDEMO_BARGRAPH
  #define SHOW_GUIDEMO_BARGRAPH          (0)
#endif
#ifndef   SHOW_GUIDEMO_BITMAP
  #define SHOW_GUIDEMO_BITMAP            (0)
#endif
#ifndef   SHOW_GUIDEMO_COLORBAR
  #define SHOW_GUIDEMO_COLORBAR          (0)
#endif
#ifndef   SHOW_GUIDEMO_CURSOR
  #define SHOW_GUIDEMO_CURSOR            (0)
#endif
#ifndef   SHOW_GUIDEMO_FADING
  #define SHOW_GUIDEMO_FADING            (0)   //50K
#endif
#ifndef   SHOW_GUIDEMO_GRAPH
  #define SHOW_GUIDEMO_GRAPH             (0)
#endif
#ifndef   SHOW_GUIDEMO_ICONVIEW
  #define SHOW_GUIDEMO_ICONVIEW          (1)  //da
#endif
#ifndef   SHOW_GUIDEMO_IMAGEFLOW
  #define SHOW_GUIDEMO_IMAGEFLOW         (0)
#endif
#ifndef   SHOW_GUIDEMO_LISTVIEW
  #define SHOW_GUIDEMO_LISTVIEW          (0)
#endif
#ifndef   SHOW_GUIDEMO_SKINNING
  #define SHOW_GUIDEMO_SKINNING          (0)
#endif
#ifndef   SHOW_GUIDEMO_SPEED
  #define SHOW_GUIDEMO_SPEED             (0)
#endif
#ifndef   SHOW_GUIDEMO_SPEEDOMETER
  #define SHOW_GUIDEMO_SPEEDOMETER       (0)
#endif
#ifndef   SHOW_GUIDEMO_TRANSPARENTDIALOG
  #define SHOW_GUIDEMO_TRANSPARENTDIALOG (0)
#endif
#ifndef   SHOW_GUIDEMO_TREEVIEW
  #define SHOW_GUIDEMO_TREEVIEW          (0)
#endif
#ifndef   SHOW_GUIDEMO_VSCREEN
  #define SHOW_GUIDEMO_VSCREEN           (0)
#endif
#ifndef   SHOW_GUIDEMO_WASHINGMACHINE
  #define SHOW_GUIDEMO_WASHINGMACHINE    (0)
#endif
#ifndef   SHOW_GUIDEMO_ZOOMANDROTATE
  #define SHOW_GUIDEMO_ZOOMANDROTATE     (0)
#endif

/*********************************************************************
*
*       Configuration macros
*
**********************************************************************
*/
#ifndef   GUIDEMO_SHOW_SPRITES
  #define GUIDEMO_SHOW_SPRITES    (1)
#endif
#ifndef   GUIDEMO_USE_VNC
  #define GUIDEMO_USE_VNC         (0)
#endif
#ifndef   GUIDEMO_USE_AUTO_BK
  #define GUIDEMO_USE_AUTO_BK     (1)
#endif

#define GUIDEMO_CF_SHOW_SPRITES (GUIDEMO_SHOW_SPRITES <<  0)
#define GUIDEMO_CF_USE_VNC      (GUIDEMO_USE_VNC      <<  1)
#define GUIDEMO_CF_USE_AUTO_BK  (GUIDEMO_USE_AUTO_BK  <<  2)

/*********************************************************************
*
*       GUIDEMO_CONFIG
*/
typedef struct GUIDEMO_CONFIG {
  void (* * apFunc)(void);
  int       NumDemos;
  U16       Flags;
  #if GUIDEMO_USE_VNC
    int  (* pGUI_VNC_X_StartServer)(int LayerIndex, int ServerIndex);
  #endif
} GUIDEMO_CONFIG;

typedef struct {
  const char       * pText;
  const char       * pExplanation;
} BITMAP_ITEM;

/*********************************************************************
*
*       Internal functions
*
**********************************************************************
*/
void GUIDEMO_AddIntToString   (char * acText, unsigned int Number);
void GUIDEMO_AddStringToString(char * acText, const char * acAdd);
int  GUIDEMO_CheckCancel      (void);
void GUIDEMO_ClearText        (char * acText);
void GUIDEMO_Config           (GUIDEMO_CONFIG * pConfig);
void GUIDEMO_Delay            (int t);
void GUIDEMO_DrawBk           (int DrawLogo);
U16  GUIDEMO_GetConfFlag      (U16 Flag);
int  GUIDEMO_GetTime          (void);
void GUIDEMO_HideControlWin   (void);
void GUIDEMO_HideInfoWin      (void);
void GUIDEMO_NotifyStartNext  (void);
void GUIDEMO_ShowControlWin   (void);
void GUIDEMO_ShowInfo         (const char * acInfo);
void GUIDEMO_ShowInfoWin      (void);
void GUIDEMO_ShowIntro        (const char * acText, const char * acDescription);
void GUIDEMO_UpdateControlText(void);
void GUIDEMO_Wait             (int TimeWait);
void GUIDEMO_Init             (void);

/*********************************************************************
*
*       Demo modules
*
**********************************************************************
*/
void GUIDEMO_Automotive       (void);
void GUIDEMO_BarGraph         (void);
void GUIDEMO_Bitmap           (void);
void GUIDEMO_ColorBar         (void);
void GUIDEMO_Cursor           (void);
void GUIDEMO_Fading           (void);
void GUIDEMO_Graph            (void);
void GUIDEMO_IconView         (void);
void GUIDEMO_ImageFlow        (void);
void GUIDEMO_Intro            (void);
void GUIDEMO_Listview         (void);
void GUIDEMO_Skinning         (void);
void GUIDEMO_Speed            (void);
void GUIDEMO_Speedometer      (void);
void GUIDEMO_TransparentDialog(void);
void GUIDEMO_Treeview         (void);
void GUIDEMO_VScreen          (void);
void GUIDEMO_WashingMachine   (void);
void GUIDEMO_ZoomAndRotate    (void);

/*********************************************************************
*
*       Externs
*
**********************************************************************
*/
// extern GUI_CONST_STORAGE GUI_BITMAP bmSeggerLogo1;
// extern GUI_CONST_STORAGE GUI_BITMAP bmSeggerLogo70x35;

// extern GUI_CONST_STORAGE GUI_FONT   GUI_FontRounded16;
// extern GUI_CONST_STORAGE GUI_FONT   GUI_FontRounded22;
// extern GUI_CONST_STORAGE GUI_FONT   GUI_FontSouvenir18;
// extern GUI_CONST_STORAGE GUI_FONT   GUI_FontD6x8;

//----- emWIN USER MESSAGES ----//
typedef enum
{
	GUI_USER_MSG_POWER_OFF = WM_USER,
	GUI_USER_LANG_CHANGED,		// Redraw UI with new language is required // 
	GUI_USER_MSG_NEW_GAUGE,		// New simulated Gauge message received // 
	GUI_USER_MSG_NEW_TARGET_ID,		// New simulated targetID message received // 
	GUI_USER_MSG_NEW_GROUND_ID,		// New simulated GroundID message received // 
	GUI_USER_MSG_NEW_DEPTH,	// New simulated Depth data received // 
	GUI_USER_MSG_NEW_TS_TOUCH,	// New touch screen event detected // 
	GUI_USER_MSG_LAST	= GUI_USER_MSG_NEW_DEPTH,
		
	GUI_USER_MSG_COUNT = GUI_USER_MSG_LAST-WM_USER
} emWIN_USER_MESSAGEs;


#if defined(__cplusplus)
  }
#endif

#endif // avoid multiple inclusion

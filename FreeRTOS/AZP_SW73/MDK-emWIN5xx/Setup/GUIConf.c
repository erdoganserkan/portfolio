/*********************************************************************
*                SEGGER MICROCONTROLLER GmbH & Co. KG                *
*        Solutions for real time microcontroller applications        *
**********************************************************************
*                                                                    *
*        (c) 2003-2010     SEGGER Microcontroller GmbH & Co KG       *
*                                                                    *
*        Internet: www.segger.com    Support:  support@segger.com    *
*                                                                    *
**********************************************************************

----------------------------------------------------------------------
File        : GUIConf.c
Purpose     : Display controller initialization
---------------------------END-OF-HEADER------------------------------
*/

#include "GUI.h"
#include "LCDConf.h"
#include "GLCD.h"
#include "AppFont.h"
#include "RuntimeLoader.h"
#include "Serial.h"

/*********************************************************************
*
*       Defines, configurable
*
**********************************************************************
*/

//
// Define the average block size
//
#define GUI_BLOCKSIZE 0x100

/*********************************************************************
*
*       Static data
*
**********************************************************************
*/
 

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
/*********************************************************************
*
*       GUI_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   available memory for the GUI.
*/
void GUI_X_Config(void) {
  //
  // Assign memory to emWin
  //
  GUI_ALLOC_AssignMemory((void *)LCD_GUI_RAM_BASE, GUI_NUMBYTES);
	// The line below makes application hangs & crashes due to stack overflow //
		// It seems that debug framework is not initialized yet // 
	//INFOL("USED TOTAL RAM (%u KB)\n", \
		((LCD_GUI_RAM_BASE + GUI_NUMBYTES)-SDRAM_BASE_ADDR)/1024);
  GUI_ALLOC_SetAvBlockSize(GUI_BLOCKSIZE);
  //Number of tasks from which emWin is used at most.
  //void GUITASK_SetMaxTask(int MaxTask);   
  GUI_SetDefaultFont(APP_24B_FONT);
}

/*************************** End of file ****************************/

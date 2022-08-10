/****************************************************************************
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * products. This software is supplied "AS IS" without any warranties.
 * NXP Semiconductors assumes no responsibility or liability for the
 * use of the software, conveys no license or title under any patent,
 * copyright, or mask work right to the product. NXP Semiconductors
 * reserves the right to make changes in the software without
 * notification. NXP Semiconductors also make no representation or
 * warranty that such application will be suitable for the specified
 * use without further testing or modification.
****************************************************************************/
 
#include "GUI.h"
#include <stdint.h>

/*This is done for EA 7"(800x480) LCD*/
#define LCD_X_SIZE 800
#define LCD_Y_SIZE 480

extern GUI_CONST_STORAGE GUI_BITMAP bmLPCWarewebbanner160x90;
/* Mgnifies LPCware banner on the screen */


/**************************************************************************//**
 * @brief  Main function
 *****************************************************************************/
void MultiBufferDEMO(void)
{
  int32_t magx = 0;
  int32_t magy = 0;
	
  /* Initialize emWin GUI library*/
  GUI_Init();
  
  while (1) {
    
    /* call this function before drawing */
    GUI_MULTIBUF_Begin();
		/*Draw image */
    GUI_DrawGradientH(0, 0, LCD_X_SIZE, LCD_Y_SIZE, GUI_RED, GUI_BLUE);
    GUI_DrawBitmapMag(&bmLPCWarewebbanner160x90,0,0, magx, magy);
		/* call this function after drawing */
    if(magy<7){
      magx=magx+1;
      magy=magy+1;
      }
      else
      {
        magx=0;
				magy=0;
      }
		
    #ifdef _WINDOWS
			GUI_Delay(250);
		#else
			GUI_Delay(1);
		#endif
    GUI_MULTIBUF_End();
	}
  

}

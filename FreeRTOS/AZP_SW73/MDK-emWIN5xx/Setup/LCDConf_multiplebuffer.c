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
File        : LCDConf.c
Purpose     : Display controller configuration (single layer)
---------------------------END-OF-HEADER------------------------------
*/

#include "LPC177x_8x.h"
#include "system_LPC177x_8x.h"

#include "SEGGER.h"
#include "BSP.h"
#include "GUI.h"
#include "GUIDRV_Lin.h"

#include "LCDConf.h"
#include "LCD.h"
#include "GLCD.h"

#ifdef __ICCARM__
  #pragma diag_suppress=Pe188  // Avoid "enumerated type mixed with another type" warning as we are not using enumerated values from CMSIS files for NVIC_* parameters
#endif

#ifndef   XSIZE_PHYS
  #error Physical X size of display is not defined!
#endif
#ifndef   YSIZE_PHYS
  #error Physical Y size of display is not defined!
#endif
#ifndef   COLOR_CONVERSION
  #error Color conversion not defined!
#endif
#ifndef   DISPLAY_DRIVER
  #error No display driver defined!
#endif

/*********************************************************************
*
*       Defines, sfrs
*
**********************************************************************
*/
//
// Touch screen
//
//Xp---->P0.24(ADC0_IN[1])//用来检测Y轴电压
//Xm---->P0.23(ADC0_IN[0])
//Yp---->P0.12(ADC0_IN[6])//用来检测X轴电压
//Ym---->P1.31(ADC0_IN[5])
#define ADC_CLKDIV                 ((PeripheralClock - 1) / 12400000)  // ADC clock should be 12.4 MHz or less

#define IOCON_ENABLE_INPUT_BUFFER  (1 << 7)   //设置为数字端口模式
#define IOCON_PULL_DOWN            (1 << 3)   //设置引脚模式为下拉
#define IOCON_PULL_UP              (2 << 3)   //设置引脚模式为上拉

#define IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(Port, Mask)  \
          {                                                  \
            U32 Temp;                                        \
            Temp  = LPC_IOCON->Port;                         \
            Temp &= ~((0x1Ful << 0) | (1 << 7));             \
            Temp |= Mask;                                    \
            LPC_IOCON->Port = Temp;                          \
          }


/*********************************************************************
*
*       Static data
*
**********************************************************************
*/

#if GUI_SUPPORT_TOUCH  // Used when touch screen support is enabled

	static U8 _HasTouch;
	//
	// OS timers
	//
	// static OS_TIMER _TouchTimer;

	/*********************************************************************
	*
	*       Function Prototypes
	*
	**********************************************************************
	*/
	static void _ExecTouch(void);
	void TOUCH_SCREEN_IRQ_HANDLER (void);
	#if MULTIBUFF_USE_ISR 
	void LCD_IRQHandler(void);
	#endif

	/*********************************************************************
	*
	*       Local functions
	*
	**********************************************************************
	*/

	void TOUCH_SCREEN_IRQ_HANDLER (void)  
	{  
		if ( TOUCH_SCREEN_TIMER->IR & (0x1<<0) )
		{
			TOUCH_SCREEN_TIMER->IR = 0x1<<0;		/* clear interrupt flag */
			_ExecTouch();
		}
	}
#endif
#if MULTIBUFF_USE_ISR 
/***********************************************************
* LCD IRQ Handler
* This is base register update interrupt 
***********************************************************/

static int32_t  _PendingBuffer;

void LCD_IRQHandler(void) {
	unsigned long Addr, BufferSize;
	//Check of base update interrupt
	if((LPC_LCD->INTSTAT)&(1<<2)){
		if (_PendingBuffer >= 0) {
		//
		// Calculate address of the given buffer
		//
			BufferSize = XSIZE_PHYS * YSIZE_PHYS * PIXEL_WIDTH;
			Addr = LCD_VRAM_BASE_ADDR + BufferSize *_PendingBuffer;
			//
			// Make the given buffer visible
			//
			LPC_LCD->UPBASE = Addr;
			//
			// Send a confirmation that the buffer is visible now
			//
			GUI_MULTIBUF_Confirm(_PendingBuffer);
			_PendingBuffer = -1;
		}
		LPC_LCD->INTCLR |= 1<<2;
	}
}
#endif

/*********************************************************************
*
*       _SetDisplayOrigin()
*/
static void _SetDisplayOrigin(int x, int y) {
  (void)x;
  //
  // Set start address for display data and enable LCD controller
  //
  LPC_LCD->UPBASE = LCD_VRAM_BASE_ADDR + (y * XSIZE_PHYS * PIXEL_WIDTH);  // Needs to be set, before LCDC is enabled
}

#if GUI_SUPPORT_TOUCH  // Used when touch screen support is enabled

/*********************************************************************
*
*       _CheckTouch()
*
* Function decription:
*   Checks if we have a touch right in this moment.
*/
static void _CheckTouch(void) {
  //
  // xPlus = GPIO, out, low, enable input buffer, pull-down
  //
  IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_24, (IOCON_PULL_DOWN | IOCON_ENABLE_INPUT_BUFFER));
  LPC_GPIO0->DIR |= (1uL << 24);
  LPC_GPIO0->CLR |= (1uL << 24);
  //
  // xMinus = GPIO, in, floating, enable input buffer
  //
  IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_23, IOCON_ENABLE_INPUT_BUFFER);
  LPC_GPIO0->DIR &= ~(1uL << 23);
  //
  // yPlus = GPIO, in, enable input buffer, pull-up
  //
  IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_12, (IOCON_PULL_UP | IOCON_ENABLE_INPUT_BUFFER));
  LPC_GPIO0->DIR &= ~(1uL << 12);
  //
  // yMinus = GPIO, in, pull-up, enable input buffer
  //
  IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P1_31, (IOCON_PULL_UP | IOCON_ENABLE_INPUT_BUFFER));
  LPC_GPIO1->DIR &= ~(1uL << 31);
  //
  // Check for yMinus to be low, this means we have a touch
  //
  BSP_DelayMS(1);
  if ((LPC_GPIO1->PIN & (1uL << 31)) == 0) {
    _HasTouch = 1;
  } else {
    _HasTouch = 0;
  }
}

/*********************************************************************
*
*       _ExecTouch
*
* Function description:
*   Check for new touch event. Static x, y coordinates will be updated
*   by the _CheckUpdateTouch() routine. If no touch event has occurred
*   we do nothing.
*/
static void _ExecTouch(void) {
  GUI_TOUCH_Exec();
}

#endif  // GUI_SUPPORT_TOUCH

/*********************************************************************
*
*       _InitController
*
* Function description:
*   Initializes the LCD controller and touch screen
*
*/
static void _InitController(unsigned LayerIndex) {
	#if MULTIBUFF_USE_ISR 	
    _PendingBuffer = -1;
	#endif	
  //
  // Set display size and video-RAM address
  //
  LCD_SetSizeEx (XSIZE_PHYS, YSIZE_PHYS, LayerIndex);
  LCD_SetVSizeEx(VXSIZE_PHYS, VYSIZE_PHYS, LayerIndex);
  LCD_SetVRAMAddrEx(LayerIndex, (void*)LCD_VRAM_BASE_ADDR);
  //
  // Init LCD
  //
  GLCD_Init();
#if MULTIBUFF_USE_ISR 		
	// IRQ Priority has been set in BSP_Init_IRQ_Priorities() function // 
	// NVIC_SetPriority(LCD_IRQn, 2);
	NVIC_ClearPendingIRQ(LCD_IRQn);
	NVIC_EnableIRQ(LCD_IRQn);
	LPC_LCD->INTMSK = 1<<2;	
#endif


#if GUI_SUPPORT_TOUCH  // Used when touch screen support is enabled
  {
    U32 TouchOrientation;
    U32 pclk;
    //
    // Initialize touch screen
    //
    LPC_SC->PCONP |= (1 << 12);  // Enable clock for ADC
    LPC_ADC->CR    = 0
                     | (1          <<  1)  // Sel AD0[1]
                     | (ADC_CLKDIV <<  8)
                     | (1          << 21)  // Enable ADC
                     ;
    //
    // Calibrate touch
    //
    TouchOrientation = (GUI_MIRROR_X * LCD_GetMirrorXEx(0)) |
                       (GUI_MIRROR_Y * LCD_GetMirrorYEx(0)) |
                       (GUI_SWAP_XY  * LCD_GetSwapXYEx (0)) ;
    GUI_TOUCH_SetOrientation(TouchOrientation);
    if (LCD_GetSwapXYEx(0)) {
      GUI_TOUCH_Calibrate(GUI_COORD_X, 0, XSIZE_PHYS, TOUCH_AD_LEFT, TOUCH_AD_RIGHT);   // x axis swapped
      GUI_TOUCH_Calibrate(GUI_COORD_Y, 0, YSIZE_PHYS, TOUCH_AD_TOP , TOUCH_AD_BOTTOM);  // y axis swapped
    } else {
      GUI_TOUCH_Calibrate(GUI_COORD_X, 0, XSIZE_PHYS, TOUCH_AD_LEFT, TOUCH_AD_RIGHT);   // x axis
      GUI_TOUCH_Calibrate(GUI_COORD_Y, 0, YSIZE_PHYS, TOUCH_AD_TOP , TOUCH_AD_BOTTOM);  // y axis
    }
    //
    // Start touch timer
    //
    LPC_SC->PCONP |= (0x1<<2);
    pclk = SystemCoreClock/4;
    TOUCH_SCREEN_TIMER->PR  = pclk/1000000; /* Set prescaler to get 1 M counts/sec */
    TOUCH_SCREEN_TIMER->MR0 = 1000 * TOUCH_TIMER_INTERVAL;
    TOUCH_SCREEN_TIMER->MCR = (0x3<<0);	    /* Interrupt and Reset on MR0 */
    NVIC_EnableIRQ(TOUCH_SCREEN_TIMER_INT);
    TOUCH_SCREEN_TIMER->TCR = 1;            /* Enable timer 1 */
  }
#endif
}

/*********************************************************************
*
*       Public code
*
**********************************************************************
*/
static void _LCD_CopyBuffer(int LayerIndex, int IndexSrc, int IndexDst) 
{
	unsigned long BufferSize, AddrSrc, AddrDst;
	//
	// Calculate the size of one frame buffer
	//
	BufferSize = XSIZE_PHYS * YSIZE_PHYS * PIXEL_WIDTH;
	//
	// Calculate source- and destination address
	//
	AddrSrc = LCD_VRAM_BASE_ADDR  + BufferSize * IndexSrc;
	AddrDst = LCD_VRAM_BASE_ADDR  + BufferSize * IndexDst;
	GUI_MEMCPY((void *)AddrDst, (void *)AddrSrc, BufferSize);
}

/*********************************************************************
*
*       LCD_X_Config
*
* Purpose:
*   Called during the initialization process in order to set up the
*   display driver configuration.
*/
void LCD_X_Config(void) {
  //
  // At first initialize use of multiple buffers on demand
  //
  #if (NUM_BUFFERS > 1)
    GUI_MULTIBUF_Config(NUM_BUFFERS);
  #endif
  //
  // Set display driver and color conversion for 1st layer
  //
  GUI_DEVICE_CreateAndLink(DISPLAY_DRIVER, COLOR_CONVERSION, 0, 0);
  LCD_SetDevFunc(0, LCD_DEVFUNC_COPYBUFFER, (void (*)())_LCD_CopyBuffer);
  //
  // Display driver configuration, required for Lin-driver
  //
  LCD_SetPosEx     (0, 0, 0);
  if (LCD_GetSwapXYEx(0)) {
    LCD_SetSizeEx  (0, YSIZE_PHYS , XSIZE_PHYS);
    LCD_SetVSizeEx (0, VYSIZE_PHYS, VXSIZE_PHYS);
  } else {
    LCD_SetSizeEx  (0, XSIZE_PHYS , YSIZE_PHYS);
    LCD_SetVSizeEx (0, VXSIZE_PHYS, VYSIZE_PHYS);
  }
  LCD_SetVRAMAddrEx(0, (void*)LCD_VRAM_BASE_ADDR);
  //
  // Set user palette data (only required if no fixed palette is used)
  //
  #if defined(PALETTE)
    LCD_SetLUTEx(0, PALETTE);
  #endif
}

/*********************************************************************
*
*       LCD_X_DisplayDriver
*
* Purpose:
*   This function is called by the display driver for several purposes.
*   To support the according task the routine needs to be adapted to
*   the display controller. Please note that the commands marked with
*   'optional' are not cogently required and should only be adapted if
*   the display controller supports these features.
*
* Parameter:
*   LayerIndex - Index of layer to be configured
*   Cmd        - Please refer to the details in the switch statement below
*   pData      - Pointer to a LCD_X_DATA structure
*
* Return Value:
*   < -1 - Error
*     -1 - Command not handled
*      0 - Ok
*/
int LCD_X_DisplayDriver(unsigned LayerIndex, unsigned Cmd, void * pData) {
  int r;
  LCD_X_SETORG_INFO      * pSetOrg;
	#if (!MULTIBUFF_USE_ISR) 
	unsigned long Addr;
	#endif

  (void) LayerIndex;

  switch (Cmd) {
  //
  // Required
  //
  case LCD_X_INITCONTROLLER:
    //
    // Called during the initialization process in order to set up the
    // display controller and put it into operation. If the display
    // controller is not initialized by any external routine this needs
    // to be adapted by the customer...
    //
    // ...
    _InitController(0);
  return 0;
  case LCD_X_SETORG:
    //
    // Required for setting the display origin which is passed in the 'xPos' and 'yPos' element of p
    //
    pSetOrg = (LCD_X_SETORG_INFO *)pData;
    _SetDisplayOrigin(pSetOrg->xPos, pSetOrg->yPos);
    return 0;
	 case LCD_X_SHOWBUFFER: {
		LCD_X_SHOWBUFFER_INFO * pDatax;
		pDatax = (LCD_X_SHOWBUFFER_INFO *)pData;

		//
		// Remember buffer index to be used by ISR
		//
			#if MULTIBUFF_USE_ISR
				_PendingBuffer = pDatax->Index;
			#else
				//
				// Calculate address of the given buffer
				//
				Addr = LCD_VRAM_BASE_ADDR + (U32)FRAME_SIZE * pDatax->Index;
				//
				// Make the given buffer visible
				//
				LPC_LCD->UPBASE = Addr;
				//
				// Send a confirmation that the buffer is visible now
				//
				GUI_MULTIBUF_Confirm(pDatax->Index);
			#endif
		}
  default:
    r = -1;
  }
  return r;
}

/*********************************************************************
*
*       Global functions for GUI touch
*
**********************************************************************
*/

#if GUI_SUPPORT_TOUCH  // Used when touch screen support is enabled

/*********************************************************************
*
*       GUI_TOUCH_X_ActivateX()4
*
* Function decription:
*   Called from GUI, if touch support is enabled.
*   Switches on voltage on X-axis,
*   prepares measurement for Y-axis.
*   Voltage on Y-axis is switched off.
*/
void GUI_TOUCH_X_ActivateX(void) {
  U32 v;

  _CheckTouch();
  if (_HasTouch) {
    //
    // Setup ADC channel 1(P0.24)
    //
    v            = LPC_ADC->CR;
    v           &= ~(0xFFul);
    v           |= (1 << 1);
    LPC_ADC->CR  = v;   //选择通道1
    //
    // xPlus = ADC0_IN[1]
    //设置P0.24引脚为模拟输入引脚，使用引脚功能1
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_24, 1);  
    //
    // yPlus = GPIO out, high, enable input buffer
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_12, IOCON_ENABLE_INPUT_BUFFER);
    LPC_GPIO0->DIR |= (1uL << 12);
    LPC_GPIO0->SET |= (1uL << 12);
    //
    // xMinus = GPIO in, floating
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_23, 0);
    LPC_GPIO0->DIR &= ~(1uL << 23);
    //
    // yMinus = GPIO out, low
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P1_31, 0);
    LPC_GPIO1->DIR |= (1uL << 31);
    LPC_GPIO1->CLR |= (1uL << 31);
  }
}

/*********************************************************************
*
*       GUI_TOUCH_X_ActivateY()2
*
* Function decription:
*   Called from GUI, if touch support is enabled.
*   Switches on voltage on Y-axis,
*   prepares measurement for X-axis.
*   Voltage on X-axis is switched off.
*/
void GUI_TOUCH_X_ActivateY(void) {
  U32 v;

  if (_HasTouch) {
    //
    // Setup ADC channel 6(P0.12)
    //
    v            = LPC_ADC->CR;
    v           &= ~(0xFFul);
    v           |= (1 << 6);    //选择通道6
    LPC_ADC->CR  = v;
    //
    // xPlus = GPIO out, high, enable input buffer
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_24, IOCON_ENABLE_INPUT_BUFFER);
    LPC_GPIO0->DIR |= (1uL << 24);
    LPC_GPIO0->SET |= (1uL << 24);
    //
    // yPlus = ADC0_IN[6]
    //P0.12选择模拟输入模式，引脚选择第三功能
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_12, 3);  
    //
    // xMinus = GPIO out, low
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P0_23, 0);
    LPC_GPIO0->DIR |= (1uL << 23);
    LPC_GPIO0->CLR |= (1uL << 23);
    //
    // yMinus = GPIO in, floating
    //
    IOCON_CLR_FUNC_MODE_ADMODE_AND_ADD_MASK(P1_31, 0);
    LPC_GPIO1->DIR &= ~(1uL << 31);
  }
}

/*********************************************************************
*
*       GUI_TOUCH_X_MeasureX()3
*
* Function decription:
*   Called from GUI, if touch support is enabled.
*   Measures voltage of X-axis.
*/
int  GUI_TOUCH_X_MeasureX(void) {
  U32 v;
  U32 x;
  int i;

  if (_HasTouch) {
    x = 0;

    for (i = 0; i < 16; i++) {
      LPC_ADC->CR |= (1 << 24);  // Start measurement
      do {
        v = LPC_ADC->DR[6];
      } while ((v & (1UL << 31)) == 0);
      x += ((v & 0xFFFF) >> 4);
    }
    x >>= 4;
    return x;
  }
  return -1;
}

/*********************************************************************
*
*       GUI_TOUCH_X_MeasureY()1
*
* Function decription:
*   Called from GUI, if touch support is enabled.
*   Measures voltage of Y-axis.
*/
int  GUI_TOUCH_X_MeasureY(void) {
  U32 v;
  U32 y;
  int i;

  if (_HasTouch) {
    y = 0;

    for (i = 0; i < 16; i++) {
      LPC_ADC->CR |= (1 << 24);  // Start measurement
      do {
        v = LPC_ADC->DR[1];
      } while ((v & (1UL << 31)) == 0);
      y += ((v & 0xFFFF) >> 4);
    }
    y >>= 4;
    return y;
  }
  return -1;
}

#endif  // GUI_SUPPORT_TOUCH

/*************************** End of file ****************************/

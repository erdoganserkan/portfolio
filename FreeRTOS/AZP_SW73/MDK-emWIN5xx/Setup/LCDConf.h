#ifndef LCDCONF_H
#define LCDCONF_H
 
// SRK_NOTE:: Denemelerimde 4' ten az buffer kullan1l1rsa ekranda artifact olusuyordu.  
#define NUM_BUFFERS  					4 				// Number of multiple buffers to be used
#if(NUM_BUFFERS > 1)
	//#define MULTIBUFF_USE_ISR   	1
#endif

/*********************************************************************
*
*       Layer configuration (to be modified)
*
**********************************************************************
*/

#ifndef USE_24BPP_MODE
  #define USE_24BPP_MODE  0  // Selection if using 16 BPP or 24 BPP mode
#endif

//
// Physical display size
//
#define XSIZE_PHYS GLCD_X_SIZE
#define YSIZE_PHYS GLCD_Y_SIZE

//
// Color conversion
//
#if USE_24BPP_MODE
  #define COLOR_CONVERSION  GUICC_888
#else
	#define COLOR_CONVERSION  GUICC_M565		/*GUICC_M565: RB SWAP GUICC_565:RB NORMAL*/
#endif

//
// Pixel width in bytes
//
#if USE_24BPP_MODE
  #define PIXEL_WIDTH  4
#else
  #define PIXEL_WIDTH  2
#endif

//
// Display driver
//
#if USE_24BPP_MODE
  #define DISPLAY_DRIVER  &GUIDRV_Lin_32_API
#else
  #define DISPLAY_DRIVER  &GUIDRV_Lin_16_API
#endif

//
// Video RAM address
//
//#define VRAM_ADDR_PHYS  (U32)&_aVRAM[0]

//
// Touch controller settings
#define TOUCH_AD_LEFT         320       //触摸屏最左端ADC采样值
#define TOUCH_AD_RIGHT        3550      //触摸屏最右端ADC采样值
#define TOUCH_AD_TOP          800       //触摸屏最上端ADC采样值
#define TOUCH_AD_BOTTOM       3050      //触摸屏最下端ADC采样值
#define TOUCH_TIMER_INTERVAL  10        //触摸屏采样间隔
#define NUM_VSCREENS 					1 				// Number of virtual screens to be used

/*********************************************************************
*
*       Configuration checking
*
**********************************************************************
*/
#ifndef   VXSIZE_PHYS
  #define VXSIZE_PHYS (XSIZE_PHYS)
#endif
#ifndef   VYSIZE_PHYS
  #define VYSIZE_PHYS (YSIZE_PHYS * NUM_BUFFERS)
#endif

#define FRAME_SIZE 	(XSIZE_PHYS * YSIZE_PHYS * PIXEL_WIDTH)

#endif /* LCDCONF_H */

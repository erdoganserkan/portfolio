#ifndef MYFB_H
#define MYFB_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <unistd.h>
#include <math.h>
#include "Logger.h"

#define XMAX TFT_SCREEN_X_RES
#define YMAX TFT_SCREEN_Y_RES


typedef struct
{
	Int16 x;			// must be signed integer //
	Int16 y;			// must be signed integer //
}point;

typedef struct
{
	point LeftUp;
	point RightDown;
	Uint8 *DataPtr;
} ScreenAreaType;

class myFrameBuffer:public QObject
{

public:
	myFrameBuffer(char *devName);
	~myFrameBuffer();
	inline void locatePixel(Int16 x, Int16 y,u_int16_t pixelColor);
	inline void SetXLine(Int16 x1,Int16 x2,Int16 y,u_int16_t pixelColor);
	u_int16_t getRBGValue(colorRGB pixelColor);
	void drawRectangle(Int16 x1, Int16 y1, Int16 x2, Int16 y2, colorRGB rgbColor);
	void drawSymmetricalColoredRectangle(Uint16 x1, Uint16 y1, Uint16 x2, Uint16 y2, Uint8 ColorData);
	void drawCircle(point CenterP, Int16 Radious, colorRGB circleColor);
	void drawHalfCircle(point CenterP, Int16 Radious, colorRGB circleColor);
	void drawQuarterCircle(point CenterP,Int16 Radious, colorRGB circleColor);
	void FillRectange(Int16 x1, Int16 y1, Int16 x2, Int16 y2, Uint16 Data);

	void StoreFBData(ScreenAreaType *AreaDef);
	void RestoreFBData(ScreenAreaType *AreaDef);

private:

	int FrameBufferFD;						/*frameBuffer device file handler integer*/
	char deviceName[30];					/*path to device node in file system*/
	struct fb_fix_screeninfo fixInfo;
	struct fb_var_screeninfo varInfo;
	unsigned int lineLenght;			/*lineLenght of frameBuffer in BYTEs*/
	void *mmapAddress;					/* mmap() function return address to process*/
	int mmapSize;						/* mmap() function allocated memory size*/
	void releaseFB(void);
	bool initFB(char *deviceName);

	//---------------- LOGGING ---------------//
	Logger *CLogger;
};

#define LOCATE_PIXEL(x,y,pixelColor)   	\
{\
	/***  pixel(x,y)=(line_width * y) + x. *****/\
	*((Uint16 *)mmapAddress + ((fixInfo.line_length/2)*y) +x) = (Uint16)pixelColor;\
}


inline void  myFrameBuffer::locatePixel(Int16 x, Int16 y, Uint16 pixelColor)
{
	/***  pixel(x,y)=(line_width * y) + x. *****/
	*((Uint16 *)mmapAddress + ((fixInfo.line_length/2)*y) +x) = (Uint16)pixelColor;
}


inline u_int16_t myFrameBuffer::getRBGValue(colorRGB pixelColor)
{
	u_int16_t pixel;

/*	Red len=5, off=11 : Green len=6, off=6 : Blue len=5, off=0	*/
/*  pixel_value = ((0xFF >> (8 - 5) << 11)| ((0xFF >> (8 - 6) << 6) | ((0xFF >> (8 - 5) << 0) = 0xFFFF **** White	*/

	pixel = ((pixelColor.red >> (8-varInfo.red.length)) << varInfo.red.offset) |
	((pixelColor.green >> (8-varInfo.green.length)) << varInfo.green.offset) |
	((pixelColor.blue >>(8-varInfo.blue.length)) << varInfo.blue.offset);

	return pixel;
}


#endif

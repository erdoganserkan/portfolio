#ifndef MYFRAMEBUFFER_C
#define MYFRAMEBUFFER_C

#include "myFB.h"
#include <math.h>

#define CALC_WITH_ABS_FUNC FALSE

myFrameBuffer::myFrameBuffer(char *devName)
{
	//--- Set variables to initial state ----------//
	CLogger = NULL;
	mmapAddress = NULL;
	mmapSize = 0;
	FrameBufferFD = -1;			// This is the return value of open() syscall @ failure state
									// so dont change this, default state is FAILURE
	memset(deviceName,0,sizeof(deviceName));

	//-------- Create Logger ---------//
	CLogger = new Logger(GetApp()->CalcLogDevicePath(MYFB_RLOG_DEVICE).toAscii().data(),MYFB_LOG_LEVEL);

	//------- Do Class Initialization ---------//
	strcpy(deviceName,devName);
	if(initFB(deviceName)==false)
	{
		CL_LOG(ERROR,"initFB() FAILED on (%s)\n",deviceName);
	}
}


myFrameBuffer::~myFrameBuffer()
{
	releaseFB();

	if(CLogger != NULL)
	{
		delete CLogger;
		CLogger = NULL;
	}
}


void myFrameBuffer::StoreFBData(ScreenAreaType *AreaDef)
{
	Uint16 XLength = AreaDef->RightDown.x - AreaDef->LeftUp.x;
	Uint16 YLength = AreaDef->RightDown.y - AreaDef->LeftUp.y;
	AreaDef->DataPtr = (Uint8 *)calloc(1,(YLength*XLength*2));

	if(AreaDef->DataPtr == NULL)
	{
		CL_LOG(ERROR,"calloc() FAILED :R:(%s)\n",strerror(errno));
		return;
	}
	for(Uint16 y=AreaDef->LeftUp.y ; y<AreaDef->RightDown.y ; y++)
	{
		Uint8 *ScrPtr = (Uint8 *)mmapAddress + (TFT_SCREEN_X_RES*y*2) + (AreaDef->LeftUp.x*2);
		Uint8 *DestPtr = AreaDef->DataPtr + (y-AreaDef->LeftUp.y)*(XLength*2);
		memcpy(DestPtr,ScrPtr,XLength*2);
	}
	CL_LOG(DEBUG,"FINISHED\n");
}


void myFrameBuffer::RestoreFBData(ScreenAreaType *AreaDef)
{
	Uint16 XLength = AreaDef->RightDown.x - AreaDef->LeftUp.x;

	CL_LOG(DEBUG,"CALLED with Xrange[%d-%d] ;; YRange[%d-%d]\n", \
		AreaDef->LeftUp.x, AreaDef->RightDown.x, AreaDef->LeftUp.y, AreaDef->RightDown.y);

	if(AreaDef->DataPtr == NULL)
	{
		CL_LOG(ERROR,"Parameter is NULL\n\n");
		return;
	}
	for(Uint16 y=AreaDef->LeftUp.y ; y<AreaDef->RightDown.y ; y++)
	{
		Uint8 *DestPtr  = (Uint8 *)mmapAddress + (TFT_SCREEN_X_RES*y*2) + (AreaDef->LeftUp.x*2);
		Uint8 *ScrPtr = AreaDef->DataPtr + (y-AreaDef->LeftUp.y)*(XLength*2);
		memcpy(DestPtr,ScrPtr,XLength*2);
	}
	CL_LOG(DEBUG,"FINISHED\n");
}



bool myFrameBuffer::initFB(char *deviceName)
{
	FrameBufferFD = open(deviceName, O_RDWR);
	if (FrameBufferFD < 0)
	{
		CL_LOG(ERROR, "open(%s) FAILED :R: (%s)\n", deviceName, strerror(errno));
		return false;
	}

	if (ioctl(FrameBufferFD, FBIOGET_FSCREENINFO, &fixInfo) < 0)
	{
		CL_LOG(ERROR,"getting FIXED screen info FAILED :R: (%s)\n", strerror(errno));
		close(FrameBufferFD);
		return false;
	}
	else
		CL_LOG(DEBUG,"getting FIXED screen info SUCCESS \n");

	if (ioctl(FrameBufferFD, FBIOGET_VSCREENINFO, &varInfo) < 0)
	{
		CL_LOG(ERROR, "getting VARIABLE screen info FAILED :R: (%s)\n", strerror(errno));
	    close(FrameBufferFD);
	    return false;
	}
	else
		CL_LOG(DEBUG,"getting VARIABLE screen info SUCCESS \n");

	/* Calculate the size to mmap */
	mmapSize = fixInfo.line_length * varInfo.yres;

	/* Now mmap the framebuffer. */
	mmapAddress = mmap(NULL, mmapSize, PROT_READ | PROT_WRITE, MAP_SHARED, FrameBufferFD,0);
	if(mmapAddress == NULL)
	{
		CL_LOG(ERROR, "mmap() FAILED :R: (%s) \n",strerror(errno));
		close(FrameBufferFD);
		return false;
	}

	CL_LOG(DEBUG,"initFB() SUCCESFULL \n");
	return true;
}


void myFrameBuffer::FillRectange(Int16 x1, Int16 y1, Int16 x2, Int16 y2, Uint16 ColorData)
{

}


void myFrameBuffer::releaseFB(void)
{
	CL_LOG(PRNY,"ENTERED \n");

	if((mmapAddress != NULL) && (mmapSize != 0))
	{
		munmap(mmapAddress,0);
		CL_LOG(DEBUG,"munmap() SUCCESS \n");
	}

	if(FrameBufferFD != -1)			// -1 is default value, check constructor for init value
	{
		close(FrameBufferFD);
		CL_LOG(DEBUG,"fclose() SUCCESS \n");
	}

	CL_LOG(PRNY,"EXITING \n");
}


void myFrameBuffer::drawSymmetricalColoredRectangle(Uint16 x1, Uint16 y1, Uint16 x2, Uint16 y2, Uint8 ColorData)
{
	Uint16 BytePerPixel = fixInfo.line_length/TFT_SCREEN_X_RES;
	if(y1 > y2)
	{
		Uint16 TempU16 = y1;
		y1 = y2;
		y2 = TempU16;
	}
	if(x1 > x2)
	{
		Uint16 TempU16 = x1;
		x1 = x2;
		x2 = TempU16;
	}

	Uint8 *StartAddress = ((Uint8 *)mmapAddress) + y1*(fixInfo.line_length) + x1*BytePerPixel;
	for(Uint16 y=y1;y<y2;y++)
	{
		memset(StartAddress,ColorData,(x2-x1)*BytePerPixel);
		StartAddress += fixInfo.line_length;
 	}
}


void myFrameBuffer::drawRectangle(Int16 x1, Int16 y1, Int16 x2, Int16 y2, colorRGB rgbColor)
{
	Int16 x = 0;
	Int16 y = 0;
	u_int16_t color = getRBGValue(rgbColor);
	if(x1 > x2)
	{
		Int16 TempU16 = x1;
		x1 = x2;
		x2 = TempU16;
	}
	if(y1 > y2)
	{
		Int16 TempU16 = y1;
		y1 = y2;
		y2 = TempU16;
	}
	for(x = x1; x < x2 ; ++x)
	{
		for(y = y1; y < y2 ; ++y)
		{
			locatePixel(x,y,color);
		}
	}
}


void myFrameBuffer::drawCircle(point CenterP, Int16 Radious, colorRGB circleColor)
{
	Int16 x=0,y=0;
	Int16 XMin = CenterP.x - Radious;
	Int16 XMax = CenterP.x + Radious;
	Int16 YMax = CenterP.y + Radious;
	Int16 YMin = CenterP.y - Radious;

	CL_LOG(PRNY, "ENTERED \n");
	u_int16_t colorOfCircle = getRBGValue(circleColor);
	for(x = XMin; x <= XMax ; ++x)
	{
		for(y = YMin; y <= YMax ; ++y)
		{
		#if(CALC_WITH_ABS_FUNC == FALSE)
			if(x < CenterP.x)
			{
				if(y < CenterP.y)
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
			else
			{
				if(y < CenterP.y)
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
		#else
			if((abs(x-CENTERX)*abs(x-CENTERX)+abs(y-CENTERY)*abs(y-CENTERY)) <= (RADIOUS*RADIOUS))
				locatePixel(x,y,colorOfCircle);
		#endif
		}
	}

}


void myFrameBuffer::drawHalfCircle(point CenterP, Int16 Radious, colorRGB circleColor)
{
	Int16 x=0,y=0;
	Int16 XMin = CenterP.x - Radious;
	Int16 XMax = CenterP.x + Radious;
	Int16 YMin = CenterP.y - Radious;

	CL_LOG(PRNY,"ENTERED \n");
	u_int16_t colorOfCircle = getRBGValue(circleColor);
	for(x = XMin; x <= XMax ; ++x)
	{
		for(y = YMin; y <= CenterP.y ; ++y)
		{
		#if(CALC_WITH_ABS_FUNC == FALSE)
			if(x < CenterP.x)
			{
				if(y < CenterP.y)
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
			else
			{
				if(y < CenterP.y)
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
		#else
			if((abs(x-CENTERX)*abs(x-CENTERX)+abs(y-CENTERY)*abs(y-CENTERY)) <= (RADIOUS*RADIOUS))
				locatePixel(x,y,colorOfCircle);
		#endif
		}
	}

}


void myFrameBuffer::drawQuarterCircle(point CenterP,Int16 Radious, colorRGB circleColor)
{
	Int16 x=0,y=0;
	Int16 XMax = CenterP.x + Radious;
	Int16 YMin = CenterP.y - Radious;
	CL_LOG(PRNY,"ENTERED \n");
	u_int16_t colorOfCircle = getRBGValue(circleColor);
	for(x = CenterP.x; x <= XMax; ++x)
	{
		for(y = YMin; y <= CenterP.y ; ++y)
		 {
			if(x < CenterP.x)
			{
				if(y < CenterP.y)
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((CenterP.x-x)*(CenterP.x-x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
			else
			{
				if(y < CenterP.y)
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(CenterP.y-y)*(CenterP.y-y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
				else
				{
					if(((x-CenterP.x)*(x-CenterP.x) +(y-CenterP.y)*(y-CenterP.y)) <= (Radious*Radious))
						locatePixel(x,y,colorOfCircle);
				}
			}
		}
	}
}


#endif

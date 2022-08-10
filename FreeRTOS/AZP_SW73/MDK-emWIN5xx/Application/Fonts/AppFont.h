#ifndef APP_FONT_H
#define APP_FONT_H

#include "GUI_Type.h"

#define FONTBIG						1
#define FONTD6x8					0
#define FONTROUNDED16			1
#define FONTROUNDED22			1
#define FONTSOUVENIR18		1

extern GUI_CONST_STORAGE GUI_FONT GUI_FontD6x8;
#if(0)	
	extern GUI_CONST_STORAGE GUI_FONT GUI_FontArial24B;
	extern GUI_CONST_STORAGE GUI_FONT GUI_FontArial32B;
	// Real application fonts, requires very much ROM space // 
	#define APP_32B_FONT	(&GUI_FontArial32B)
	#define APP_24B_FONT	(&GUI_FontArial24B)
#elif(1)
	extern GUI_CONST_STORAGE GUI_FONT GUI_FontArial24Bstd;
	extern GUI_CONST_STORAGE GUI_FONT GUI_FontArial32Bstd;
	// Real application fonts but used @runtime, does not require very much ROM space // 
	// But there is a problem ONLY in RUSSIAN language // 
	#define APP_16B_FONT	(&GUI_FontArray[ARIAL_16B_INDX])
	#define APP_19B_FONT	(&GUI_FontArray[ARIAL_19B_INDX])
	#define APP_24B_FONT	(&GUI_FontArray[ARIAL_24B_INDX])
	#define APP_32B_FONT	(&GUI_FontArray[ARIAL_32B_INDX])
#else
	// Debug fonts, Load for application quick load & unload during development & debugging // 
	#define APP_16B_FONT	GUI_FONT_16B_ASCII
	#define APP_19B_FONT	GUI_FONT_16B_ASCII
	#define APP_32B_FONT	GUI_FONT_32B_ASCII
	#define APP_24B_FONT	GUI_FONT_24B_ASCII
#endif
// MY FONTs created with Font Converter // 
//extern GUI_CONST_STORAGE GUI_FONT GUI_FontTimesNewRoman20;
//extern GUI_CONST_STORAGE GUI_FONT GUI_FontTimesNewRoman32;

extern void RuntimeFontsInit(void);

#endif

#ifndef POPUP_H
#define POPUP_H

#include <stdint.h>
#include "PROGBAR.h"
#include "WM.h"

#define POPUP_MSG_XSIZE		(250)
#define POPUP_MSG_YSIZE		(120)

#define POPUP_BIG_XSIZE		(350)
#define POPUP_BIG_YSIZE		(200)

#define PROGBAR_XSIZE	(150)
#define PROGBAR_YSIZE	(30)
#define POPUP_TEXT_XSIZE		PROGBAR_XSIZE
#define POPUP_TEXT_YSIZE		PROGBAR_YSIZE

#define POPUP_BIGICON_YSIZE		(50)
#define POPUP_BIGICON_YINTERVAL	(POPUP_BIGICON_YSIZE/4)
#define POPUP_BIGICON_XSIZE		(50)
#define POPUP_BIGICON_XINTERVAL	(POPUP_BIGICON_XSIZE/4)
#define POPUP_SMALLICON_YSIZE	(30)
#define POPUP_SMALLICON_XSIZE	(30)
#define POPUP_ICON_XSTART			(10)

#define POPUP_INTERVAL_AROUND_BIGSTR	(10)
#define POPUP_FADING_DELAY_MS		(750)
#define POPUP_MOVING_DELAY_MS		(750)
#define POPUP_SHIFTING_DELAY_MS		(750)

// BATTERY POPUPs // 
#define BAT_POPUP_STR_LEFTX		10
#define BAT_POPUP_STR_RIGHTX	230
#define BAT_POPUP_STR_LEFTY		20
#define BAT_POPUP_STR_RIGHTY	220

#define BAT_POPUP_ICON_LEFTX	233
#define BAT_POPUP_ICON_LEFTY	18
#define BAT_POPUP_ICON_SIZEX	143
#define BAT_POPUP_ICON_SIZEY	188

#define BAT_POPUP_DISPLAY_MS	2000
#define BAT_POPUP_CD_MS				250

// LANG POPUP // 
#define LANG_POPUP_FLAG_ANIM_MS		150
#define LANG_POPUP_SIZEX		411
#define LANG_POPUP_SIZEY		219

#define FLAG_SIZEX	74
#define FLAG_SIZEY	55

#define FLAG_UPY_POS		75
#define FLAG_DOWNY_POS	180
#define FLAG_POS1X			62	// up
#define FLAG_POS2X			156	// up
#define FLAG_POS3X			252	// up
#define FLAG_POS4X			347	// up
#define FLAG_POS5X			62	// down 
#define FLAG_POS6X			156	// down
#define FLAG_POS7X			252	// down
#define FLAG_POS8X			347	// down 

// FACTORY POPUP //
#define FACTORY_POPUP_SIZEX LANG_POPUP_SIZEX
#define FACTORY_POPUP_SIZEY LANG_POPUP_SIZEY

#define FACTORY_ICON_POSX		205
#define FACTORY_ICON_POSY		50
#define FACTORY_STR_UPX			27
#define FACTORY_STR_UPY			91
#define FACTORY_STR_DOWNX			390
#define FACTORY_STR_DOWNY			140

#define YES_STR_POSX	65
#define YES_STR_POSY	168
#define NO_STR_POSX		280
#define NO_STR_POSY		YES_STR_POSY

#define YES_BUTTON_POSX			81
#define NO_BUTTON_POSX			291
#define BUTTONs_POSY				200
#define BUTTON_SIZEX	112
#define BUTTON_SIZEY	44

// SENSE, VOLUME, BRIGHTNESS POPUPs //
#define TOT_BAR_COUNT	19
#define SENS_POPUP_SIZEX LANG_POPUP_SIZEX
#define SENS_POPUP_SIZEY LANG_POPUP_SIZEY
#define VOLUME_POPUP_SIZEX LANG_POPUP_SIZEX
#define VOLUME_POPUP_SIZEY LANG_POPUP_SIZEY
#define BRIGHT_POPUP_SIZEX LANG_POPUP_SIZEX
#define BRIGHT_POPUP_SIZEY LANG_POPUP_SIZEY
#define BAR_XPOS_START	95
#define BAR_POSX_INT 18
#define BAR_YPOS		90
#define BAR_SIZEX		10
#define BAR_SIZEY		100
#define SETTING_ICON_XPOS		72
#define SETTING_ICON_YPOS		60
#define CIRCLE_XPOS		30
#define CIRCLE_YPOS		180
#define CIRCLE_DIAMETER		20
#define CIRCLE_PEN_SIZE		5
#define BACKGROUND_COLOR		(0x795E0A)

// INVERT POPUP & FERROS POPUP // 
#define BACKGRND_CLR	0x00836511
#define ACTIVE_BUTTON_POSX			81
#define PASSIVE_BUTTON_POSX			291
#define BUTTONs2_POSY				250
#define BUTTON2_SIZEX	100
#define BUTTON2_SIZEY	100

#define PICS_SIZEX	138
#define PICS_SIZEY	138
#define PIC1_POSX		((ACTIVE_BUTTON_POSX + (BUTTON2_SIZEX/2))-(PICS_SIZEX/2) +20)
#define PIC2_POSX		((PASSIVE_BUTTON_POSX + (BUTTON2_SIZEX/2))-(PICS_SIZEX/2)+20)
#define PICs_POSY		(BUTTONs2_POSY-10-PICS_SIZEY)

#define SETTINGS_STR_UPX		((WM_GetWindowSizeX(hWin)-200)/2)
#define SETTINGS_STR_DOWNX		(SETTINGS_STR_UPX + 200)
#define SETTINGS_STR_UPY		5
#define SETTINGS_STR_DOWNY		55

#define YES_STR_POSX2	65
#define YES_STR_POSY2	175
#define NO_STR_POSX2	280
#define NO_STR_POSY2		YES_STR_POSY2

// METAL ANALIZI RESULT POPUP //
#define MA_RES_POPUP_SIZEX LANG_POPUP_SIZEX
#define MA_RES_POPUP_SIZEY LANG_POPUP_SIZEY

typedef struct
{
	uint8_t type;	// type, as a creation parameter // 
	uint8_t last_key;	// Last key that popup handles before exit (KEY_OK or KEY_ESC/KEY_MENU)
	union {
		struct	// GB popup specific data // 
		{
			uint8_t Result;
			uint16_t ID;
		} GBPopupData;		
		const char *Msg;
		struct {
			uint8_t BatMode;	// eBatteryPopupState //
			uint8_t CDStarted;	// Count down Started or Not // 
			uint8_t ActiveNum;
		} BatPopupData;
	} PopupData;
	WM_HWIN hPop;	// Popup Handle // 
	WM_HWIN hParent;	// Popup Parent Window Handle // 
	WM_HWIN hPrevFocused;
} sPopup;

typedef enum
{
	BAT_WARNING	= 0,
	BAT_EMPTY	= 1,
	
	BAT_POPUP_STATE_COUNT
} eBatteryPopupState;

typedef enum
{
	MESSAGE_POPUP				= 0,		// Used for user information diplay // 
	GB_POPUP						= 1,		// Used for GB Screen Result Display // 
	BATTERY_POPUP				= 2,		// Used for "Battery Warning" & "Battery Empty" indications // 
	LANG_POPUP					= 3,		// Used for language selection // 
	FACTORY_POPUP				= 4,
	VOLUME_POPUP				= 5,
	SENS_POPUP					= 6,
	BRIGHT_POPUP				= 7,
	FERROS_POPUP				= 8,
	
	POPUP_TYPE_CONUT
	
} ePopupType;

extern void ShowMessages(WM_HWIN hWin);
extern void ShowPopup(sPopup *pPara);

#endif

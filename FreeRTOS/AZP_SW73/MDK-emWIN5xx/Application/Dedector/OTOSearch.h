#ifndef OTO_SEARCH_H
#define OTO_SEARCH_H

#include <stdint.h>
#define OTO_BAR_COUNT			20
#define OTO_BAR_BCKGRND_COLOR		(0x808080)

#define GAUGE_WINDOW_STARTX			100
#define GAUGE_WINDOW_STARTY			49
#define GAUGE_WINDOW_SIZEX			300
#define GAUGE_WINDOW_SIZEY			159
#define GAUGES_SIZEX			12
#define GAUGES_INTERVALX	3
#define GAUGES_BORDERX		50
#define GAUGES_STARTY			134

#define INFO_WINDOW_STARTX			405
#define INFO_WINDOW_STARTY			46
#define INFO_WINDOW_SIZEX				69
#define INFO_WINDOW_SIZEY				159

#define GSTR_WINDOW_STARTX			100
#define GSTR_WINDOW_STARTY			210
#define GSTR_WINDOW_SIZEX				375
#define GSTR_WINDOW_SIZEY				53
#define GSTR_BACK_COLOR					(0x00813A19)

#define GSTR_UPX		(((480 + STATUS_BAR_X_SIZE)>>1) - 40)
#define GSTR_DOWNX	(GSTR_UPX + 80)
#define GSTR_UPY		220
#define GSTR_DOWNY	252

#define SCRNAME_WINDOW_STARTX			100
#define SCRNAME_WINDOW_STARTY			0
#define SCRNAME_WINDOW_SIZEX				373
#define SCRNAME_WINDOW_SIZEY				45

#define TARGET_PICs_SIZEX	80
#define TARGET_PICs_SIZEY	80

typedef struct
{
	uint16_t xup;
	uint16_t yup;
} OTOBarType;

extern uint8_t OTOSearch(void);
#endif

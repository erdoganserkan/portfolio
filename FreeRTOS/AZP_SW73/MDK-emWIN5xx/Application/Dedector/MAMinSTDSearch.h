#ifndef MA_SEARCH_H
#define MA_SEARCH_H

#include <stdint.h>

// GAUGE STR @ left side of screen // 
#define MA_GAUGE_NUM_POSX	113
#define MA_GAUGE_NUM_POSY	99
#define MA_GAUGE_NUM_SIZEX	52
#define MA_GAUGE_NUM_SIZEY	52

// FERRO / NFERRO ICON WINDOW @ right side of screen // 
#define MA_FERRO_PIC_POSX	408
#define MA_FERRO_PIC_POSY	92
#define MA_FERRO_PIC_SIZEY	65

// MID WINDOW (scope or target-id pictures/strings)
#define MA_DRAWING_LINE_WIDTH			6
#define MA_MID_WIN_POSX		187
#define MA_MID_WIN_POSY		73
#define MA_MID_WIN_SIZEX	203
#define MA_MID_WIN_SIZEY	98
#define SCOPE_GRAPH_SIZEX			(MA_MID_WIN_SIZEX - (2*MA_DRAWING_LINE_WIDTH))
#define SCOPE_GRAPH_SIZEY			(MA_MID_WIN_SIZEY - (2*MA_DRAWING_LINE_WIDTH))
#define SCOPE_GRAPH_POSX			MA_DRAWING_LINE_WIDTH
#define SCOPE_GRAPH_POSY			MA_DRAWING_LINE_WIDTH

// GAUGE WINDOW active area // 
#define MA_GAUGE_BAR_COUNT		16
#define MA_GAUGE_WINDOW_POSX	99
#define MA_GAUGE_WINDOW_POSY	178
#define MA_GAUGE_WINDOW_SIZEX	374
#define MA_GAUGE_WINDOW_SIZEY	86

#define MA_GAUGE_CIRCLE_POSX	187
#define MA_GAUGE_CIRCLE_POSY	58
#define MA_GAUGE_CIRCLE_RADIUS	24


// SCREEN STR //
#define MA_SCR_STR_UPX		150	
#define MA_SCR_STR_UPY		10
#define MA_SCR_STR_DOWNX	450
#define MA_SCR_STR_DOWNY	50


extern uint8_t MAMinSTDSearch(void);

#endif

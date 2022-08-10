#ifndef DEPTH_CALC_H
#define DEPTH_CALC_H

#include <stdio.h>
#include <stdint.h>

#define DEPTH_CALC_WINDOW_POSX		0
#define DEPTH_CALC_WINDOW_POSY		0
#define DEPTH_CALC_WINDOW_SIZEX		480
#define DEPTH_CALC_WINDOW_SIZEY		272

#define DEPTH_GUI_STEPs_COUNT	5
#define LEFT_NUM_POSX			304
#define LEFT_NUM_POSY			86
#define RIGHT_NUM_POSX		391
#define RIGHT_NUM_POSY		86

#define STR_RECT_UPX			LEFT_NUM_POSX
#define STR_RECT_UPY			5
#define STR_RECT_DOWNX		470
#define STR_RECT_DOWNY		56
#define STR_RECT_DOWNY2		260

#define LOWNUM_LEFT_UPX		331
#define LOWNUM_LEFT_UPY		129
#define REALNUM_LEFT_UPX	326	
#define REALNUM_LEFT_UPY	166
#define HIGHNUM_LEFT_UPX	330
#define HIGHNUM_LEFT_UPY	208

#define LOWNUM_RIGHT_UPX		419
#define LOWNUM_RIGHT_UPY		129
#define REALNUM_RIGHT_UPX		414
#define REALNUM_RIGHT_UPY		166
#define HIGHNUM_RIGHT_UPX		418
#define HIGHNUM_RIGHT_UPY		208

#define EN_STR_UPX (LOWNUM_LEFT_UPX + 2)
#define EN_STR_UPY 68
#define BOY_STR_UPX (LOWNUM_RIGHT_UPX + 2)
#define BOY_STR_UPY 68

// Result Related Components // 
#define DPT_RULER_POSX	29
#define DPT_RULER_POSY	111
#define DPT_RULER_SIZEX	35
#define DPT_RULER_SIZEY	151 
#define DPT_RULER_STRs_POSX	55
#define DPT_RULER_FIRST_STR_POSY	108
#define DPT_RULER_STRs_Y_INTERVAL	29

#define DPT_COIL_POSX		130
#define DPT_COIL_POSY		53
#define DPT_COIL_SIZEX	137
#define DPT_COIL_SIZEY	196

#define DPT_TARGET_POSX	119
#define DPT_TARGET_POSY_MIN	85
#define DPT_TARGET_POSY_MAX	230
#define DPT_TARGET_SIZEX		116
#define DPT_TARGET_SIZEY		57

#define DPT_REPORT_POSX		311
#define DPT_REPORT_POSY		6
#define DPT_REPORT_SIZEX	161
#define DPT_REPORT_SIZEY	256


// Shared Variables // 
extern uint8_t DepthCalcEnabled;	

// Shared Functions // 
extern uint8_t DEPTHCalc(void);

#endif

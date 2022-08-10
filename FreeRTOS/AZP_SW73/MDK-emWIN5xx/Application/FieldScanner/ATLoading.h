#ifndef AT_LOADING_H
#define AT_LOADING_H

#include <stdio.h>
#include <stdint.h>

//------------------------------------------//
//-------------- AT LOADING -----------------// 
//------------------------------------------//
// Loading str is 32 punto // 
#define AT_LOADING_STR_LEFT_X		40	
#define AT_LOADING_STR_LEFT_Y		160
#define AT_LOADING_STR_RIGHT_X		440
#define AT_LOADING_STR_RIGHT_Y		195

#define AT_LOADING_ICON_LEFT_X		50
#define AT_LOADING_ICON_LEFT_Y		29
#define AT_LOADING_ICON_SIZE_X		128
#define AT_LOADING_ICON_SIZE_Y		128

#define AT_LOADING_NEXT_SCR_STR_LEFT_X	200
#define AT_LOADING_NEXT_SCR_STR_LEFT_Y	39

#define AT_LOADING_BAR_WIN_LEFT_X			179
#define AT_LOADING_BAR_WIN_LEFT_Y			198
#define AT_LOADING_BAR_SIZE_X			129
#define AT_LOADING_BAR_SIZE_Y			21

#define AT_LOADING_SCREEN_DURATION_MS	(5000)
#define AT_LOADING_BAR_COUNT			5

extern uint8_t AT_Loading(void);

#endif

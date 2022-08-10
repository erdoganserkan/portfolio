#ifndef AT_BRIGHT_VOL_H
#define AT_BRIGHT_VOL_H

#include <stdio.h>
#include <stdint.h>

// Loading str is 32 punto // 
#define AT_BRIGHT_VOL_STR_LEFT_X	137
#define AT_BRIGHT_VOL_STR_LEFT_Y	30

#define AT_BRIGHT_VOL_ICON_LEFT_X	97
#define AT_BRIGHT_VOL_ICON_LEFT_Y	74

// number punto is smaller than battery_auto_power_off popup // 
#define AT_BRIGHT_VOL_NUM_LEFT_X	249
#define AT_BRIGHT_VOL_NUM_LEFT_Y	108

#define AT_BRIGHT_VOL_WIN_LEFT_X		254
#define AT_BRIGHT_VOL_WIN_LEFT_Y		65
#define AT_BRIGHT_VOL_WIN_SIZE_X		141
#define AT_BRIGHT_VOL_WIN_SIZE_Y		(AT_BRIGHT_VOL_BAR_SIZE_Y * AT_BRIGHT_VOL_BAR_COUNT)

#define AT_BRIGHT_VOL_BAR_LEFT_X	(AT_BRIGHT_VOL_WIN_LEFT_X + AT_BRIGHT_VOL_WIN_SIZE_X - AT_BRIGHT_VOL_BAR_SIZE_X)
#define AT_BRIGHT_VOL_BAR_LEFT_Y	AT_BRIGHT_VOL_WIN_LEFT_Y	// topmost bar // 
#define AT_BRIGHT_VOL_BAR_SIZE_X    40
#define AT_BRIGHT_VOL_BAR_SIZE_Y    16
#define AT_BRIGHT_VOL_BAR_COUNT     10

extern uint8_t AT_BrightVol(void);

#endif

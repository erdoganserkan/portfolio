#ifndef AT_LANG_H
#define AT_LANG_H

#include <stdio.h>
#include <stdint.h>

#define AT_LANG_ICON_LEFT_X     71
#define AT_LANG_ICON_LEFT_Y     99
#define AT_LANG_ICON_SIZE_X     137
#define AT_LANG_ICON_SIZE_Y     130

#define AT_LANG_STR_LEFT_X      64
#define AT_LANG_STR_LEFT_Y      50

#define AT_LANG_ICON_WIN_LEFT_X 249
#define AT_LANG_ICON_WIN_LEFT_Y 37
#define AT_LANG_ICON_WIN_SIZE_X 186
#define AT_LANG_ICON_WIN_SIZE_Y 205

#define AT_LANG_PTR_LANG1_LEFT_X    (AT_LANG_ICON_WIN_LEFT_X)
#define AT_LANG_PTR_LANG1_LEFT_Y    (AT_LANG_ICON_WIN_LEFT_Y)

#define AT_LANG_PTR_LANG2_LEFT_X    (344 - 0)
#define AT_LANG_PTR_LANG2_LEFT_Y    (AT_LANG_ICON_WIN_LEFT_Y)

#define AT_LANG_PTR_LANG3_LEFT_X    (AT_LANG_ICON_WIN_LEFT_X)
#define AT_LANG_PTR_LANG3_LEFT_Y    (106 - 0)

#define AT_LANG_PTR_LANG4_LEFT_X    (344 - 0)
#define AT_LANG_PTR_LANG4_LEFT_Y    (106 - 0)

#define AT_LANG_PTR_LANG5_LEFT_X    (AT_LANG_ICON_WIN_LEFT_X)
#define AT_LANG_PTR_LANG5_LEFT_Y    (173 - 0)

#define AT_LANG_PTR_LANG6_LEFT_X    (344 - 0)
#define AT_LANG_PTR_LANG6_LEFT_Y    (173 - 0)

#define AT_LANG_COUNT   6

extern uint8_t AT_Lang(void);

#endif

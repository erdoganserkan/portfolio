#ifndef AT_DISTANCE_H
#define AT_DISTANCE_H

#include <stdio.h>
#include <stdint.h>

#define AT_RANGE1_NUM_STR "250"
#define AT_RANGE2_NUM_STR "500"
#define AT_RANGE3_NUM_STR "1000"
#define AT_RANGE4_NUM_STR "1500"

#define AT_DISTANCE_ICON_LEFT_X     81
#define AT_DISTANCE_ICON_LEFT_Y     93
#define AT_DISTANCE_ICON_SIZE_X     137
#define AT_DISTANCE_ICON_SIZE_Y     132

#define AT_DISTANCE_PART_SIZE_X		158
#define AT_DISTANCE_PART_SIZE_Y		50

#define AT_DISTANCE_RANGE_WIN_LEFT_X    266
#define AT_DISTANCE_RANGE_WIN_LEFT_Y    43
#define AT_DISTANCE_RANGE_WIN_SIZE_X    AT_DISTANCE_PART_SIZE_X
#define AT_DISTANCE_RANGE_WIN_SIZE_Y    (AT_DISTANCE_PART_SIZE_Y * AT_DISTANCE_RANGE_COUNT)

extern uint8_t AT_Distance(void);

#endif

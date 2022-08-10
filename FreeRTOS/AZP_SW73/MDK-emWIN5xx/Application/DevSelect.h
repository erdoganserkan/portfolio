#ifndef DEV_SELECT_H
#define DEV_SELECT_H

#include <stdio.h>
#include <stdint.h>

#define DEV_SELECT_STR_LEFT_X   	90
#define DEV_SELECT_STR_LEFT_Y   	1
#define DEV_SELECT_STR_RIGHT_X  	370
#define DEV_SELECT_STR_RIGHT_Y   	30

#define DEV_SELECT_DETECTOR_STR_LEFT_X   25
#define DEV_SELECT_DETECTOR_STR_LEFT_Y   36
#define DEV_SELECT_DETECTOR_STR_RIGHT_X   270
#define DEV_SELECT_DETECTOR_STR_RIGHT_Y   60

#define DEV_SELECT_FS_STR_LEFT_X   270
#define DEV_SELECT_FS_STR_LEFT_Y   70
#define DEV_SELECT_FS_STR_RIGHT_X   460
#define DEV_SELECT_FS_STR_RIGHT_Y   100

#define DEV_SELECT_DETECTOR_LEFT_X 21
#define DEV_SELECT_DETECTOR_LEFT_Y 62
#define DEV_SELECT_DETECTOR_SIZE_X 200
#define DEV_SELECT_DETECTOR_SIZE_Y 200

//:TODO: Change this later
#define DEV_SELECT_FS_LEFT_X    231
#define DEV_SELECT_FS_LEFT_Y    110
#define DEV_SELECT_FS_SIZE_X    232
#define DEV_SELECT_FS_SIZE_Y    150

#define DEV_SELECT_TIMEOUT_MS	(60*1000)	/* 60 seconds */

extern uint8_t Dev_Select(void);


#endif

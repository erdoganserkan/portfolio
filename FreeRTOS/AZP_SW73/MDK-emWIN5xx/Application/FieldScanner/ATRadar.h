#ifndef FS_H
#define FS_H

#include <stdint.h>

#define RADAR_ANIM_MS_FAST			50	// for fastest fs frequency // 
#define RADAR_ANIM_MS_SLOW			500	// for lowest fs frequency // 

#define RADAR_PICs_COUNT		(FS_RADAR_PICs_MAX - FS_RADAR_PICs_MIN)
#define RADAR_PICs_START_INDX	FS_RADAR_PICs_MIN
#define RADAR_PICs_UPX			149
#define RADAR_PICs_UPY			79
#define RADAR_PICs_SIZEX		186
#define RADAR_PICs_SIZEY		176

extern uint8_t ATRadar(void);

#endif
